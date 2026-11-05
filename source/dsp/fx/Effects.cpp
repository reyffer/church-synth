#include "Effects.h"

#include "../MultiFilter.h"
#include "FxCommon.h"

namespace cs::fx
{
// ===========================================================================
// Effect list and controls. Order permanent; defaults are applied when the
// effect type of a slot changes.
// ===========================================================================
namespace
{
using P = std::array<const char*, kFxParams>;
using D = std::array<float, kFxParams>;
const FxInfo kFxInfo[FxTypeCount] = {
    { "None", "", P {}, D {}, false },
    { "Delay", "Spatial", P { "Time", "Fine", "Feedback", "Low Cut", "High Cut", "Width", "Ping-Pong", "Dry/Wet" },
      D { 0.78f, 0.5f, 0.4f, 0.1f, 0.8f, 1.0f, 0.0f, 0.3f }, true },
    { "Tape Echo", "Spatial", P { "Time", "Fine", "Intensity", "Input Drive", "Wow & Flutter", "Width", "Ping-Pong", "Dry/Wet" },
      D { 0.78f, 0.5f, 0.45f, 0.3f, 0.3f, 1.0f, 0.0f, 0.3f }, true },
    { "Pitch Shift Delay", "Spatial", P { "Time", "Pitch", "Feedback", "Harmonize", "Spray", "Stereo Detune", "Tone", "Dry/Wet" },
      D { 0.7f, 0.5f, 0.35f, 0.0f, 0.0f, 0.2f, 0.8f, 0.3f }, true },
    { "Reverb", "Spatial", P { "Pre-Delay", "Size", "Decay", "Damping", "Low Cut", "High Cut", "Width", "Dry/Wet" },
      D { 0.1f, 0.6f, 0.5f, 0.5f, 0.15f, 0.8f, 1.0f, 0.25f }, false },
    { "Shimmer", "Spatial", P { "Pitch", "Shimmer", "Size", "Decay", "Modulation", "High Cut", "Ducking", "Width", "Dry/Wet" },
      D { 0.667f, 0.4f, 0.7f, 0.6f, 0.3f, 0.7f, 0.0f, 1.0f, 0.3f }, false },
    { "Compressor", "Dynamics", P { "Threshold", "Ratio", "Attack", "Release", "Makeup", "Auto Makeup", "Output", "Dry/Wet" },
      D { 0.6f, 0.3f, 0.3f, 0.4f, 0.0f, 1.0f, 0.667f, 1.0f }, false },
    { "Multiband", "Dynamics", P { "Low X-Over", "High X-Over", "Amount", "Attack", "Release", "Low Out", "Mid Out", "High Out", "Output" },
      D { 0.4f, 0.4f, 0.5f, 0.3f, 0.4f, 0.5f, 0.5f, 0.5f, 0.5f }, false },
    { "Multi Filter", "Filter & EQ", P { "Type", "Cutoff", "Resonance", "Dry/Wet" },
      D { 0.0f, 0.5f, 0.3f, 1.0f }, false },
    { "Param EQ", "Filter & EQ", P { "Low Shelf Freq", "Low Shelf Gain", "Peak 1 Freq", "Peak 1 Gain", "Peak 2 Freq", "Peak 2 Gain",
                                     "Peak 3 Freq", "Peak 3 Gain", "High Shelf Freq", "High Shelf Gain", "Peak Q", "Scale" },
      D { 0.3f, 0.5f, 0.25f, 0.5f, 0.5f, 0.5f, 0.75f, 0.5f, 0.6f, 0.5f, 0.4f, 0.5f }, false },
    { "Distortion", "Distortion", P { "Type", "Drive", "Auto Gain", "Output", "Filter", "Filter Type", "Cutoff", "Resonance", "Slope", "Dark", "Dry/Wet" },
      D { 0.0f, 0.4f, 1.0f, 0.667f, 0.0f, 0.0f, 0.7f, 0.2f, 0.0f, 0.0f, 1.0f }, false },
    { "Bitcrusher", "Distortion", P { "Bit Depth", "Downsample", "Jitter", "Low Cut", "High Cut", "Dry/Wet" },
      D { 0.5f, 0.2f, 0.0f, 0.0f, 1.0f, 1.0f }, false },
    { "Erode", "Distortion", P { "Mod Type", "Frequency", "Amount", "Noise Blend", "Band Freq", "Bandwidth", "Spread", "Drive", "Dry/Wet" },
      D { 0.0f, 0.6f, 0.3f, 0.0f, 0.6f, 0.5f, 0.3f, 0.0f, 0.5f }, false },
    { "Super Unison", "Modulation", P { "Voices", "Detune", "Drift", "Width", "Low Cut", "High Cut", "Dry/Wet" },
      D { 0.43f, 0.45f, 0.3f, 1.0f, 0.0f, 1.0f, 0.6f }, false },
    { "Chorus", "Modulation", P { "Delay", "Depth", "Rate", "Feedback", "Voices", "Shape", "Stereo", "Dry/Wet" },
      D { 0.3f, 0.3f, 0.3f, 0.5f, 0.33f, 0.0f, 1.0f, 0.4f }, true },
    { "Vintage Chorus", "Modulation", P { "Mode", "Rate", "Depth", "Phase", "Dry/Wet", "Width" },
      D { 0.0f, 0.5f, 0.5f, 1.0f, 0.5f, 1.0f }, false },
    { "Flanger", "Modulation", P { "Delay", "Depth", "Rate", "Feedback", "Low Cut", "High Cut", "Stereo", "Shape", "Dry/Wet" },
      D { 0.3f, 0.5f, 0.3f, 0.75f, 0.0f, 1.0f, 0.5f, 0.0f, 0.5f }, true },
    { "Vintage Flanger", "Modulation", P { "Rate", "Delay", "Feedback", "Depth", "Wide", "Mono Input", "Dry/Wet" },
      D { 0.3f, 0.3f, 0.6f, 0.6f, 1.0f, 0.0f, 0.5f }, true },
    { "Phaser", "Modulation", P { "Frequency", "Feedback", "LFO Wave", "Depth", "Rate", "Poles", "Stereo", "Dry/Wet" },
      D { 0.5f, 0.5f, 0.0f, 0.5f, 0.3f, 0.4f, 0.5f, 0.5f }, true },
    { "Panner", "Modulation", P { "Amount", "Rate", "Invert", "Mono Bass", "Bass Cutoff", "Natural Law", "Shape" },
      D { 0.5f, 0.3f, 0.0f, 0.0f, 0.3f, 1.0f, 0.0f }, true },
    { "Rotary", "Modulation", P { "Speed", "Slow Rate", "Fast Rate", "Ramp", "Horn/Drum", "Drive", "Spread", "Dry/Wet", "Brake" },
      D { 0.0f, 0.5f, 0.6f, 0.5f, 0.5f, 0.2f, 0.8f, 1.0f, 0.0f }, false },
};

// Linear dry/wet: for effects whose wet signal is a processed copy of the dry one.
} // namespace

int fxSyncParam (int type) noexcept
{
    switch (type)
    {
        case FxDelay: case FxTapeEcho: case FxPitchDelay: case FxVintageFlanger: return 0;
        case FxPanner: return 1;
        case FxChorus: case FxFlanger: return 2;
        case FxPhaser: return 4;
        default: return -1;
    }
}

namespace
{
inline float mixDW (float dry, float wet, float m) noexcept { return dry + m * (wet - dry); }
// Equal-power dry/wet: for echoes, reverbs and choruses, whose wet signal is
// decorrelated from the dry one, so inserting them keeps the loudness steady.
inline float mixEP (float dry, float wet, float m) noexcept
{
    const auto& sine = SineTable::get();
    const float q = 0.25f * std::clamp (m, 0.0f, 1.0f); // sin/cos of (pi/2 * m)
    return dry * sine (q + 0.25f) + wet * sine (q);
}

// Width on a wet stereo pair (0 = mono, 1 = as is).
inline void applyWidth (float& l, float& r, float w) noexcept
{
    const float m = 0.5f * (l + r), s = 0.5f * (l - r) * w;
    l = m + s;
    r = m - s;
}

struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    float z1[2] {}, z2[2] {};
    void clear() noexcept { z1[0] = z1[1] = z2[0] = z2[1] = 0.0f; }
    void set (float nb0, float nb1, float nb2, float na0, float na1, float na2) noexcept
    {
        b0 = nb0 / na0; b1 = nb1 / na0; b2 = nb2 / na0; a1 = na1 / na0; a2 = na2 / na0;
    }
    void peak (float f, float db, float q, float sr) noexcept
    {
        const float A = std::pow (10.0f, db / 40.0f), w = 2.0f * kPi * f / sr, al = std::sin (w) / (2.0f * q), c = std::cos (w);
        set (1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A);
    }
    void shelf (float f, float db, float sr, bool high) noexcept
    {
        const float A = std::pow (10.0f, db / 40.0f), w = 2.0f * kPi * f / sr, c = std::cos (w);
        const float al = std::sin (w) / 2.0f * std::sqrt (2.0f), sq = 2.0f * std::sqrt (A) * al;
        if (! high)
            set (A * ((A + 1) - (A - 1) * c + sq), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - sq),
                 (A + 1) + (A - 1) * c + sq, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - sq);
        else
            set (A * ((A + 1) + (A - 1) * c + sq), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - sq),
                 (A + 1) - (A - 1) * c + sq, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - sq);
    }
    float tick (int ch, float x) noexcept
    {
        const float y = b0 * x + z1[ch];
        z1[ch] = b1 * x - a1 * y + z2[ch];
        z2[ch] = b2 * x - a2 * y;
        return y;
    }
};

