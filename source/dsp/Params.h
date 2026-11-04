#pragma once

// Plain parameter snapshot read by the DSP core.
// The plugin layer copies host parameters into this struct once per block.
// No JUCE dependency, so the core can be tested and benchmarked on its own.

#include "fx/FxParams.h"

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

enum EngineType : int { EngineAnalog = 0, EngineWavetable, EngineTypeCount };

enum WtFmType : int { FmLinear = 0, FmExponential, FmRing, FmTypeCount };
enum WtRetrig : int { RetrigKey = 0, RetrigRandom, RetrigFree, RetrigModOsc, RetrigCount };
enum WtPhaseShape : int { PdPulseWidth = 0, PdSkew, PdMirror, PdSync, PdSteps, PdBend, PdSqueeze, PdCount };
enum WtFoldShape : int { FoldSine = 0, FoldTriangle, FoldSoft, FoldCount };
enum ModTuneMode : int { ModTuneRatio = 0, ModTuneAbsolute, ModTuneRelative, ModTuneFixed, ModTuneCount };
// Mod oscillator waves: the 5 basic waves, then 5 noise colours.
enum ModWave : int { ModSine = 0, ModTriangle, ModSaw, ModRamp, ModSquare,
                     ModBlueNoise, ModWhiteNoise, ModPinkNoise, ModRedNoise, ModRumble, ModWaveCount };
enum OscRoute : int { RouteFilters = 0, RouteFxA, RouteFxB, RouteDirect, RouteCount };

// --- Modulation -------------------------------------------------------------
// Source order is stored by name in presets; keep names stable.
enum ModSource : int
{
    SrcVelocity = 0, SrcAftertouch, SrcModWheel, SrcKeytrack, SrcExpression,
    SrcEnvAmp, SrcEnv2, SrcEnv3,
    SrcLfo1, SrcLfo2, SrcLfo3,
    SrcFunc1, SrcFunc2, SrcFunc3,
    SrcRand1, SrcRand2, SrcRand3,
    SrcComb1, SrcComb2, SrcComb3,
    SrcMacro1, SrcMacro2, SrcMacro3, SrcMacro4,
    NumModSources
};

// What restarts an LFO / function / random generator.
enum Retrig : int { RetrigPolyKbd = 0, RetrigMonoKbd, RetrigLegatoKbd, RetrigFreeRun,
                    RetrigLfo1, RetrigLfo2, RetrigLfo3, RetrigSourceCount };
// What restarts Env 2 / Env 3.
enum EnvRetrig : int { EnvRetrigPolyKbd = 0, EnvRetrigLegatoKbd, EnvRetrigLfo1, EnvRetrigLfo2, EnvRetrigLfo3, EnvRetrigCount };

enum LfoWave : int { LfoSine = 0, LfoTriangle, LfoSquare, LfoSaw, LfoRamp, LfoSampleHold, LfoWaveCount };
enum FuncMode : int { FuncLoop = 0, FuncOneShot, FuncEnvelope, FuncEnvLoop, FuncModeCount };
enum RandomType : int { RandRandom = 0, RandVoiceMod, RandSampleHold, RandTuring, RandBinary, RandTypeCount };
enum VoiceModMode : int { VmCycle = 0, VmReassign, VmRandom, VmModeCount };
enum CombType : int { CombSum = 0, CombDiff, CombMultiply, CombDivide, CombCrossfade, CombLag,
                      CombThreshold, CombOffset, CombRemap, CombEnvFollow, CombTypeCount };
enum FollowInput : int { FollowEngine1 = 0, FollowEngine2, FollowUtility, FollowFilter1, FollowFilter2, FollowVoice, FollowInputCount };

constexpr int kNumSyncDivs = 19;
constexpr int kNumLfos = 3, kNumFuncs = 3, kNumRandoms = 3, kNumCombs = 3;

