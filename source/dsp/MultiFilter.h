#pragma once

// All filter types behind one interface. One instance per filter slot per voice.
//
// Controls per type (params: cutoff, reso, drive, a, b, c, submode):
//   Classic      submode = slope/passband (17), drive, analog resonance (ClassicFilter)
//   Cluster      submode 0 Peak / 1 Bandpass / 2 Notch / 3 LP+HP; a = spread, b = number of peaks (1..5)
//   Phaser       reso = feedback; a = poles (2..12)
//   Formant      cutoff = formant shift (1 kHz = neutral); a = vowel morph; reso = Q; b = blend (dry/wet)
//   Surgeon      submode 0 LP / 1 HP / 2 Notch / 3 BP (~60 dB/oct); a = spread (Notch/BP width)
//   LoFi         cutoff = sample-rate frequency; a = jitter; submode 0 none / 1 pre / 2 post / 3 pre+post filter;
//                b = pre-filter offset; c = post-filter offset
//   Comb         submode 0 LP / 1 HP / 2 BP / 3 Feed-forward; cutoff = pitch; reso = gain; a = damping; b = all-pass
//   Drive        submode 0 Diode / 1 Overdrive / 2 Soft clip / 3 Tape / 4 Transistor; drive = amount in the loop
//   Allpass      submode 0 Subtle / 1 Medium / 2 Hard; reso = bandwidth; a = dry level; b = feedback (-/+); drive = shaper
//   Ladder       24 dB transistor-ladder style; drive
//   Sallen-Key   submode 0 LP12 / 1 HP6; screaming resonance; drive
//   OTA 4-pole   submode 0 LP24 / 1 LP12; smooth resonance; drive
//   SVF Morph    a = mode: BP at 0, then LP -> Notch -> HP
//   Multimode    submode 0 LP24 / 1 LP12 / 2 BP24 / 3 BP12 / 4 HP24 / 5 HP12 / 6 Notch (pole mixing)
//   LPG          submode 0 LPG / 1 VCA / 2 Both; a = level (open/close), b = decay time
//
// Filter FM: cutoff * 2^(fm * 4 octaves * signal), per sample.

#include "ClassicFilter.h"

namespace cs
{
namespace filt
{
inline float prewarp (float hz, float sr) noexcept
{
    return std::tan (kPi * std::clamp (hz, 10.0f, 0.47f * sr) / sr);
}

// Trapezoidal state-variable 2-pole, stereo.
struct Svf
{
    float k = 1.414f, g = 0.1f, a1 = 0, a2 = 0, a3 = 0;
    float ic1[2] {}, ic2[2] {};

    void reset() noexcept { ic1[0] = ic1[1] = ic2[0] = ic2[1] = 0.0f; }
    void set (float gIn, float kIn) noexcept
    {
        g = gIn;
        k = kIn;
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }
    // Returns band; writes low/high.
    float tick (int ch, float x, float& lp, float& hp, bool saturate = false, float satLevel = 2.0f) noexcept
    {
        const float v3 = x - ic2[ch];
        float v1 = a1 * ic1[ch] + a2 * v3;
        const float v2 = ic2[ch] + a2 * ic1[ch] + a3 * v3;
        if (saturate)
            v1 = softClip (v1 / satLevel) * satLevel;
        ic1[ch] = 2.0f * v1 - ic1[ch];
        ic2[ch] = 2.0f * v2 - ic2[ch];
        lp = v2;
        hp = x - k * v1 - v2;
        return v1;
    }
};

// Trapezoidal one-pole, stereo. lp() returns low-pass; ap() all-pass.
struct OnePole
{
    float G = 0.1f, s[2] {};
    void reset() noexcept { s[0] = s[1] = 0.0f; }
    void set (float g) noexcept { G = g / (1.0f + g); }
    float lp (int ch, float x) noexcept
    {
        const float v = (x - s[ch]) * G;
        const float y = v + s[ch];
        s[ch] = y + v;
        return y;
    }
    float ap (int ch, float x) noexcept { return 2.0f * lp (ch, x) - x; }
};

// Butterworth damping (k = 1/Q) for the 2-pole sections of an order-2n filter.
inline float butterK (int section, int sections) noexcept
{
    return 2.0f * std::sin ((2.0f * (float) section + 1.0f) * kPi / (4.0f * (float) sections));
}

inline float shape (int mode, float x) noexcept
{
    switch (mode)
    {
        case 0: return x > 0.0f ? softClip (x * 1.3f) : 0.6f * softClip (x * 0.7f); // diode: asymmetric
        case 1: return softClip (x * 1.5f) * 0.8f;                                    // overdrive
        case 2: return x / (1.0f + std::abs (x));                                     // soft clip
        case 3: return softClip (x) * (1.0f - 0.15f * std::min (1.0f, x * x));       // tape: rounded, squashed
        default: return std::clamp (x - 0.15f * x * x * x, -1.2f, 1.2f);              // transistor: harder, brighter
    }
}
} // namespace filt

class MultiFilter
{
public:
    void setSampleRate (float sr) noexcept
    {
        sampleRate = sr;
        classic.setSampleRate (sr);
    }

