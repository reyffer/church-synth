#pragma once

// Sequencer / arpeggiator: turns the keys being held into timed notes.
//
// Timing is kept in "steps" (one step = the Rate division). When the host is
// playing, the step position follows the song position, so patterns line up
// with the bar; otherwise the pattern starts the moment the first key goes
// down and runs on its own clock at the host (or internal) tempo.
//
// The pattern is 16 columns x 6 tracks. Each track can have its own length,
// rate divider and random amount. Random offsets are derived from a seed, so
// "Regen" is just a new seed and a pattern always plays back the same way.
//
// Real-time safe: fixed-size arrays only, no allocation.

#include "ModSources.h"
#include "Params.h"

#include <array>
#include <atomic>
#include <cstdint>

namespace cs
{
constexpr int kSeqSteps = 16;

// Track order is stored in patterns: append only.
enum SeqTrackId : int { TrackSteps = 0, TrackPitch, TrackVelocity, TrackOctave, TrackGate, TrackSlide, NumSeqTracks };
inline constexpr const char* kSeqTrackNames[NumSeqTracks] = { "Steps", "Pitch", "Velocity", "Octave", "Gate", "Slide" };

// Per-track rate dividers (the track moves once every N steps).
constexpr int kNumSeqDividers = 7;
inline constexpr int kSeqDividers[kNumSeqDividers] = { 1, 2, 3, 4, 6, 8, 16 };

// Auto-regen periods in quarter-note beats (index 0 = off). Append only.
constexpr int kNumAutoRegen = 9;
inline constexpr const char* kAutoRegenNames[kNumAutoRegen] = { "Off", "1/16 bar", "1/8 bar", "1/4 bar", "1/2 bar",
                                                                "1 bar", "2 bars", "4 bars", "8 bars" };
inline constexpr double kAutoRegenBeats[kNumAutoRegen] = { 0.0, 0.25, 0.5, 1.0, 2.0, 4.0, 8.0, 16.0, 32.0 };

// Scales: weight per semitone above the played note (0 = not in the scale).
// Weights only matter for random pitches (generative scales favour some notes).
// Order is stored in presets: append only.
struct SeqScale
{
    const char* name;
    std::array<float, 12> weights;
};
constexpr int kNumSeqScales = 18;
constexpr int kScaleCustom = 17;
extern const SeqScale kSeqScales[kNumSeqScales];

inline constexpr const char* kSeqModeNames[SeqModeCount] = { "Arp", "Seq" };
inline constexpr const char* kSeqDirectionNames[SeqDirectionCount] = { "Forward", "Backward", "Ping-Pong", "Random" };
inline constexpr const char* kArpOrderNames[ArpOrderCount] = { "As Played", "Up", "Down", "Up & Down Incl.", "Up & Down Excl.", "Random" };

struct SeqStep
{
    bool on = true;
    float prob = 1.0f;     // 0..1
    int pitch = 0;         // semitones above the played note, 0..12 (Seq mode)
    float velocity = 0.8f; // 0..1
    int octave = 0;        // -2..2
    float gate = 0.5f;     // length in steps, 0.05..4
    float slide = 0.0f;    // 0 = no slide, else glide time 20 ms .. 500 ms
    uint32_t lockSeed = 0; // != 0: this column keeps its random values on Regen
};

struct SeqTrackSettings
{
    bool enabled = true;
    int length = kSeqSteps; // 1..16 (used with Polymetry)
    int divider = 1;        // one of kSeqDividers
    float random = 0.0f;    // 0..1
};

struct SeqPattern
{
    std::array<SeqStep, kSeqSteps> steps {};
    std::array<SeqTrackSettings, NumSeqTracks> tracks {};
    uint32_t seed = 1;
    std::array<float, 12> customWeights { 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1 };
};

// What a column actually plays once random offsets are applied.
struct SeqResolved
{
    bool on;
    float prob;
    int pitch, octave;
    float velocity, gate, slide;
};

// Values of one track at one column (random offsets applied). Used by the
// engine and by the editor (to show what will play) and "Apply random".
SeqResolved seqResolve (const SeqPattern& pat, const SeqParams& p, const int* columns, uint32_t liveSeed) noexcept;

// Pattern editing helpers (message thread).
void seqRotate (SeqPattern& pat, int track, int direction) noexcept; // track < 0: all tracks
void seqApplyRandom (SeqPattern& pat, const SeqParams& p, uint32_t liveSeed) noexcept; // bake offsets into the steps
void seqClearRandom (SeqPattern& pat) noexcept; // random amounts and locks back to 0
uint32_t seqNextSeed (uint32_t seed) noexcept;
float seqSlideSeconds (float slide) noexcept;
int seqSnapToScale (int semis, const std::array<float, 12>& weights) noexcept;

class SeqArp
{
public:
    struct Sink
    {
        // slideFrom >= 0: glide the voice playing that note to the new one (legato, no retrigger).
        virtual void seqNoteOn (int note, float velocity, int slideFrom, float slideSeconds) noexcept = 0;
        virtual void seqNoteOff (int note) noexcept = 0;
        virtual ~Sink() = default;
    };

