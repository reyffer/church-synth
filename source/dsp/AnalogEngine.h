#pragma once

// Analog engine: 3 band-limited oscillators, hard sync (osc 2 -> osc 1),
// linear FM into osc 1 and 2 from the modulator (osc 3 <-> noise crossfade),
// coloured noise, unison (Classic / Chord / Super) and per-note drift.
// One instance per voice.

#include "Oscillator.h"

namespace cs
{
class AnalogEngine
{
public:
    void setSampleRate (float sr) noexcept
    {
        sampleRate = sr;
        noise.setSampleRate (sr);
    }

    // Called when an idle voice starts a note: re-seeds drift and phases.
    void startNote (const AnalogEngineParams& p, uint32_t seed) noexcept
    {
        rng.seed (seed);
        noise.seed (seed * 2654435761u + 1u);
        const bool randomPhase = p.drift > 0.0f || (p.unisonOn && unisonCount (p) > 1);
        for (int u = 0; u < kMaxUnison; ++u)
            for (int o = 0; o < kNumOscs; ++o)
            {
                driftOffset[u][o] = rng.bi();
                osc[u][o].reset (randomPhase ? rng.uni() : 0.0f);
            }
        activeUnison = unisonCount (p);
    }

    // Renders n samples (n <= kBlock) and ADDS them into outL/outR.
    // notePitch: played pitch in semitones incl. glide/bend/transpose.
    // fixedPitch: 60 + bend/transpose, used when key tracking is off.
    void render (const AnalogEngineParams& p, float notePitch, float fixedPitch,
                 int n, float* outL, float* outR) noexcept
    {
        if (! p.on)
            return;

        // --- unison layout -------------------------------------------------
        const int count = unisonCount (p);
        if (count > activeUnison) // newly enabled sub-voices start at random phases
            for (int u = activeUnison; u < count; ++u)
                for (int o = 0; o < kNumOscs; ++o)
                    osc[u][o].reset (rng.uni());
        activeUnison = count;

        float offset[kMaxUnison], panL[kMaxUnison], panR[kMaxUnison];
        layoutUnison (p, count, offset, panL, panR);

        // --- per-oscillator increments (control rate) -----------------------
        const float enginePitch = (p.keytrack ? notePitch : fixedPitch) + p.coarse + p.fine;
        const float driftSemis = p.drift * 0.15f; // up to +/-15 cents
        float inc[kMaxUnison][kNumOscs];
        float duty[kNumOscs];
        for (int o = 0; o < kNumOscs; ++o)
        {
            const auto& op = p.osc[(size_t) o];
            duty[o] = 0.5f + (std::clamp (op.width, 0.0f, 1.0f) - 0.5f) * 0.96f;
            float base = enginePitch + op.coarse;
            if (o > 0)
            {
                base += op.fine;
                if (! op.keytrack)
                    base -= (p.keytrack ? notePitch : fixedPitch) - 60.0f;
            }
            for (int u = 0; u < count; ++u)
            {
                const float pitch = base + offset[u] + driftSemis * driftOffset[u][o];
                inc[u][o] = std::min (pitchToHz (pitch) / sampleRate, 0.45f);
            }
        }

        // --- which oscillators actually need computing ----------------------
        const float vol1 = p.osc[0].volume, vol2 = p.osc[1].volume, vol3 = p.osc[2].volume;
        const float fm1 = 8.0f * p.osc[0].fm * p.osc[0].fm;
        const float fm2 = 8.0f * p.osc[1].fm * p.osc[1].fm;
        const bool usesFm = fm1 > 0.0f || fm2 > 0.0f;
        const float modNoise = std::clamp (p.modSource, 0.0f, 1.0f);
        const bool need3 = vol3 > 0.0f || (usesFm && modNoise < 1.0f);
        const bool need2 = vol2 > 0.0f;
        const bool need1 = vol1 > 0.0f || (need2 && p.sync);
        const bool needNoise = p.noiseVolume > 0.0f || (usesFm && modNoise > 0.0f);
        if (needNoise)
            noise.setColour (p.noiseColor);

        const float gainNorm = unisonNorm (p, count);
        const float engVol = p.volume;
        const int wave1 = p.osc[0].wave, wave2 = p.osc[1].wave, wave3 = p.osc[2].wave;

        for (int i = 0; i < n; ++i)
        {
            const float nz = needNoise ? noise.tick() : 0.0f;
            float sumL = 0.0f, sumR = 0.0f;

            for (int u = 0; u < count; ++u)
            {
                auto* o = osc[u];
                const float o3 = need3 ? o[2].tick (inc[u][2], wave3, duty[2]) : 0.0f;
                const float mod = o3 + modNoise * (nz - o3);

                float o1 = 0.0f, wrapD = -1.0f;
                if (need1)
                {
                    const float i1 = std::clamp (inc[u][0] * (1.0f + fm1 * mod), 0.0f, 0.45f);
                    o1 = o[0].tick (i1, wave1, duty[0]);
                    wrapD = o[0].lastWrapD;
                }

                float o2 = 0.0f;
                if (need2)
                {
                    const float i2 = std::clamp (inc[u][1] * (1.0f + fm2 * mod), 0.0f, 0.45f);
                    o2 = o[1].tick (i2, wave2, duty[1], p.sync ? wrapD : -1.0f);
                }

                const float s = o1 * vol1 + o2 * vol2 + o3 * vol3;
                sumL += s * panL[u];
                sumR += s * panR[u];
            }

            const float nOut = nz * p.noiseVolume;
            outL[i] += (sumL * gainNorm + nOut) * engVol;
            outR[i] += (sumR * gainNorm + nOut) * engVol;
        }
    }

    static int unisonCount (const AnalogEngineParams& p) noexcept
    {
        if (! p.unisonOn)
            return 1;
        if (p.unisonMode == UnisonSuper)
            return 7;
        return std::clamp (p.unisonVoices, 1, kMaxUnison);
    }

private:
    float sampleRate = 48000.0f;
    BlepOscillator osc[kMaxUnison][kNumOscs];
    float driftOffset[kMaxUnison][kNumOscs] {};
    ColourNoise noise;
    Rng rng;
    int activeUnison = 1;

    static float detuneSemis (float d) noexcept
    {
        d = std::clamp (d, 0.0f, 1.0f);
        return 6.0f * d * d * d; // fine control at low settings, +/-6 st at max
    }

    static void layoutUnison (const AnalogEngineParams& p, int count,
                              float* offset, float* panL, float* panR) noexcept
    {
        static constexpr int kChords[kNumChords][5] = {
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

    static float unisonNorm (const AnalogEngineParams& p, int count) noexcept
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
};
} // namespace cs