    void reset() noexcept
    {
        classic.reset();
        for (auto& s : svf) s.reset();
        for (auto& o : op) o.reset();
        for (auto& row : lad) for (auto& v : row) v = 0.0f;
        fbL = fbR = 0.0f;
        holdL = holdR = 0.0f;
        lofiPhase = 1.0f;
        for (auto& buf : comb) for (auto& x : buf) x = 0.0f;
        combPos = 0;
        combLp[0] = combLp[1] = 0.0f;
        vactrol = 0.0f;
    }

    // Once per control block. cutoffHz already includes key tracking and modulation.
    void setup (const FilterParams& p, float cutoffHz) noexcept
    {
        type = std::clamp (p.type, 0, (int) FilterTypeCount - 1);
        sub = p.submode;
        reso = std::clamp (p.reso, 0.0f, 1.0f);
        drive = std::clamp (p.drive, 0.0f, 1.0f);
        a = std::clamp (p.a, 0.0f, 1.0f);
        b = std::clamp (p.b, 0.0f, 1.0f);
        c = std::clamp (p.c, 0.0f, 1.0f);
        fmDepth = std::clamp (p.fm, 0.0f, 1.0f) * 4.0f;
        cutoff = std::clamp (cutoffHz, 16.0f, 0.47f * sampleRate);
        driveGain = 1.0f + drive * drive * 8.0f;

        if (type == FilterLpg)
        {
            // Vactrol, at control rate: quick to open, slow to close. a = level, b = decay time.
            const float dt = (float) kBlock / sampleRate;
            const float decay = 0.03f + b * b * 2.0f * (sub == 2 ? 1.5f : 1.0f);
            const float t = a > vactrol ? 0.004f : decay;
            vactrol += (a - vactrol) * (1.0f - std::exp (-dt / t));
        }

        switch (type)
        {
            case FilterClassicType:
                classic.setup (p.mode, cutoff, reso, drive, p.analog);
                break;
            case FilterCluster: setupCluster (1.0f); break;
            case FilterFormant: setupFormant (1.0f); break;
            case FilterSurgeon: setupSurgeon (1.0f); break;
            default: setupBasic (1.0f); break;
        }
    }

    // fmSig: the selected FM source for this sample (mono).
    void process (float& l, float& r, float fmSig) noexcept
    {
        if (fmDepth > 0.0f)
        {
            const float m = std::exp2 (std::clamp (fmDepth * fmSig, -6.0f, 6.0f));
            if (type == FilterClassicType)
                classic.modulateCutoff (m);
            else if (type == FilterCluster)
                setupCluster (m);
            else if (type == FilterFormant)
                setupFormant (m);
            else if (type == FilterSurgeon)
                setupSurgeon (m);
            else
                setupBasic (m);
        }

        switch (type)
        {
            case FilterClassicType: classic.process (l, r); return;
            case FilterCluster: l = cluster (0, l); r = cluster (1, r); return;
            case FilterPhaser: l = phaser (0, l, fbL); r = phaser (1, r, fbR); return;
            case FilterFormant: l = formant (0, l); r = formant (1, r); return;
            case FilterSurgeon: l = surgeon (0, l); r = surgeon (1, r); return;
            case FilterLoFi: lofi (l, r); return;
            case FilterComb: combProcess (l, r); return;
            case FilterDrive: l = driveFilter (0, l); r = driveFilter (1, r); return;
            case FilterAllpass: l = allpassChain (0, l, fbL); r = allpassChain (1, r, fbR); return;
            case FilterLadder: l = ladder (0, l, false); r = ladder (1, r, false); return;
            case FilterSallenKey: l = sallenKey (0, l); r = sallenKey (1, r); return;
            case FilterOta:
                if (sub == 1) { l = otaTwoPole (0, l); r = otaTwoPole (1, r); }
                else { l = ladder (0, l, true); r = ladder (1, r, true); }
                return;
            case FilterSvfMorph: l = svfMorph (0, l); r = svfMorph (1, r); return;
            case FilterMultimode: l = multimode (0, l); r = multimode (1, r); return;
            case FilterLpg: lpg (l, r); return;
            default: return;
        }
    }

private:
    float sampleRate = 48000.0f;
    int type = FilterClassicType, sub = 0;
    float reso = 0, drive = 0, a = 0.5f, b = 0.5f, c = 0.5f, fmDepth = 0, cutoff = 1000, driveGain = 1;

