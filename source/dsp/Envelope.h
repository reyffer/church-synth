#pragma once

// ADSR / ADR envelope with per-segment curves, evaluated at control rate
// (the voice interpolates between control points per sample).

#include "MathUtil.h"
#include "Params.h"

namespace cs
{
class Envelope
{
public:
    enum Stage { Idle, Attack, Decay, Sustain, Release };

    void setSampleRate (float sr) noexcept { sampleRate = sr; }

    // Starts (or restarts) the attack from the current level, so retriggers never click.
    void gateOn() noexcept
    {
        startLevel = level;
        stage = Attack;
        t = 0.0f;
    }

    void gateOff() noexcept
    {
        if (stage == Idle)
            return;
        if (adrMode && stage != Release)
        {
            releasePending = true; // ADR: finish the decay, then release
            return;
        }
        beginRelease();
    }

    void kill() noexcept
    {
        stage = Idle;
        level = 0.0f;
        releasePending = false;
    }

    bool isActive() const noexcept { return stage != Idle; }
    Stage getStage() const noexcept { return stage; }
    float getLevel() const noexcept { return level; }

    // Advance n samples and return the new level.
    float advance (const EnvParams& p, int n) noexcept
    {
        adrMode = p.adr;
        const float dtSec = (float) n / sampleRate;

        switch (stage)
        {
            case Idle:
                level = 0.0f;
                break;

            case Attack:
                t += dtSec / std::max (p.attack, 0.0005f);
                if (t >= 1.0f)
                {
                    level = 1.0f;
                    stage = Decay;
                    t = 0.0f;
                }
                else
                    level = startLevel + (1.0f - startLevel) * shape (t, -p.attCurve);
                break;

            case Decay:
            {
                t += dtSec / std::max (p.decay, 0.0005f);
                const float s = p.sustain;
                if (t >= 1.0f)
                {
                    level = s;
                    if (adrMode)
                        beginRelease();
                    else
                        stage = Sustain;
                }
                else
                    level = s + (1.0f - s) * (1.0f - shape (t, p.decCurve));
                break;
            }

            case Sustain:
                level = p.sustain; // follows the knob while held
                if (releasePending)
                    beginRelease();
                break;

            case Release:
            {
                const float rel = p.releaseLink ? p.decay : p.release;
                t += dtSec / std::max (rel, 0.0005f);
                if (t >= 1.0f)
                {
                    level = 0.0f;
                    stage = Idle;
                }
                else
                    level = startLevel * (1.0f - shape (t, p.decCurve));
                break;
            }
        }
        return level;
    }

private:
    float sampleRate = 48000.0f;
    Stage stage = Idle;
    float level = 0.0f, startLevel = 0.0f, t = 0.0f;
    bool adrMode = false, releasePending = false;

    void beginRelease() noexcept
    {
        startLevel = level;
        stage = Release;
        t = 0.0f;
        releasePending = false;
    }

    // Maps linear progress t (0..1) to a curved progress. curve 0 = linear.
    static float shape (float t, float curve) noexcept
    {
        const float k = curve * 0.3f;
        if (std::abs (k) < 1.0e-3f)
            return t;
        return std::expm1 (-k * t) / std::expm1 (-k);
    }
};
} // namespace cs
