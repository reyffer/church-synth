#pragma once

// Wavetable engine: mip-mapped wavetable oscillator with frame morphing,
// FM / ring mod, phase modulation (+ retrigger modes), phase transform
// (7 shapes), wavefolding (3 shapes), unison and a modulation oscillator
// (or the other engine) feeding all the "mod" knobs. One instance per voice.

#include "Content.h"
#include "Oscillator.h"
#include "Unison.h"

namespace cs
{
class WavetableEngine
{
public:
    void setSampleRate (float sr) noexcept
    {
        sampleRate = sr;
        modNoise.setSampleRate (sr);
    }

    void startNote (const EngineParams& p, uint32_t seed) noexcept
    {
        rng.seed (seed);
        modNoise.seed (seed * 747796405u + 3u);
        modOsc.reset (0.0f);
        const auto& w = p.wt;
        if (w.retrig != RetrigFree)
            for (int u = 0; u < kMaxUnison; ++u)
                phase[u] = startPhase (w, u);
        activeUnison = unison::unisonCount (p);
    }

    // Same contract as AnalogEngine::render.
    // meter: if non-null, receives the peak of the wavetable oscillator (meter[0]) and of the
    // modulator heard through its own volume (meter[1]), after the engine volume.
    void render (const EngineParams& p, float notePitch, float fixedPitch,
                 int n, float* outL, float* outR, const float* extMod, float* modOut, float* meter = nullptr) noexcept
    {
        const auto& w = p.wt;
        const Wavetable* table = w.table;

        const int count = unison::unisonCount (p);
        if (count > activeUnison)
            for (int u = activeUnison; u < count; ++u)
                phase[u] = wrap01 (w.initPhase + rng.uni());
        activeUnison = count;

        float offset[kMaxUnison], panL[kMaxUnison], panR[kMaxUnison];
        unison::layoutUnison (p, count, offset, panL, panR);
        const float gainNorm = unison::unisonNorm (p, count);

        // --- pitch (control rate) ------------------------------------------
        const float keyPitch = p.keytrack ? notePitch : fixedPitch;
        const float enginePitch = keyPitch + p.coarse + p.fine;
        float inc[kMaxUnison];
        for (int u = 0; u < count; ++u)
            inc[u] = std::min (pitchToHz (enginePitch + offset[u]) / sampleRate, 0.45f);

        float modInc;
        switch (w.modTuneMode)
        {
            case ModTuneAbsolute: modInc = pitchToHz (keyPitch + w.modCoarse + w.modFine) / sampleRate; break;
            case ModTuneRelative: modInc = pitchToHz (enginePitch + w.modCoarse + w.modFine) / sampleRate; break;
            case ModTuneFixed: modInc = std::max (w.modHz, 0.01f) * std::exp2 (w.modFine / 12.0f) / sampleRate; break;
            case ModTuneRatio:
            default: modInc = pitchToHz (enginePitch) * std::max (w.modRatio, 0.01f) * std::exp2 (w.modFine / 12.0f) / sampleRate; break;
        }
        modInc = std::clamp (modInc, 0.0f, 0.45f);

        // --- frame selection --------------------------------------------------
        int frameA = 0, frameB = 0;
        float frameMix = 0.0f;
        if (table != nullptr)
        {
            const float pos = std::clamp (w.position, 0.0f, 1.0f) * (float) (table->getNumFrames() - 1);
            if (w.morph)
            {
                frameA = (int) pos;
                frameB = std::min (frameA + 1, table->getNumFrames() - 1);
                frameMix = pos - (float) frameA;
            }
            else
                frameA = frameB = (int) std::lround (pos);
        }

        // --- feature amounts ------------------------------------------------
        const float fmK = w.fmType == FmExponential ? 4.0f * w.fmAmount : 8.0f * w.fmAmount * w.fmAmount;
        const bool doFm = w.fmAmount > 0.0f;
        const float pmK = 0.5f * w.pmAmount;
        const bool doPd = w.pdAmount > 0.0f || w.pdMod > 0.0f;
        const bool doFold = w.foldAmount > 0.0f || w.foldMod > 0.0f;
        const bool needMod = doFm || pmK > 0.0f || w.pdMod > 0.0f || w.foldMod > 0.0f
                             || w.modVolume > 0.0f || w.retrig == RetrigModOsc;
        const bool modIsNoise = w.modWave >= ModBlueNoise;
        if (needMod && modIsNoise)
            modNoise.setColour (noiseColourFor (w.modWave));

        // Mip level from the fastest increment this block can reach.
        int level[kMaxUnison];
        for (int u = 0; u < count; ++u)
        {
            float worst = inc[u];
            if (doFm && w.fmType != FmRing)
                worst *= w.fmType == FmExponential ? std::exp2 (fmK) : 1.0f + fmK;
            level[u] = Wavetable::levelFor (worst);
        }

        const float wtVol = w.wtVolume, modVol = w.modVolume, engVol = p.volume;

        float pkW = 0.0f, pkM = 0.0f; // meters
        for (int i = 0; i < n; ++i)
        {
            // Modulator: the other engine, a noise colour or a basic wave.
            float mod = 0.0f, modWrapD = -1.0f;
            if (extMod != nullptr)
                mod = extMod[i];
            else if (needMod)
            {
                if (modIsNoise)
                    mod = modNoise.tick();
                else
                {
                    mod = modOsc.tick (modInc, w.modWave, 0.5f);
                    modWrapD = modOsc.lastWrapD;
                }
            }

            float sumL = 0.0f, sumR = 0.0f;
            if (table != nullptr)
            {
                for (int u = 0; u < count; ++u)
                {
                    float uinc = inc[u];
                    if (doFm)
                    {
                        if (w.fmType == FmLinear)
                            uinc *= 1.0f + fmK * mod; // through-zero allowed
                        else if (w.fmType == FmExponential)
                            uinc *= std::exp2 (fmK * mod);
                    }
                    float ph = phase[u] + uinc;
                    ph -= std::floor (ph);
                    if (w.retrig == RetrigModOsc && modWrapD >= 0.0f)
                        ph = wrap01 (w.initPhase + uinc * modWrapD);
                    phase[u] = ph;

                    float rp = ph;
                    if (pmK > 0.0f)
                        rp = wrap01 (rp + pmK * mod);
                    if (doPd)
                        rp = transformPhase (rp, w.pdShape, std::clamp (w.pdAmount + w.pdMod * mod, 0.0f, 1.0f));

                    float v = read (*table, level[u], frameA, frameB, frameMix, rp);

                    if (doFold)
                        v = fold (v, w.foldShape, std::clamp (w.foldAmount + w.foldMod * mod, 0.0f, 1.0f));
                    if (doFm && w.fmType == FmRing)
                        v *= 1.0f + w.fmAmount * (mod - 1.0f);

                    sumL += v * panL[u];
                    sumR += v * panR[u];
                }
            }

            const float mOut = mod * modVol;
            const float l = sumL * gainNorm * wtVol + mOut;
            const float r = sumR * gainNorm * wtVol + mOut;
            outL[i] += l * engVol;
            outR[i] += r * engVol;
            modOut[i] = 0.5f * (l + r);
            if (meter != nullptr)
            {
                pkW = std::max (pkW, 0.5f * std::abs (sumL + sumR));
                pkM = std::max (pkM, std::abs (mOut));
            }
        }
        if (meter != nullptr)
        {
            meter[0] = pkW * gainNorm * std::abs (wtVol * engVol);
            meter[1] = pkM * std::abs (engVol);
            meter[2] = meter[3] = 0.0f;
        }
    }