    ClassicFilter classic;
    filt::Svf svf[10];
    filt::OnePole op[16];
    float lad[2][4] {}; // ladder stage states
    float g = 0.1f, G = 0.1f, k = 0.0f;
    float fbL = 0, fbR = 0;
    int numStages = 1;

    // LoFi
    float holdL = 0, holdR = 0, lofiPhase = 1.0f, lofiInc = 0.1f;
    uint32_t lofiRng = 22222u;

    // Comb
    static constexpr int kCombSize = 4096;
    float comb[2][kCombSize] {};
    int combPos = 0;
    float combDelay = 100.0f, combLp[2] {};

    // LPG
    float vactrol = 0.0f;

    // ---------------------------------------------------------------- setup
    void setupBasic (float m) noexcept
    {
        g = filt::prewarp (cutoff * m, sampleRate);
        G = g / (1.0f + g);
        switch (type)
        {
            case FilterPhaser:
            {
                numStages = 2 + 2 * (int) std::lround (a * 5.0f); // 2..12
                for (int i = 0; i < numStages; ++i)
                    op[i].set (g);
                break;
            }
            case FilterLoFi:
            {
                lofiInc = std::clamp (cutoff * m / sampleRate, 0.0005f, 1.0f);
                const float pre = filt::prewarp (cutoff * m * 0.5f * std::exp2 ((b - 0.5f) * 4.0f), sampleRate);
                const float post = filt::prewarp (cutoff * m * 0.5f * std::exp2 ((c - 0.5f) * 4.0f), sampleRate);
                svf[0].set (pre, 1.2f);
                svf[1].set (post, 1.2f);
                break;
            }
            case FilterComb:
            {
                combDelay = std::clamp (sampleRate / std::max (cutoff * m, 30.0f), 2.0f, (float) kCombSize - 4.0f);
                op[0].set (filt::prewarp (20000.0f * std::pow (0.01f, a), sampleRate)); // damping low-pass
                op[1].set (filt::prewarp (40.0f, sampleRate));                         // HP mode / DC guard
                op[2].set (filt::prewarp (200.0f * std::pow (60.0f, b), sampleRate));   // all-pass in the loop
                break;
            }
            case FilterDrive:
                svf[0].set (g, 2.0f * (1.0f - 0.97f * reso));
                break;
            case FilterAllpass:
            {
                numStages = sub <= 0 ? 4 : sub == 1 ? 8 : 16;
                const float spread = 0.05f + 1.5f * reso; // octaves around the cutoff
                for (int i = 0; i < numStages; ++i)
                {
                    const float off = numStages > 1 ? ((float) i / (float) (numStages - 1) - 0.5f) * spread : 0.0f;
                    op[i].set (filt::prewarp (cutoff * m * std::exp2 (off), sampleRate));
                }
                break;
            }
            case FilterLadder:
            case FilterOta:
            case FilterMultimode:
                k = 4.0f * reso * (type == FilterMultimode ? 0.98f : 1.02f);
                if (type == FilterOta && sub == 1)
                    svf[0].set (g, 2.0f * (1.0f - 0.985f * reso));
                break;
            case FilterSallenKey:
                svf[0].set (g, 2.0f * (1.0f - 0.995f * reso));
                op[0].set (g);
                break;
            case FilterSvfMorph:
                svf[0].set (g, 2.0f * (1.0f - 0.98f * reso));
                break;
            case FilterLpg:
                svf[0].set (filt::prewarp (20.0f * std::pow (900.0f, vactrol) * m, sampleRate), 1.2f);
                break;
            default:
                break;
        }
    }

