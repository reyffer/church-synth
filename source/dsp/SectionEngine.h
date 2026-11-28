#pragma once

// Section engine: ensemble strings and brass (SPEC §3.5). One key plays several
// independent players, then their sum goes through a fixed body / bell resonator.
// Our own model of how a section works, not of any product:
//
//  - Players (1..8): each a band-limited saw (strings) or pulse (brass) (BlepOscillator) with its own seeded
//    tuning offset (Intonation), slow drift, vibrato rate / depth / phase, entry
//    time (Looseness) and seat in the stereo field (Width around Position). The
//    seeds come from the key, so a chord always sounds the same.
//  - Dynamics drives level, brightness and vibrato depth together. Each player has
//    a 12 dB low-pass whose cutoff follows Dynamics (and Brightness); for Brass the
//    cutoff rises faster, the pulse narrows (a flatter spectrum, like lips buzzing
//    harder) and a soft saturator adds a little more, so loud brass gets much
//    brighter, not just louder.
//  - Attack: Strings = Attack (soft .. marcato) + Bite (bow-pressure burst of
//    brightness and noise); Brass = Attack (soft .. sforzando, with an overshoot at
//    the top), Scoop (pitch starts below and rises in 30-80 ms) and Blat (a short
//    bright burst on loud attacks).
//  - Noise: bow (strings) or breath (brass), band-passed around the note.
//  - Body / bell: a parallel bank of fixed band-pass resonators (16 for the string
//    bodies, 8 for the bells) plus some direct signal, normalised to unity overall.
//    Vibrato moves each harmonic through these fixed peaks: the shimmer of real
//    strings and the bloom of brass. Stereo (one bank per side) so the seating stays.
//  - Legato (Mono Legato voicing): the slur dips brightness and level briefly.
//
// No unison (the players replace it). A hard cap of 64 player oscillators per
// instance: players per note = min(Players, 64 / max voices).

#include "MathUtil.h"
#include "Oscillator.h"
#include "Params.h"

#include <array>
#include <cmath>

namespace cs
{
namespace sectiondsp
{
constexpr int kMaxPlayers = 8;
constexpr int kMaxPeaks = 16;
constexpr int kPlayerBudget = 64; // per instance

struct Peak
{
    float hz, db, q;
};

struct BodyShape
{
    int count = 0;
    Peak peak[kMaxPeaks] {};
    float direct = 0.25f; // dry part inside the body output
    float norm = 1.0f;    // makes the whole response unity on average (log frequency, 100 Hz - 8 kHz)
};

// Body shapes, in SectionBody order (append only). Our own response shapes.
class Bodies
{
public:
    static const Bodies& get()
    {
        static const Bodies b;
        return b;
    }
    const BodyShape& shape (int i) const noexcept { return s[(size_t) std::clamp (i, 0, SectionBodyCount - 1)]; }

private:
    std::array<BodyShape, SectionBodyCount> s {};

    static void scaled (BodyShape& dst, const BodyShape& src, float fScale, float tiltDbPerOct, float qScale)
    {
        dst.count = src.count;
        dst.direct = src.direct;
        for (int k = 0; k < src.count; ++k)
        {
            auto p = src.peak[k];
            const float f = p.hz * fScale;
            p.db += tiltDbPerOct * std::log2 (std::max (1.0f, f / 500.0f));
            p.hz = f;
            p.q *= qScale;
            dst.peak[k] = p;
        }
    }