class Wavetable;   // see Content.h
class NoiseSample; // see Content.h

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

// Filter types. Order is stored in presets: append only.
enum FilterType : int
{
    FilterClassicType = 0, FilterCluster, FilterPhaser, FilterFormant, FilterSurgeon, FilterLoFi,
    FilterComb, FilterDrive, FilterAllpass,
    FilterLadder, FilterSallenKey, FilterOta, FilterSvfMorph, FilterMultimode, FilterLpg,
    FilterTypeCount
};
enum FilterFmSource : int { FmSrcEngine1 = 0, FmSrcEngine2, FmSrcUtility, FmSrcNoise, FmSrcCount };
enum FilterRoutingMode : int { RoutingSum = 0, RoutingSplit, RoutingSplitReversed, RoutingModeCount };

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

struct WavetableParams
{
    const Wavetable* table = nullptr; // set by the host layer from the content library
    float position = 0.0f;            // 0..1
    bool morph = true;                // smooth vs stepped frame transitions
    float wtVolume = 1.0f;            // level of the wavetable itself (pre-output)

    int fmType = FmLinear;
    float fmAmount = 0.0f;

    float pmAmount = 0.0f;
    int retrig = RetrigKey;
    float initPhase = 0.0f; // 0..1 cycles

    int pdShape = PdPulseWidth;
    float pdAmount = 0.0f;
    float pdMod = 0.0f; // modulator depth on the phase transform

    int foldShape = FoldSine;
    float foldAmount = 0.0f;
    float foldMod = 0.0f;

    float unisonPhase = 1.0f; // 0 = all unison voices start in phase, 1 = random

    // Modulation oscillator
    int modTuneMode = ModTuneRatio;
    float modCoarse = 0.0f; // semitones (Absolute / Relative)
    float modRatio = 1.0f;  // Ratio mode
    float modHz = 110.0f;   // Fixed mode
    float modFine = 0.0f;   // semitones
    float modVolume = 0.0f; // direct output level of the mod oscillator
    int modWave = ModSine;
};

struct EngineParams
{
    bool on = true;
    int type = EngineAnalog;
    bool modFromEngine = false; // modulator = the other main engine (cross-mod)
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

    WavetableParams wt {};
};

struct NoiseParams
{
    bool on = true;
    const NoiseSample* sample = nullptr;
    bool randomPhase = true; // false = start at the beginning on every note
    bool keytrack = false;
    float tune = 0.0f;   // semitones, +/-36
    float filter = 0.0f; // -1 = darker (LP) .. 0 = open .. +1 = thinner (HP)
    bool loop = true;
    float length = 1.0f; // one-shot length 0..1 of the sample
    float filterMix = 0.0f;
    float volume = 0.0f;
};

struct UtilityParams
{
    bool on = true;
    std::array<NoiseParams, 2> noise {};
    bool oscOn = true;
    int oscWave = WaveSine;
    float oscWidth = 0.5f;
    float oscCoarse = -12.0f;
    float oscFine = 0.0f;
    bool oscKeytrack = true;
    float oscFilterMix = 0.0f;
    float oscVolume = 0.0f;
    int oscRoute = RouteFilters;
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