    void setupCluster (float m) noexcept
    {
        numStages = 1 + (int) std::lround (b * 4.0f); // peaks 1..5
        const float ratio = std::exp2 (0.1f + a * 2.4f); // spacing between peaks
        const float q = 1.0f + reso * 24.0f;
        if (sub == 3) // LP + HP: low-pass below and high-pass above the cutoff
        {
            svf[0].set (filt::prewarp (cutoff * m / ratio, sampleRate), 2.0f * (1.0f - 0.95f * reso));
            svf[1].set (filt::prewarp (cutoff * m * ratio, sampleRate), 2.0f * (1.0f - 0.95f * reso));
            return;
        }
        for (int i = 0; i < numStages; ++i)
        {
            const float off = (float) i - 0.5f * (float) (numStages - 1);
            svf[i].set (filt::prewarp (cutoff * m * std::pow (ratio, off), sampleRate), 1.0f / q);
        }
    }

    void setupFormant (float m) noexcept
    {
        // Vowel formants (Hz) A E I O U, morphed by a; shifted by cutoff/1000.
        static constexpr float kF[5][3] = { { 750, 1200, 2700 }, { 450, 1800, 2600 }, { 300, 2200, 3000 },
                                            { 500, 850, 2600 }, { 330, 750, 2400 } };
        const float pos = a * 4.0f;
        const int i0 = std::min ((int) pos, 3);
        const float t = pos - (float) i0;
        const float shift = std::clamp (cutoff * m / 1000.0f, 0.25f, 4.0f);
        const float q = 3.0f + reso * 20.0f;
        for (int f = 0; f < 3; ++f)
        {
            const float hz = (kF[i0][f] + t * (kF[i0 + 1][f] - kF[i0][f])) * shift;
            svf[f].set (filt::prewarp (hz, sampleRate), 1.0f / q);
        }
    }

    void setupSurgeon (float m) noexcept
    {
        constexpr int sections = 5; // 10th order, ~60 dB/oct
        const float w = std::exp2 (0.2f + a * 3.0f); // notch / band width
        for (int s = 0; s < sections; ++s)
        {
            const float kk = filt::butterK (s, sections);
            if (sub <= 1)
                svf[s].set (filt::prewarp (cutoff * m, sampleRate), kk);
            else
            {
                svf[s].set (filt::prewarp (cutoff * m / w, sampleRate), kk);       // lower edge
                svf[s + 5].set (filt::prewarp (cutoff * m * w, sampleRate), kk);   // upper edge
            }
        }
    }

    // -------------------------------------------------------------- process
    float cluster (int ch, float x) noexcept
    {
        float lp, hp;
        switch (sub)
        {
            case 1: // bandpass: sum of peaks
            {
                float y = 0.0f;
                for (int i = 0; i < numStages; ++i)
                    y += svf[i].tick (ch, x, lp, hp) * svf[i].k;
                return y * 1.2f / std::sqrt ((float) numStages);
            }
            case 2: // notches in series
            {
                float y = x;
                for (int i = 0; i < numStages; ++i)
                {
                    const float bp = svf[i].tick (ch, y, lp, hp);
                    y = lp + hp;
                    (void) bp;
                }
                return y;
            }
            case 3:
            {
                float lp1, hp1, lp2, hp2;
                svf[0].tick (ch, x, lp1, hp1);
                svf[1].tick (ch, x, lp2, hp2);
                return lp1 + hp2;
            }
            default: // peaks added to the dry signal
            {
                float y = x;
                for (int i = 0; i < numStages; ++i)
                    y += svf[i].tick (ch, x, lp, hp) * svf[i].k * (1.0f + reso * 3.0f);
                return y * 0.6f;
            }
        }
    }

    float phaser (int ch, float x, float& fb) noexcept
    {
        // Feedback is soft-clipped: with audio-rate FM the all-pass chain is time-varying
        // and can otherwise build up energy without limit.
        float y = x + softClip (fb) * 0.9f * (reso * 2.0f - 1.0f) * 0.95f;
        for (int i = 0; i < numStages; ++i)
            y = op[i].ap (ch, y);
        fb = y;
        return 0.5f * (x + y);
    }

    float formant (int ch, float x) noexcept
    {
        static constexpr float kGain[3] = { 1.0f, 0.7f, 0.35f };
        float lp, hp, y = 0.0f;
        for (int f = 0; f < 3; ++f)
            y += svf[f].tick (ch, x, lp, hp) * svf[f].k * kGain[f] * 2.0f;
        return x + b * (y - x);
    }