// ===========================================================================
class DelayFx final : public Effect
{
public:
    explicit DelayFx (bool tape) : tape (tape) {}
    void prepare (float sr) override
    {
        sampleRate = sr;
        for (auto& d : line) d.prepare ((int) (2.6f * sr));
        time.setTime (0.08f, sr);
        reset();
    }
    void reset() override
    {
        for (auto& d : line) d.clear();
        lowCut.reset(); highCut.reset();
        time.v = 0.3f * sampleRate;
    }
    void process (float* l, float* r, int n, const float* p, bool sync, const FxContext& ctx) noexcept override
    {
        float t = timeSeconds (p[0], sync, ctx.bpm, 1.0f, 2000.0f) + (p[1] - 0.5f) * 0.06f;
        const float target = std::clamp (t * sampleRate, 1.0f, (float) line[0].capacity() - 64.0f);
        const float fb = tape ? std::min (p[2] * 1.1f, 1.05f) : p[2] * 0.98f;
        const float drive = tape ? 1.0f + 6.0f * p[3] * p[3] : 1.0f;
        const float wow = tape ? p[4] : 0.0f;
        const float width = p[5];
        const bool ping = flag (p[6]);
        const float mix = p[7];
        if (tape)
        {
            lowCut.set (filt::prewarp (90.0f, sampleRate));
            highCut.set (filt::prewarp (4500.0f, sampleRate));
        }
        else
        {
            lowCut.set (filt::prewarp (expMap (p[3], 20.0f, 2000.0f), sampleRate));
            highCut.set (filt::prewarp (expMap (p[4], 1000.0f, 20000.0f), sampleRate));
        }
        for (int i = 0; i < n; ++i)
        {
            float d = time.next (target);
            if (wow > 0.0f)
                d += wow * (0.004f * sampleRate * lfoSlow.tick (0.6f, sampleRate) + 0.0006f * sampleRate * lfoFast.tick (6.3f, sampleRate));
            float wl = line[0].read (d), wr = line[1].read (d);
            // Filter the repeats (inside the loop, so each echo gets darker / thinner).
            wl = highCut.lp (0, wl); wl -= lowCut.lp (0, wl);
            wr = highCut.lp (1, wr); wr -= lowCut.lp (1, wr);
            if (tape)
            {
                wl = softClip (wl);
                wr = softClip (wr);
            }
            float inL = l[i], inR = r[i];
            if (tape)
            {
                inL = softClip (inL * drive) / std::sqrt (drive);
                inR = softClip (inR * drive) / std::sqrt (drive);
            }
            if (ping)
            {
                line[0].push (0.5f * (inL + inR) + fb * wr);
                line[1].push (fb * wl);
            }
            else
            {
                line[0].push (inL + fb * wl);
                line[1].push (inR + fb * wr);
            }
            applyWidth (wl, wr, width);
            l[i] = mixEP (l[i], wl, mix);
            r[i] = mixEP (r[i], wr, mix);
        }
    }

private:
    bool tape;
    float sampleRate = 48000.0f;
    Delay line[2];
    Smooth time;
    filt::OnePole lowCut, highCut;
    FxLfo lfoSlow, lfoFast;
};

// ===========================================================================
class PitchDelayFx final : public Effect
{
public:
    void prepare (float sr) override
    {
        sampleRate = sr;
        for (auto& d : line) d.prepare ((int) (2.6f * sr));
        for (auto& s : shift) s.prepare (sr);
        for (auto& s : harm) s.prepare (sr);
        time.setTime (0.08f, sr);
        reset();
    }
    void reset() override
    {
        for (auto& d : line) d.clear();
        for (auto& s : shift) s.clear();
        for (auto& s : harm) s.clear();
        tone.reset();
        time.v = 0.3f * sampleRate;
    }
    void process (float* l, float* r, int n, const float* p, bool sync, const FxContext& ctx) noexcept override
    {
        const float target = std::clamp (timeSeconds (p[0], sync, ctx.bpm, 1.0f, 2000.0f) * sampleRate, 1.0f, (float) line[0].capacity() - 64.0f);
        const float semis = std::round (linMap (p[1], -12.0f, 12.0f));
        const float fb = p[2] * 0.95f;
        const int harmonize = choice (p[3], 3); // normal, + octave up, + octave down
        const float spray = p[4];
        const float det = p[5] * 30.0f; // cents
        tone.set (filt::prewarp (expMap (p[6], 800.0f, 20000.0f), sampleRate));
        const float mix = p[7];
        const float ratioL = std::exp2 ((semis - det / 100.0f) / 12.0f), ratioR = std::exp2 ((semis + det / 100.0f) / 12.0f);
        const float hRatio = harmonize == 1 ? 2.0f : 0.5f;
        float* io[2] = { l, r };
        for (int i = 0; i < n; ++i)
        {
            const float d = time.next (target);
            for (int ch = 0; ch < 2; ++ch)
            {
                const float del = line[ch].read (d);
                float w = shift[ch].process (del, ch == 0 ? ratioL : ratioR, spray);
                if (harmonize > 0)
                    w = 0.7f * (w + harm[ch].process (del, hRatio, spray));
                w = tone.lp (ch, w);
                line[ch].push (io[ch][i] + fb * softClip (w));
                io[ch][i] = mixEP (io[ch][i], w, mix);
            }
        }
    }

private:
    float sampleRate = 48000.0f;
    Delay line[2];
    PitchShifter shift[2], harm[2];
    Smooth time;
    filt::OnePole tone;
};

