#pragma once

// Unison layout shared by the Analog and Wavetable engines:
// Classic (evenly detuned), Chord (12 chord shapes) and Super (7-voice supersaw spread).

#include "MathUtil.h"
#include "Params.h"

namespace cs::unison
{
inline int unisonCount (const EngineParams& p) noexcept
{
    if (! p.unisonOn)
        return 1;
    if (p.unisonMode == UnisonSuper)
        return 7;
    return std::clamp (p.unisonVoices, 1, kMaxUnison);
}


inline float detuneSemis (float d) noexcept
{
    d = std::clamp (d, 0.0f, 1.0f);
    return 6.0f * d * d * d; // fine control at low settings, +/-6 st at max
}

inline constexpr int kChords[kNumChords][5] = {
    { 0, 12, -1, -1, -1 },  // Octave
    { 0, 7, -1, -1, -1 },   // Fifth
    { 0, 7, 12, -1, -1 },   // Fifth + octave
    { 0, 4, 7, -1, -1 },    // Major
    { 0, 3, 7, -1, -1 },    // Minor
    { 0, 2, 7, -1, -1 },    // Sus2
    { 0, 5, 7, -1, -1 },    // Sus4
    { 0, 4, 7, 11, -1 },    // Major 7
    { 0, 3, 7, 10, -1 },    // Minor 7
    { 0, 4, 7, 10, -1 },    // Dominant 7
    { 0, 4, 7, 14, -1 },    // Add 9
    { 0, 4, 7, 9, 14 },     // 6/9
};

// Intervals of the first `count` chord-unison voices (the same notes Chord mode plays).
inline int chordIntervals (const EngineParams& p, int* out) noexcept
{
    const auto& ch = kChords[std::clamp (p.chord, 0, kNumChords - 1)];
    int len = 0;
    while (len < 5 && ch[len] >= 0)
        ++len;
    const int count = std::clamp (p.unisonVoices, 1, kMaxUnison);
    for (int u = 0; u < count; ++u)
        out[u] = ch[u % len] + 12 * (u / len);
    return count;
}

inline void layoutUnison (const EngineParams& p, int count,
                          float* offset, float* panL, float* panR) noexcept
{
    static constexpr float kSuperSpread[7] = { -1.0f, -0.62f, -0.29f, 0.0f, 0.31f, 0.65f, 0.97f };

    if (count == 1)
    {
        offset[0] = 0.0f;
        panL[0] = panR[0] = 1.0f;
        return;
    }

    const float spread = detuneSemis (p.detune);
    const float width = std::clamp (p.stereo, 0.0f, 1.0f);

    for (int u = 0; u < count; ++u)
    {
        float pos; // -1..1 stereo/detune position
        if (p.unisonMode == UnisonSuper)
        {
            pos = kSuperSpread[u];
            offset[u] = pos * spread;
        }
        else if (p.unisonMode == UnisonChord)
        {
            const auto& ch = kChords[std::clamp (p.chord, 0, kNumChords - 1)];
            int len = 0;
            while (len < 5 && ch[len] >= 0)
                ++len;
            offset[u] = (float) (ch[u % len] + 12 * (u / len));
            pos = (u == 0) ? 0.0f : ((u & 1) ? 1.0f : -1.0f) * (0.4f + 0.6f * (float) u / (float) count);
        }
        else
        {
            pos = -1.0f + 2.0f * (float) u / (float) (count - 1);
            offset[u] = pos * spread;
        }
        panGains (pos * width, panL[u], panR[u]);
    }

    if (p.unisonMode == UnisonSuper)
    {
        const float mix = std::clamp (p.superMix, 0.0f, 1.0f);
        for (int u = 0; u < 7; ++u)
        {
            const float g = (u == 3) ? 1.0f - 0.55f * mix : 0.85f * mix;
            panL[u] *= g;
            panR[u] *= g;
        }
    }
}

inline float unisonNorm (const EngineParams& p, int count) noexcept
{
    if (count == 1)
        return 1.0f;
    if (p.unisonMode == UnisonSuper)
    {
        const float mix = std::clamp (p.superMix, 0.0f, 1.0f);
        const float c = 1.0f - 0.55f * mix, s = 0.85f * mix;
        return 1.0f / std::sqrt (c * c + 6.0f * s * s);
    }
    return 1.0f / std::sqrt ((float) count);
}
} // namespace cs::unison