    void setSampleRate (float sr) noexcept { sampleRate = sr; }
    void reset() noexcept; // forget keys and sounding notes (no events)
    void setPattern (const SeqPattern& p) noexcept;
    const SeqPattern& getPattern() const noexcept { return pattern; }

    // Chord arp: intervals of an engine in Unison Chord mode (count 0 = off).
    void setChord (const int* intervals, int count) noexcept;

    // Keyboard input. hold = latch (Hold button), pedal = sustain pedal.
    void keyDown (int note, float velocity, bool hold, bool pedal) noexcept;
    void keyUp (int note, bool hold, bool pedal) noexcept;
    void pedalChanged (bool hold, bool pedal) noexcept;
    int numKeys() const noexcept { return keyCount; }
    int keyNote (int i) const noexcept { return keys[(size_t) i].note; }
    float keyVelocity (int i) const noexcept { return keys[(size_t) i].vel; }
    bool keyPhysical (int i) const noexcept { return keys[(size_t) i].physical; }

    // Ends every sounding note now (sends note-offs) and stops.
    void stop (Sink& sink) noexcept;
    bool busy() const noexcept { return running || playingCount > 0; }

    // Fires the events that are due, then returns how many samples to render
    // before the next one (1..maxSamples).
    int run (int maxSamples, const SeqParams& p, const Transport& t, Sink& sink) noexcept;

    // For the editor: column of the Steps track now playing (-1 = stopped), live seed.
    std::atomic<int> uiStep { -1 };
    std::atomic<uint32_t> uiSeed { 1 };

private:
    struct Key
    {
        int note;
        float vel;
        bool physical;
    };
    struct Playing
    {
        int note;
        double offTime; // in steps
    };

    float sampleRate = 48000.0f;
    SeqPattern pattern;
    uint32_t seed = 1, lastPatternSeed = 1;

    std::array<Key, 32> keys {};
    int keyCount = 0;
    bool restartPending = false;

    std::array<Playing, 64> playing {};
    int playingCount = 0;
    std::array<int, 32> lastNotes {};
    int lastNoteCount = 0;

    int chord[kMaxUnison] {};
    int chordCount = 0;

    bool running = false, wasHostPlaying = false;
    double phase = 0.0;         // steps
    double lastStepBeats = 0.0; // rate in beats of the last run (to rescale on change)
    int64_t nextStep = 0;       // index of the next step to fire
    int64_t lastRegenIndex = -1;
    int arpIndex = 0;
    uint32_t rngState = 0x12345678u, dirSalt = 0;

    float rnd() noexcept;
    static double stepTime (int64_t k, float swing) noexcept;
    void columnsFor (int64_t k, const SeqParams& p, int* columns) const noexcept;
    void fireStep (int64_t k, const SeqParams& p, double stepBeats, Sink& sink) noexcept;
    void addPlaying (int note, double offTime, Sink& sink) noexcept;
    int findPlaying (int note) const noexcept;
    void removePlayingAt (int i) noexcept;
    void removeKeyAt (int i) noexcept;
    void purgeReleasedKeys (bool hold, bool pedal) noexcept;
    int buildArpList (const SeqParams& p, int* out, float* vel) noexcept;
};
} // namespace cs