    float surgeon (int ch, float x) noexcept
    {
        float lp, hp, y = x;
        switch (sub)
        {
            case 0: for (int s = 0; s < 5; ++s) { svf[s].tick (ch, y, lp, hp); y = lp; } return y;
            case 1: for (int s = 0; s < 5; ++s) { svf[s].tick (ch, y, lp, hp); y = hp; } return y;
            case 2: // notch: low-pass below + high-pass above, in parallel
            {
                float lo = x, hi = x;
                for (int s = 0; s < 5; ++s) { svf[s].tick (ch, lo, lp, hp); lo = lp; }
                for (int s = 0; s < 5; ++s) { svf[s + 5].tick (ch, hi, lp, hp); hi = hp; }
                return lo + hi;
            }
            default: // band: high-pass at the lower edge, then low-pass at the upper edge
            {
                for (int s = 0; s < 5; ++s) { svf[s].tick (ch, y, lp, hp); y = hp; }
                for (int s = 0; s < 5; ++s) { svf[s + 5].tick (ch, y, lp, hp); y = lp; }
                return y;
            }
        }
    }

    void lofi (float& l, float& r) noexcept
    {
        float lp, hp;
        const bool pre = sub == 1 || sub == 3, post = sub == 2 || sub == 3;
        if (pre)
        {
            svf[0].tick (0, l, lp, hp); l = lp;
            svf[0].tick (1, r, lp, hp); r = lp;
        }
        lofiPhase += lofiInc;
        if (lofiPhase >= 1.0f)
        {
            lofiRng ^= lofiRng << 13; lofiRng ^= lofiRng >> 17; lofiRng ^= lofiRng << 5;
            const float jitter = a * ((float) (lofiRng >> 8) * (1.0f / 16777216.0f) - 0.5f);
            lofiPhase -= std::floor (lofiPhase) + jitter * 0.9f;
            holdL = l;
            holdR = r;
        }
        l = holdL;
        r = holdR;
        if (post)
        {
            svf[1].tick (0, l, lp, hp); l = lp;
            svf[1].tick (1, r, lp, hp); r = lp;
        }
    }

    void combProcess (float& l, float& r) noexcept
    {
        const float gain = std::clamp (reso, 0.0f, 1.0f) * 0.985f;
        const float d = combDelay;
        float* io[2] = { &l, &r };
        for (int ch = 0; ch < 2; ++ch)
        {
            // fractional read d samples back
            float rp = (float) combPos - d;
            if (rp < 0.0f) rp += (float) kCombSize;
            const int i0 = (int) rp;
            const float fr = rp - (float) i0;
            const float del = comb[ch][i0] + fr * (comb[ch][(i0 + 1) & (kCombSize - 1)] - comb[ch][i0]);
            const float x = *io[ch];

            float y;
            if (sub == 3) // feed-forward
            {
                comb[ch][combPos] = x;
                y = 0.6f * (x + gain * del);
            }
            else
            {
                float fb = op[0].lp (ch, del);          // damping
                if (b > 0.01f) fb = op[2].ap (ch, fb);  // dispersion
                if (sub == 1) fb = fb - op[1].lp (ch, fb); // HP mode: thinner loop
                if (sub == 2) fb = fb - op[1].lp (ch, fb) * 0.5f;
                const float sign = sub == 1 ? -1.0f : 1.0f;
                const float w = softClip (x + sign * gain * fb);
                comb[ch][combPos] = w;
                y = sub == 2 ? w - x : w;
                y *= 0.5f;
            }
            *io[ch] = y;
        }
        combPos = (combPos + 1) & (kCombSize - 1);
    }

    float driveFilter (int ch, float x) noexcept
    {
        float lp, hp;
        const float in = filt::shape (sub, x * driveGain) / std::sqrt (driveGain);
        // Saturating band feedback gives the "angry" resonance.
        const float bp = svf[0].tick (ch, in, lp, hp, true, 0.6f + 1.4f * (1.0f - drive));
        return filt::shape (sub, (lp + 0.3f * reso * bp) * (1.0f + drive)) * 0.9f;
    }

    float allpassChain (int ch, float x, float& fb) noexcept
    {
        const float fbAmt = (b - 0.5f) * 1.9f; // -0.95..0.95
        float y = x + fbAmt * softClip (fb);
        if (drive > 0.0f)
            y = softClip (y * driveGain) / std::sqrt (driveGain);
        for (int i = 0; i < numStages; ++i)
            y = op[i].ap (ch, y);
        fb = y;
        return a * x + (1.0f - a * 0.5f) * y;
    }