    // Exposed for tests.
    static float transformPhase (float p, int shape, float a) noexcept
    {
        if (a <= 0.0f)
            return p;
        switch (shape)
        {
            case PdPulseWidth: // squeeze the first half-cycle into a shorter span
            {
                const float k = 0.5f - 0.48f * a;
                return p < k ? p * (0.5f / k) : 0.5f + (p - k) * (0.5f / (1.0f - k));
            }
            case PdSkew: // rational bend of the phase ramp
            {
                const float k = 1.0f + 7.0f * a;
                return p / (p + (1.0f - p) * k);
            }
            case PdMirror: // play forwards then backwards
            {
                const float m = p < 0.5f ? 2.0f * p : 2.0f - 2.0f * p;
                return p + a * (m - p);
            }
            case PdSync: // hard-sync-like multiplication of the phase
                return wrap01 (p * (1.0f + 7.0f * a));
            case PdSteps: // quantise the phase into fewer steps
            {
                const float steps = 2.0f + (1.0f - a) * (1.0f - a) * 126.0f;
                return std::floor (p * steps) / steps;
            }
            case PdBend: // sine warp of the phase
                return wrap01 (p + 0.15f * a * SineTable::get() (p));
            case PdSqueeze: // compress the cycle towards the middle, flat edges
            default:
                return std::clamp (0.5f + (p - 0.5f) * (1.0f + 3.0f * a), 0.0f, 0.9999f);
        }
    }

    static float fold (float x, int shape, float a) noexcept
    {
        if (a <= 0.0f)
            return x;
        const float g = 1.0f + 7.0f * a;
        switch (shape)
        {
            case FoldSine:
                return SineTable::get() (wrap01 (0.25f * g * x)); // sin(pi/2 * g * x)
            case FoldTriangle:
            {
                const float t = wrap01 ((g * x + 1.0f) * 0.25f);
                return 1.0f - 4.0f * std::abs (t - 0.5f);
            }
            case FoldSoft:
            default:
            {
                const float folded = SineTable::get() (wrap01 (0.25f * g * x));
                return softClip (1.5f * (x + a * (folded - x)));
            }
        }
    }

private:
    float sampleRate = 48000.0f;
    float phase[kMaxUnison] {};
    int activeUnison = 1;
    BlepOscillator modOsc;
    ColourNoise modNoise;
    Rng rng;

    float startPhase (const WavetableParams& w, int u) noexcept
    {
        if (w.retrig == RetrigRandom)
            return rng.uni();
        return wrap01 (w.initPhase + (u == 0 ? 0.0f : w.unisonPhase * rng.uni()));
    }

    static float noiseColourFor (int modWave) noexcept
    {
        switch (modWave)
        {
            case ModBlueNoise: return 0.6f;
            case ModWhiteNoise: return 0.0f;
            case ModPinkNoise: return -0.35f;
            case ModRedNoise: return -0.7f;
            default: return -1.0f; // rumble
        }
    }

    static float read (const Wavetable& t, int level, int fa, int fb, float mixAB, float p) noexcept
    {
        const int size = t.getLevelSize (level);
        const float x = p * (float) size;
        int idx = (int) x;
        const float fr = x - (float) idx;
        idx = std::min (idx, size - 1);
        const float* a = t.frame (level, fa);
        const float va = a[idx] + fr * (a[idx + 1] - a[idx]);
        if (fa == fb || mixAB <= 0.0f)
            return va;
        const float* b = t.frame (level, fb);
        const float vb = b[idx] + fr * (b[idx + 1] - b[idx]);
        return va + mixAB * (vb - va);
    }
};
} // namespace cs
