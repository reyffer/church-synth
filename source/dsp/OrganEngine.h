#pragma once

// Organ engine: a tonewheel organ model (SPEC §3.5). Our own model of the
// instrument family, built from how the instrument works:
//
//  - 91 tonewheels, each a near-sine. Frequencies follow the tonewheel gear
//    ratios (20 rev/s motor x 12 gear ratios x 2..256 teeth), slightly off equal
//    temperament; an Equal Temperament switch is available.
//  - Free-running phases: a wheel's phase comes from an instance-wide clock, so a
//    key never starts the same way twice, and drawbars that land on the same
//    wheel (foldback) add exactly in phase.
//  - 9 drawbars (16' 5 1/3' 8' 4' 2 2/3' 2' 1 3/5' 1 1/3' 1'), 0..8, about 3 dB a step.
//  - Foldback: footages above the top wheel fold down an octave; the 16' can fold
//    up in the lowest octave (Bass All the Way Down off).
//  - Wheel Character: 2nd / 3rd harmonics and a fixed level spread per wheel
//    (the lowest octave of wheels is the richest).
//  - Leakage: the wheel sharing each drawbar's generator compartment (4 octaves
//    away) is faintly audible.
//  - Key click: each drawbar's contact closes at its own moment (0..3 ms) with a
//    short noise burst; a softer burst at key up. Click 0 = soft contacts.
//  - Percussion: 2nd or 3rd harmonic on a shared decay (SynthCore keeps it, so it
//    only restarts after every key is up), Fast / Slow, Normal / Soft; Normal
//    lowers the drawbars about 3 dB; optionally mutes the 1' (as the original).
//
// No unison (one tone per key). Mono output. One instance per voice; its state is
// small, so it sits beside the other engine types like they do.

#include "MathUtil.h"
#include "Params.h"

#include <array>

namespace cs
{
namespace organ
{
constexpr int kNumWheels = 91;
constexpr int kFootSemis[kNumDrawbars] = { -12, 7, 0, 12, 19, 24, 28, 31, 36 }; // relative to 8'
constexpr int kLowestWheelNote = 24; // wheel 1 = C1 (MIDI 24), wheel 91 = F#8 (MIDI 114)

struct WheelTable
{
    float hz[kNumWheels + 1] {};    // tonewheel tuning, index 1..91
    float equal[kNumWheels + 1] {}; // equal temperament
    float spread[kNumWheels + 1] {}; // fixed per-wheel level offset -1..1 (scaled by Character)
    static const WheelTable& get()
    {
        static const WheelTable t;
        return t;
    }

private:
    WheelTable()
    {
        // Drive gear / driven gear tooth counts for the 12 notes from C, and teeth doubling per octave.
        static const double ratio[12] = { 85.0 / 104.0, 71.0 / 82.0, 67.0 / 73.0, 105.0 / 108.0, 103.0 / 100.0, 84.0 / 77.0,
                                          74.0 / 64.0, 98.0 / 80.0, 96.0 / 74.0, 88.0 / 64.0, 67.0 / 46.0, 108.0 / 70.0 };
        uint32_t h = 0x2545F491u;
        for (int w = 1; w <= kNumWheels; ++w)
        {
            const int note = (w - 1) % 12, oct = (w - 1) / 12;
            hz[w] = (float) (20.0 * ratio[note] * (double) (2 << oct));
            equal[w] = 440.0f * std::exp2 ((float) (w - 1 + kLowestWheelNote - 69) / 12.0f);
            h ^= h << 13; h ^= h >> 17; h ^= h << 5;
            spread[w] = (float) (h >> 8) * (2.0f / 16777216.0f) - 1.0f;
        }
    }
};

// Drawbar level 0..8 -> gain: 8 = 0 dB, each step about -3 dB, 0 = off (linear below 1).
inline float drawbarGain (float level) noexcept
{
    level = std::clamp (level, 0.0f, 8.0f);
    constexpr float g1 = 0.08838835f; // 2^-3.5, level 1
    return level >= 1.0f ? std::exp2 ((level - 8.0f) * 0.5f) : level * g1;
}

// The wheel a footage of a key plays (1..91), with foldback.
inline int wheelFor (int key, int drawbar, bool bassDown) noexcept
{
    int w = key + kFootSemis[drawbar] - (kLowestWheelNote - 1);
    if (drawbar == 0 && ! bassDown && w >= 1 && w <= 12)
        w += 12; // 16' folds up in the lowest octave
    while (w < 1)
        w += 12;
    while (w > kNumWheels)
        w -= 12;
    return w;
}

inline int wheelForSemis (int key, int semis) noexcept
{
    int w = key + semis - (kLowestWheelNote - 1);
    while (w < 1)
        w += 12;
    while (w > kNumWheels)
        w -= 12;
    return w;
}
} // namespace organ

// What the voice passes the organ each block (from SynthCore, see GlobalMod).
struct OrganContext
{
    double clock = 0.0;               // seconds
    float percFrom = 0.0f, percTo = 0.0f; // shared percussion level over this block
};

class OrganEngine
{
public:
    void setSampleRate (float sr) noexcept
    {
        sampleRate = sr;
        clickLp = 0.5f;
        clickHp = 1.0f - std::exp (-kTwoPi * 400.0f / sr);
    }

