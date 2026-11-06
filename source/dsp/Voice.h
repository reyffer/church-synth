#pragma once

#include "AnalogEngine.h"
#include "UtilityEngine.h"
#include "WavetableEngine.h"
#include "MultiFilter.h"
#include "Envelope.h"
#include "ModMatrix.h"
#include "ModSources.h"

namespace cs
{
// Output buses a voice writes to. Main = pre-FX sum; FxA / FxB = Split routing or
// utility osc routed to a bus; Direct = bypasses the FX.
enum VoiceBus : int { BusMain = 0, BusFxA, BusFxB, BusDirect, NumVoiceBuses };
struct VoiceOut
{
    float* l[NumVoiceBuses];
    float* r[NumVoiceBuses];
};

class Voice
{
public:
    void setSampleRate (float sr) noexcept
    {
        sampleRate = sr;
        ampSmooth = 1.0f - std::exp (-1.0f / (0.002f * sr)); // ~2 ms
        for (int e = 0; e < kNumEngines; ++e)
        {
            analog[e].setSampleRate (sr);
            wavetable[e].setSampleRate (sr);
        }
        utility.setSampleRate (sr);
        env2.setSampleRate (sr);
        env3.setSampleRate (sr);
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
    bool singleUnison = false; // chord arp: engines in Unison Chord mode play one note (the arp spreads the chord)

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
            startEngine (p, e, seed + 7919u * (uint32_t) e);
        utility.startNote (p.utility, seed + 104729u);
        for (auto& m : prevMod)
            for (auto& v : m)
                v = 0.0f;
        for (auto& f : filt)
            f.reset();
        for (int f = 0; f < kNumFilters; ++f)
            cutoffLog[f] = std::log2 (cutoffFor (p, f));
        env.kill();
        env.gateOn();
        envPrev = 0.0f;
        ampGainSnap = true;
        startMod (p, seed);
    }

    // Voice Modulator step for each Random slot, chosen by the SynthCore at note-on.
    void setVoiceModSteps (const int* steps) noexcept
    {
        for (int k = 0; k < kNumRandoms; ++k)
            vmStep[k] = steps[k];
    }

    // Retrigger/legato on a voice that is already sounding (no phase reset, no click).
    // glideSeconds >= 0 overrides the Glide setting (sequencer slides).
    void retarget (const SynthParams& p, int newNote, float vel, bool retriggerEnv, bool glideFromCurrent, uint64_t stamp,
                   float glideSeconds = -1.0f) noexcept
    {
        note = newNote;
        velocity = vel;
        age = stamp;
        gate = true;
        sustained = false;
        targetPitch = (float) newNote;
        if (! glideFromCurrent)
            pitch = targetPitch;
        glideStart (p, glideSeconds);
        if (retriggerEnv)
        {
            env.gateOn();
            if (p.env2Retrig == EnvRetrigPolyKbd) env2.gateOn();
            if (p.env3Retrig == EnvRetrigPolyKbd) env3.gateOn();
            retriggerPolySources (p);
        }
        else
        {
            if (p.env2Retrig == EnvRetrigLegatoKbd) env2.gateOn();
            if (p.env3Retrig == EnvRetrigLegatoKbd) env3.gateOn();
        }
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
        {
            env.gateOff();
            env2.gateOff();
            env3.gateOff();
        }
    }

    void kill() noexcept
    {
        active = false;
        gate = false;
        stealing = false;
        sustained = false;
        env.kill();
        env2.kill();
        env3.kill();
    }

    // Renders n <= kBlock samples and adds them to outL/outR, after computing
    // this voice's modulation sources and applying the routing.
    void render (const SynthParams& p, const GlobalMod& g, const ModRouting& routing,
                 float bendSemis, int n, const VoiceOut& out) noexcept
    {
        if (! active)
            return;
        // Source settings use last block's modulated values (so LFO rate etc. can be modulated).
        const SynthParams& sp = localValid ? local : p;
        computeSources (sp, g, n);

        const SynthParams* use = &p;
        const bool chordSingle = singleUnison && hasChordEngine (p);
        if (routing.numTouched > 0 || chordSingle)
        {
            local = p;
            if (routing.numTouched > 0)
                applyRouting (routing);
            if (chordSingle)
                for (auto& e : local.engine)
                    if (e.unisonMode == UnisonChord)
                        e.unisonOn = false;
            use = &local;
            localValid = true;
        }
        else
            localValid = false;

        const float u = srcUnipolar (use->ampModSource);
        ampGain = 1.0f - std::clamp (use->ampVelocity, 0.0f, 1.0f) * (1.0f - u);
        renderAudio (*use, bendSemis, n, out);
    }

