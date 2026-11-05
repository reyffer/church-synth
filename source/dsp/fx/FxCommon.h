#pragma once

// Building blocks shared by the effects. Buffers are sized in prepare()
// (message thread), never on the audio thread.

#include "../MathUtil.h"
#include "../ModSources.h"

#include <vector>

namespace cs::fx
{
inline float dbToGain (float db) noexcept { return std::pow (10.0f, db * 0.05f); }
inline float gainToDb (float g) noexcept { return 20.0f * std::log10 (std::max (g, 1.0e-9f)); }

// 0..1 knob -> exponential range.
inline float expMap (float x, float lo, float hi) noexcept { return lo * std::pow (hi / lo, std::clamp (x, 0.0f, 1.0f)); }
inline float linMap (float x, float lo, float hi) noexcept { return lo + (hi - lo) * std::clamp (x, 0.0f, 1.0f); }
inline int choice (float x, int count) noexcept { return std::clamp ((int) (x * (float) count), 0, count - 1); }
inline bool flag (float x) noexcept { return x >= 0.5f; }

// Time knob: free (ms, exponential) or tempo-synced (division index).
inline float timeSeconds (float knob, bool sync, double bpm, float loMs, float hiMs) noexcept
{
    if (sync)
    {
        const auto& d = kSyncDivs[choice (knob, kNumSyncDivs)];
        return (float) (d.beats * 60.0 / std::max (bpm, 20.0));
    }
    return expMap (knob, loMs, hiMs) * 0.001f;
}
inline float rateHz (float knob, bool sync, double bpm, float lo = 0.02f, float hi = 20.0f) noexcept
{
    if (sync)
    {
        const auto& d = kSyncDivs[choice (knob, kNumSyncDivs)];
        return (float) (std::max (bpm, 20.0) / 60.0 / d.beats);
    }
    return expMap (knob, lo, hi);
}

// One-pole smoother for per-sample parameter glides.
struct Smooth
{
    float v = 0.0f, coef = 0.001f;
    void setTime (float seconds, float sr) noexcept { coef = 1.0f - std::exp (-1.0f / std::max (seconds * sr, 1.0f)); }
    float next (float target) noexcept { return v += (target - v) * coef; }
};

// Mono delay line with fractional (cubic Hermite) reads.
class Delay
{
public:
    void prepare (int maxSamples)
    {
        int size = 1;
        while (size < maxSamples + 4)
            size <<= 1;
        buf.assign ((size_t) size, 0.0f);
        mask = size - 1;
        pos = 0;
    }
    void clear() noexcept { std::fill (buf.begin(), buf.end(), 0.0f); }
    int capacity() const noexcept { return mask - 3; }

    void push (float x) noexcept
    {
        buf[(size_t) pos] = x;
        pos = (pos + 1) & mask;
    }
    // Reads d samples back from the most recent push (d >= 1).
    float read (float d) const noexcept
    {
        d = std::clamp (d, 1.0f, (float) (mask - 3));
        const float rp = (float) pos - d;
        const int i = (int) std::floor (rp);
        const float t = rp - (float) i;
        const float y0 = at (i - 1), y1 = at (i), y2 = at (i + 1), y3 = at (i + 2);
        const float c1 = 0.5f * (y2 - y0);
        const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        return ((c3 * t + c2) * t + c1) * t + y1;
    }
    float readInt (int d) const noexcept { return at (pos - d); }

private:
    std::vector<float> buf;
    int mask = 0, pos = 0;
    float at (int i) const noexcept { return buf[(size_t) (i & mask)]; }
};

// Simple sine/triangle LFO for effects.
struct FxLfo
{
    float phase = 0.0f;
    float tick (float hz, float sr, bool triangle = false) noexcept
    {
        phase += hz / sr;
        phase -= std::floor (phase);
        return triangle ? (phase < 0.5f ? 4.0f * phase - 1.0f : 3.0f - 4.0f * phase) : SineTable::get() (phase);
    }
    float at (float offset, bool triangle = false) const noexcept
    {
        const float p = wrap01 (phase + offset);
        return triangle ? (p < 0.5f ? 4.0f * p - 1.0f : 3.0f - 4.0f * p) : SineTable::get() (p);
    }
};

// Doppler pitch shifter: two taps sweeping through a window, crossfaded.
class PitchShifter
{
public:
    void prepare (float sr)
    {
        sampleRate = sr;
        window = std::max (64.0f, 0.06f * sr); // 60 ms grains
        line.prepare ((int) (window * 2.0f) + 8);
    }
    void clear() noexcept { line.clear(); }
    void setPhase (float p) noexcept { phase = wrap01 (p); }
    float process (float x, float ratio, float spray = 0.0f) noexcept
    {
        line.push (x);
        phase += (1.0f - ratio) / window;
        if (phase >= 1.0f || phase < 0.0f)
        {
            phase -= std::floor (phase);
            jitter = spray * (rng.uni() * 0.5f);
        }
        const float p2 = wrap01 (phase + 0.5f);
        const float d1 = 1.0f + (phase + jitter) * window, d2 = 1.0f + p2 * window;
        const float w1 = SineTable::get() (0.5f * phase), w2 = SineTable::get() (0.5f * p2); // sin(pi*p)
        return line.read (d1) * w1 * w1 + line.read (d2) * w2 * w2;
    }

private:
    Delay line;
    float sampleRate = 48000.0f, window = 2880.0f, phase = 0.0f, jitter = 0.0f;
    Rng rng;
};

// 2x oversampler with half-band FIR filters (for nonlinear effects).
class Oversampler2x
{
public:
    static constexpr int kTaps = 31;
    Oversampler2x()
    {
        // Windowed-sinc half-band low-pass at fs/4 of the oversampled rate.
        const int mid = kTaps / 2;
        float sum = 0.0f;
        for (int i = 0; i < kTaps; ++i)
        {
            const float n = (float) (i - mid);
            const float sinc = n == 0.0f ? 0.5f : std::sin (0.5f * kPi * n) / (kPi * n);
            const float w = 0.42f - 0.5f * std::cos (2.0f * kPi * (float) i / (float) (kTaps - 1))
                            + 0.08f * std::cos (4.0f * kPi * (float) i / (float) (kTaps - 1));
            h[i] = sinc * w;
            sum += h[i];
        }
        for (auto& c : h)
            c /= sum;
    }
    void clear() noexcept
    {
        for (auto& v : up) v = 0.0f;
        for (auto& v : down) v = 0.0f;
    }

    // Processes one input sample through f() at twice the rate.
    template <typename F>
    float process (float x, F&& f) noexcept
    {
        float out = 0.0f;
        for (int k = 0; k < 2; ++k)
        {
            push (up, k == 0 ? 2.0f * x : 0.0f);
            const float s = f (fir (up));
            push (down, s);
            if (k == 1)
                out = fir (down);
        }
        return out;
    }

private:
    float h[kTaps] {};
    float up[kTaps] {}, down[kTaps] {};
    static void push (float* buf, float v) noexcept
    {
        for (int i = kTaps - 1; i > 0; --i)
            buf[i] = buf[i - 1];
        buf[0] = v;
    }
    float fir (const float* buf) const noexcept
    {
        float acc = 0.0f;
        for (int i = 0; i < kTaps; ++i)
            acc += h[i] * buf[i];
        return acc;
    }
};
} // namespace cs::fx