// ===========================================================================
// Feedback-delay-network reverb (8 lines, Hadamard mixing), optional shimmer.
class FdnReverb
{
public:
    void prepare (float sr)
    {
        sampleRate = sr;
        for (auto& d : lines) d.prepare ((int) (0.2f * sr));
        for (auto& d : diff) d.prepare ((int) (0.03f * sr));
        pre.prepare ((int) (0.27f * sr));
        for (auto& s : shifter) s.prepare (sr);
        clear();
    }
    void clear() noexcept
    {
        for (auto& d : lines) d.clear();
        for (auto& d : diff) d.clear();
        pre.clear();
        for (auto& s : shifter) s.clear();
        for (auto& v : damp) v = 0.0f;
        inLow.reset(); inHigh.reset();
        shimmerFb = 0.0f;
        duckEnv = 0.0f;
    }

    struct Settings
    {
        float preDelay = 0.02f, size = 0.6f, decay = 2.0f, damping = 0.5f, lowCut = 40.0f, highCut = 12000.0f;
        float width = 1.0f, mix = 0.25f, modulation = 0.2f;
        float shimmer = 0.0f, shimmerRatio = 2.0f, ducking = 0.0f;
    };

    void process (float* l, float* r, int n, const Settings& s) noexcept
    {
        static constexpr float kLenMs[8] = { 31.1f, 37.3f, 41.7f, 47.9f, 53.1f, 59.3f, 67.9f, 73.7f };
        static constexpr float kDiffMs[4] = { 4.7f, 3.6f, 12.7f, 9.3f };
        const float scale = 0.35f + 1.65f * std::clamp (s.size, 0.0f, 1.0f);
        float len[8], gain[8];
        for (int k = 0; k < 8; ++k)
        {
            len[k] = kLenMs[k] * 0.001f * sampleRate * scale;
            gain[k] = std::pow (10.0f, -3.0f * len[k] / (std::max (s.decay, 0.1f) * sampleRate));
        }
        const float dampCoef = 0.05f + 0.9f * (1.0f - std::clamp (s.damping, 0.0f, 1.0f)) * 0.95f;
        inLow.set (filt::prewarp (s.lowCut, sampleRate));
        inHigh.set (filt::prewarp (s.highCut, sampleRate));
        const float preD = std::max (1.0f, s.preDelay * sampleRate);
        const float modDepth = s.modulation * 0.0025f * sampleRate;
        const float duckRel = std::exp (-1.0f / (0.25f * sampleRate));

        for (int i = 0; i < n; ++i)
        {
            float in = 0.5f * (l[i] + r[i]);
            const float a = std::abs (in);
            duckEnv = a > duckEnv ? a : duckEnv * duckRel;
            in = inHigh.lp (0, in);
            in -= inLow.lp (0, in);
            pre.push (in + s.shimmer * shimmerFb);
            float x = pre.read (preD);
            for (int k = 0; k < 4; ++k) // input diffusion (series all-passes)
            {
                const float dl = kDiffMs[k] * 0.001f * sampleRate * (0.6f + 0.4f * scale);
                const float dd = diff[k].read (dl);
                const float v = x + 0.6f * dd;
                diff[k].push (v);
                x = dd - 0.6f * v;
            }

            float o[8];
            for (int k = 0; k < 8; ++k)
            {
                float d = len[k];
                if (k < 4 && modDepth > 0.0f)
                    d += modDepth * mod.at (0.25f * (float) k);
                o[k] = lines[k].read (d);
                damp[k] += dampCoef * (o[k] - damp[k]); // high-frequency damping
                o[k] = damp[k];
            }
            mod.tick (0.37f, sampleRate);

            // Hadamard 8x8 mixing (fast Walsh-Hadamard transform), energy-preserving.
            float h[8];
            for (int k = 0; k < 8; ++k) h[k] = o[k];
            for (int len2 = 1; len2 < 8; len2 <<= 1)
                for (int j = 0; j < 8; j += len2 << 1)
                    for (int m = j; m < j + len2; ++m)
                    {
                        const float u = h[m], v = h[m + len2];
                        h[m] = u + v;
                        h[m + len2] = u - v;
                    }
            const float norm = 0.35355339f; // 1/sqrt(8)
            for (int k = 0; k < 8; ++k)
                lines[k].push (x * ((k & 1) ? -0.5f : 0.5f) + gain[k] * h[k] * norm);

            float wl = o[0] - o[2] + o[4] - o[6];
            float wr = o[1] - o[3] + o[5] - o[7];
            wl *= 0.5f;
            wr *= 0.5f;

            if (s.shimmer > 0.0f)
            {
                const float sl = shifter[0].process (wl, s.shimmerRatio);
                const float sr2 = shifter[1].process (wr, s.shimmerRatio);
                shimmerFb = softClip (0.5f * (sl + sr2));
            }
            if (s.ducking > 0.0f)
            {
                const float g = 1.0f - s.ducking * std::min (1.0f, duckEnv * 2.0f);
                wl *= g;
                wr *= g;
            }
            applyWidth (wl, wr, s.width);
            l[i] = mixEP (l[i], wl, s.mix);
            r[i] = mixEP (r[i], wr, s.mix);
        }
    }

private:
    float sampleRate = 48000.0f;
    Delay lines[8], diff[4], pre;
    float damp[8] {};
    filt::OnePole inLow, inHigh;
    FxLfo mod;
    PitchShifter shifter[2];
    float shimmerFb = 0.0f, duckEnv = 0.0f;
};

class ReverbFx final : public Effect
{
public:
    explicit ReverbFx (bool shimmer) : shimmer (shimmer) {}
    void prepare (float sr) override { rv.prepare (sr); }
    void reset() override { rv.clear(); }
    void process (float* l, float* r, int n, const float* p, bool, const FxContext&) noexcept override
    {
        FdnReverb::Settings s;
        if (! shimmer)
        {
            s.preDelay = p[0] * 0.25f;
            s.size = p[1];
            s.decay = expMap (p[2], 0.2f, 20.0f);
            s.damping = p[3];
            s.lowCut = expMap (p[4], 20.0f, 1000.0f);
            s.highCut = expMap (p[5], 1000.0f, 20000.0f);
            s.width = p[6];
            s.mix = p[7];
        }
        else
        {
            const float semis = std::round (linMap (p[0], -12.0f, 24.0f));
            s.shimmerRatio = std::exp2 (semis / 12.0f);
            s.shimmer = p[1] * 0.85f;
            s.size = p[2];
            s.decay = expMap (p[3], 0.5f, 30.0f);
            s.modulation = p[4];
            s.highCut = expMap (p[5], 1000.0f, 20000.0f);
            s.damping = 0.6f * (1.0f - p[5]);
            s.ducking = p[6];
            s.width = p[7];
            s.mix = p[8];
            s.lowCut = 120.0f;
            s.preDelay = 0.02f;
        }
        rv.process (l, r, n, s);
    }

private:
    bool shimmer;
    FdnReverb rv;
};

// ===========================================================================
class CompressorFx final : public Effect
{
public:
    void prepare (float sr) override { sampleRate = sr; reset(); }
    void reset() override { grDb = 0.0f; }
    void process (float* l, float* r, int n, const float* p, bool, const FxContext&) noexcept override
    {
        const float thr = linMap (p[0], -60.0f, 0.0f);
        const float ratio = expMap (p[1], 1.0f, 20.0f);
        const float att = std::exp (-1.0f / (expMap (p[2], 0.1f, 100.0f) * 0.001f * sampleRate));
        const float rel = std::exp (-1.0f / (expMap (p[3], 10.0f, 1000.0f) * 0.001f * sampleRate));
        float makeup = p[4] * 24.0f;
        if (flag (p[5]))
            makeup += -thr * (1.0f - 1.0f / ratio) * 0.5f;
        const float out = dbToGain (makeup + linMap (p[6], -24.0f, 12.0f));
        const float mix = p[7];
        constexpr float knee = 6.0f;
        for (int i = 0; i < n; ++i)
        {
            const float lvl = gainToDb (std::max (std::abs (l[i]), std::abs (r[i])));
            const float over = lvl - thr;
            float target = 0.0f;
            if (over > knee * 0.5f)
                target = over * (1.0f - 1.0f / ratio);
            else if (over > -knee * 0.5f)
                target = (1.0f - 1.0f / ratio) * (over + knee * 0.5f) * (over + knee * 0.5f) / (2.0f * knee);
            const float c = target > grDb ? att : rel;
            grDb = target + c * (grDb - target);
            const float g = dbToGain (-grDb) * out;
            l[i] = mixDW (l[i], l[i] * g, mix);
            r[i] = mixDW (r[i], r[i] * g, mix);
        }
    }
    float gainReductionDb() const noexcept { return grDb; }

private:
    float sampleRate = 48000.0f, grDb = 0.0f;
};

