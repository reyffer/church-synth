#pragma once

// Top-level synth: voice allocation, MIDI state and block rendering.
// Real-time safe: no allocation, locks or I/O after setSampleRate().

#include "SeqArp.h"
#include "Voice.h"
#include "fx/FxRack.h"

#include <atomic>

namespace cs
{
class SynthCore
{
public:
    SynthParams params;

    void setSampleRate (float sr) noexcept;
    void reset() noexcept;

    // Keyboard input. With the Seq/Arp on, keys go to it and it plays the voices.
    void noteOn (int note, float velocity) noexcept;  // velocity 0..1
    void noteOff (int note) noexcept;

    // Sequencer pattern (copy; cheap) and live state for the editor.
    void setSeqPattern (const SeqPattern& p) noexcept { seq.setPattern (p); }
    const SeqArp& getSeq() const noexcept { return seq; }
    void setSustain (bool down) noexcept;
    void setPitchBend (float normalised) noexcept { bend = std::clamp (normalised, -1.0f, 1.0f); } // -1..1
    void setModWheel (float v) noexcept { modWheel = v; }
    void setChannelPressure (float v) noexcept { channelPressure = v; }
    void setPolyPressure (int note, float v) noexcept;
    void setExpression (float v) noexcept { expression = v; }
    void setTransport (const Transport& t) noexcept { transport = t; }

    // Copies and prepares a routing table (cheap; call when it changes).
    void setRouting (const ModRouting& r) noexcept
    {
        routing = r;
        routing.prepare();
    }
    const ModRouting& getRouting() const noexcept { return routing; }

    // Current value of a source on the most recently started voice (for UI/tests).
    float debugSourceValue (int source) const noexcept;
    float voiceSourceValue (int voice, int source) const noexcept { return voices[std::clamp (voice, 0, kMaxVoices - 1)].sourceValue (source); }
    void allNotesOff() noexcept;

    // Renders n samples into outL/outR (overwrites): voices -> FX -> master.
    void render (float* outL, float* outR, int n) noexcept;

    int activeVoiceCount() const noexcept;

    // Editor telemetry: published after each render while uiActive is set (cheap, lock-free).
    std::atomic<bool> monoCheck { false }; // editor's Mono button: listen in mono (not saved)
    std::atomic<bool> uiActive { false };
    std::atomic<float> uiSource[NumModSources] {};
    std::atomic<int> uiVoices { 0 };
    // Sound LEDs (see Meter in Voice.h): the loudest voice's peak per part since the editor
    // last read it (it reads with exchange (0)).
    mutable std::atomic<float> uiMeter[NumMeters] {}; // mutable: the editor reads them with exchange (0)
    // Travelling dots: per voice slot. age < 0: not sounding (else larger = newer).
    // env: Envelope::getPosition (-1 idle or not computed); lfo / func: 0..1, -1 when
    // the source isn't running (nothing uses it). Global-retrig LFOs / functions report
    // the shared instance on every voice.
    struct UiVoice
    {
        std::atomic<int> age { -1 };
        std::atomic<float> env[3] {}, envLevel[3] {}, lfo[kNumLfos] {}, func[kNumFuncs] {};
    };
    UiVoice uiVoice[kMaxVoices];

    // FX: effect objects are owned by an fx::FxHost (see FxHost.h) bound to this rack.
    fx::FxRack& getFxRack() noexcept { return fxRack; }
    uint64_t getRenderEpoch() const noexcept { return renderEpoch.load (std::memory_order_acquire); }

private:
    float sampleRate = 48000.0f;
    Voice voices[kMaxVoices];
    uint64_t stamp = 0;
    uint32_t seedCounter = 12345u;
    int rotateIndex = 0;
    float bend = 0.0f, modWheel = 0.0f, channelPressure = 0.0f, expression = 0.0f;
    Transport transport;
    ModRouting routing;

    // Global (shared) instances for generators in Mono / Legato / Free-running modes.
    Lfo gLfo[kNumLfos];
    FunctionGen gFunc[kNumFuncs];
    RandomGen gRand[kNumRandoms];
    Rng gRng;
    GlobalMod global;
    bool monoTrigger = false, legatoTrigger = false;
    int lastStarted = 0;

    // Voice Modulator bookkeeping.
    int vmCounter[kNumRandoms] {};
    int vmNoteStep[kNumRandoms][128];
    void assignVoiceModSteps (Voice& v, int note) noexcept;
    void tickGlobalSources (int n, double blockPpq) noexcept;
    bool anyVoiceActive() const noexcept;
    void trackSilence (const float* outL, const float* outR, int n) noexcept;
    int silentSamples = 0, idleAfter = 12000;

public:
    uint64_t idleBlocks = 0; // render calls skipped while idle (tests / benchmark)

private:
    void computeUsedSources() noexcept;
    void renderChunk (float* outL, float* outR, int n) noexcept;
    bool sustainPedal = false, holdLatched = false;
    float lastPitch = 60.0f;
    static constexpr int kMaxChunk = 2048;
    fx::FxRack fxRack;
    std::atomic<uint64_t> renderEpoch { 0 };
    SynthParams fxScratch; // params with global (FX) modulation applied
    float globalSourceValue (int s) const noexcept;
    const SynthParams& modulatedGlobalParams() noexcept; // params + routes to global destinations (FX, Seq)
    const SynthParams* gParams = &params;
    static constexpr float kLimitThreshold = 0.891f; // -1 dBFS
    float limEnv = 0.0f, limRelease = 0.0001f;

    // Sequencer / arpeggiator.
    struct SeqSink final : SeqArp::Sink
    {
        SynthCore& core;
        explicit SeqSink (SynthCore& c) : core (c) {}
        void seqNoteOn (int note, float velocity, int slideFrom, float slideSeconds) noexcept override;
        void seqNoteOff (int note) noexcept override;
    };
    SeqArp seq;
    SeqSink seqSink { *this };
    bool seqWasOn = false, holdWas = false;
    bool keyDown[128] {};
    float keyVel[128] {};
    bool chordArp = false;
    void updateSeqState() noexcept;
    void playNoteOn (int note, float velocity, bool fromSeq) noexcept;
    void playNoteOff (int note, bool ignoreSustain) noexcept;
    float bus[NumVoiceBuses][2][kMaxChunk] {};

    // Mono note stack (most recent last).
    int heldNotes[128] {};
    float heldVel[128] {};
    int numHeld = 0;

    bool isMono() const noexcept { return params.voicing == VoicingMono || params.voicing == VoicingMonoLegato; }
    bool sustainActive() const noexcept { return sustainPedal || params.hold; }
    float glideFrom (bool otherKeysHeld) const noexcept;
    uint32_t nextSeed() noexcept { return seedCounter += 0x6D2B79F5u; }

    void monoNoteOn (int note, float vel) noexcept;
    void monoNoteOffFallback() noexcept;
    void polyNoteOn (int note, float vel) noexcept;
    int findVoiceToSteal (int limit) const noexcept;
    void pushHeld (int note, float vel) noexcept;
    void removeHeld (int note) noexcept;
};
} // namespace cs
