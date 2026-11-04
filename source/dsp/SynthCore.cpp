#include "SynthCore.h"

#include <cstring>

namespace cs
{
void SynthCore::setSampleRate (float sr) noexcept
{
    sampleRate = sr;
    for (auto& v : voices)
        v.setSampleRate (sr);
    reset();
}

void SynthCore::reset() noexcept
{
    for (auto& v : voices)
        v.kill();
    for (auto& row : vmNoteStep)
        for (auto& x : row)
            x = -1;
    for (int k = 0; k < kNumLfos; ++k)
        gLfo[k].reset (params.lfo[(size_t) k], gRng);
    for (auto& f : gFunc)
        f.reset();
    for (auto& r : gRand)
        r.reset (gRng);
    numHeld = 0;
    sustainPedal = false;
    bend = 0.0f;
}

void SynthCore::computeUsedSources() noexcept
{
    bool* used = global.used;
    for (int s = 0; s < NumModSources; ++s)
        used[s] = false;
    auto mark = [used] (int s) { if (s >= 0 && s < NumModSources) used[s] = true; };

    for (int r = 0; r < routing.count; ++r)
        if (routing.slotOf[r] >= 0)
        {
            mark (routing.routes[r].source);
            if (std::abs (routing.routes[r].amount) > 0.0f)
                mark (routing.routes[r].sidechain);
        }
    if (params.ampVelocity > 0.0f)
        mark (params.ampModSource);

    // Follow dependencies until nothing new is marked.
    for (int pass = 0; pass < 6; ++pass)
    {
        bool changed = false;
        auto markDep = [&] (int s) {
            if (s >= 0 && s < NumModSources && ! used[s]) { used[s] = true; changed = true; }
        };
        auto retrigDep = [&] (int retrig) {
            if (retrig >= RetrigLfo1 && retrig <= RetrigLfo3) markDep (SrcLfo1 + retrig - RetrigLfo1);
        };
        for (int k = 0; k < kNumCombs; ++k)
            if (used[SrcComb1 + k])
            {
                markDep (params.comb[(size_t) k].source);
                markDep (params.comb[(size_t) k].mod);
            }
        for (int k = 0; k < kNumRandoms; ++k)
            if (used[SrcRand1 + k])
            {
                if (params.random[(size_t) k].type == RandSampleHold) markDep (params.random[(size_t) k].shSource);
                retrigDep (params.random[(size_t) k].retrig);
            }
        for (int k = 0; k < kNumLfos; ++k)
            if (used[SrcLfo1 + k]) retrigDep (params.lfo[(size_t) k].retrig);
        for (int k = 0; k < kNumFuncs; ++k)
            if (used[SrcFunc1 + k]) retrigDep (params.func[(size_t) k].retrig);
        if (used[SrcEnv2] && params.env2Retrig >= EnvRetrigLfo1) markDep (SrcLfo1 + params.env2Retrig - EnvRetrigLfo1);
        if (used[SrcEnv3] && params.env3Retrig >= EnvRetrigLfo1) markDep (SrcLfo1 + params.env3Retrig - EnvRetrigLfo1);
        if (! changed)
            break;
    }
}

void SynthCore::tickGlobalSources (int n, double blockPpq) noexcept
{
    bool anyGate = false;
    for (const auto& v : voices)
        anyGate = anyGate || v.isGateOn();

    global.transport = transport;
    global.transport.ppq = blockPpq;

    auto triggered = [this] (int retrig) {
        return (retrig == RetrigMonoKbd && monoTrigger) || (retrig == RetrigLegatoKbd && legatoTrigger);
    };

    for (int k = 0; k < kNumLfos; ++k)
    {
        const auto& lp = params.lfo[(size_t) k];
        global.lfoIsGlobal[k] = retrigIsGlobal (lp.retrig);
        if (! global.lfoIsGlobal[k] || ! global.used[SrcLfo1 + k])
            continue;
        if (triggered (lp.retrig))
            gLfo[k].reset (lp, gRng);
        // Free-running + synced + host playing: lock the phase to the song position.
        float phaseOverride = -1.0f;
        if (lp.retrig == RetrigFreeRun && lp.rate.sync && transport.playing)
        {
            const double beats = kSyncDivs[std::clamp (lp.rate.div, 0, kNumSyncDivs - 1)].beats;
            const double ph = blockPpq / beats + (double) lp.phase;
            phaseOverride = (float) (ph - std::floor (ph));
        }
        gLfo[k].tick (lp, rateToHz (lp.rate, transport), n, sampleRate, gRng, phaseOverride);
        global.lfo[k] = gLfo[k].value;
        global.lfoWrap[k] = gLfo[k].wrapped || triggered (lp.retrig);
    }
    for (int k = 0; k < kNumFuncs; ++k)
    {
        const auto& fp = params.func[(size_t) k];
        global.funcIsGlobal[k] = retrigIsGlobal (fp.retrig);
        if (! global.funcIsGlobal[k] || ! global.used[SrcFunc1 + k])
            continue;
        if (triggered (fp.retrig))
            gFunc[k].reset();
        gFunc[k].tick (fp, rateToHz (fp.rate, transport), anyGate, n, sampleRate);
        global.func[k] = gFunc[k].value;
    }
    float globalSrc[NumModSources] {};
    globalSrc[SrcModWheel] = modWheel;
    globalSrc[SrcAftertouch] = channelPressure;
    globalSrc[SrcExpression] = expression;
    for (int k = 0; k < kNumLfos; ++k) globalSrc[SrcLfo1 + k] = global.lfo[k];
    for (int k = 0; k < kNumFuncs; ++k) globalSrc[SrcFunc1 + k] = global.func[k];
    for (int k = 0; k < 4; ++k) globalSrc[SrcMacro1 + k] = params.macro[(size_t) k];
    for (int k = 0; k < kNumRandoms; ++k)
    {
        const auto& rp = params.random[(size_t) k];
        global.randIsGlobal[k] = retrigIsGlobal (rp.retrig);
        if (! global.randIsGlobal[k] || rp.type == RandVoiceMod || ! global.used[SrcRand1 + k])
            continue;
        if (triggered (rp.retrig))
            gRand[k].reset (gRng);
        gRand[k].tick (rp, rateToHz (rp.rate, transport), n, sampleRate, gRng, globalSrc, 0);
        global.rnd[k] = gRand[k].value;
    }
    monoTrigger = legatoTrigger = false;
}

int SynthCore::activeVoiceCount() const noexcept
{
    int c = 0;
    for (const auto& v : voices)
        c += v.isActive() ? 1 : 0;
    return c;
}

float SynthCore::glideFrom (bool otherKeysHeld) const noexcept
{
    if (params.glide <= 0.0f || (params.glideLegato && ! otherKeysHeld))
        return -1.0f; // no glide
    return lastPitch;
}

void SynthCore::pushHeld (int note, float vel) noexcept
{
    removeHeld (note);
    if (numHeld < 128)
    {
        heldNotes[numHeld] = note;
        heldVel[numHeld] = vel;
        ++numHeld;
    }
}

void SynthCore::removeHeld (int note) noexcept
{
    for (int i = 0; i < numHeld; ++i)
        if (heldNotes[i] == note)
        {
            for (int j = i; j < numHeld - 1; ++j)
            {
                heldNotes[j] = heldNotes[j + 1];
                heldVel[j] = heldVel[j + 1];
            }
            --numHeld;
            return;
        }
}

void SynthCore::setPolyPressure (int note, float v) noexcept
{
    for (auto& voice : voices)
        if (voice.isActive() && voice.getNote() == note)
            voice.polyPressure = v;
}

float SynthCore::debugSourceValue (int source) const noexcept
{
    return voices[lastStarted].sourceValue (source);
}

void SynthCore::assignVoiceModSteps (Voice& v, int note) noexcept
{
    int steps[kNumRandoms];
    for (int k = 0; k < kNumRandoms; ++k)
    {
        const auto& rp = params.random[(size_t) k];
        const int count = std::clamp (rp.vmVoices, 1, 8);
        switch (rp.vmMode)
        {
            case VmReassign:
                if (vmNoteStep[k][note] < 0)
                    vmNoteStep[k][note] = vmCounter[k]++ % count;
                steps[k] = vmNoteStep[k][note] % count;
                break;
            case VmRandom:
                steps[k] = (int) (gRng.uni() * (float) count) % count;
                break;
            case VmCycle:
            default:
                steps[k] = vmCounter[k]++ % count;
                break;
        }
    }
    v.setVoiceModSteps (steps);
}

void SynthCore::noteOn (int note, float velocity) noexcept
{
    note = std::clamp (note, 0, 127);
    velocity = std::clamp (velocity, 0.0f, 1.0f);
    ++stamp;
    monoTrigger = true;
    if (numHeld == 0)
        legatoTrigger = true;
    if (isMono())
        monoNoteOn (note, velocity);
    else
        polyNoteOn (note, velocity);
    lastPitch = (float) note;
}

void SynthCore::noteOff (int note) noexcept
{
    if (isMono())
    {
        monoNoteOff (note);
        return;
    }
    removeHeld (note);
    for (auto& v : voices)
        if (v.isActive() && v.isGateOn() && v.getNote() == note)
        {
            if (sustainActive())
                v.sustained = true;
            else
                v.release();
        }
}

void SynthCore::setSustain (bool down) noexcept
{
    sustainPedal = down;
    if (sustainActive())
        return;
    for (auto& v : voices)
        if (v.sustained)
            v.release();
    if (isMono() && numHeld == 0 && voices[0].isGateOn())
        voices[0].release();
}

void SynthCore::allNotesOff() noexcept
{
    numHeld = 0;
    for (auto& v : voices)
        if (v.isActive())
            v.release();
}

// ---------------------------------------------------------------------------
void SynthCore::polyNoteOn (int note, float vel) noexcept
{
    pushHeld (note, vel);
    const int limit = std::clamp (params.maxVoices, 1, kMaxVoices);
    const bool others = numHeld > 1;
    const float from = glideFrom (others);
    const float startPitch = from < 0.0f ? (float) note : from;

    // Reassign mode: reuse the voice that last played this note.
    if (params.voicing == VoicingPolyReassign)
    {
        for (int i = 0; i < limit; ++i)
        {
            auto& v = voices[i];
            if (v.getNote() == note && v.isActive() && ! v.isStealing())
            {
                v.retarget (params, note, vel, true, from >= 0.0f, stamp);
                return;
            }
        }
    }

    // A free voice (round-robin from the last one used in Rotate mode).
    for (int k = 0; k < limit; ++k)
    {
        const int i = (params.voicing == VoicingPolyRotate) ? (rotateIndex + 1 + k) % limit : k;
        if (! voices[i].isActive())
        {
            assignVoiceModSteps (voices[i], note);
            voices[i].start (params, note, vel, startPitch, stamp, nextSeed());
            lastStarted = i;
            rotateIndex = i;
            return;
        }
    }

    // Steal: prefer the oldest released voice, otherwise the oldest held one.
    const int s = findVoiceToSteal (limit);
    assignVoiceModSteps (voices[s], note);
    voices[s].steal (note, vel, startPitch, stamp, nextSeed());
    lastStarted = s;
    rotateIndex = s;
}

int SynthCore::findVoiceToSteal (int limit) const noexcept
{
    int best = 0;
    bool bestReleased = false;
    uint64_t bestAge = ~0ull;
    for (int i = 0; i < limit; ++i)
    {
        const auto& v = voices[i];
        const bool rel = v.isReleasing() || v.sustained;
        if ((rel && ! bestReleased) || (rel == bestReleased && v.getAge() < bestAge))
        {
            best = i;
            bestReleased = rel;
            bestAge = v.getAge();
        }
    }
    return best;
}

// ---------------------------------------------------------------------------
void SynthCore::monoNoteOn (int note, float vel) noexcept
{
    const bool wasHeld = numHeld > 0;
    pushHeld (note, vel);
    auto& v = voices[0];
    const float from = glideFrom (wasHeld);

    lastStarted = 0;
    if (! v.isActive())
    {
        assignVoiceModSteps (v, note);
        v.start (params, note, vel, from < 0.0f ? (float) note : from, stamp, nextSeed());
        return;
    }
    const bool legato = params.voicing == VoicingMonoLegato && wasHeld && v.isGateOn();
    v.retarget (params, note, vel, ! legato, from >= 0.0f, stamp);
}

void SynthCore::monoNoteOff (int note) noexcept
{
    removeHeld (note);
    auto& v = voices[0];
    if (! v.isActive() || ! v.isGateOn())
        return;

    if (numHeld > 0)
    {
        // Fall back to the most recent key still held (legato, no retrigger).
        const int prev = heldNotes[numHeld - 1];
        if (prev != v.getNote())
        {
            const float from = glideFrom (true);
            v.retarget (params, prev, heldVel[numHeld - 1], false, from >= 0.0f, stamp);
            lastPitch = (float) prev;
        }
        return;
    }
    if (sustainActive())
        v.sustained = true;
    else
        v.release();
}

// ---------------------------------------------------------------------------
void SynthCore::render (float* outL, float* outR, int n) noexcept
{
    std::memset (outL, 0, sizeof (float) * (size_t) n);
    std::memset (outR, 0, sizeof (float) * (size_t) n);

    // Voices above the current polyphony limit are released.
    const int limit = isMono() ? 1 : std::clamp (params.maxVoices, 1, kMaxVoices);
    for (int i = limit; i < kMaxVoices; ++i)
        if (voices[i].isGateOn())
            voices[i].release();

    const float bendSemis = bend >= 0.0f ? bend * params.bendUp : bend * params.bendDown;
    constexpr float kVoiceGain = 0.25f; // headroom for chords

    computeUsedSources();
    global.modWheel = modWheel;
    global.aftertouch = channelPressure;
    global.expression = expression;
    const double ppqPerSample = transport.bpm / 60.0 / (double) sampleRate;

    for (int pos = 0; pos < n; pos += kBlock)
    {
        const int len = std::min (kBlock, n - pos);
        tickGlobalSources (len, transport.ppq + ppqPerSample * pos);
        for (auto& v : voices)
            v.render (params, global, routing, bendSemis, len, outL + pos, outR + pos);
    }
    transport.ppq += ppqPerSample * n; // keeps free-running sync coherent if the host doesn't update it

    const float g = params.masterVolume * kVoiceGain;
    for (int i = 0; i < n; ++i)
    {
        outL[i] *= g;
        outR[i] *= g;
    }
}
} // namespace cs