// ===========================================================================
// 3-band: compresses loud bands down and lifts quiet bands up ("both ways").
class MultibandFx final : public Effect
{
public:
    void prepare (float sr) override { sampleRate = sr; reset(); }
    void reset() override
    {
        for (auto& s : lp1) s.reset();
        for (auto& s : hp1) s.reset();
        for (auto& s : lp2) s.reset();
        for (auto& s : hp2) s.reset();
        for (auto& e : env) e = 0.0f;
        for (auto& g : gainDb) g = 0.0f;
    }
    void process (float* l, float* r, int n, const float* p, bool, const FxContext&) noexcept override
    {
        constexpr float kBw = 1.41421356f;
        const float f1 = expMap (p[0], 50.0f, 1000.0f), f2 = std::max (f1 * 1.5f, expMap (p[1], 1000.0f, 10000.0f));
        const float g1 = filt::prewarp (f1, sampleRate), g2 = filt::prewarp (f2, sampleRate);
        for (int k = 0; k < 2; ++k)
        {
            lp1[k].set (g1, kBw); hp1[k].set (g1, kBw);
            lp2[k].set (g2, kBw); hp2[k].set (g2, kBw);
        }
        const float amount = p[2];
        const float att = std::exp (-1.0f / (expMap (p[3], 0.5f, 100.0f) * 0.001f * sampleRate));
        const float rel = std::exp (-1.0f / (expMap (p[4], 20.0f, 1000.0f) * 0.001f * sampleRate));
        const float outBand[3] = { dbToGain (linMap (p[5], -12.0f, 12.0f)), dbToGain (linMap (p[6], -12.0f, 12.0f)),
                                   dbToGain (linMap (p[7], -12.0f, 12.0f)) };
        // Automatic make-up for the downward part, so the default setting stays about level.
        const float makeupDb = 10.0f * (1.0f - 1.0f / (1.0f + 5.0f * amount));
        const float outGain = dbToGain (linMap (p[8], -12.0f, 12.0f) + makeupDb);
        const float envCoef = 1.0f - std::exp (-1.0f / (0.005f * sampleRate));
        float* io[2] = { l, r };

        for (int i = 0; i < n; ++i)
        {
            float band[3][2];
            for (int ch = 0; ch < 2; ++ch)
            {
                const float x = io[ch][i];
                float lo, hi, tmp;
                // Linkwitz-Riley 4th order = two cascaded Butterworth 2-poles.
                lp1[0].tick (ch, x, lo, tmp);
                lp1[1].tick (ch, lo, lo, tmp);
                hp1[0].tick (ch, x, tmp, hi);
                hp1[1].tick (ch, hi, tmp, hi);
                float mid, high;
                lp2[0].tick (ch, hi, mid, tmp);
                lp2[1].tick (ch, mid, mid, tmp);
                hp2[0].tick (ch, hi, tmp, high);
                hp2[1].tick (ch, high, tmp, high);
                band[0][ch] = lo;
                band[1][ch] = mid;
                band[2][ch] = high;
            }
            float yl = 0.0f, yr = 0.0f;
            for (int b = 0; b < 3; ++b)
            {
                const float pk = std::max (std::abs (band[b][0]), std::abs (band[b][1]));
                env[b] += envCoef * (pk * pk - env[b]);
                const float lvl = 10.0f * std::log10 (env[b] + 1.0e-12f);
                float target = 0.0f;
                if (lvl > -24.0f)
                    target = -(lvl + 24.0f) * (1.0f - 1.0f / (1.0f + 5.0f * amount)); // downward
                else if (lvl < -40.0f && lvl > -70.0f)
                    target = std::min (12.0f, (-40.0f - lvl) * 0.5f) * amount; // upward
                const float c = target < gainDb[b] ? att : rel;
                gainDb[b] = target + c * (gainDb[b] - target);
                const float g = dbToGain (gainDb[b]) * outBand[b];
                yl += band[b][0] * g;
                yr += band[b][1] * g;
            }
            l[i] = yl * outGain;
            r[i] = yr * outGain;
        }
    }

private:
    float sampleRate = 48000.0f;
    filt::Svf lp1[2], hp1[2], lp2[2], hp2[2];
    float env[3] {}, gainDb[3] {};
};

// ===========================================================================
class MultiFilterFx final : public Effect
{
public:
    void prepare (float sr) override
    {
        sampleRate = sr;
        for (auto& c : comb) c.prepare ((int) (sr / 20.0f) + 8);
        reset();
    }
    void reset() override
    {
        for (auto& s : svf) s.reset();
        for (auto& c : comb) c.clear();
    }
    void process (float* l, float* r, int n, const float* p, bool, const FxContext&) noexcept override
    {
        const int type = choice (p[0], 12); // LP12/24/36, HP12/24/36, BP12/24/36, Notch, Comb FF, Comb FB
        const float fc = expMap (p[1], 20.0f, 20000.0f);
        const float reso = p[2];
        const float mix = p[3];
        const int stages = type < 9 ? type % 3 + 1 : 1;
        const float g = filt::prewarp (fc, sampleRate);
        for (int s = 0; s < stages; ++s)
            svf[s].set (g, s == stages - 1 ? 2.0f * (1.0f - 0.97f * reso) : 1.41421356f);
        const float delay = std::clamp (sampleRate / fc, 1.0f, (float) comb[0].capacity());
        float* io[2] = { l, r };
        for (int i = 0; i < n; ++i)
            for (int ch = 0; ch < 2; ++ch)
            {
                const float x = io[ch][i];
                float y = x, lp, hp;
                if (type < 9)
                {
                    for (int s = 0; s < stages; ++s)
                    {
                        const float bp = svf[s].tick (ch, y, lp, hp);
                        y = type < 3 ? lp : type < 6 ? hp : bp * svf[s].k;
                    }
                }
                else if (type == 9)
                {
                    svf[0].tick (ch, y, lp, hp);
                    y = lp + hp;
                }
                else
                {
                    const float d = comb[ch].read (delay);
                    if (type == 10) { comb[ch].push (x); y = 0.5f * (x + reso * 0.98f * d); }
                    else { y = softClip (x + reso * 0.98f * d); comb[ch].push (y); y *= 0.5f; }
                }
                io[ch][i] = mixDW (x, y, mix);
            }
    }

private:
    float sampleRate = 48000.0f;
    filt::Svf svf[3];
    Delay comb[2];
};