    Bodies()
    {
        // Violin: air mode, the main wood modes, a bridge "hill" around 2-3 kHz, dark top.
        static const Peak violin[16] = { { 275, 0, 12 }, { 405, -4, 10 }, { 470, 2, 14 }, { 550, 3, 12 },
                                         { 700, -3, 9 }, { 900, -1, 8 }, { 1150, 0, 7 }, { 1400, -2, 8 },
                                         { 1750, 0, 6 }, { 2100, 3, 5 }, { 2500, 5, 4 }, { 2950, 4, 4 },
                                         { 3500, 1, 5 }, { 4300, -2, 5 }, { 5200, -5, 4 }, { 6500, -9, 3 } };
        auto& v = s[SectionBodyViolin];
        v.count = 16;
        for (int k = 0; k < 16; ++k)
            v.peak[k] = violin[k];
        scaled (s[SectionBodyViola], v, 0.8f, -1.0f, 1.0f);
        scaled (s[SectionBodyCello], v, 0.45f, -1.5f, 0.9f);
        s[SectionBodyCello].direct = 0.35f;
        scaled (s[SectionBodyBass], v, 0.3f, -2.0f, 0.8f);
        s[SectionBodyBass].direct = 0.45f;
        // Ensemble: violin and cello modes interleaved, broader, so no one body dominates.
        {
            auto& e = s[SectionBodyEnsemble];
            e.count = 16;
            e.direct = 0.35f;
            for (int k = 0; k < 8; ++k)
            {
                auto a = v.peak[2 * k + 1], b = s[SectionBodyCello].peak[2 * k];
                a.q *= 0.55f;
                b.q *= 0.55f;
                e.peak[2 * k] = b;
                e.peak[2 * k + 1] = a;
            }
        }
        auto set8 = [this] (int i, std::initializer_list<Peak> l, float direct) {
            auto& b = s[(size_t) i];
            b.count = 0;
            b.direct = direct;
            for (const auto& p : l)
                b.peak[b.count++] = p;
        };
        // Bells: a broad formant that moves down from trumpet to horn; the mute is nasal.
        set8 (SectionBodyTrumpet, { { 180, -8, 1.0f }, { 400, -3, 1.5f }, { 800, 1, 2 }, { 1300, 5, 2.5f },
                                    { 1900, 3, 3 }, { 2800, 1, 3 }, { 4000, -2, 3 }, { 6000, -7, 2 } }, 0.2f);
        set8 (SectionBodyTrombone, { { 120, -3, 1.2f }, { 300, 1, 1.5f }, { 550, 4, 2 }, { 900, 3, 2.5f },
                                     { 1400, 0, 3 }, { 2200, -2, 3 }, { 3300, -5, 3 }, { 5000, -10, 2 } }, 0.25f);
        set8 (SectionBodyHorn, { { 100, -2, 1.2f }, { 250, 2, 1.5f }, { 420, 5, 2 }, { 700, 2, 2.5f },
                                 { 1100, -2, 3 }, { 1700, -6, 3 }, { 2600, -10, 3 }, { 4000, -15, 2 } }, 0.2f);
        set8 (SectionBodyMuted, { { 200, -14, 1.0f }, { 500, -8, 2 }, { 1000, -2, 3 }, { 1700, 8, 6 },
                                  { 2300, 3, 5 }, { 3200, -1, 4 }, { 4500, -5, 3 }, { 7000, -10, 2 } }, 0.05f);
        for (auto& b : s)
            normalise (b);
    }

    // Average power of the analog prototype response over log frequency -> 1.
    static void normalise (BodyShape& b)
    {
        double sum = 0.0;
        constexpr int n = 160;
        for (int i = 0; i < n; ++i)
        {
            const double f = 100.0 * std::pow (80.0, (double) i / (n - 1));
            double re = b.direct, im = 0.0;
            for (int k = 0; k < b.count; ++k)
            {
                const auto& p = b.peak[k];
                const double g = std::pow (10.0, p.db / 20.0), x = f / p.hz, a = x / p.q;
                // H = j a / (1 - x^2 + j a)
                const double dr = 1.0 - x * x, den = dr * dr + a * a;
                re += g * (a * a) / den;
                im += g * (a * dr) / den;
            }
            sum += re * re + im * im;
        }
        b.norm = (float) (1.0 / std::sqrt (sum / n));
    }
};
} // namespace sectiondsp

class SectionEngine
{
public:
    void setSampleRate (float sr) noexcept
    {
        sampleRate = sr;
        (void) sectiondsp::Bodies::get(); // build the shared shapes off the audio thread
        loadedBody = -1;
    }

