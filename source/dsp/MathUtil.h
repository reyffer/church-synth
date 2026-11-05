#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace cs
{
constexpr float kPi = 3.14159265358979323846f;
constexpr float kTwoPi = 2.0f * kPi;

inline float wrap01 (float p) noexcept { return p - std::floor (p); }

inline float pitchToHz (float midiPitch) noexcept
{
    return 440.0f * std::exp2 ((midiPitch - 69.0f) * (1.0f / 12.0f));
}

// Smooth odd saturator (Pade-style tanh approximation), exact clip at |x| >= 3.
inline float softClip (float x) noexcept
{
    x = std::clamp (x, -3.0f, 3.0f);
    const float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

// Saturator with automatic makeup: the output follows the input's loudness (30 ms
// RMS followers) times `boost`, so the character changes with drive but the level
// doesn't depend on how hot the signal arrives. Stereo; bias adds even harmonics.
struct AutoGainSaturator
{
    float inEnv[2] {}, outEnv[2] {}, coef = 0.0007f;
    void setSampleRate (float sr) noexcept { coef = 1.0f - std::exp (-1.0f / (0.03f * sr)); }
    void reset() noexcept { inEnv[0] = inEnv[1] = outEnv[0] = outEnv[1] = 0.0f; }
    float process (int ch, float x, float gain, float bias, float boost) noexcept
    {
        const float y = softClip (x * gain + bias) - softClip (bias);
        inEnv[ch] += (x * x - inEnv[ch]) * coef;
        outEnv[ch] += (y * y - outEnv[ch]) * coef;
        return y * boost * std::sqrt ((inEnv[ch] + 1.0e-12f) / (outEnv[ch] + 1.0e-12f));
    }
};

// Simple, fast per-voice random generator.
struct Rng
{
    uint32_t state = 0x9E3779B9u;
    void seed (uint32_t s) noexcept { state = s ? s : 0x9E3779B9u; }
    uint32_t next() noexcept
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    }
    float uni() noexcept { return (float) (next() >> 8) * (1.0f / 16777216.0f); } // 0..1
    float bi() noexcept { return uni() * 2.0f - 1.0f; }                           // -1..1
};

// Balance pan law: centre = unity on both sides, hard pan = 0 on the far side.
inline void panGains (float pan, float& l, float& r) noexcept
{
    pan = std::clamp (pan, -1.0f, 1.0f);
    l = std::min (1.0f, 1.0f - pan);
    r = std::min (1.0f, 1.0f + pan);
}

// Sine lookup with linear interpolation; phase in cycles (0..1).
class SineTable
{
public:
    static constexpr int kSize = 2048;
    static const SineTable& get()
    {
        static const SineTable t;
        return t;
    }
    float operator() (float phase) const noexcept
    {
        const float x = phase * (float) kSize;
        const int i = (int) x;
        const float f = x - (float) i;
        const int i0 = i & (kSize - 1);
        return table[i0] + f * (table[i0 + 1] - table[i0]);
    }

private:
    SineTable()
    {
        for (int i = 0; i <= kSize; ++i)
            table[i] = std::sin (kTwoPi * (float) i / (float) kSize);
    }
    float table[kSize + 1];
};
} // namespace cs
