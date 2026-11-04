#pragma once

// Utility engine: two sample-based noise players and one virtual-analog
// oscillator (typically a sub). Always present alongside the two main engines.

#include "Content.h"
#include "Oscillator.h"

namespace cs
{
class NoisePlayer
{
public:
    void setSampleRate (float sr) noexcept { sampleRate = sr; }

    void startNote (const NoiseParams& p, Rng& rng) noexcept
    {
        pos = 0.0;
        if (p.randomPhase && p.sample != nullptr && p.sample->length() > 0)
            pos = (double) rng.uni() * (double) p.sample->length();
        finished = false;
        lpL = lpR = 0.0f;
    }

    // Adds n samples into l/r (already scaled by volume).
    void render (const NoiseParams& p, float notePitch, int n, float* l, float* r) noexcept
    {
        const NoiseSample* s = p.sample;
        if (! p.on || s == nullptr || p.volume <= 0.0f || finished)
            return;
        const size_t len = s->length();
        if (len < 2)
            return;

        const float semis = p.tune + (p.keytrack ? notePitch - 60.0f : 0.0f);
        const double rate = (double) (s->sampleRate / sampleRate) * (double) std::exp2 (semis / 12.0f);
        const double end = p.loop ? (double) len : (double) len * (double) std::clamp (p.length, 0.0f, 1.0f);
        const double fadeLen = 0.004 * (double) s->sampleRate; // short fade at a one-shot's cut point

        // Tilt filter: < 0 low-pass (darker), > 0 high-pass (thinner).
        const float f = std::clamp (p.filter, -1.0f, 1.0f);
        const bool useLp = f < -0.001f, useHp = f > 0.001f;
        const float hz = useLp ? 20000.0f * std::pow (0.005f, -f) : 20.0f * std::pow (400.0f, f);
        const float c = 1.0f - std::exp (-kTwoPi * std::min (hz, 0.45f * sampleRate) / sampleRate);

        const float* dl = s->left.data();
        const float* dr = s->right.data();
        const float vol = p.volume;

        for (int i = 0; i < n; ++i)
        {
            if (pos >= end)
            {
                if (p.loop)
                    pos -= end;
                else
                {
                    finished = true;
                    break;
                }
            }
            const size_t i0 = (size_t) pos;
            const size_t i1 = (i0 + 1 < len) ? i0 + 1 : (p.loop ? 0 : i0);
            const float fr = (float) (pos - (double) i0);
            float sl = dl[i0] + fr * (dl[i1] - dl[i0]);
            float sr = dr[i0] + fr * (dr[i1] - dr[i0]);

            if (! p.loop && end - pos < fadeLen)
            {
                const float g = (float) ((end - pos) / fadeLen);
                sl *= g;
                sr *= g;
            }

            if (useLp || useHp)
            {
                lpL += c * (sl - lpL);
                lpR += c * (sr - lpR);
                sl = useLp ? lpL : sl - lpL;
                sr = useLp ? lpR : sr - lpR;
            }
            l[i] += sl * vol;
            r[i] += sr * vol;
            pos += rate;
        }
    }

private:
    float sampleRate = 48000.0f;
    double pos = 0.0;
    bool finished = false;
    float lpL = 0.0f, lpR = 0.0f;
};

class UtilityEngine
{
public:
    void setSampleRate (float sr) noexcept
    {
        sampleRate = sr;
        for (auto& np : noise)
            np.setSampleRate (sr);
    }

    void startNote (const UtilityParams& p, uint32_t seed) noexcept
    {
        rng.seed (seed);
        for (size_t k = 0; k < 2; ++k)
            noise[k].startNote (p.noise[k], rng);
        osc.reset (0.0f);
    }

    // Adds into the Filter 1 / Filter 2 inputs, and into `post` for audio
    // that bypasses the filters (oscillator routed to FX A / FX B / Direct).
    void render (const UtilityParams& p, float notePitch, float fixedPitch, int n,
                 float* f1L, float* f1R, float* f2L, float* f2R, float* postL, float* postR) noexcept
    {
        if (! p.on)
            return;

        for (size_t k = 0; k < 2; ++k)
        {
            const auto& np = p.noise[k];
            if (! np.on || np.volume <= 0.0f || np.sample == nullptr)
                continue;
            float l[kBlock] {}, r[kBlock] {};
            noise[k].render (np, notePitch, n, l, r);
            split (np.filterMix, n, l, r, f1L, f1R, f2L, f2R);
        }

        if (p.oscOn && p.oscVolume > 0.0f)
        {
            const float pitch = (p.oscKeytrack ? notePitch : fixedPitch) + p.oscCoarse + p.oscFine;
            const float inc = std::min (pitchToHz (pitch) / sampleRate, 0.45f);
            const float duty = 0.5f + (std::clamp (p.oscWidth, 0.0f, 1.0f) - 0.5f) * 0.96f;
            float l[kBlock] {}, r[kBlock] {};
            for (int i = 0; i < n; ++i)
                l[i] = r[i] = osc.tick (inc, p.oscWave, duty) * p.oscVolume;

            if (p.oscRoute == RouteFilters)
                split (p.oscFilterMix, n, l, r, f1L, f1R, f2L, f2R);
            else
                for (int i = 0; i < n; ++i)
                {
                    postL[i] += l[i];
                    postR[i] += r[i];
                }
        }
    }

private:
    float sampleRate = 48000.0f;
    NoisePlayer noise[2];
    BlepOscillator osc;
    Rng rng;

    static void split (float mixToF2, int n, const float* l, const float* r,
                       float* f1L, float* f1R, float* f2L, float* f2R) noexcept
    {
        const float b = std::clamp (mixToF2, 0.0f, 1.0f), a = 1.0f - b;
        for (int i = 0; i < n; ++i)
        {
            f1L[i] += l[i] * a;
            f1R[i] += r[i] * a;
            f2L[i] += l[i] * b;
            f2R[i] += r[i] * b;
        }
    }
};
} // namespace cs