// ===========================================================================
class ParamEqFx final : public Effect
{
public:
    void prepare (float sr) override { sampleRate = sr; reset(); }
    void reset() override { for (auto& b : bands) b.clear(); }
    void process (float* l, float* r, int n, const float* p, bool, const FxContext&) noexcept override
    {
        const float scale = p[11] * 2.0f;
        const float q = expMap (p[10], 0.3f, 8.0f);
        const float nyq = 0.45f * sampleRate;
        auto gain = [scale] (float x) { return linMap (x, -15.0f, 15.0f) * scale; };
        bands[0].shelf (std::min (expMap (p[0], 30.0f, 500.0f), nyq), gain (p[1]), sampleRate, false);
        bands[1].peak (std::min (expMap (p[2], 50.0f, 16000.0f), nyq), gain (p[3]), q, sampleRate);
        bands[2].peak (std::min (expMap (p[4], 50.0f, 16000.0f), nyq), gain (p[5]), q, sampleRate);
        bands[3].peak (std::min (expMap (p[6], 50.0f, 16000.0f), nyq), gain (p[7]), q, sampleRate);
        bands[4].shelf (std::min (expMap (p[8], 2000.0f, 16000.0f), nyq), gain (p[9]), sampleRate, true);
        for (int i = 0; i < n; ++i)
            for (auto& b : bands)
            {
                l[i] = b.tick (0, l[i]);
                r[i] = b.tick (1, r[i]);
            }
    }

private:
    float sampleRate = 48000.0f;
    Biquad bands[5];
};

// ===========================================================================
inline float distShape (int type, float x) noexcept
{
    switch (type)
    {
        case 0: return softClip (x);                                             // Soft Clip
        case 1: return std::clamp (x, -1.0f, 1.0f);                              // Hard Clip
        case 2: return x > 0.0f ? softClip (x) : softClip (0.6f * x) / 0.6f * 0.8f; // Tube (asymmetric)
        case 3: return softClip (x * 0.8f) * (1.0f - 0.1f * std::min (1.0f, x * x)) * 1.2f; // Tape
        case 4: return softClip (1.5f * x) * 0.8f;                               // Overdrive
        case 5: return std::clamp (2.0f * x + 0.3f * std::abs (x), -1.0f, 1.0f) * 0.9f; // Fuzz
        case 6: return softClip (x + 0.4f * x * std::abs (x));                    // Crunch
        case 7: return x > 0.0f ? 1.0f - std::exp (-x) : -0.4f * (1.0f - std::exp (0.6f * x)); // Diode
        case 8: return SineTable::get() (wrap01 (0.25f * x));                     // Sine Fold
        case 9: { const float t = wrap01 ((x + 1.0f) * 0.25f); return 1.0f - 4.0f * std::abs (t - 0.5f); } // Triangle Fold
        case 10: return softClip (std::max (x, 0.0f)) * 1.6f - 0.4f;              // Half Rectify
        case 11: return softClip (std::abs (x)) * 1.6f - 0.6f;                    // Full Rectify
        case 12: return softClip (x + 0.35f) - softClip (0.35f);                  // Asymmetric
        case 13: { const float q = 6.0f; return std::round (softClip (x) * q) / q; } // Bit Shape
        case 14: return x / (1.0f + std::abs (x));                                // Saturate
        default: { const float f = SineTable::get() (wrap01 (0.2f * x)); return std::clamp (f * 1.6f + 0.2f * x, -1.0f, 1.0f); } // Destroy
    }
}

class DistortionFx final : public Effect
{
public:
    void prepare (float sr) override { sampleRate = sr; reset(); }
    void reset() override
    {
        for (auto& o : os) o.clear();
        for (auto& s : svf) s.reset();
        dark.reset();
    }
    void process (float* l, float* r, int n, const float* p, bool, const FxContext&) noexcept override
    {
        const int type = choice (p[0], 16);
        const float drive = dbToGain (p[1] * 36.0f);
        const bool autoGain = flag (p[2]);
        const float out = dbToGain (linMap (p[3], -24.0f, 12.0f)) * (autoGain ? 1.0f / std::sqrt (drive) : 1.0f);
        const int filterPos = choice (p[4], 3); // off / pre / post
        const int fType = choice (p[5], 3);     // LP / HP / BP
        const float fc = expMap (p[6], 40.0f, 18000.0f);
        const int stages = choice (p[8], 3) + 1;
        for (int s = 0; s < stages; ++s)
            svf[s].set (filt::prewarp (fc, sampleRate), s == stages - 1 ? 2.0f * (1.0f - 0.95f * p[7]) : 1.41421356f);
        const bool isDark = flag (p[9]);
        dark.set (filt::prewarp (6000.0f, sampleRate));
        const float mix = p[10];
        float* io[2] = { l, r };
        for (int i = 0; i < n; ++i)
            for (int ch = 0; ch < 2; ++ch)
            {
                const float x = io[ch][i];
                float y = x;
                if (filterPos == 1) y = filter (ch, y, fType, stages);
                y = os[ch].process (y, [&] (float s) { return distShape (type, s * drive); });
                if (filterPos == 2) y = filter (ch, y, fType, stages);
                if (isDark) y = dark.lp (ch, y);
                io[ch][i] = mixDW (x, y * out, mix);
            }
    }

private:
    float sampleRate = 48000.0f;
    Oversampler2x os[2];
    filt::Svf svf[3];
    filt::OnePole dark;
    float filter (int ch, float x, int type, int stages) noexcept
    {
        float lp, hp;
        for (int s = 0; s < stages; ++s)
        {
            const float bp = svf[s].tick (ch, x, lp, hp);
            x = type == 0 ? lp : type == 1 ? hp : bp * svf[s].k;
        }
        return x;
    }
};

// ===========================================================================
class BitcrusherFx final : public Effect
{
public:
    void prepare (float sr) override { sampleRate = sr; reset(); }
    void reset() override { counter = 0.0f; held[0] = held[1] = 0.0f; lc.reset(); hc.reset(); }
    void process (float* l, float* r, int n, const float* p, bool, const FxContext&) noexcept override
    {
        const float bits = linMap (p[0], 1.5f, 16.0f);
        const float levels = std::exp2 (bits - 1.0f);
        const float factor = expMap (p[1], 1.0f, 80.0f);
        const float jitter = p[2];
        lc.set (filt::prewarp (expMap (p[3], 20.0f, 2000.0f), sampleRate));
        hc.set (filt::prewarp (expMap (p[4], 1000.0f, 20000.0f), sampleRate));
        const float mix = p[5];
        for (int i = 0; i < n; ++i)
        {
            counter += 1.0f;
            if (counter >= hold)
            {
                counter -= hold;
                hold = factor * (1.0f + jitter * (rng.uni() - 0.5f));
                held[0] = std::round (l[i] * levels) / levels;
                held[1] = std::round (r[i] * levels) / levels;
            }
            float yl = held[0], yr = held[1];
            yl = hc.lp (0, yl); yl -= lc.lp (0, yl);
            yr = hc.lp (1, yr); yr -= lc.lp (1, yr);
            l[i] = mixDW (l[i], yl, mix);
            r[i] = mixDW (r[i], yr, mix);
        }
    }

private:
    float sampleRate = 48000.0f, counter = 0.0f, hold = 1.0f, held[2] {};
    filt::OnePole lc, hc;
    Rng rng;
};

