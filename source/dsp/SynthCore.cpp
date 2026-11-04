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
    numHeld = 0;
    sustainPedal = false;
    bend = 0.0f;
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

void SynthCore::noteOn (int note, float velocity) noexcept
{
    note = std::clamp (note, 0, 127);
    velocity = std::clamp (velocity, 0.0f, 1.0f);
    ++stamp;
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
            voices[i].start (params, note, vel, startPitch, stamp, nextSeed());
            rotateIndex = i;
            return;
        }
    }

    // Steal: prefer the oldest released voice, otherwise the oldest held one.
    const int s = findVoiceToSteal (limit);
    voices[s].steal (note, vel, startPitch, stamp, nextSeed());
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

    if (! v.isActive())
    {
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

    for (int pos = 0; pos < n; pos += kBlock)
    {
        const int len = std::min (kBlock, n - pos);
        for (auto& v : voices)
            v.render (params, bendSemis, len, outL + pos, outR + pos);
    }

    const float g = params.masterVolume * kVoiceGain;
    for (int i = 0; i < n; ++i)
    {
        outL[i] *= g;
        outR[i] *= g;
    }
}
} // namespace cs