    // Linear zero-delay-feedback 4-stage solve, then nonlinear stage processing.
    // ota = true: smoother saturation per stage, slightly cleaner resonance.
    float ladder (int ch, float x, bool ota) noexcept
    {
        float* s = lad[ch];
        const float sigma = G * G * G * s[0] * (1.0f - G) + G * G * s[1] * (1.0f - G) + G * s[2] * (1.0f - G) + s[3] * (1.0f - G);
        const float G4 = G * G * G * G;
        const float in = x * driveGain * (1.0f + 0.4f * k); // bass compensation
        const float y4est = (G4 * in + sigma) / (1.0f + k * G4);
        float u = ota ? softClip (0.7f * (in - k * y4est)) / 0.7f : softClip (in - k * y4est);
        for (int i = 0; i < 4; ++i)
        {
            if (ota)
                u = softClip (u * 0.8f) * 1.25f;
            const float v = (u - s[i]) * G;
            const float y = v + s[i];
            s[i] = y + v;
            u = y;
        }
        return u / std::sqrt (driveGain);
    }

    float otaTwoPole (int ch, float x) noexcept
    {
        float lp, hp;
        svf[0].tick (ch, softClip (x * driveGain) / std::sqrt (driveGain), lp, hp, true, 1.2f);
        return lp;
    }

    float sallenKey (int ch, float x) noexcept
    {
        float lp, hp;
        const float in = softClip (x * driveGain) / std::sqrt (driveGain);
        const float bp = svf[0].tick (ch, in, lp, hp, true, 0.5f); // hard-saturating resonance
        if (sub == 1) // 6 dB high-pass with resonant peak
            return (in - op[0].lp (ch, in)) + bp * svf[0].k * reso * 1.5f;
        return lp;
    }

    float svfMorph (int ch, float x) noexcept
    {
        float lp, hp;
        const float bp = svf[0].tick (ch, x, lp, hp, true, 3.0f);
        if (a < 0.05f)
            return bp * svf[0].k;
        const float m = (a - 0.05f) / 0.95f;
        const float notch = lp + hp;
        return m < 0.5f ? lp + (notch - lp) * (m * 2.0f) : notch + (hp - notch) * ((m - 0.5f) * 2.0f);
    }

    float multimode (int ch, float x) noexcept
    {
        float* s = lad[ch];
        const float sigma = G * G * G * s[0] * (1.0f - G) + G * G * s[1] * (1.0f - G) + G * s[2] * (1.0f - G) + s[3] * (1.0f - G);
        const float G4 = G * G * G * G;
        const float in = x * driveGain;
        const float y4est = (G4 * in + sigma) / (1.0f + k * G4);
        const float u = softClip (in - k * y4est);
        float y[5];
        y[0] = u;
        for (int i = 0; i < 4; ++i)
        {
            const float v = (y[i] - s[i]) * G;
            y[i + 1] = v + s[i];
            s[i] = y[i + 1] + v;
        }
        float out;
        switch (sub) // pole mixing (binomial combinations of the stage outputs)
        {
            case 1: out = y[2]; break;                                                      // LP12
            case 2: out = 4.0f * (y[2] - 2.0f * y[3] + y[4]); break;                         // BP24
            case 3: out = 2.0f * (y[1] - y[2]); break;                                       // BP12
            case 4: out = y[0] - 4.0f * y[1] + 6.0f * y[2] - 4.0f * y[3] + y[4]; break;      // HP24
            case 5: out = y[0] - 2.0f * y[1] + y[2]; break;                                  // HP12
            case 6: out = y[0] - 2.0f * y[1] + 2.0f * y[2]; break;                           // Notch
            default: out = y[4]; break;                                                      // LP24
        }
        return out / std::sqrt (driveGain);
    }

    void lpg (float& l, float& r) noexcept
    {
        float lp, hp;
        if (sub != 1) // LPG or Both: low-pass that opens with the level
        {
            svf[0].tick (0, l, lp, hp); l = lp;
            svf[0].tick (1, r, lp, hp); r = lp;
        }
        if (sub != 0) // VCA or Both
        {
            const float gv = vactrol * vactrol;
            l *= gv;
            r *= gv;
        }
    }
};
} // namespace cs