// ===========================================================================
// Erode: a very short delay whose time is wobbled at audio rate by a
// band-limited modulator, which smears and corrodes the signal.
class ErodeFx final : public Effect
{
public:
    void prepare (float sr) override
    {
        sampleRate = sr;
        for (auto& d : line) d.prepare ((int) (0.012f * sr));
        reset();
    }
    void reset() override
    {
        for (auto& d : line) d.clear();
        bp.reset();
        phase = 0.0f;
    }
    void process (float* l, float* r, int n, const float* p, bool, const FxContext&) noexcept override
    {
        const int modType = choice (p[0], 5); // sine, triangle, saw, square, noise
        const float hz = expMap (p[1], 20.0f, 20000.0f);
        const float depth = p[2] * p[2] * 0.004f * sampleRate; // up to 4 ms
        const float noiseBlend = p[3];
        bp.set (filt::prewarp (expMap (p[4], 50.0f, 15000.0f), sampleRate), linMap (p[5], 0.1f, 2.0f));
        const float spread = p[6] * 0.5f;
        const float drive = 1.0f + 8.0f * p[7] * p[7];
        const float mix = p[8];
        const float base = 0.0005f * sampleRate + depth;
        const float inc = hz / sampleRate;
        float* io[2] = { l, r };
        for (int i = 0; i < n; ++i)
        {
            phase = wrap01 (phase + inc);
            for (int ch = 0; ch < 2; ++ch)
            {
                const float ph = wrap01 (phase + (ch == 1 ? spread : 0.0f));
                float m;
                switch (modType)
                {
                    case 0: m = SineTable::get() (ph); break;
                    case 1: m = ph < 0.5f ? 4.0f * ph - 1.0f : 3.0f - 4.0f * ph; break;
                    case 2: m = 1.0f - 2.0f * ph; break;
                    case 3: m = ph < 0.5f ? 1.0f : -1.0f; break;
                    default: m = rng.bi(); break;
                }
                m += noiseBlend * (rng.bi() - m);
                float lp, hp;
                m = bp.tick (ch, m, lp, hp) * bp.k * 2.0f;
                const float x = io[ch][i];
                line[ch].push (softClip (x * drive) / std::sqrt (drive));
                const float y = line[ch].read (base + depth * std::clamp (m, -1.0f, 1.0f));
                io[ch][i] = mixDW (x, y, mix);
            }
        }
    }

private:
    float sampleRate = 48000.0f, phase = 0.0f;
    Delay line[2];
    filt::Svf bp;
    Rng rng;
};

// ===========================================================================
// Super Unison: several pitch-shifted copies (constant detune) spread in stereo.
class SuperUnisonFx final : public Effect
{
public:
    static constexpr int kMax = 8;
    void prepare (float sr) override
    {
        sampleRate = sr;
        for (auto& s : shift) s.prepare (sr);
        reset();
    }
    void reset() override
    {
        for (int v = 0; v < kMax; ++v)
        {
            shift[v].clear();
            shift[v].setPhase ((float) v * 0.618f); // staggered grains: no shared flutter
        }
        lc.reset(); hc.reset();
        for (auto& d : drift) d = 0.0f;
    }
    void process (float* l, float* r, int n, const float* p, bool, const FxContext&) noexcept override
    {
        const int voices = 2 + choice (p[0], 7);
        const float cents = 100.0f * p[1] * std::sqrt (p[1]); // up to +/-100 cents, fine control at the bottom
        const float driftAmt = p[2];
        const float width = p[3];
        lc.set (filt::prewarp (expMap (p[4], 20.0f, 2000.0f), sampleRate));
        hc.set (filt::prewarp (expMap (p[5], 1000.0f, 20000.0f), sampleRate));
        const float mix = p[6];
        // Slow random drift per voice, updated once per block.
        float ratio[kMax], pl[kMax], pr[kMax];
        for (int v = 0; v < voices; ++v)
        {
            drift[v] += (rng.bi() * 0.5f - drift[v]) * 0.02f;
            const float pos = voices > 1 ? -1.0f + 2.0f * (float) v / (float) (voices - 1) : 0.0f;
            ratio[v] = std::exp2 ((pos * cents + driftAmt * drift[v] * 20.0f) / 1200.0f);
            // Alternate sides so neighbouring detunes don't sit together.
            const float side = (v % 2 == 0 ? 1.0f : -1.0f) * std::abs (pos);
            panGains (side * width, pl[v], pr[v]);
        }
        const float norm = 1.3f / std::sqrt ((float) voices);
        for (int i = 0; i < n; ++i)
        {
            const float in = 0.5f * (l[i] + r[i]);
            float wl = 0.0f, wr = 0.0f;
            for (int v = 0; v < voices; ++v)
            {
                const float y = shift[v].process (in, ratio[v]);
                wl += y * pl[v];
                wr += y * pr[v];
            }
            wl *= norm; wr *= norm;
            wl = hc.lp (0, wl); wl -= lc.lp (0, wl);
            wr = hc.lp (1, wr); wr -= lc.lp (1, wr);
            l[i] = mixEP (l[i], wl, mix);
            r[i] = mixEP (r[i], wr, mix);
        }
    }

private:
    float sampleRate = 48000.0f;
    PitchShifter shift[kMax];
    float drift[kMax] {};
    filt::OnePole lc, hc;
    Rng rng;
};

// ===========================================================================
class ChorusFx final : public Effect
{
public:
    void prepare (float sr) override
    {
        sampleRate = sr;
        for (auto& d : line) d.prepare ((int) (0.08f * sr));
        reset();
    }
    void reset() override { for (auto& d : line) d.clear(); fb[0] = fb[1] = 0.0f; }
    void process (float* l, float* r, int n, const float* p, bool sync, const FxContext& ctx) noexcept override
    {
        const float base = expMap (p[0], 1.0f, 30.0f) * 0.001f * sampleRate;
        const float depth = p[1] * base * 0.9f;
        const float hz = rateHz (p[2], sync, ctx.bpm, 0.02f, 10.0f);
        const float feedback = linMap (p[3], -0.9f, 0.9f);
        const int voices = 1 + choice (p[4], 4);
        const bool square = flag (p[5]);
        const float stereo = p[6];
        const float mix = p[7];
        float* io[2] = { l, r };
        for (int i = 0; i < n; ++i)
        {
            lfo.tick (hz, sampleRate);
            for (int ch = 0; ch < 2; ++ch)
            {
                float w = 0.0f;
                for (int v = 0; v < voices; ++v)
                {
                    float m = lfo.at ((float) v / (float) voices + (ch == 1 ? 0.25f * stereo : 0.0f));
                    if (square)
                        m = softClip (m * 4.0f);
                    w += line[ch].read (base + depth * m);
                }
                w /= std::sqrt ((float) voices);
                line[ch].push (io[ch][i] + feedback * softClip (fb[ch]));
                fb[ch] = w;
                io[ch][i] = mixEP (io[ch][i], w, mix);
            }
        }
    }

private:
    float sampleRate = 48000.0f, fb[2] {};
    Delay line[2];
    FxLfo lfo;
};

