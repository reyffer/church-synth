#pragma once

#include "AnalogEngine.h"
#include "ClassicFilter.h"
#include "Envelope.h"

namespace cs
{
class Voice
{
public:
    void setSampleRate (float sr) noexcept
    {
        sampleRate = sr;
        for (auto& e : eng)
            e.setSampleRate (sr);
        for (auto& f : filt)
            f.setSampleRate (sr);
        env.setSampleRate (sr);
    }

    bool isActive() const noexcept { return active; }
    bool isGateOn() const noexcept { return gate; }
    bool isReleasing() const noexcept { return active && ! gate; }
    bool isStealing() const noexcept { return stealing; }
    int getNote() const noexcept { return stealing ? pendingNote : note; }
    uint64_t getAge() const noexcept { return age; }
    float getLevel() const noexcept { return env.getLevel(); }

    bool sustained = false; // gate released while sustain pedal is down

    // Begin a note on an idle voice (resets oscillators/filters).
    void start (const SynthParams& p, int newNote, float vel, float fromPitch, uint64_t stamp, uint32_t seed) noexcept
    {
        note = newNote;
        velocity = vel;
        age = stamp;
        gate = true;
        active = true;
        sustained = false;
        stealing = false;
        fade = 1.0f;
        targetPitch = (float) newNote;
        pitch = fromPitch;
        glideStart (p);
        for (int e = 0; e < kNumEngines; ++e)
            eng[e].startNote (p.engine[e], seed + 7919u * (uint32_t) e);
        for (auto& f : filt)
            f.reset();
        for (int f = 0; f < kNumFilters; ++f)
            cutoffLog[f] = std::log2 (cutoffFor (p, f));
        env.kill();
        env.gateOn();
        envPrev = 0.0f;
    }

    // Retrigger/legato on a voice that is already sounding (no phase reset, no click).
    void retarget (const SynthParams& p, int newNote, float vel, bool retriggerEnv, bool glideFromCurrent, uint64_t stamp) noexcept
    {
        note = newNote;
        velocity = vel;
        age = stamp;
        gate = true;
        sustained = false;
        targetPitch = (float) newNote;
        if (! glideFromCurrent)
            pitch = targetPitch;
        glideStart (p);
        if (retriggerEnv)
            env.gateOn();
    }

    // Fade out quickly, then start the pending note on this voice.
    void steal (int newNote, float vel, float fromPitch, uint64_t stamp, uint32_t seed) noexcept
    {
        stealing = true;
        pendingReleased = false;
        pendingNote = newNote;
        pendingVel = vel;
        pendingFrom = fromPitch;
        pendingStamp = stamp;
        pendingSeed = seed;
        age = stamp;
        gate = true;
    }

    void release() noexcept
    {
        gate = false;
        sustained = false;
        if (stealing)
            pendingReleased = true; // honour the note-off once the pending note starts
        else
            env.gateOff();
    }

    void kill() noexcept
    {
        active = false;
        gate = false;
        stealing = false;
        sustained = false;
        env.kill();
    }