    void startNote (const EngineParams&, uint32_t seed) noexcept
    {
        rng.seed (seed ^ 0xC0FFEEu);
        fresh = true;
        for (auto& pl : player)
        {
            pl.amp = 0.0f;
            pl.lp1[0] = pl.lp1[1] = pl.lp2[0] = pl.lp2[1] = 0.0f;
            pl.osc.reset (rng.uni());
        }
        for (auto& c : bp)
            for (auto& st : c)
                st = {};
        nsv = {};
        dcX[0] = dcX[1] = dcY[0] = dcY[1] = 0.0f;
        dynSm = -1.0f;
        slurEnv = 0.0f;
        restrike();
    }

    // Played again on a sounding voice (not legato): a new stroke, from the current level.
    void restrike() noexcept
    {
        tNote = 0;
        attackEnv = 1.0f;
    }

    // Legato note change (Mono Legato): the slur.
    void slur() noexcept { slurEnv = 1.0f; }

    // Renders n samples (n <= kBlock) and ADDS them to outL/outR. modOut gets the mono
    // output before the engine Volume. meter (optional): [0] players, [1] body, [3] noise.
    CS_NOINLINE void render (const EngineParams& p, float notePitch, float fixedPitch, int n, int maxVoices, float* outL, float* outR,
                 float* modOut, float* meter) noexcept
    {
        if (! p.on)
            return;
        const auto& sp = p.sec;
        const bool brass = sp.family == SectionBrass;
        const float keyF = (p.keytrack ? notePitch : fixedPitch) + p.coarse + p.fine;
        const int cap = std::max (1, sectiondsp::kPlayerBudget / std::max (1, maxVoices));
        const int np = std::clamp ((int) std::lround (sp.players), 1, std::min (sectiondsp::kMaxPlayers, cap));
        if (fresh || np != lastPlayers)
            seatPlayers (keyF, np, sp);
        if (sp.body != loadedBody)
            loadBody (sp.body);

        const float invSr = 1.0f / sampleRate;
        const float dt = (float) n * invSr;
        const float t = (float) tNote * invSr;

        // --- Dynamics (smoothed: a mod wheel moves in steps) ----------------------------
        const float dynTarget = std::clamp (sp.dynamics, 0.0f, 1.0f);
        if (dynSm < 0.0f)
            dynSm = dynTarget;
        const float dyn0 = dynSm;
        dynSm += (dynTarget - dynSm) * (1.0f - std::exp (-dt / 0.02f));
        const float dyn = dynSm;

        // Envelopes of the attack (control rate).
        const float bite = std::clamp (sp.bite, 0.0f, 1.0f);
        attackEnv *= std::exp (-dt / (brass ? 0.04f : 0.05f));
        slurEnv *= std::exp (-dt / 0.045f);

        // Level from Dynamics: about 18 dB (strings) / 22 dB (brass) from pp to ff.
        auto level = [brass] (float d) { return std::exp2 (-(brass ? 22.0f : 18.0f) / 6.0206f * (1.0f - d)); };
        const float slurLvl = 1.0f - 0.22f * slurEnv;
        const float lv0 = level (dyn0) * slurLvl, lv1 = level (dyn) * slurLvl;

        // Brightness: player low-pass cutoff in harmonics of the note.
        const float f0 = pitchToHz (keyF);
        const float bright = std::clamp (sp.brightness, 0.0f, 1.0f) - 0.5f;
        float oct = brass ? 0.3f + 6.7f * std::pow (dyn, 1.5f) : 1.5f + 4.5f * std::pow (dyn, 0.8f);
        oct += 3.0f * bright;
        oct += (brass ? 3.0f * dyn * dyn : 2.0f) * bite * attackEnv; // blat / bite
        oct -= 1.5f * slurEnv;
        const float fc = std::clamp (f0 * std::exp2 (oct), 60.0f, 0.45f * sampleRate);
        const float a = 1.0f - std::exp (-kTwoPi * fc * invSr);
        // Brass: the lips make a pulse that narrows as the player blows harder (a flat, bright
        // spectrum at ff), power-normalised so Dynamics' level curve stays in charge.
        const float duty = brass ? 0.5f - 0.42f * std::pow (dyn, 1.2f) : 0.5f;
        const float pulseNorm = brass ? 0.577f / (2.0f * std::sqrt (duty * (1.0f - duty))) : 1.0f;
        const int wave = brass ? WaveSquare : WaveSaw;
        const float drive = brass ? 1.0f + 2.0f * dyn * dyn * (1.0f + 2.0f * bite * attackEnv) : 0.0f;
        const float driveNorm = brass ? 1.0f / softClip (std::min (3.0f, drive)) : 1.0f;

        // Player attack (per player, after its entry delay).
        const float att = std::clamp (sp.attack, 0.0f, 1.0f);
        const float tau = (brass ? 0.12f : 0.18f) * std::exp2 (-3.9f * att); // soft .. hard
        const float attCoef = 1.0f - std::exp (-dt / tau);
        const float sfz = brass ? std::max (0.0f, att - 0.7f) / 0.3f * 0.6f * dyn : 0.0f;

        // Vibrato (cents) and its fade-in.
        const float vd = std::clamp (sp.vibDepth, 0.0f, 1.0f) * (brass ? 14.0f : 28.0f) * (0.55f + 0.45f * dyn);
        const float vdelay = std::max (0.0f, sp.vibDelay);
        const float fade = vdelay <= 0.0f ? 1.0f : std::clamp ((t - 0.3f * vdelay) / vdelay, 0.0f, 1.0f);
        const float vrate = 4.0f + 3.0f * std::clamp (sp.vibRate, 0.0f, 1.0f);
        const float into = std::clamp (sp.intonation, 0.0f, 1.0f);
        const float scoopCents = brass ? -300.0f * std::clamp (sp.scoop, 0.0f, 1.0f) : 0.0f;
        const float scoopTau = (0.03f + 0.05f * (1.0f - dyn)) / 3.0f;

        // Seating (control rate, so Width / Position can be modulated): equal power.
        const float width = std::clamp (sp.width, 0.0f, 1.0f), pos = std::clamp (sp.position, -1.0f, 1.0f);
        auto panGainsEP = [] (float pan, float& l, float& r) {
            const float ang = (std::clamp (pan, -1.0f, 1.0f) + 1.0f) * (kPi * 0.25f);
            l = std::cos (ang);
            r = std::sin (ang);
        };
        float npL, npR;
        panGainsEP (pos, npL, npR);

        float dryL[kBlock] {}, dryR[kBlock] {};
        const float norm = 1.0f / std::sqrt ((float) np);
        for (int k = 0; k < np; ++k)
        {
            auto& pl = player[k];
            const float tp = t - pl.delay;
            // Pitch for this block.
            pl.vibPh = wrap01 (pl.vibPh + vrate * pl.vibRate * dt);
            pl.driftPh = wrap01 (pl.driftPh + pl.driftRate * dt);
            float cents = into * (7.5f * pl.detune + 3.0f * SineTable::get() (pl.driftPh));
            cents += vd * pl.vibDepth * fade * SineTable::get() (pl.vibPh);
            if (scoopCents < 0.0f && tp >= 0.0f)
                cents += scoopCents * std::exp (-tp / scoopTau);
            const float inc = std::min (f0 * std::exp2 (cents * (1.0f / 1200.0f)) * invSr, 0.45f);
            // Level: rises after the entry delay, sforzando overshoot on hard brass.
            const float amp0 = pl.amp;
            if (tp >= 0.0f)
            {
                const float target = 1.0f + sfz * std::exp (-tp / 0.25f);
                pl.amp += (target - pl.amp) * attCoef;
            }
            float g = amp0 * norm, gStep = (pl.amp - amp0) * norm / (float) n;
            float gl, gr;
            panGainsEP (pos + width * 0.85f * pl.seat, gl, gr);
            float s1 = pl.lp1[0], s2 = pl.lp2[0];
            for (int i = 0; i < n; ++i)
            {
                float x = pl.osc.tick (inc, wave, duty) * pulseNorm;
                s1 += a * (x - s1);
                s2 += a * (s1 - s2);
                x = s2;
                if (brass)
                    x = softClip (x * drive) * driveNorm;
                x *= g;
                g += gStep;
                dryL[i] += x * gl;
                dryR[i] += x * gr;
            }
            pl.lp1[0] = s1;
            pl.lp2[0] = s2;
        }

        // Brass: the saturated pulse is asymmetric, so block its DC (about 8 Hz).
        if (brass)
        {
            const float rdc = 1.0f - kTwoPi * 8.0f * invSr;
            float* ch[2] = { dryL, dryR };
            for (int c = 0; c < 2; ++c)
                for (int i = 0; i < n; ++i)
                {
                    const float x = ch[c][i];
                    const float y = x - dcX[c] + rdc * dcY[c];
                    dcX[c] = x;
                    dcY[c] = y;
                    ch[c][i] = y;
                }
        }

        // --- noise: bow / breath around the note ------------------------------------------
        float nz[kBlock] {};
        const float noiseAmt = std::clamp (sp.noise, 0.0f, 1.0f);
        bool anyNoise = false;
        if (noiseAmt > 0.0f)
        {
            const float nf = std::clamp (f0 * (brass ? 3.0f : 2.0f), 150.0f, 0.4f * sampleRate);
            const float g = std::tan (kPi * nf * invSr), kq = brass ? 1.3f : 0.8f;
            const float a1 = 1.0f / (1.0f + g * (g + kq)), a2 = g * a1, a3 = g * a2;
            const float boost = 1.0f + 3.0f * bite * attackEnv + (brass ? -0.5f : 1.5f) * slurEnv;
            const float avgAmp = player[0].amp;
            const float lvl = noiseAmt * 0.35f * (0.2f + 0.8f * dyn) * boost * std::min (1.0f, avgAmp + attackEnv * 0.5f);
            for (int i = 0; i < n; ++i)
            {
                const float x = rng.bi();
                const float v3 = x - nsv.ic2;
                const float v1 = a1 * nsv.ic1 + a2 * v3;
                const float v2 = nsv.ic2 + a2 * nsv.ic1 + a3 * v3;
                nsv.ic1 = 2.0f * v1 - nsv.ic1;
                nsv.ic2 = 2.0f * v2 - nsv.ic2;
                // band around the note plus a little air above it
                nz[i] = (v1 * 0.8f + (x - kq * v1 - v2) * 0.12f) * lvl;
            }
            anyNoise = true;
        }

        // --- body / bell -----------------------------------------------------------------
        const float mix = std::clamp (sp.bodyMix, 0.0f, 1.0f);
        constexpr float kOut = 2.0f;
        float pkP = 0.0f, pkB = 0.0f, pkN = 0.0f;
        float lvl = lv0;
        const float lvStep = (lv1 - lv0) / (float) n;
        for (int i = 0; i < n; ++i)
        {
            float x[2] = { dryL[i] + nz[i] * npL, dryR[i] + nz[i] * npR };
            float y[2];
            for (int ch = 0; ch < 2; ++ch)
            {
                float body = direct * x[ch];
                if (mix > 0.0f)
                {
                    auto* st = bp[ch];
                    for (int k = 0; k < peaks; ++k)
                    {
                        const float v = b0[k] * x[ch] + st[k].s1;
                        st[k].s1 = -a1c[k] * v + st[k].s2;
                        st[k].s2 = -b0[k] * x[ch] - a2c[k] * v;
                        body += gk[k] * v;
                    }
                }
                y[ch] = (x[ch] + mix * (body * bodyNorm - x[ch])) * lvl * kOut;
                if (meter != nullptr)
                    pkB = std::max (pkB, std::abs (body * bodyNorm * mix * lvl * kOut));
            }
            lvl += lvStep;
            outL[i] += y[0] * p.volume;
            outR[i] += y[1] * p.volume;
            modOut[i] = 0.5f * (y[0] + y[1]);
            if (meter != nullptr)
            {
                pkP = std::max (pkP, std::abs (0.5f * (dryL[i] + dryR[i])) * lvl * kOut);
                pkN = std::max (pkN, std::abs (nz[i]) * lvl * kOut);
            }
        }
        if (meter != nullptr)
        {
            const float v = std::abs (p.volume);
            meter[0] = pkP * v;
            meter[1] = pkB * v;
            meter[2] = 0.0f;
            meter[3] = anyNoise ? pkN * v : 0.0f;
        }
        tNote = std::min (tNote + n, 1 << 30);
    }

private:
    struct Player
    {
        BlepOscillator osc;
        float detune = 0.0f, driftRate = 0.1f, driftPh = 0.0f, vibRate = 1.0f, vibDepth = 1.0f, vibPh = 0.0f;
        float delay = 0.0f, seat = 0.0f, amp = 0.0f;
        float lp1[2] {}, lp2[2] {};
    };
    struct BpState
    {
        float s1 = 0.0f, s2 = 0.0f;
    };
    struct NoiseSvf
    {
        float ic1 = 0.0f, ic2 = 0.0f;
    };