// ===========================================================================
// Vintage Chorus: two bucket-brigade-style lines with the classic I / II / I+II modes.
class VintageChorusFx final : public Effect
{
public:
    void prepare (float sr) override
    {
        sampleRate = sr;
        for (auto& d : line) d.prepare ((int) (0.02f * sr));
        reset();
    }
    void reset() override
    {
        for (auto& d : line) d.clear();
        pre.reset(); post.reset();
    }
    void process (float* l, float* r, int n, const float* p, bool, const FxContext&) noexcept override
    {
        // Mode I: slow and gentle; II: faster and much deeper (about 4x the pitch
        // movement of I); I + II: fast, shallow shimmer. Rate and Depth scale these.
        static constexpr float kRate[3] = { 0.5f, 0.83f, 9.75f };
        static constexpr float kDepthMs[3] = { 1.1f, 2.4f, 0.15f };
        static constexpr float kBaseMs[3] = { 3.5f, 3.5f, 2.5f };
        const int mode = choice (p[0], 3);
        const float hz = kRate[mode] * expMap (p[1], 0.5f, 2.0f);
        const float depth = kDepthMs[mode] * 0.001f * sampleRate * (0.3f + 1.4f * p[2]);
        const float base = kBaseMs[mode] * 0.001f * sampleRate;
        const float phaseOff = 0.5f * p[3]; // 1 = opposite phase on L and R (wide)
        const float mix = p[4];
        // Width: 0 = each side its own chorus line; 1 = the classic sum / difference
        // output (right side gets the inverted chorus), very wide in stereo and
        // the chorus folds back to the dry sound in mono.
        // (needs the dry signal: fades out as Dry/Wet goes from 50 % to 100 %)
        const float width = std::clamp (p[5], 0.0f, 1.0f) * std::clamp (2.0f * (1.0f - mix), 0.0f, 1.0f);
        pre.set (filt::prewarp (9000.0f, sampleRate));
        post.set (filt::prewarp (8000.0f, sampleRate));
        for (int i = 0; i < n; ++i)
        {
            lfo.tick (hz, sampleRate, true);
            const float in = pre.lp (0, 0.5f * (l[i] + r[i]));
            line[0].push (softClip (in * 1.2f) / 1.2f);
            float wl = line[0].read (base + depth * lfo.at (0.0f, true));
            float wr = line[0].read (base + depth * lfo.at (phaseOff, true));
            wl = post.lp (0, wl);
            wr = post.lp (1, wr);
            wr += width * (-wl - wr);
            l[i] = mixEP (l[i], wl, mix);
            r[i] = mixEP (r[i], wr, mix);
        }
    }

private:
    float sampleRate = 48000.0f;
    Delay line[2];
    filt::OnePole pre, post;
    FxLfo lfo;
};

// ===========================================================================
class FlangerFx final : public Effect
{
public:
    explicit FlangerFx (bool vintage) : vintage (vintage) {}
    void prepare (float sr) override
    {
        sampleRate = sr;
        for (auto& d : line) d.prepare ((int) (0.025f * sr));
        reset();
    }
    void reset() override
    {
        for (auto& d : line) d.clear();
        lc.reset(); hc.reset();
    }
    void process (float* l, float* r, int n, const float* p, bool sync, const FxContext& ctx) noexcept override
    {
        float base, depth, hz, feedback, stereo, mix;
        bool tri, monoIn = false;
        if (! vintage)
        {
            base = expMap (p[0], 0.1f, 10.0f) * 0.001f * sampleRate;
            depth = p[1];
            hz = rateHz (p[2], sync, ctx.bpm, 0.02f, 10.0f);
            feedback = linMap (p[3], -0.95f, 0.95f);
            lc.set (filt::prewarp (expMap (p[4], 20.0f, 2000.0f), sampleRate));
            hc.set (filt::prewarp (expMap (p[5], 1000.0f, 20000.0f), sampleRate));
            stereo = p[6] * 0.5f;
            tri = flag (p[7]);
            mix = p[8];
        }
        else
        {
            hz = rateHz (p[0], sync, ctx.bpm, 0.02f, 8.0f);
            base = expMap (p[1], 0.3f, 8.0f) * 0.001f * sampleRate;
            feedback = p[2] * 0.95f;
            depth = p[3];
            stereo = p[4] * 0.25f;
            monoIn = flag (p[5]);
            mix = p[6];
            tri = true;
            lc.set (filt::prewarp (30.0f, sampleRate));
            hc.set (filt::prewarp (6500.0f, sampleRate));
        }
        float* io[2] = { l, r };
        for (int i = 0; i < n; ++i)
        {
            lfo.tick (hz, sampleRate, tri);
            const float mono = 0.5f * (l[i] + r[i]);
            for (int ch = 0; ch < 2; ++ch)
            {
                const float m = 0.5f * (1.0f + lfo.at (ch == 1 ? stereo : 0.0f, tri)); // 0..1
                const float d = 1.0f + base * (1.0f - depth + depth * m);
                float w = line[ch].read (d);
                w = hc.lp (ch, w);
                w -= lc.lp (ch, w);
                const float in = monoIn ? mono : io[ch][i];
                line[ch].push (in + feedback * (vintage ? softClip (w) : w));
                io[ch][i] = mixDW (io[ch][i], w, mix);
            }
        }
    }

private:
    bool vintage;
    float sampleRate = 48000.0f;
    Delay line[2];
    filt::OnePole lc, hc;
    FxLfo lfo;
};

// ===========================================================================
class PhaserFx final : public Effect
{
public:
    void prepare (float sr) override { sampleRate = sr; reset(); }
    void reset() override
    {
        for (auto& a : ap) a.reset();
        fb[0] = fb[1] = 0.0f;
    }
    void process (float* l, float* r, int n, const float* p, bool sync, const FxContext& ctx) noexcept override
    {
        const float centre = expMap (p[0], 100.0f, 5000.0f);
        const float feedback = linMap (p[1], -0.9f, 0.9f);
        const int wave = choice (p[2], 6);
        const float depth = p[3] * 2.0f; // octaves either side
        const float hz = rateHz (p[4], sync, ctx.bpm, 0.02f, 10.0f);
        const int poles = 2 + 2 * choice (p[5], 6);
        const float stereo = p[6] * 0.5f;
        const float mix = p[7];
        float* io[2] = { l, r };
        for (int i = 0; i < n; i += kBlock)
        {
            const int len = std::min (kBlock, n - i);
            // LFO at control rate; coefficients per block.
            float m[2];
            for (int k = 0; k < len; ++k)
                lfo.tick (hz, sampleRate);
            for (int ch = 0; ch < 2; ++ch)
            {
                const float ph = wrap01 (lfo.phase + (ch == 1 ? stereo : 0.0f));
                switch (wave)
                {
                    case 0: m[ch] = SineTable::get() (ph); break;
                    case 1: m[ch] = ph < 0.5f ? 4.0f * ph - 1.0f : 3.0f - 4.0f * ph; break;
                    case 2: m[ch] = softClip (4.0f * SineTable::get() (ph)); break;
                    case 3: m[ch] = 1.0f - 2.0f * ph; break;
                    case 4: m[ch] = 2.0f * ph - 1.0f; break;
                    default:
                        if (ph < lastPh[ch]) target[ch] = rng.bi();
                        m[ch] = target[ch];
                        break;
                }
                lastPh[ch] = ph;
                const float g = filt::prewarp (centre * std::exp2 (depth * m[ch]), sampleRate);
                for (int s = 0; s < poles; ++s)
                    ap[ch * 12 + s].set (g);
            }
            for (int k = i; k < i + len; ++k)
                for (int ch = 0; ch < 2; ++ch)
                {
                    float y = io[ch][k] + feedback * softClip (fb[ch]);
                    for (int s = 0; s < poles; ++s)
                        y = ap[ch * 12 + s].ap (0, y);
                    fb[ch] = y;
                    io[ch][k] = mixDW (io[ch][k], 0.5f * (io[ch][k] + y), mix * 2.0f > 1.0f ? 1.0f : mix * 2.0f);
                }
        }
    }

private:
    float sampleRate = 48000.0f, fb[2] {}, lastPh[2] {}, target[2] {};
    filt::OnePole ap[24]; // 12 per channel (each uses its channel-0 state)
    FxLfo lfo;
    Rng rng;
};


