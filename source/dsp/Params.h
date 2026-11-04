#pragma once

// Plain parameter snapshot read by the DSP core.
// The plugin layer copies host parameters into this struct once per block.
// No JUCE dependency, so the core can be tested and benchmarked on its own.

#include <array>

namespace cs
{
constexpr int kMaxVoices = 16;
constexpr int kMaxUnison = 8;
constexpr int kNumOscs = 3;
constexpr int kNumEngines = 2;
constexpr int kNumFilters = 2;
constexpr int kBlock = 16; // control-rate block size in samples

enum Wave : int { WaveSine = 0, WaveTriangle, WaveSaw, WaveRamp, WaveSquare, WaveCount };

enum UnisonMode : int { UnisonClassic = 0, UnisonChord, UnisonSuper, UnisonModeCount };

enum Voicing : int { VoicingMono = 0, VoicingMonoLegato, VoicingPolyReassign, VoicingPolyRotate, VoicingCount };

enum ClassicMode : int
{
    LP6 = 0, LP12, LP24, LP36,
    BP12, BP24, BP36,
    HP6, HP12, HP24, HP36,
    Notch12, Notch24, Notch36,
    AP12, AP24, AP36,
    ClassicModeCount
};

constexpr int kNumChords = 12;

struct OscParams
{
    int wave = WaveSaw;
    float width = 0.5f;   // 0..1, 0.5 = symmetric (50 % pulse / even triangle)
    float volume = 0.0f;  // 0..1
    float coarse = 0.0f;  // semitones
    float fine = 0.0f;    // semitones (osc 2 and 3 only)
    bool keytrack = true; // osc 2 and 3 only
    float fm = 0.0f;      // 0..1, FM amount from the modulator (osc 1 and 2 only)
};

struct AnalogEngineParams
{
    bool on = true;
    float coarse = 0.0f; // semitones, +/-60
    float fine = 0.0f;   // semitones, +/-1
    float drift = 0.0f;  // 0..1
    bool keytrack = true;

    std::array<OscParams, kNumOscs> osc {};
    bool sync = false; // osc 2 hard-synced to osc 1

    float noiseColor = 0.0f; // -1 red .. 0 white .. +1 blue
    float noiseVolume = 0.0f;
    float modSource = 0.0f; // 0 = osc 3, 1 = noise (crossfade)

    bool unisonOn = false;
    int unisonMode = UnisonClassic;
    int unisonVoices = 4; // 1..8 (Classic and Chord modes)
    float detune = 0.25f; // 0..1, max +/-6 st
    float stereo = 0.5f;  // 0..1
    int chord = 3;        // index into chord table
    float superMix = 0.6f;

    float volume = 0.7f;
    float filterMix = 0.0f; // 0 = Filter 1 only, 1 = Filter 2 only
};

struct FilterParams
{
    bool on = true;
    int mode = LP12;
    float cutoff = 20000.0f; // Hz
    float reso = 0.0f;       // 0..1
    float drive = 0.0f;      // 0..1
    bool analog = true;      // nonlinear resonance
    float keytrack = 0.0f;   // 0..1 (1 = cutoff follows pitch fully)
    float volume = 1.0f;
    float pan = 0.0f; // -1..1
};

struct EnvParams
{
    float attack = 0.002f; // seconds
    float decay = 0.4f;
    float sustain = 1.0f;
    float release = 0.25f;
    float attCurve = 0.0f; // -20..20 (negative = log/fast start, positive = exp/slow start)
    float decCurve = 8.0f; // -20..20 (positive = exponential-style fast initial fall)
    bool adr = false;
    bool releaseLink = false;
};

struct SynthParams
{
    std::array<AnalogEngineParams, kNumEngines> engine {};
    std::array<FilterParams, kNumFilters> filter {};
    float filterRouting = 0.0f; // 0 = F1 -> F2 (series), 1 = F1 // F2 (parallel)

    EnvParams ampEnv {};
    float ampVelocity = 0.5f; // 0..1, how much velocity scales the output level
    float voicePan = 0.0f;

    int voicing = VoicingPolyRotate;
    int maxVoices = 8;
    float glide = 0.0f; // seconds
    bool glideLegato = false;
    float bendUp = 2.0f;   // semitones
    float bendDown = 2.0f; // semitones
    int transposeOct = 0;
    float transposeFine = 0.0f; // semitones
    bool hold = false;

    float masterVolume = 0.8f;

    std::array<float, 4> macro {}; // reserved for the modulation milestone

    SynthParams()
    {
        engine[0].osc[0].volume = 1.0f;
        engine[1].on = false;
        engine[1].osc[0].volume = 1.0f;
        filter[1].on = false;
    }
};
} // namespace cs