    // --- Milestone 4 (kept last so older aggregate initialisers stay valid) ---
    int type = FilterClassicType;
    int submode = 0;      // mode within the type (see MultiFilter.h)
    float a = 0.5f, b = 0.5f, c = 0.5f; // type-specific controls (see MultiFilter.h)
    float fm = 0.0f;      // audio-rate cutoff modulation depth 0..1
    int fmSource = FmSrcEngine1;
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

struct RateParams
{
    float hz = 1.0f;
    bool sync = false;
    int div = 7; // index into the sync division table (1/4 by default)
};

struct LfoParams
{
    int wave = LfoSine;
    float symmetry = 0.5f;
    RateParams rate {};
    float phase = 0.0f;    // 0..1 start phase
    float keytrack = 0.0f; // -2..2: rate follows the played note
    float fade = 0.0f;     // seconds to fade in after a (re)trigger
    float smooth = 0.0f;   // seconds of output smoothing
    int retrig = RetrigPolyKbd;
    bool bipolar = true;
};

struct FunctionShape; // see ModSources.h

struct FunctionParams
{
    const FunctionShape* shape = nullptr; // set by the host layer
    RateParams rate {};
    int mode = FuncLoop;
    int retrig = RetrigPolyKbd;
    bool bipolar = false;
    float smooth = 0.0f; // seconds
    float scale = 1.0f;  // -1..1
};

struct RandomParams
{
    int type = RandRandom;
    RateParams rate { 4.0f, false, 10 };
    float smooth = 0.0f;   // 0..1 of a step
    float distance = 1.0f; // 0..1
    float jitter = 0.0f;   // 0..1
    bool bipolar = true;
    int retrig = RetrigFreeRun;
    int vmVoices = 8;
    int vmMode = VmCycle;
    std::array<float, 8> vmSteps { 0.6f, -0.3f, 0.9f, -0.8f, 0.2f, -0.5f, 0.4f, -0.1f };
    int shSource = -1; // -1 = internal noise, otherwise a ModSource
    float shRise = 0.0f, shFall = 0.0f; // seconds
    bool shLink = true;
    float flip = 0.0f; // Turing
    int length = 8;    // Turing
    float probability = 0.5f, correlation = 0.0f; // Binary
};

struct CombParams
{
    int type = CombSum;
    int source = SrcLfo1;
    int mod = SrcLfo2;
    float amount = 0.5f;
    int remapShape = 0;
    int followInput = FollowVoice;
    float followGain = 1.0f;      // input gain (linear, 0..4)
    float followThreshold = 0.0f; // 0..1
    float followAttack = 0.01f, followRelease = 0.2f; // seconds
};

struct KeyCurves
{
    float velocity = 0.0f, aftertouch = 0.0f, keytrack = 0.0f; // -1..1, 0 = linear
};

struct SynthParams
{
    std::array<EngineParams, kNumEngines> engine {};
    UtilityParams utility {};
    std::array<FilterParams, kNumFilters> filter {};
    float filterRouting = 0.0f; // 0 = F1 -> F2 (series), 1 = F1 // F2 (parallel)
    int filterOutput = RoutingSum; // Sum (both filters to the FX input) or Split (F1 -> FX A, F2 -> FX B)

    EnvParams ampEnv {};
    float ampVelocity = 0.5f; // Amp Mod amount 0..1 (source = ampModSource)
    int ampModSource = SrcVelocity;
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

    std::array<float, 4> macro {};

    // --- Modulation sources -------------------------------------------------
    EnvParams env2 { 0.002f, 0.5f, 0.0f, 0.4f, 0.0f, 8.0f, false, false };
    EnvParams env3 { 0.002f, 0.5f, 0.0f, 0.4f, 0.0f, 8.0f, false, false };
    int env2Retrig = EnvRetrigPolyKbd, env3Retrig = EnvRetrigPolyKbd;
    std::array<LfoParams, kNumLfos> lfo {};
    std::array<FunctionParams, kNumFuncs> func {};
    std::array<RandomParams, kNumRandoms> random {};
    std::array<CombParams, kNumCombs> comb {};
    KeyCurves curves {};
    std::array<int, kNumEngines> coarseQuantMask { 0xFFF, 0xFFF }; // allowed intervals when Coarse is modulated
    int utilityQuantMask = 0xFFF;

    // --- FX section (global, after the voices) ---------------------------------
    fx::FxParams fx {};

    SynthParams()
    {
        engine[0].osc[0].volume = 1.0f;
        engine[1].on = false;
        engine[1].osc[0].volume = 1.0f;
        filter[1].on = false;
    }
};
} // namespace cs