// ===========================================================================
// Rotary speaker: a tube-ish preamp, an 800 Hz crossover, a rotating horn (highs)
// and drum (lows). Each rotor is a Doppler delay plus amplitude modulation,
// picked up by two "mics" (Spread = angle between them). Speed: Slow / Fast,
// Brake stops it; the horn changes speed quickly, the heavier drum slowly (Ramp scales both).
class RotaryFx final : public Effect
{
public:
    void prepare (float sr) override
    {
        sampleRate = sr;
        for (auto& d : line)
            d.prepare ((int) (0.006f * sr) + 8);
        xover.set (filt::prewarp (800.0f, sr));
        xover2.set (filt::prewarp (800.0f, sr));
        cab.set (filt::prewarp (7000.0f, sr));
        reset();
    }
    void reset() override
    {
        for (auto& d : line)
            d.clear();
        xover.reset(); xover2.reset(); cab.reset();
        hornHz = drumHz = -1.0f;
    }
    void process (float* l, float* r, int n, const float* p, bool, const FxContext&) noexcept override
    {
        // Speed is a switch (>= 0.5 = fast) so a mod wheel or a CC can flip it; Brake stops the rotors.
        const int speed = flag (p[8]) ? 2 : (flag (p[0]) ? 1 : 0); // 0 slow, 1 fast, 2 stopped
        const float slowHorn = expMap (p[1], 0.3f, 1.6f), fastHorn = expMap (p[2], 4.0f, 8.5f);
        const float targetHorn = speed == 0 ? slowHorn : speed == 1 ? fastHorn : 0.0f;
        const float targetDrum = speed == 0 ? slowHorn * 0.85f : speed == 1 ? fastHorn * 0.87f : 0.0f;
        if (hornHz < 0.0f) // first block: start at speed
        {
            hornHz = targetHorn;
            drumHz = targetDrum;
        }
        const float ramp = expMap (p[3], 0.3f, 4.0f);
        const float dt = (float) n / sampleRate;
        hornHz += (targetHorn - hornHz) * (1.0f - std::exp (-dt / (0.3f * ramp)));
        drumHz += (targetDrum - drumHz) * (1.0f - std::exp (-dt / (1.3f * ramp)));
        const float hornLevel = std::min (1.0f, 2.0f * p[4]), drumLevel = std::min (1.0f, 2.0f * (1.0f - p[4]));
        const float drive = p[5];
        const float g = 1.0f + 9.0f * drive * drive;
        const float makeup = 1.0f / softClip (0.5f * g) * 0.5f;
        const float micOffset = 0.5f * std::clamp (p[6], 0.0f, 1.0f); // up to half a turn apart
        const float mix = p[7];
        const float base = 0.0012f * sampleRate, hornDepth = 0.00045f * sampleRate, drumDepth = 0.00025f * sampleRate;
        const float hornInc = hornHz / sampleRate, drumInc = drumHz / sampleRate;
        auto& sine = SineTable::get();
        for (int i = 0; i < n; ++i)
        {
            float x = 0.5f * (l[i] + r[i]);
            if (drive > 0.0f)
                x = (softClip (x * g + 0.1f * drive) - softClip (0.1f * drive)) * makeup;
            const float lo = xover2.lp (0, xover.lp (0, x));
            const float hi = x - lo;
            line[0].push (hi);
            line[1].push (lo);
            hornPh += hornInc; hornPh -= std::floor (hornPh);
            drumPh += drumInc; drumPh -= std::floor (drumPh);
            float out[2];
            for (int ch = 0; ch < 2; ++ch)
            {
                const float off = ch == 0 ? 0.0f : micOffset;
                const float hs = sine (wrap01 (hornPh + off)), hc = sine (wrap01 (hornPh + off + 0.25f));
                const float ds = sine (wrap01 (drumPh + off)), dc = sine (wrap01 (drumPh + off + 0.25f));
                const float horn = line[0].read (base + hornDepth * hs) * (1.0f - 0.55f * (0.5f - 0.5f * hc));
                const float drum = line[1].read (base + drumDepth * ds) * (1.0f - 0.3f * (0.5f - 0.5f * dc));
                out[ch] = cab.lp (ch, 1.25f * (hornLevel * horn + drumLevel * drum));
            }
            l[i] = mixEP (l[i], out[0], mix);
            r[i] = mixEP (r[i], out[1], mix);
        }
    }

private:
    float sampleRate = 48000.0f, hornHz = -1.0f, drumHz = -1.0f, hornPh = 0.0f, drumPh = 0.3f;
    Delay line[2];
    filt::OnePole xover, xover2, cab;
};

// ===========================================================================
class PannerFx final : public Effect
{
public:
    void prepare (float sr) override { sampleRate = sr; reset(); }
    void reset() override { bass.reset(); }
    void process (float* l, float* r, int n, const float* p, bool sync, const FxContext& ctx) noexcept override
    {
        const float amount = p[0];
        const float hz = rateHz (p[1], sync, ctx.bpm, 0.02f, 20.0f);
        const float inv = flag (p[2]) ? -1.0f : 1.0f;
        const bool monoBass = flag (p[3]);
        bass.set (filt::prewarp (expMap (p[4], 50.0f, 500.0f), sampleRate));
        const bool natural = flag (p[5]);
        const int shape = choice (p[6], 3);
        for (int i = 0; i < n; ++i)
        {
            float m = lfo.tick (hz, sampleRate, shape == 1);
            if (shape == 2) m = softClip (m * 5.0f);
            const float pos = std::clamp (m * amount * inv, -1.0f, 1.0f);
            float gl, gr;
            if (natural)
            {
                gl = SineTable::get() (0.125f * (1.0f - pos) + 0.0f) * 1.41421356f; // cos-law, unity at centre
                gr = SineTable::get() (0.125f * (1.0f + pos)) * 1.41421356f;
            }
            else
                panGains (pos, gl, gr);
            float lowL = 0.0f, lowR = 0.0f, xl = l[i], xr = r[i];
            if (monoBass)
            {
                lowL = bass.lp (0, xl);
                lowR = bass.lp (1, xr);
                xl -= lowL;
                xr -= lowR;
            }
            const float lowMono = 0.5f * (lowL + lowR);
            l[i] = xl * gl + lowMono;
            r[i] = xr * gr + lowMono;
        }
    }

private:
    float sampleRate = 48000.0f;
    filt::OnePole bass;
    FxLfo lfo;
};
} // namespace

const FxInfo& fxInfo (int type) noexcept
{
    return kFxInfo[std::clamp (type, 0, (int) FxTypeCount - 1)];
}

std::unique_ptr<Effect> makeEffect (int type, float sampleRate)
{
    std::unique_ptr<Effect> e;
    switch (type)
    {
        case FxDelay: e = std::make_unique<DelayFx> (false); break;
        case FxTapeEcho: e = std::make_unique<DelayFx> (true); break;
        case FxPitchDelay: e = std::make_unique<PitchDelayFx>(); break;
        case FxReverb: e = std::make_unique<ReverbFx> (false); break;
        case FxShimmer: e = std::make_unique<ReverbFx> (true); break;
        case FxCompressor: e = std::make_unique<CompressorFx>(); break;
        case FxMultiband: e = std::make_unique<MultibandFx>(); break;
        case FxMultiFilter: e = std::make_unique<MultiFilterFx>(); break;
        case FxParamEq: e = std::make_unique<ParamEqFx>(); break;
        case FxDistortion: e = std::make_unique<DistortionFx>(); break;
        case FxBitcrusher: e = std::make_unique<BitcrusherFx>(); break;
        case FxErode: e = std::make_unique<ErodeFx>(); break;
        case FxSuperUnison: e = std::make_unique<SuperUnisonFx>(); break;
        case FxChorus: e = std::make_unique<ChorusFx>(); break;
        case FxVintageChorus: e = std::make_unique<VintageChorusFx>(); break;
        case FxFlanger: e = std::make_unique<FlangerFx> (false); break;
        case FxVintageFlanger: e = std::make_unique<FlangerFx> (true); break;
        case FxPhaser: e = std::make_unique<PhaserFx>(); break;
        case FxPanner: e = std::make_unique<PannerFx>(); break;
        case FxRotary: e = std::make_unique<RotaryFx>(); break;
        default: return nullptr;
    }
    e->type = type;
    e->prepare (sampleRate);
    return e;
}
} // namespace cs::fx
