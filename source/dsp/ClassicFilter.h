#pragma once

// "Classic" multimode filter: 6/12/24/36 dB LP/HP, 12/24/36 dB BP/Notch/AP.
// Built from stereo trapezoidal (zero-delay-feedback) state-variable stages.
// Higher slopes cascade stages with Butterworth damping; resonance lives on
// the last stage. "Analog" mode saturates the resonant state so high
// resonance thickens and self-limits instead of whistling cleanly.

#include "MathUtil.h"
#include "Params.h"

namespace cs
{
class ClassicFilter
{
public:
    void setSampleRate (float sr) noexcept
    {
        sampleRate = sr;
        preSat.setSampleRate (sr);
        postSat.setSampleRate (sr);
    }

    void reset() noexcept
    {
        for (auto& s : st)
            s = {};
        op = {};
        dcX[0] = dcX[1] = dcY[0] = dcY[1] = 0.0f;
        preSat.reset();
        postSat.reset();
    }

    // Call once per control block.
    void setup (int newMode, float cutoffHz, float reso, float drive, bool isAnalog) noexcept
    {
        mode = std::clamp (newMode, 0, (int) ClassicModeCount - 1);
        analog = isAnalog;
        const float fc = std::clamp (cutoffHz, 16.0f, 0.46f * sampleRate);
        g = std::tan (kPi * fc / sampleRate);

        const int slopeIdx = slopeIndex (mode); // 0 = 6 dB, 1 = 12, 2 = 24, 3 = 36
        numStages = slopeIdx == 0 ? 0 : slopeIdx;
        static constexpr float kBw[4][3] = { { 0, 0, 0 },
                                              { 1.41421f, 0, 0 },
                                              { 1.84776f, 0.76537f, 0 },
                                              { 1.93185f, 1.41421f, 0.51764f } };
        reso = std::clamp (reso, 0.0f, 1.0f);
        const float resoShape = reso * (2.0f - reso); // more travel near the top
        for (int i = 0; i < numStages; ++i)
        {
            float k = kBw[slopeIdx][i];
            if (i == numStages - 1)
                k = k * (1.0f - 0.985f * resoShape);
            setStage (i, k);
        }

        opG = g / (1.0f + g);
        baseG = g;
        for (int i = 0; i < numStages; ++i)
            baseK[i] = st[i].k;

        // Drive (0..1), three places so it is heard even with the filter closed:
        //  - input: up to +18 dB into an asymmetric soft clipper (odd + even harmonics),
        //  - in the loop: the band-pass state saturates (resonance thickens and squashes),
        //  - output: a clipper after the filter (grit above the cutoff).
        // The two clippers have automatic makeup, so the level doesn't depend on how hot
        // the oscillators are; full drive ends up about 4 dB louder, like pushing a filter.
        drive = std::clamp (drive, 0.0f, 1.0f);
        driveGain = std::pow (10.0f, drive * 18.0f / 20.0f);
        driveBias = 0.25f * drive;
        driveBoost = 1.0f + 0.25f * drive;
        driveWet = std::min (1.0f, drive * 8.0f);
        loopSat = 1.5f * drive;
        postGain = 1.0f + 10.0f * drive;
        postBoost = 1.0f + 0.3f * drive;
        dcCoef = 1.0f - 2.0f * kPi * 12.0f / sampleRate;
    }

    // Audio-rate cutoff modulation: scale the prewarped cutoff by m (cheap, no tan()).
    void modulateCutoff (float m) noexcept
    {
        g = std::min (baseG * m, 30.0f);
        for (int i = 0; i < numStages; ++i)
            setStage (i, baseK[i]);
        opG = g / (1.0f + g);
    }

    void process (float& l, float& r) noexcept
    {
        if (driveWet > 0.0f)
        {
            l += driveWet * (dcBlock (0, preSat.process (0, l, driveGain, driveBias, driveBoost)) - l);
            r += driveWet * (dcBlock (1, preSat.process (1, r, driveGain, driveBias, driveBoost)) - r);
        }

        if (numStages == 0)
        {
            l = onePole (0, l);
            r = onePole (1, r);
        }
        else
            for (int i = 0; i < numStages; ++i)
            {
                l = stage (i, 0, l);
                r = stage (i, 1, r);
            }

        if (driveWet > 0.0f)
        {
            l += driveWet * (postSat.process (0, l, postGain, 0.0f, postBoost) - l);
            r += driveWet * (postSat.process (1, r, postGain, 0.0f, postBoost) - r);
        }
    }

    static int slopeIndex (int m) noexcept
    {
        switch (m)
        {
            case LP6: case HP6: return 0;
            case LP12: case BP12: case HP12: case Notch12: case AP12: return 1;
            case LP24: case BP24: case HP24: case Notch24: case AP24: return 2;
            default: return 3;
        }
    }

private:
    struct Stage
    {
        float k = 1.41421f, a1 = 0, a2 = 0, a3 = 0;
        float ic1[2] {}, ic2[2] {};
    };

    float sampleRate = 48000.0f;
    int mode = LP12, numStages = 1;
    bool analog = true;
    float g = 0.1f, opG = 0.1f, baseG = 0.1f;
    float baseK[3] { 1.414f, 1.414f, 1.414f };
    float driveGain = 1.0f, driveWet = 0.0f, driveBias = 0.0f, driveBoost = 1.0f;
    float loopSat = 0.0f, postGain = 1.0f, postBoost = 1.0f, dcCoef = 0.998f;
    AutoGainSaturator preSat, postSat;
    float dcX[2] {}, dcY[2] {};
    Stage st[3];
    struct { float s[2] {}; } op;

    void setStage (int i, float k) noexcept
    {
        auto& s = st[i];
        s.k = k;
        s.a1 = 1.0f / (1.0f + g * (g + k));
        s.a2 = g * s.a1;
        s.a3 = g * s.a2;
    }

    float stage (int i, int ch, float x) noexcept
    {
        auto& s = st[i];
        const float v3 = x - s.ic2[ch];
        float v1 = s.a1 * s.ic1[ch] + s.a2 * v3;
        const float v2 = s.ic2[ch] + s.a2 * s.ic1[ch] + s.a3 * v3;
        if (analog && i == numStages - 1)
            v1 = softClip (v1 * 0.5f) * 2.0f; // tame and colour the resonant band
        if (loopSat > 0.0f)
            v1 = softClip (v1 * (1.0f + loopSat)) / (1.0f + loopSat); // drive inside the loop
        s.ic1[ch] = 2.0f * v1 - s.ic1[ch];
        s.ic2[ch] = 2.0f * v2 - s.ic2[ch];

        const float lp = v2, bp = v1, hp = x - s.k * v1 - v2;
        switch (mode)
        {
            case LP12: case LP24: case LP36: return lp;
            case HP12: case HP24: case HP36: return hp;
            case BP12: case BP24: case BP36: return bp * s.k;
            case Notch12: case Notch24: case Notch36: return lp + hp;
            case AP12: case AP24: case AP36: return lp + hp - s.k * bp;
            default: return lp;
        }
    }

    // The asymmetric shaper makes a little DC; take it out before the filter.
    float dcBlock (int ch, float x) noexcept
    {
        const float y = x - dcX[ch] + dcCoef * dcY[ch];
        dcX[ch] = x;
        dcY[ch] = y;
        return y;
    }

    float onePole (int ch, float x) noexcept
    {
        const float v = (x - op.s[ch]) * opG;
        const float lp = v + op.s[ch];
        op.s[ch] = lp + v;
        return mode == HP6 ? x - lp : lp;
    }
};
} // namespace cs
