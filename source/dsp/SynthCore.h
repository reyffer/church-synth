#pragma once

// Top-level synth: voice allocation, MIDI state and block rendering.
// Real-time safe: no allocation, locks or I/O after setSampleRate().

#include "Voice.h"

namespace cs
{
class SynthCore
{
public:
    SynthParams params;

    void setSampleRate (float sr) noexcept;
    void reset() noexcept;

    void noteOn (int note, float velocity) noexcept;  // velocity 0..1
    void noteOff (int note) noexcept;
    void setSustain (bool down) noexcept;
    void setPitchBend (float normalised) noexcept { bend = std::clamp (normalised, -1.0f, 1.0f); } // -1..1
    void setModWheel (float v) noexcept { modWheel = v; }
    void allNotesOff() noexcept;

    // Renders n samples into outL/outR (overwrites).
    void render (float* outL, float* outR, int n) noexcept;

    int activeVoiceCount() const noexcept;

private:
    float sampleRate = 48000.0f;
    Voice voices[kMaxVoices];
    uint64_t stamp = 0;
    uint32_t seedCounter = 12345u;
    int rotateIndex = 0;
    float bend = 0.0f, modWheel = 0.0f;
    bool sustainPedal = false, holdLatched = false;
    float lastPitch = 60.0f;

    // Mono note stack (most recent last).
    int heldNotes[128] {};
    float heldVel[128] {};
    int numHeld = 0;

    bool isMono() const noexcept { return params.voicing == VoicingMono || params.voicing == VoicingMonoLegato; }
    bool sustainActive() const noexcept { return sustainPedal || params.hold; }
    float glideFrom (bool otherKeysHeld) const noexcept;
    uint32_t nextSeed() noexcept { return seedCounter += 0x6D2B79F5u; }

    void monoNoteOn (int note, float vel) noexcept;
    void monoNoteOff (int note) noexcept;
    void polyNoteOn (int note, float vel) noexcept;
    int findVoiceToSteal (int limit) const noexcept;
    void pushHeld (int note, float vel) noexcept;
    void removeHeld (int note) noexcept;
};
} // namespace cs
