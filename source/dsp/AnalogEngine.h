#pragma once

// Analog engine: 3 band-limited oscillators, hard sync (osc 2 -> osc 1),
// linear FM into osc 1 and 2 from the modulator (osc 3 <-> noise crossfade),
// coloured noise, unison (Classic / Chord / Super) and per-note drift.
// One instance per voice.

#include "Oscillator.h"
#include "Unison.h"

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
    void startNote (const EngineParams& p, uint32_t seed) noexcept
    {
        rng.seed (seed);
        noise.seed (seed * 2654435761u + 1u);
        const bool randomPhase = p.drift > 0.0f || (p.unisonOn && unison::unisonCount (p) > 1);
        for (int u = 0; u < kMaxUnison; ++u)
            for (int o = 0; o < kNumOscs; ++o)
            {
                driftOffset[u][o] = rng.bi();
                osc[u][o].reset (randomPhase ? rng.uni() : 0.0f);
            }
        activeUnison = unison::unisonCount (p);
    }

    // Renders n samples (n <= kBlock) and ADDS them into outL/outR.
    // notePitch: played pitch in semitones incl. glide/bend/transpose.
    // fixedPitch: 60 + bend/transpose, used when key tracking is off.
    // extMod: if non-null, the other engine's output replaces the Osc3/Noise modulator.
    // modOut: receives this engine's mono output before its Volume (for cross-mod).
    // meter: if non-null, receives the peak output of Osc 1..3 and the noise
    // (after their volumes and the engine volume) in meter[0..3].
    void render (const EngineParams& p, float notePitch, float fixedPitch,
                 int n, float* outL, float* outR, const float* extMod, float* modOut, float* meter = nullptr) noexcept
    {
        if (! p.on)
            return;

        // --- unison layout -------------------------------------------------
        const int count = unison::unisonCount (p);
        if (count > activeUnison) // newly enabled sub-voices start at random phases
            for (int u = activeUnison; u < count; ++u)
                for (int o = 0; o < kNumOscs; ++o)
                    osc[u][o].reset (rng.uni());
        activeUnison = count;

        float offset[kMaxUnison], panL[kMaxUnison], panR[kMaxUnison];
        unison::layoutUnison (p, count, offset, panL, panR);

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
        const bool need3 = vol3 > 0.0f || (usesFm && extMod == nullptr && modNoise < 1.0f);
        const bool need2 = vol2 > 0.0f;
        const bool need1 = vol1 > 0.0f || (need2 && p.sync);
        const bool needNoise = p.noiseVolume > 0.0f || (usesFm && extMod == nullptr && modNoise > 0.0f);
        if (needNoise)
            noise.setColour (p.noiseColor);

        const float gainNorm = unison::unisonNorm (p, count);
        const float engVol = p.volume;
        const int wave1 = p.osc[0].wave, wave2 = p.osc[1].wave, wave3 = p.osc[2].wave;

        float pk1 = 0.0f, pk2 = 0.0f, pk3 = 0.0f, pkN = 0.0f;
        for (int i = 0; i < n; ++i)
        {
            const float nz = needNoise ? noise.tick() : 0.0f;
            float sumL = 0.0f, sumR = 0.0f;
            float a1 = 0.0f, a2 = 0.0f, a3 = 0.0f; // each oscillator summed over unison (meters)

            for (int u = 0; u < count; ++u)
            {
                auto* o = osc[u];
                const float o3 = need3 ? o[2].tick (inc[u][2], wave3, duty[2]) : 0.0f;
                const float mod = extMod != nullptr ? extMod[i] : o3 + modNoise * (nz - o3);

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
                a1 += o1;
                a2 += o2;
                a3 += o3;
            }
            if (meter != nullptr)
            {
                pk1 = std::max (pk1, std::abs (a1));
                pk2 = std::max (pk2, std::abs (a2));
                pk3 = std::max (pk3, std::abs (a3));
                pkN = std::max (pkN, std::abs (nz));
            }

            const float nOut = nz * p.noiseVolume;
            const float l = sumL * gainNorm + nOut, r = sumR * gainNorm + nOut;
            outL[i] += l * engVol;
            outR[i] += r * engVol;
            modOut[i] = 0.5f * (l + r);
        }
        if (meter != nullptr)
        {
            // Unison sub-voices are panned; about 0.7 of the sum reaches each side.
            const float g = gainNorm * std::abs (engVol) * 0.7071f;
            meter[0] = pk1 * std::abs (vol1) * g;
            meter[1] = pk2 * std::abs (vol2) * g;
            meter[2] = pk3 * std::abs (vol3) * g;
            meter[3] = pkN * std::abs (p.noiseVolume * engVol);
        }
    }

private:
    float sampleRate = 48000.0f;
    BlepOscillator osc[kMaxUnison][kNumOscs];
    float driftOffset[kMaxUnison][kNumOscs] {};
    ColourNoise noise;
    Rng rng;
    int activeUnison = 1;

};
} // namespace cs