    float sampleRate = 48000.0f;
    Player player[sectiondsp::kMaxPlayers];
    BpState bp[2][sectiondsp::kMaxPeaks];
    float b0[sectiondsp::kMaxPeaks] {}, a1c[sectiondsp::kMaxPeaks] {}, a2c[sectiondsp::kMaxPeaks] {}, gk[sectiondsp::kMaxPeaks] {};
    int peaks = 0, loadedBody = -1, lastPlayers = 0;
    float direct = 1.0f, bodyNorm = 1.0f;
    NoiseSvf nsv;
    float dcX[2] {}, dcY[2] {};
    int tNote = 0;
    bool fresh = true;
    float dynSm = -1.0f, attackEnv = 0.0f, slurEnv = 0.0f;
    Rng rng;

    // Each player's own values, seeded from the key: a chord always sounds the same.
    void seatPlayers (float keyF, int np, const SectionParams& sp) noexcept
    {
        Rng r;
        r.seed (0x9E3779B9u ^ (uint32_t) ((int) std::lround (keyF) + 1000) * 2654435761u);
        float minDelay = 1.0e9f;
        const float loose = std::clamp (sp.looseness, 0.0f, 1.0f) * 0.04f;
        // Seats: spread evenly over the width, in a shuffled order (tuning and seat unrelated).
        int seat[sectiondsp::kMaxPlayers];
        for (int k = 0; k < np; ++k)
            seat[k] = k;
        for (int k = np - 1; k > 0; --k)
            std::swap (seat[k], seat[(int) (r.uni() * (float) (k + 1)) % (k + 1)]);
        for (int k = 0; k < np; ++k)
        {
            auto& pl = player[k];
            // Detunes spread across -1..1 (evenly, then jittered) so even few players differ.
            pl.detune = np == 1 ? 0.0f : std::clamp ((2.0f * ((float) k + 0.5f) / (float) np - 1.0f) + 0.25f * r.bi(), -1.0f, 1.0f);
            pl.driftRate = 0.05f + 0.15f * r.uni();
            pl.driftPh = r.uni();
            pl.vibRate = 1.0f + 0.08f * r.bi();
            pl.vibDepth = 0.7f + 0.6f * r.uni();
            pl.vibPh = r.uni();
            pl.delay = loose * r.uni();
            minDelay = std::min (minDelay, pl.delay);
            pl.seat = np == 1 ? 0.0f : 2.0f * (float) seat[k] / (float) (np - 1) - 1.0f;
            if (fresh || k >= lastPlayers)
                pl.amp = 0.0f;
        }
        for (int k = 0; k < np; ++k)
            player[k].delay -= minDelay; // the first player starts at once
        lastPlayers = np;
        fresh = false;
    }

    void loadBody (int bodyIndex) noexcept
    {
        const auto& shape = sectiondsp::Bodies::get().shape (bodyIndex);
        peaks = shape.count;
        direct = shape.direct;
        bodyNorm = shape.norm;
        for (int k = 0; k < peaks; ++k)
        {
            const auto& pk = shape.peak[k];
            const float w = kTwoPi * std::min (pk.hz, 0.45f * sampleRate) / sampleRate;
            const float alpha = std::sin (w) / (2.0f * pk.q);
            const float a0 = 1.0f + alpha;
            b0[k] = alpha / a0;
            a1c[k] = -2.0f * std::cos (w) / a0;
            a2c[k] = (1.0f - alpha) / a0;
            gk[k] = std::pow (10.0f, pk.db / 20.0f);
        }
        loadedBody = bodyIndex;
    }
};
} // namespace cs
