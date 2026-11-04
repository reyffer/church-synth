#pragma once

// Band-limited virtual-analog oscillator.
//
// Discontinuities (steps) and corners (slope changes) are corrected with
// two-sample polyBLEP / polyBLAMP residuals. To apply the residual to the
// sample *before* an event, the oscillator runs with one sample of latency.
// This lets hard sync and through-segment events be handled exactly.

#include "MathUtil.h"
#include "Params.h"

namespace cs
{
class BlepOscillator
{
public:
    void reset (float startPhase) noexcept
    {
        phase = wrap01 (startPhase);
        held = 0.0f;
        heldCorr = 0.0f;
        curCorr = 0.0f;
        lastWrapD = -1.0f;
    }

    // inc: phase increment per sample (cycles), must be in [0, 0.5).
    // syncD: if >= 0, a master oscillator wrapped this sample; value is how
    //        far (in samples, 0..1) the current sample lies after that wrap.
    float tick (float inc, int wave, float duty, float syncD = -1.0f) noexcept
    {
        lastWrapD = -1.0f;
        curCorr = 0.0f;

        if (inc <= 1.0e-9f)
            inc = 1.0e-9f;

        if (syncD < 0.0f)
        {
            const float a = phase;
            const float b = phase + inc;
            scan (a, b, inc, 0.0f, wave, duty);
            phase = b >= 1.0f ? b - 1.0f : b;
        }
        else
        {
            const float pre = 1.0f - syncD;
            const float a = phase;
            const float mid = phase + inc * pre;
            scan (a, mid, inc, syncD, wave, duty);

            const float pm = wrap01 (mid);
            const float jump = naive (0.0f, wave, duty) - naive (pm, wave, duty);
            addStep (jump, syncD);
            if (wave == WaveTriangle)
                addRamp ((slope (0.0f, duty) - slope (pm, duty)) * inc, syncD);

            const float b = inc * syncD;
            scan (0.0f, b, inc, 0.0f, wave, duty);
            phase = b;
            lastWrapD = syncD;
        }

        const float out = held + heldCorr;
        held = naive (phase, wave, duty) + curCorr;
        heldCorr = 0.0f;
        return out;
    }

    // Distance (0..1 samples) after the most recent wrap in the last tick, or -1.
    float lastWrapD = -1.0f;

    float getPhase() const noexcept { return phase; }

private:
    float phase = 0.0f;
    float held = 0.0f;     // naive value of the delayed sample (+ its own residual)
    float heldCorr = 0.0f; // residual still to be added to the delayed sample
    float curCorr = 0.0f;  // residual for the sample being computed

    static float naive (float p, int wave, float duty) noexcept
    {
        switch (wave)
        {
            case WaveSine:
                return SineTable::get() (p);
            case WaveTriangle:
                return p < duty ? -1.0f + 2.0f * p / duty
                                : 1.0f - 2.0f * (p - duty) / (1.0f - duty);
            case WaveSaw:
                return 1.0f - 2.0f * p;
            case WaveRamp:
                return 2.0f * p - 1.0f;
            case WaveSquare:
            default:
                return (p < duty ? 1.0f : -1.0f) - (2.0f * duty - 1.0f);
        }
    }

    static float slope (float p, float duty) noexcept // triangle slope per cycle
    {
        return p < duty ? 2.0f / duty : -2.0f / (1.0f - duty);
    }

    // Step of height j, d samples before the current sample.
    void addStep (float j, float d) noexcept
    {
        heldCorr += j * 0.5f * d * d;
        curCorr += j * (d - 0.5f * d * d - 0.5f);
    }

    // Slope change of s (output units per sample), d samples before the current sample.
    void addRamp (float s, float d) noexcept
    {
        const float e = 1.0f - d;
        heldCorr += s * d * d * d * (1.0f / 6.0f);
        curCorr += s * e * e * e * (1.0f / 6.0f);
    }

    // Find events with a < c <= b (unwrapped phase). tail = how far the end of
    // this segment lies before the current sample.
    void scan (float a, float b, float inc, float tail, int wave, float duty) noexcept
    {
        if (wave == WaveSine)
            return;

        const float invInc = 1.0f / inc;
        auto dist = [&] (float c) { return std::min ((b - c) * invInc + tail, 0.9999f); };

        const bool wraps = a < 1.0f && b >= 1.0f;
        if (wraps)
            lastWrapD = dist (1.0f);

        switch (wave)
        {
            case WaveSaw:
                if (wraps) addStep (2.0f, dist (1.0f));
                break;
            case WaveRamp:
                if (wraps) addStep (-2.0f, dist (1.0f));
                break;
            case WaveSquare:
                if (wraps) addStep (2.0f, dist (1.0f));
                if (a < duty && b >= duty) addStep (-2.0f, dist (duty));
                if (a < 1.0f + duty && b >= 1.0f + duty) addStep (-2.0f, dist (1.0f + duty));
                break;
            case WaveTriangle:
            {
                const float k = (2.0f / duty + 2.0f / (1.0f - duty)) * inc;
                if (wraps) addRamp (k, dist (1.0f));
                if (a < duty && b >= duty) addRamp (-k, dist (duty));
                if (a < 1.0f + duty && b >= 1.0f + duty) addRamp (-k, dist (1.0f + duty));
                break;
            }
            default:
                break;
        }
    }
};

// Colour-able noise: -1 = red (dark), 0 = white, +1 = blue (bright).
class ColourNoise
{
public:
    void setSampleRate (float sr) noexcept { sampleRate = sr; }
    void seed (uint32_t s) noexcept { rng.seed (s); }

    void setColour (float c) noexcept
    {
        colour = std::clamp (c, -1.0f, 1.0f);
        if (colour < 0.0f)
        {
            const float fc = 20000.0f * std::pow (0.01f, -colour); // down to 200 Hz
            coef = 1.0f - std::exp (-kTwoPi * std::min (fc, 0.45f * sampleRate) / sampleRate);
            makeup = 1.0f + 3.0f * -colour;
        }
        else
        {
            const float fc = 20.0f * std::pow (250.0f, colour); // up to 5 kHz
            coef = 1.0f - std::exp (-kTwoPi * fc / sampleRate);
            makeup = 1.0f + colour;
        }
    }

    float tick() noexcept
    {
        const float w = rng.bi();
        lp += coef * (w - lp);
        return (colour < 0.0f ? lp : w - lp) * makeup;
    }

private:
    Rng rng;
    float sampleRate = 48000.0f;
    float colour = 0.0f, coef = 1.0f, makeup = 1.0f, lp = 0.0f;
};
} // namespace cs