    // A new note on an idle voice: phases are taken from the wheel clock on the first block.
    void startNote (const EngineParams&, uint32_t seed) noexcept
    {
        rng.seed (seed ^ 0x51ED27Bu);
        fresh = true;
        released = false;
        for (auto& s : slot)
            s.gain = 0.0f;
        lpState = hpState = burst = 0.0f;
        relBurstAt = -1;
        closeContacts();
    }

    // The key is played again on a sounding voice: contacts reopen and close (a new click).
    void retrigger() noexcept
    {
        released = false;
        relBurstAt = -1;
        closeContacts();
    }

    // Key up: a softer click as the contacts open (the amp envelope ends the sound).
    void noteOff() noexcept
    {
        if (released)
            return;
        released = true;
        relBurstAt = tContact + (int) (rng.uni() * 0.002f * sampleRate);
    }

    // Renders n samples (n <= kBlock) and ADDS them to outL/outR. modOut receives the
    // mono output before the engine Volume (cross-mod / filter FM source).
    // meter (optional): [0] tonewheels, [1] percussion, [3] key click.
    void render (const EngineParams& p, float notePitch, float fixedPitch, int n, float* outL, float* outR,
                 float* modOut, float* meter, const OrganContext& ctx) noexcept
    {
        if (! p.on)
            return;
        const auto& op = p.org;
        const auto& wt = organ::WheelTable::get();
        const float keyF = (p.keytrack ? notePitch : fixedPitch) + p.coarse + p.fine;

        // Which wheels this key plays. Re-based only if the pitch moves far (glide / bend):
        // the frequency ratio covers the rest, so pitch stays continuous.
        const bool rebase = fresh || std::abs (keyF - (float) baseKey) > 0.75f || op.bassDown != lastBassDown
                            || op.percThird != lastPercThird;
        if (rebase)
        {
            baseKey = (int) std::lround (keyF);
            lastBassDown = op.bassDown;
            lastPercThird = op.percThird;
            for (int d = 0; d < kNumDrawbars; ++d)
            {
                const int w = organ::wheelFor (baseKey, d, op.bassDown);
                slot[d].wheel = w;
                const int partner = w + 48 <= organ::kNumWheels ? w + 48 : (w - 48 >= 1 ? w - 48 : 0);
                slot[kLeak0 + d].wheel = partner;
            }
            slot[kPerc].wheel = organ::wheelForSemis (baseKey, op.percThird ? 19 : 12);
            if (fresh)
                for (auto& s : slot)
                    if (s.wheel > 0)
                    {
                        const double ph = (double) wt.hz[s.wheel] * ctx.clock;
                        s.phase = (float) (ph - std::floor (ph));
                    }
            fresh = false;
        }
        const float ratio = std::exp2 ((keyF - (float) baseKey) * (1.0f / 12.0f));
        const float invSr = 1.0f / sampleRate;

        // --- target gains (control rate, ramped across the block) ------------------
        const bool percOn = op.perc;
        const float barScale = (percOn && ! op.percSoft) ? 0.7079f : 1.0f; // Normal percussion: drawbars -3 dB
        const float ch = std::clamp (op.character, 0.0f, 1.0f);
        const float leak = std::clamp (op.leakage, 0.0f, 1.0f);
        const float leakGain = leak * leak * 0.03f;
        float target[kNumSlots] {};
        float rawBar[kNumDrawbars];
        for (int d = 0; d < kNumDrawbars; ++d)
        {
            rawBar[d] = organ::drawbarGain (op.drawbar[(size_t) d]);
            const bool cut = d == 8 && percOn && op.percCut1;
            target[d] = cut ? 0.0f : rawBar[d] * barScale;
            target[kLeak0 + d] = slot[kLeak0 + d].wheel > 0 ? rawBar[d] * leakGain : 0.0f;
        }
        const float percLevel = op.percSoft ? 0.5f : 1.0f;
        const bool percLive = percOn && ! released;
        const float percB = percLive ? ctx.percTo * percLevel : 0.0f;

        // Contacts: ramp length from Click (sharp contacts click, soft ones don't).
        const float clickAmt = std::clamp (op.click, 0.0f, 1.0f);
        const float rampSamples = std::max (1.0f, (0.003f + (0.00005f - 0.003f) * std::sqrt (clickAmt)) * sampleRate);
        const float invRamp = 1.0f / rampSamples;
        const bool contactsSettled = (float) tContact > (float) maxDelay + rampSamples;

        float acc[kBlock] {}, accP[kBlock] {};
        const auto& sine = SineTable::get();
        const float invN = 1.0f / (float) n;
        for (int s = 0; s < kNumSlots; ++s)
        {
            auto& sl = slot[s];
            if (sl.wheel <= 0)
                continue;
            const bool isPerc = s == kPerc;
            const float wheelHz = op.equalTemp ? wt.equal[sl.wheel] : wt.hz[sl.wheel];
            const float inc = std::min (wheelHz * ratio * invSr, 0.49f);
            const float g0 = sl.gain; // last block's end: continuous
            const float g1 = isPerc ? percB : target[s];
            sl.gain = g1;
            if (g0 <= 0.0f && g1 <= 0.0f)
            {
                // Pushed in: the wheel keeps turning (its phase stays true to the clock).
                const float ph = sl.phase + inc * (float) n;
                sl.phase = ph - std::floor (ph);
                continue;
            }
            const float lvl = 1.0f + ch * 0.17f * wt.spread[sl.wheel];
            const bool lowWheel = sl.wheel <= 12;
            const float h2 = ch * (lowWheel ? 0.12f : 0.03f);
            const float h3 = ch * (lowWheel ? 0.10f : 0.015f);
            float g = g0 * lvl;
            const float gStep = (g1 - g0) * lvl * invN;
            float ph = sl.phase;
            float* dst = isPerc ? accP : acc;
            const int contactSlot = s >= kLeak0 ? s - kLeak0 : (isPerc ? 8 : s);
            const int delay = contactDelay[contactSlot];
            for (int i = 0; i < n; ++i)
            {
                const float x = sine (ph);
                float y = x;
                if (h2 > 0.0f)
                {
                    const float x2 = x * x;
                    y = x * (1.0f + h3 * (3.0f - 4.0f * x2)) + h2 * (1.0f - 2.0f * x2);
                }
                float c = 1.0f;
                if (! contactsSettled)
                    c = std::clamp ((float) (tContact + i - delay) * invRamp, 0.0f, 1.0f);
                dst[i] += y * g * c;
                g += gStep;
                ph += inc;
                ph -= ph >= 1.0f ? 1.0f : 0.0f;
            }
            sl.phase = ph;
        }

        // --- key click bursts (only around key down / up) -----------------------------
        float clk[kBlock] {};
        bool anyClick = false;
        if (clickAmt > 0.0f && (! contactsSettled || burst > 1.0e-5f || relBurstAt >= tContact))
        {
            const float tone = std::clamp (op.clickTone, 0.0f, 1.0f);
            clickLp = 1.0f - std::exp (-kTwoPi * 1500.0f * std::exp2 (3.0f * tone) * invSr);
            const float decay = std::exp (-invSr / 0.0012f);
            float sumBars = 0.0f;
            for (int d = 0; d < kNumDrawbars; ++d)
                sumBars += rawBar[d];
            for (int i = 0; i < n; ++i)
            {
                const int t = tContact + i;
                if (! contactsSettled)
                    for (int d = 0; d < kNumDrawbars; ++d)
                        if (t == contactDelay[d])
                            burst += rawBar[d] * clickAmt * 0.35f;
                if (t == relBurstAt)
                    burst += std::min (1.5f, sumBars) * clickAmt * 0.12f;
                const float nz = rng.bi() * burst;
                burst *= decay;
                lpState += (nz - lpState) * clickLp;
                hpState += (lpState - hpState) * clickHp;
                clk[i] = lpState - hpState;
            }
            anyClick = true;
        }
        tContact = std::min (tContact + n, 1 << 28);

        // --- output ----------------------------------------------------------------
        constexpr float kOut = 0.3f;
        const float vol = p.volume;
        float pkW = 0.0f, pkP = 0.0f, pkC = 0.0f;
        for (int i = 0; i < n; ++i)
        {
            const float w = acc[i] * kOut, pc = accP[i] * kOut, c = clk[i];
            const float y = w + pc + c;
            outL[i] += y * vol;
            outR[i] += y * vol;
            modOut[i] = y;
            if (meter != nullptr)
            {
                pkW = std::max (pkW, std::abs (w));
                pkP = std::max (pkP, std::abs (pc));
                pkC = std::max (pkC, std::abs (c));
            }
        }
        if (meter != nullptr)
        {
            const float v = std::abs (vol);
            meter[0] = pkW * v;
            meter[1] = pkP * v;
            meter[2] = 0.0f;
            meter[3] = anyClick ? pkC * v : 0.0f;
        }
    }

private:
    static constexpr int kPerc = kNumDrawbars, kLeak0 = kNumDrawbars + 1, kNumSlots = 2 * kNumDrawbars + 1;
    struct Slot
    {
        int wheel = 0;
        float phase = 0.0f, gain = 0.0f;
    };
    float sampleRate = 48000.0f;
    Slot slot[kNumSlots];
    int contactDelay[kNumDrawbars] {};
    int maxDelay = 0, tContact = 0, relBurstAt = -1;
    int baseKey = 60;
    bool fresh = true, released = false, lastBassDown = true, lastPercThird = true;
    float burst = 0.0f, lpState = 0.0f, hpState = 0.0f, clickLp = 0.5f, clickHp = 0.05f;
    Rng rng;

    void closeContacts() noexcept
    {
        tContact = 0;
        maxDelay = 0;
        for (int d = 0; d < kNumDrawbars; ++d)
        {
            contactDelay[d] = (int) (rng.uni() * 0.003f * sampleRate);
            maxDelay = std::max (maxDelay, contactDelay[d]);
        }
    }
};
} // namespace cs