    float sourceValue (int s) const noexcept { return (s >= 0 && s < NumModSources) ? src[s] : 0.0f; }

    static bool hasChordEngine (const SynthParams& p) noexcept
    {
        for (const auto& e : p.engine)
            if (e.on && e.unisonOn && e.unisonMode == UnisonChord)
                return true;
        return false;
    }

private:
    void renderAudio (const SynthParams& p, float bendSemis, int n, const VoiceOut& out) noexcept
    {
        // --- pitch / glide (control rate) ----------------------------------
        if (std::abs (pitch - targetPitch) > 0.0f)
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
        // Cross-mod: an engine can use the other engine's output as its modulator.
        // The source is rendered first; if both modulate each other, engine 1
        // hears engine 2's previous block (16 samples late, inherently wild).
        float in1L[kBlock] {}, in1R[kBlock] {}, in2L[kBlock] {}, in2R[kBlock] {};
        float postL[kBlock] {}, postR[kBlock] {};
        float modBuf[kNumEngines][kBlock] {};
        const auto& e1 = p.engine[0];
        const auto& e2 = p.engine[1];
        const bool x1 = e1.on && e2.on && e1.modFromEngine;
        const bool x2 = e1.on && e2.on && e2.modFromEngine;
        const int order[2] = { (x1 && ! x2) ? 1 : 0, (x1 && ! x2) ? 0 : 1 };
        for (int k = 0; k < kNumEngines; ++k)
        {
            const int e = order[k];
            const auto& ep = p.engine[(size_t) e];
            if (! ep.on)
                continue;
            if (ep.type != lastType[e])
                startEngine (p, e, (uint32_t) age * 2654435761u + (uint32_t) e);

            const int other = 1 - e;
            const bool cross = e == 0 ? x1 : x2;
            const float* ext = nullptr;
            if (cross)
                ext = (k == 0) ? prevMod[other] : modBuf[other];

            float eL[kBlock] {}, eR[kBlock] {};
            if (ep.type == EngineWavetable)
                wavetable[e].render (ep, notePitch, fixedPitch, n, eL, eR, ext, modBuf[e]);
            else
                analog[e].render (ep, notePitch, fixedPitch, n, eL, eR, ext, modBuf[e]);
            if (needFollow)
            {
                float pk = 0.0f;
                for (int i = 0; i < n; ++i)
                    pk = std::max (pk, std::abs (eL[i]) + std::abs (eR[i]));
                followPeak[FollowEngine1 + e] = 0.5f * pk;
            }

            const float toF2 = std::clamp (ep.filterMix, 0.0f, 1.0f), toF1 = 1.0f - toF2;
            for (int i = 0; i < n; ++i)
            {
                in1L[i] += eL[i] * toF1;
                in1R[i] += eR[i] * toF1;
                in2L[i] += eL[i] * toF2;
                in2R[i] += eR[i] * toF2;
            }
        }
        for (int e = 0; e < kNumEngines; ++e)
            for (int i = 0; i < kBlock; ++i)
                prevMod[e][i] = i < n ? modBuf[e][i] : 0.0f;

        // Utility renders into its own buffers (also used as an FM source and by the follower).
        float u1L[kBlock] {}, u1R[kBlock] {}, u2L[kBlock] {}, u2R[kBlock] {};
        utility.render (p.utility, notePitch, fixedPitch, n, u1L, u1R, u2L, u2R, postL, postR);
        float uPk = 0.0f;
        for (int i = 0; i < n; ++i)
        {
            in1L[i] += u1L[i];
            in1R[i] += u1R[i];
            in2L[i] += u2L[i];
            in2R[i] += u2R[i];
            uMono[i] = 0.5f * (u1L[i] + u1R[i] + u2L[i] + u2R[i]) + postL[i];
            uPk = std::max (uPk, std::abs (uMono[i]));
        }
        followPeak[FollowUtility] = uPk;
        const int postBus = p.utility.oscRoute == RouteFxA ? BusFxA : p.utility.oscRoute == RouteFxB ? BusFxB
                          : p.utility.oscRoute == RouteDirect ? BusDirect : BusMain;

        // --- filters (coefficients at control rate, cutoff smoothed in log2) ----
        float fGainL[kNumFilters], fGainR[kNumFilters];
        for (int f = 0; f < kNumFilters; ++f)
        {
            const auto& fp = p.filter[(size_t) f];
            const float target = std::log2 (cutoffFor (p, f) * std::exp2 (fp.keytrack * (notePitch - 60.0f) / 12.0f));
            cutoffLog[f] += 0.35f * (target - cutoffLog[f]);
            if (fp.on)
            {
                if (fp.type != lastFilterType[f])
                {
                    filt[f].reset();
                    lastFilterType[f] = fp.type;
                }
                filt[f].setup (fp, std::exp2 (cutoffLog[f]));
            }
            float l, r;
            panGains (fp.pan, l, r);
            fGainL[f] = l * fp.volume;
            fGainR[f] = r * fp.volume;
        }
        const bool split = p.filterOutput != RoutingSum;
        const float par = split ? 1.0f : std::clamp (p.filterRouting, 0.0f, 1.0f), ser = 1.0f - par;
        const bool f1On = p.filter[0].on, f2On = p.filter[1].on;
        const int busF1 = ! split ? BusMain : p.filterOutput == RoutingSplit ? BusFxA : BusFxB;
        const int busF2 = ! split ? BusMain : p.filterOutput == RoutingSplit ? BusFxB : BusFxA;

        // Per-sample FM source for each filter.
        const float* fmSrc[kNumFilters];
        bool needNoiseFm = false;
        for (int f = 0; f < kNumFilters; ++f)
        {
            const auto& fp = p.filter[(size_t) f];
            switch (fp.fmSource)
            {
                case FmSrcEngine2: fmSrc[f] = modBuf[1]; break;
                case FmSrcUtility: fmSrc[f] = uMono; break;
                case FmSrcNoise: fmSrc[f] = noiseFm; needNoiseFm = needNoiseFm || fp.fm > 0.0f; break;
                case FmSrcEngine1:
                default: fmSrc[f] = modBuf[0]; break;
            }
        }
        if (needNoiseFm)
            for (int i = 0; i < n; ++i)
                noiseFm[i] = modRng.bi();

        // --- amp envelope (control-rate points, per-sample interpolation) ----
        const float envNow = env.advance (p.ampEnv, n);
        if (env.takeInstantStart())
            envPrev = envNow; // 0 ms attack: start at full level on the first sample
        float vpL, vpR;
        panGains (p.voicePan, vpL, vpR);
        const float fadeStep = stealing ? -1.0f / (0.003f * sampleRate) : 0.0f; // 3 ms steal fade
        const float envStep = (envNow - envPrev) / (float) n;
        float envV = envPrev;
        // Amp Mod gain (velocity, tremolo, gates) is smoothed per sample so a legato
        // note with a new velocity or a stepped modulator doesn't click.
        if (ampGainSnap)
        {
            ampGainNow = ampGain;
            ampGainSnap = false;
        }
        const float gainTarget = ampGain;
        float gainNow = ampGainNow;
        float fPk1 = 0.0f, fPk2 = 0.0f, fPkV = 0.0f;

        for (int i = 0; i < n; ++i)
        {
            float aL = in1L[i], aR = in1R[i];
            if (f1On)
                filt[0].process (aL, aR, fmSrc[0][i]);
            aL *= fGainL[0];
            aR *= fGainR[0];

            float bL = in2L[i] + ser * aL, bR = in2R[i] + ser * aR;
            if (f2On)
                filt[1].process (bL, bR, fmSrc[1][i]);
            bL *= fGainL[1];
            bR *= fGainR[1];


            envV += envStep;
            if (stealing)
                fade = std::max (0.0f, fade + fadeStep);
            gainNow += (gainTarget - gainNow) * ampSmooth;
            const float amp = envV * gainNow * fade;
            const float gl = amp * vpL, gr = amp * vpR;
            if (split)
            {
                out.l[busF1][i] += aL * gl;
                out.r[busF1][i] += aR * gr;
                out.l[busF2][i] += bL * gl;
                out.r[busF2][i] += bR * gr;
            }
            else
            {
                out.l[BusMain][i] += (bL + par * aL) * gl;
                out.r[BusMain][i] += (bR + par * aR) * gr;
            }
            out.l[postBus][i] += postL[i] * gl;
            out.r[postBus][i] += postR[i] * gr;
            if (needFollow)
            {
                fPk1 = std::max (fPk1, std::abs (aL));
                fPk2 = std::max (fPk2, std::abs (bL));
                fPkV = std::max (fPkV, std::abs ((bL + aL + postL[i]) * amp));
            }
        }
        envPrev = envNow;
        ampGainNow = gainNow;
        if (needFollow)
        {
            followPeak[FollowFilter1] = fPk1;
            followPeak[FollowFilter2] = fPk2;
            followPeak[FollowVoice] = fPkV;
        }

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
    // --- modulation state ----------------------------------------------------
    Envelope env2, env3;
    Lfo lfo[kNumLfos];
    FunctionGen func[kNumFuncs];
    RandomGen rnd[kNumRandoms];
    Rng modRng;
    float src[NumModSources] {};
    bool srcBipolar[NumModSources] {};
    float combLag[kNumCombs] {};
    float followLevel[kNumCombs] {};
    float followPeak[FollowInputCount] {};
    bool needFollow = false;
    int vmStep[kNumRandoms] {};
    float ampGain = 1.0f;     // target Amp Mod gain for this block
    float ampGainNow = 1.0f;  // smoothed, per sample
    float ampSmooth = 0.01f;
    bool ampGainSnap = true;  // a fresh note starts at its target gain
    SynthParams local;
    bool localValid = false;

    void startMod (const SynthParams& p, uint32_t seed) noexcept
    {
        modRng.seed (seed ^ 0xA5A5A5A5u);
        env2.kill();
        env3.kill();
        env2.gateOn();
        env3.gateOn();
        for (int k = 0; k < kNumLfos; ++k)
            lfo[k].reset (p.lfo[(size_t) k], modRng);
        for (int k = 0; k < kNumFuncs; ++k)
            func[k].reset();
        for (int k = 0; k < kNumRandoms; ++k)
            rnd[k].reset (modRng);
        for (int k = 0; k < kNumCombs; ++k)
            combLag[k] = followLevel[k] = 0.0f;
        for (auto& f : followPeak)
            f = 0.0f;
        localValid = false;
    }

    void retriggerPolySources (const SynthParams& p) noexcept
    {
        for (int k = 0; k < kNumLfos; ++k)
            if (p.lfo[(size_t) k].retrig == RetrigPolyKbd)
                lfo[k].reset (p.lfo[(size_t) k], modRng);
        for (int k = 0; k < kNumFuncs; ++k)
            if (p.func[(size_t) k].retrig == RetrigPolyKbd)
                func[k].reset();
        for (int k = 0; k < kNumRandoms; ++k)
            if (p.random[(size_t) k].retrig == RetrigPolyKbd)
                rnd[k].reset (modRng);
    }

    float srcUnipolar (int s) const noexcept
    {
        if (s < 0 || s >= NumModSources)
            return 1.0f;
        return srcBipolar[s] ? 0.5f * (src[s] + 1.0f) : src[s];
    }

    // Was LFO k's cycle restarted this block (as seen by this voice)?
    bool lfoWrapped (const GlobalMod& g, int k) const noexcept
    {
        return g.lfoIsGlobal[k] ? g.lfoWrap[k] : lfo[k].wrapped;
    }

    void computeSources (const SynthParams& p, const GlobalMod& g, int n) noexcept
    {
        const float dt = (float) n / sampleRate;
        const float keyNorm = std::clamp ((pitch - 60.0f) / 60.0f, -1.0f, 1.0f);

        src[SrcVelocity] = applyCurve (velocity, p.curves.velocity);
        src[SrcAftertouch] = applyCurve (std::max (g.aftertouch, polyPressure), p.curves.aftertouch);
        src[SrcModWheel] = g.modWheel;
        src[SrcKeytrack] = applyCurve (keyNorm, p.curves.keytrack);
        src[SrcExpression] = g.expression;
        srcBipolar[SrcKeytrack] = true;

        // LFOs (in order, so one LFO can retrigger a later one in the same block)
        for (int k = 0; k < kNumLfos; ++k)
        {
            const auto& lp = p.lfo[(size_t) k];
            srcBipolar[SrcLfo1 + k] = lp.bipolar;
            if (! g.used[SrcLfo1 + k])
                continue;
            if (g.lfoIsGlobal[k])
                src[SrcLfo1 + k] = g.lfo[k];
            else
            {
                const int r = lp.retrig;
                if (r >= RetrigLfo1 && r <= RetrigLfo3 && r - RetrigLfo1 != k && lfoWrapped (g, r - RetrigLfo1))
                    lfo[k].reset (lp, modRng);
                float hz = rateToHz (lp.rate, g.transport);
                if (std::abs (lp.keytrack) > 1.0e-3f)
                    hz *= std::exp2 (lp.keytrack * (pitch - 60.0f) / 12.0f);
                lfo[k].tick (lp, hz, n, sampleRate, modRng);
                src[SrcLfo1 + k] = lfo[k].value;
            }
            srcBipolar[SrcLfo1 + k] = lp.bipolar;
        }

        // Envelopes 2 / 3 (can be retriggered by an LFO)
        auto lfoRetrig = [&] (int envRetrig) {
            const int k = envRetrig - EnvRetrigLfo1;
            return envRetrig >= EnvRetrigLfo1 && lfoWrapped (g, k);
        };
        if (lfoRetrig (p.env2Retrig) && gate) env2.gateOn();
        if (lfoRetrig (p.env3Retrig) && gate) env3.gateOn();
        src[SrcEnvAmp] = env.getLevel();
        if (g.used[SrcEnv2]) src[SrcEnv2] = env2.advance (p.env2, n);
        if (g.used[SrcEnv3]) src[SrcEnv3] = env3.advance (p.env3, n);

        // Functions
        for (int k = 0; k < kNumFuncs; ++k)
        {
            const auto& fp = p.func[(size_t) k];
            srcBipolar[SrcFunc1 + k] = fp.bipolar || fp.scale < 0.0f;
            if (! g.used[SrcFunc1 + k])
                continue;
            if (g.funcIsGlobal[k])
                src[SrcFunc1 + k] = g.func[k];
            else
            {
                const int r = fp.retrig;
                if (r >= RetrigLfo1 && r <= RetrigLfo3 && lfoWrapped (g, r - RetrigLfo1))
                    func[k].reset();
                func[k].tick (fp, rateToHz (fp.rate, g.transport), gate, n, sampleRate);
                src[SrcFunc1 + k] = func[k].value;
            }
            srcBipolar[SrcFunc1 + k] = fp.bipolar || fp.scale < 0.0f;
        }

        // Random generators (Sample & Hold can sample the sources above)
        for (int k = 0; k < kNumRandoms; ++k)
        {
            const auto& rp = p.random[(size_t) k];
            srcBipolar[SrcRand1 + k] = rp.bipolar;
            if (! g.used[SrcRand1 + k])
                continue;
            if (g.randIsGlobal[k] && rp.type != RandVoiceMod)
                src[SrcRand1 + k] = g.rnd[k];
            else
            {
                const int r = rp.retrig;
                if (r >= RetrigLfo1 && r <= RetrigLfo3 && lfoWrapped (g, r - RetrigLfo1))
                    rnd[k].reset (modRng);
                rnd[k].tick (rp, rateToHz (rp.rate, g.transport), n, sampleRate, modRng, src, vmStep[k]);
                src[SrcRand1 + k] = rnd[k].value;
            }
            srcBipolar[SrcRand1 + k] = rp.bipolar;
        }

        // Combinators (in order; a later one used as input gives last block's value)
        needFollow = false;
        for (int k = 0; k < kNumCombs; ++k)
        {
            const auto& cp = p.comb[(size_t) k];
            srcBipolar[SrcComb1 + k] = cp.type != CombEnvFollow;
            if (! g.used[SrcComb1 + k])
                continue;
            if (cp.type == CombEnvFollow)
            {
                needFollow = true;
                const float in = followPeak[std::clamp (cp.followInput, 0, FollowInputCount - 1)] * cp.followGain;
                const float thr = std::clamp (cp.followThreshold, 0.0f, 0.99f);
                const float x = std::clamp ((in - thr) / (1.0f - thr), 0.0f, 1.0f);
                const float t = x > followLevel[k] ? cp.followAttack : cp.followRelease;
                followLevel[k] += (x - followLevel[k]) * (t > 0.0f ? 1.0f - std::exp (-dt / t) : 1.0f);
            }
            const float a = sourceValue (cp.source), m = sourceValue (cp.mod);
            src[SrcComb1 + k] = combine (cp, a, m, combLag[k], followLevel[k], dt);
            srcBipolar[SrcComb1 + k] = cp.type != CombEnvFollow;
        }

        for (int k = 0; k < 4; ++k)
            src[SrcMacro1 + k] = p.macro[(size_t) k];
    }

    void applyRouting (const ModRouting& routing) noexcept
    {
        float add[kMaxRoutes] {};
        for (int r = 0; r < routing.count; ++r)
        {
            const int slot = routing.slotOf[r];
            if (slot < 0)
                continue;
            const auto& rt = routing.routes[r];
            float v = src[rt.source] * rt.amount;
            if (rt.sidechain >= 0 && rt.sidechain < NumModSources)
                v *= 1.0f - rt.scAmount + rt.scAmount * src[rt.sidechain];
            add[slot] += v;
        }

        const auto& dests = modDestinations();
        bool coarseTouched[kNumEngines] {}, utilCoarseTouched = false;
        for (int i = 0; i < routing.numTouched; ++i)
        {
            const int d = routing.touched[i];
            const auto& dest = dests[(size_t) d];
            if (dest.global)
                continue; // FX parameters are modulated once in SynthCore, not per voice
            float& f = dest.ref (local);
            f = dest.fromNorm (dest.toNorm (f) + add[i]);
            if (d == coarseDest[0]) coarseTouched[0] = true;
            if (d == coarseDest[1]) coarseTouched[1] = true;
            if (d == utilCoarseDest) utilCoarseTouched = true;
        }

        // Mod Quantize: snap modulated coarse tuning to the allowed intervals.
        for (int e = 0; e < kNumEngines; ++e)
            if (coarseTouched[e])
                local.engine[(size_t) e].coarse = quantizeSemitones (local.engine[(size_t) e].coarse, local.coarseQuantMask[(size_t) e]);
        if (utilCoarseTouched)
            local.utility.oscCoarse = quantizeSemitones (local.utility.oscCoarse, local.utilityQuantMask);
    }

    inline static const int coarseDest[kNumEngines] = { findModDest ("e1_coarse"), findModDest ("e2_coarse") };
    inline static const int utilCoarseDest = findModDest ("u_osc_coarse");

public:
    float polyPressure = 0.0f; // set by the SynthCore from poly aftertouch

private:
    float uMono[kBlock] {}, noiseFm[kBlock] {};
    int lastFilterType[kNumFilters] { -1, -1 };

    AnalogEngine analog[kNumEngines];
    WavetableEngine wavetable[kNumEngines];
    UtilityEngine utility;
    int lastType[kNumEngines] { EngineAnalog, EngineAnalog };
    float prevMod[kNumEngines][kBlock] {};

    void startEngine (const SynthParams& p, int e, uint32_t seed) noexcept
    {
        const auto& ep = p.engine[(size_t) e];
        lastType[e] = ep.type;
        if (ep.type == EngineWavetable)
            wavetable[e].startNote (ep, seed);
        else
            analog[e].startNote (ep, seed);
    }
    MultiFilter filt[kNumFilters];
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
    void glideStart (const SynthParams& p, float glideOverride = -1.0f) noexcept
    {
        const float dist = std::abs (targetPitch - pitch);
        const float time = glideOverride >= 0.0f ? glideOverride : p.glide;
        glideRate = (time > 0.0f && dist > 0.0f) ? dist / (time * sampleRate) : 0.0f;
        if (glideRate <= 0.0f)
            pitch = targetPitch;
    }

    static float cutoffFor (const SynthParams& p, int f) noexcept
    {
        return std::clamp (p.filter[(size_t) f].cutoff, 16.0f, 22000.0f);
    }
};
} // namespace cs