    // Renders n <= kBlock samples and adds them to outL/outR.
    void render (const SynthParams& p, float bendSemis, int n, float* outL, float* outR) noexcept
    {
        if (! active)
            return;

        // --- pitch / glide (control rate) ----------------------------------
        if (pitch != targetPitch)
        {
            const float step = glideRate * (float) n;
            if (std::abs (targetPitch - pitch) <= step || glideRate <= 0.0f)
                pitch = targetPitch;
            else
                pitch += targetPitch > pitch ? step : -step;
        }
        const float transpose = 12.0f * (float) p.transposeOct + p.transposeFine + bendSemis;
        const float notePitch = pitch + transpose;
        const float fixedPitch = 60.0f + transpose;

        // --- engines --------------------------------------------------------
        float in1L[kBlock] {}, in1R[kBlock] {}, in2L[kBlock] {}, in2R[kBlock] {};
        for (int e = 0; e < kNumEngines; ++e)
        {
            const auto& ep = p.engine[e];
            if (! ep.on)
                continue;
            float eL[kBlock] {}, eR[kBlock] {};
            eng[e].render (ep, notePitch, fixedPitch, n, eL, eR);
            const float toF2 = std::clamp (ep.filterMix, 0.0f, 1.0f), toF1 = 1.0f - toF2;
            for (int i = 0; i < n; ++i)
            {
                in1L[i] += eL[i] * toF1;
                in1R[i] += eR[i] * toF1;
                in2L[i] += eL[i] * toF2;
                in2R[i] += eR[i] * toF2;
            }
        }

        // --- filters (coefficients at control rate, cutoff smoothed in log2) ----
        float fGainL[kNumFilters], fGainR[kNumFilters];
        for (int f = 0; f < kNumFilters; ++f)
        {
            const auto& fp = p.filter[f];
            const float target = std::log2 (cutoffFor (p, f) * std::exp2 (fp.keytrack * (notePitch - 60.0f) / 12.0f));
            cutoffLog[f] += 0.35f * (target - cutoffLog[f]);
            if (fp.on)
                filt[f].setup (fp.mode, std::exp2 (cutoffLog[f]), fp.reso, fp.drive, fp.analog);
            float l, r;
            panGains (fp.pan, l, r);
            fGainL[f] = l * fp.volume;
            fGainR[f] = r * fp.volume;
        }
        const float par = std::clamp (p.filterRouting, 0.0f, 1.0f), ser = 1.0f - par;
        const bool f1On = p.filter[0].on, f2On = p.filter[1].on;

        // --- amp envelope (control-rate points, per-sample interpolation) ----
        const float envNow = env.advance (p.ampEnv, n);
        const float velGain = 1.0f - std::clamp (p.ampVelocity, 0.0f, 1.0f) * (1.0f - velocity);
        float vpL, vpR;
        panGains (p.voicePan, vpL, vpR);
        const float fadeStep = stealing ? -1.0f / (0.003f * sampleRate) : 0.0f; // 3 ms steal fade
        const float envStep = (envNow - envPrev) / (float) n;
        float envV = envPrev;

        for (int i = 0; i < n; ++i)
        {
            float aL = in1L[i], aR = in1R[i];
            if (f1On)
                filt[0].process (aL, aR);
            aL *= fGainL[0];
            aR *= fGainR[0];

            float bL = in2L[i] + ser * aL, bR = in2R[i] + ser * aR;
            if (f2On)
                filt[1].process (bL, bR);
            bL *= fGainL[1];
            bR *= fGainR[1];

            const float yL = bL + par * aL, yR = bR + par * aR;

            envV += envStep;
            if (stealing)
                fade = std::max (0.0f, fade + fadeStep);
            const float amp = envV * velGain * fade;
            outL[i] += yL * amp * vpL;
            outR[i] += yR * amp * vpR;
        }
        envPrev = envNow;

        if (stealing && fade <= 0.0f)
        {
            const bool releasedMeanwhile = pendingReleased;
            stealing = false;
            start (p, pendingNote, pendingVel, pendingFrom, pendingStamp, pendingSeed);
            if (releasedMeanwhile)
                release();
            return;
        }
        if (! env.isActive())
            active = false;
    }

private:
    float sampleRate = 48000.0f;
    AnalogEngine eng[kNumEngines];
    ClassicFilter filt[kNumFilters];
    Envelope env;

    bool active = false, gate = false, stealing = false, pendingReleased = false;
    int note = -1;
    float velocity = 1.0f;
    uint64_t age = 0;
    float pitch = 60.0f, targetPitch = 60.0f, glideRate = 0.0f;
    float envPrev = 0.0f, fade = 1.0f;
    float cutoffLog[kNumFilters] { 14.0f, 14.0f };

    int pendingNote = 60;
    float pendingVel = 1.0f, pendingFrom = 60.0f;
    uint64_t pendingStamp = 0;
    uint32_t pendingSeed = 1;

    // Constant-time glide: covers the whole interval in p.glide seconds.
    void glideStart (const SynthParams& p) noexcept
    {
        const float dist = std::abs (targetPitch - pitch);
        glideRate = (p.glide > 0.0f && dist > 0.0f) ? dist / (p.glide * sampleRate) : 0.0f;
        if (glideRate <= 0.0f)
            pitch = targetPitch;
    }

    static float cutoffFor (const SynthParams& p, int f) noexcept
    {
        return std::clamp (p.filter[f].cutoff, 16.0f, 22000.0f);
    }
};
} // namespace cs
