#include "Parameters.h"

#include "dsp/Content.h"
#include "dsp/ModMatrix.h"
#include "dsp/ModSources.h"
#include "dsp/fx/Effects.h"
#include "dsp/SeqArp.h"

namespace cs
{
namespace
{
const juce::StringArray kWaves { "Sine", "Triangle", "Saw", "Ramp", "Square" };
const juce::StringArray kUnisonModes { "Unison", "Chord", "Super" };
const juce::StringArray kChords { "Octave", "Fifth", "Fifth + Oct", "Major", "Minor", "Sus2",
                                  "Sus4", "Major 7", "Minor 7", "Dom 7", "Add 9", "6/9" };
const juce::StringArray kClassicModes { "Lowpass 6", "Lowpass 12", "Lowpass 24", "Lowpass 36",
                                        "Bandpass 12", "Bandpass 24", "Bandpass 36",
                                        "Highpass 6", "Highpass 12", "Highpass 24", "Highpass 36",
                                        "Notch 12", "Notch 24", "Notch 36",
                                        "All Pass 12", "All Pass 24", "All Pass 36" };
const juce::StringArray kFilterTypes { "Classic", "Cluster", "Phaser", "Formant", "Surgeon", "LoFi", "Comb",
                                      "Drive", "Allpass", "Ladder", "Sallen-Key", "OTA 4-Pole", "SVF Morph",
                                      "Multimode", "Lowpass Gate" };
const juce::StringArray kFilterFmSources { "Engine 1", "Engine 2", "Utility", "Noise" };
const juce::StringArray kFilterOutputs { "Sum", "Split (F1>FX A, F2>FX B)", "Split Reversed (F1>FX B, F2>FX A)" };
const juce::StringArray kVoicing { "Mono", "Mono Legato", "Poly Reassign", "Poly Rotate" };
const juce::StringArray kEngineTypes { "Analog", "Wavetable" };
const juce::StringArray kFmTypes { "Linear FM", "Exponential FM", "Ring Mod" };
const juce::StringArray kRetrig { "Key", "Random", "Free", "Mod Osc" };
const juce::StringArray kPdShapes { "Pulse Width", "Skew", "Mirror", "Sync", "Steps", "Bend", "Squeeze" };
const juce::StringArray kFoldShapes { "Sine", "Triangle", "Soft" };
const juce::StringArray kModTune { "Ratio", "Absolute", "Relative", "Fixed Hz" };
const juce::StringArray kModWaves { "Sine", "Triangle", "Saw", "Ramp", "Square",
                                    "Blue Noise", "White Noise", "Pink Noise", "Red Noise", "Rumble" };
const juce::StringArray kNoisePhase { "Key", "Random" };
const juce::StringArray kRoutes { "Filters", "FX A", "FX B", "Direct" };

const juce::StringArray kRetrigNames { "Poly Kbd", "Mono Kbd", "Legato Kbd", "Free Running", "LFO 1", "LFO 2", "LFO 3" };
const juce::StringArray kEnvRetrigNames { "Poly Kbd", "Legato Kbd", "LFO 1", "LFO 2", "LFO 3" };
const juce::StringArray kLfoWaves { "Sine", "Triangle", "Square", "Saw", "Ramp", "Sample & Hold" };
const juce::StringArray kFuncModes { "Loop", "One-Shot", "Envelope", "Envelope Loop" };
const juce::StringArray kRandomTypes { "Random", "Voice Modulator", "Sample & Hold", "Turing", "Binary" };
const juce::StringArray kVmModes { "Cycle", "Reassign", "Random" };
const juce::StringArray kCombTypes { "Sum", "Difference", "Multiply", "Divide", "Crossfade", "Lag",
                                     "Threshold", "Offset", "Remap", "Envelope Follower" };
const juce::StringArray kFollowInputs { "Engine 1", "Engine 2", "Utility", "Filter 1", "Filter 2", "Voice Out" };

juce::StringArray sourceNames()
{
    juce::StringArray a;
    for (int i = 0; i < NumModSources; ++i)
        a.add (modSourceName (i));
    return a;
}
juce::StringArray syncDivNames()
{
    juce::StringArray a;
    for (const auto& d : kSyncDivs)
        a.add (d.name);
    return a;
}
juce::StringArray presetNames (bool remap)
{
    juce::StringArray a;
    for (int i = 0; i < (remap ? kNumRemapShapes : kNumFunctionPresets); ++i)
        a.add (remap ? remapShapeName (i) : functionPresetName (i));
    return a;
}

juce::StringArray functionShapeNames()
{
    auto a = presetNames (false);
    a.add ("Custom"); // index kNumFunctionPresets: the shape drawn in the editor
    return a;
}

juce::StringArray factoryNames (bool wavetables)
{
    auto& lib = ContentLibrary::get();
    juce::StringArray names;
    const int count = wavetables ? lib.numFactoryWavetables() : lib.numFactoryNoises();
    for (int i = 0; i < count; ++i)
    {
        const auto e = wavetables ? lib.wavetableEntry (i) : lib.noiseEntry (i);
        names.add (juce::String (e.category) + ": " + e.name);
    }
    return names;
}

struct Builder
{
    std::vector<ParamDef> defs;
    juce::String group;

    void flt (juce::String id, juce::String name, float mn, float mx, float def,
              std::function<void (SynthParams&, float)> fn, juce::String unit = {}, float centre = -1)
    {
        ParamDef d;
        d.id = id; d.name = name; d.group = group; d.unit = unit;
        d.kind = ParamDef::Float; d.min = mn; d.max = mx; d.def = def; d.centre = centre;
        d.apply = std::move (fn);
        defs.push_back (std::move (d));
    }
    void integer (juce::String id, juce::String name, int mn, int mx, int def, std::function<void (SynthParams&, float)> fn)
    {
        ParamDef d;
        d.id = id; d.name = name; d.group = group;
        d.kind = ParamDef::Int; d.min = (float) mn; d.max = (float) mx; d.def = (float) def;
        d.apply = std::move (fn);
        defs.push_back (std::move (d));
    }
    void choice (juce::String id, juce::String name, const juce::StringArray& items, int def,
                 std::function<void (SynthParams&, float)> fn)
    {
        ParamDef d;
        d.id = id; d.name = name; d.group = group;
        d.kind = ParamDef::Choice; d.choices = items; d.def = (float) def;
        d.apply = std::move (fn);
        defs.push_back (std::move (d));
    }
    void boolean (juce::String id, juce::String name, bool def, std::function<void (SynthParams&, float)> fn)
    {
        ParamDef d;
        d.id = id; d.name = name; d.group = group;
        d.kind = ParamDef::Bool; d.def = def ? 1.0f : 0.0f;
        d.apply = std::move (fn);
        defs.push_back (std::move (d));
    }
};

inline int asInt (float v) { return (int) std::lround (v); }
inline bool asBool (float v) { return v >= 0.5f; }

void addEngine (Builder& b, size_t e)
{
    const int num = (int) e + 1;
    const juce::String p = "e" + juce::String (num) + "_";
    const juce::String n = "Eng" + juce::String (num) + " ";
    b.group = "Engine " + juce::String (num);

    b.boolean (p + "on", n + "On", num == 1, [e] (SynthParams& s, float v) { s.engine[e].on = asBool (v); });
    b.flt (p + "coarse", n + "Coarse", -60, 60, 0, [e] (SynthParams& s, float v) { s.engine[e].coarse = (float) asInt (v); }, "st");
    b.flt (p + "fine", n + "Fine", -1, 1, 0, [e] (SynthParams& s, float v) { s.engine[e].fine = v; }, "st");
    b.flt (p + "drift", n + "Drift", 0, 1, 0, [e] (SynthParams& s, float v) { s.engine[e].drift = v; });
    b.boolean (p + "kbd", n + "Key Track", true, [e] (SynthParams& s, float v) { s.engine[e].keytrack = asBool (v); });

    for (size_t o = 0; o < (size_t) kNumOscs; ++o)
    {
        const juce::String op = p + "o" + juce::String ((int) o + 1) + "_";
        const juce::String on = n + "Osc" + juce::String ((int) o + 1) + " ";
        b.choice (op + "wave", on + "Wave", kWaves, WaveSaw, [e, o] (SynthParams& s, float v) { s.engine[e].osc[o].wave = asInt (v); });
        b.flt (op + "width", on + "Width", 0, 1, 0.5f, [e, o] (SynthParams& s, float v) { s.engine[e].osc[o].width = v; });
        b.flt (op + "vol", on + "Volume", 0, 1, o == 0 ? 1.0f : 0.0f, [e, o] (SynthParams& s, float v) { s.engine[e].osc[o].volume = v; });
        b.flt (op + "coarse", on + "Coarse", -36, 36, 0, [e, o] (SynthParams& s, float v) { s.engine[e].osc[o].coarse = (float) asInt (v); }, "st");
        if (o > 0)
        {
            b.flt (op + "fine", on + "Fine", -1, 1, 0, [e, o] (SynthParams& s, float v) { s.engine[e].osc[o].fine = v; }, "st");
            b.boolean (op + "kbd", on + "Key Track", true, [e, o] (SynthParams& s, float v) { s.engine[e].osc[o].keytrack = asBool (v); });
        }
        if (o < 2)
            b.flt (op + "fm", on + "FM", 0, 1, 0, [e, o] (SynthParams& s, float v) { s.engine[e].osc[o].fm = v; });
    }
    b.boolean (p + "sync", n + "Sync 2>1", false, [e] (SynthParams& s, float v) { s.engine[e].sync = asBool (v); });

    b.flt (p + "noise_color", n + "Noise Colour", -1, 1, 0, [e] (SynthParams& s, float v) { s.engine[e].noiseColor = v; });
    b.flt (p + "noise_vol", n + "Noise Volume", 0, 1, 0, [e] (SynthParams& s, float v) { s.engine[e].noiseVolume = v; });
    b.flt (p + "mod_src", n + "Mod Osc3/Noise", 0, 1, 0, [e] (SynthParams& s, float v) { s.engine[e].modSource = v; });

    b.boolean (p + "uni_on", n + "Unison On", false, [e] (SynthParams& s, float v) { s.engine[e].unisonOn = asBool (v); });
    b.choice (p + "uni_mode", n + "Unison Mode", kUnisonModes, UnisonClassic, [e] (SynthParams& s, float v) { s.engine[e].unisonMode = asInt (v); });
    b.integer (p + "uni_voices", n + "Unison Voices", 1, kMaxUnison, 4, [e] (SynthParams& s, float v) { s.engine[e].unisonVoices = asInt (v); });
    b.flt (p + "uni_detune", n + "Unison Detune", 0, 1, 0.25f, [e] (SynthParams& s, float v) { s.engine[e].detune = v; });
    b.flt (p + "uni_stereo", n + "Unison Stereo", 0, 1, 0.5f, [e] (SynthParams& s, float v) { s.engine[e].stereo = v; });
    b.choice (p + "uni_chord", n + "Unison Chord", kChords, 3, [e] (SynthParams& s, float v) { s.engine[e].chord = asInt (v); });
    b.flt (p + "uni_mix", n + "Super Mix", 0, 1, 0.6f, [e] (SynthParams& s, float v) { s.engine[e].superMix = v; });

    b.flt (p + "vol", n + "Volume", 0, 1, 0.7f, [e] (SynthParams& s, float v) { s.engine[e].volume = v; });
    b.flt (p + "fmix", n + "Filter Mix F1/F2", 0, 1, 0, [e] (SynthParams& s, float v) { s.engine[e].filterMix = v; });

    // --- Milestone 2: engine type, cross-mod, wavetable ----------------------
    b.choice (p + "type", n + "Type", kEngineTypes, EngineAnalog, [e] (SynthParams& s, float v) { s.engine[e].type = asInt (v); });
    b.boolean (p + "mod_eng", n + "Modulator = Other Engine", false, [e] (SynthParams& s, float v) { s.engine[e].modFromEngine = asBool (v); });

    const juce::String w = p + "wt_";
    const juce::String wn = n + "WT ";
    // The table pointer itself is resolved by the content loader, not here.
    b.choice (w + "table", wn + "Table", factoryNames (true), 0, [] (SynthParams&, float) {});
    b.flt (w + "pos", wn + "Position", 0, 1, 0, [e] (SynthParams& s, float v) { s.engine[e].wt.position = v; });
    b.boolean (w + "morph", wn + "Morph", true, [e] (SynthParams& s, float v) { s.engine[e].wt.morph = asBool (v); });
    b.flt (w + "vol", wn + "Wavetable Volume", 0, 1, 1, [e] (SynthParams& s, float v) { s.engine[e].wt.wtVolume = v; });
    b.choice (w + "fm_type", wn + "FM Type", kFmTypes, FmLinear, [e] (SynthParams& s, float v) { s.engine[e].wt.fmType = asInt (v); });
    b.flt (w + "fm", wn + "FM / Ring Amount", 0, 1, 0, [e] (SynthParams& s, float v) { s.engine[e].wt.fmAmount = v; });
    b.flt (w + "pm", wn + "Phase Mod", 0, 1, 0, [e] (SynthParams& s, float v) { s.engine[e].wt.pmAmount = v; });
    b.choice (w + "retrig", wn + "Phase Retrig", kRetrig, RetrigKey, [e] (SynthParams& s, float v) { s.engine[e].wt.retrig = asInt (v); });
    b.flt (w + "phase", wn + "Start Phase", 0, 360, 0, [e] (SynthParams& s, float v) { s.engine[e].wt.initPhase = v / 360.0f; }, "deg");
    b.choice (w + "pd_type", wn + "Phase Transform Type", kPdShapes, PdPulseWidth, [e] (SynthParams& s, float v) { s.engine[e].wt.pdShape = asInt (v); });
    b.flt (w + "pd", wn + "Phase Transform", 0, 1, 0, [e] (SynthParams& s, float v) { s.engine[e].wt.pdAmount = v; });
    b.flt (w + "pd_mod", wn + "Phase Transform Mod", 0, 1, 0, [e] (SynthParams& s, float v) { s.engine[e].wt.pdMod = v; });
    b.choice (w + "fold_type", wn + "Fold Shape", kFoldShapes, FoldSine, [e] (SynthParams& s, float v) { s.engine[e].wt.foldShape = asInt (v); });
    b.flt (w + "fold", wn + "Fold", 0, 1, 0, [e] (SynthParams& s, float v) { s.engine[e].wt.foldAmount = v; });
    b.flt (w + "fold_mod", wn + "Fold Mod", 0, 1, 0, [e] (SynthParams& s, float v) { s.engine[e].wt.foldMod = v; });
    b.flt (w + "uni_phase", wn + "Unison Phase", 0, 1, 1, [e] (SynthParams& s, float v) { s.engine[e].wt.unisonPhase = v; });
    b.choice (w + "mod_tune_mode", wn + "Mod Osc Tune Mode", kModTune, ModTuneRatio, [e] (SynthParams& s, float v) { s.engine[e].wt.modTuneMode = asInt (v); });
    b.integer (w + "mod_coarse", wn + "Mod Osc Coarse", -48, 48, 0, [e] (SynthParams& s, float v) { s.engine[e].wt.modCoarse = v; });
    b.flt (w + "mod_ratio", wn + "Mod Osc Ratio", 0.125f, 16, 1, [e] (SynthParams& s, float v) { s.engine[e].wt.modRatio = v; }, "x", 2.0f);
    b.flt (w + "mod_hz", wn + "Mod Osc Fixed Hz", 0.1f, 10000, 110, [e] (SynthParams& s, float v) { s.engine[e].wt.modHz = v; }, "Hz", 300.0f);
    b.flt (w + "mod_fine", wn + "Mod Osc Fine", -1, 1, 0, [e] (SynthParams& s, float v) { s.engine[e].wt.modFine = v; }, "st");
    b.flt (w + "mod_vol", wn + "Mod Osc Volume", 0, 1, 0, [e] (SynthParams& s, float v) { s.engine[e].wt.modVolume = v; });
    b.choice (w + "mod_wave", wn + "Mod Osc Wave", kModWaves, ModSine, [e] (SynthParams& s, float v) { s.engine[e].wt.modWave = asInt (v); });
}

void addUtility (Builder& b)
{
    b.group = "Utility";
    b.boolean ("u_on", "Utility On", true, [] (SynthParams& s, float v) { s.utility.on = asBool (v); });
    for (size_t k = 0; k < 2; ++k)
    {
        const juce::String p = "u_n" + juce::String ((int) k + 1) + "_";
        const juce::String n = "Noise" + juce::String ((int) k + 1) + " ";
        b.boolean (p + "on", n + "On", true, [k] (SynthParams& s, float v) { s.utility.noise[k].on = asBool (v); });
        b.choice (p + "type", n + "Sample", factoryNames (false), k == 0 ? 0 : 9, [] (SynthParams&, float) {});
        b.choice (p + "phase", n + "Phase", kNoisePhase, 1, [k] (SynthParams& s, float v) { s.utility.noise[k].randomPhase = asInt (v) == 1; });
        b.boolean (p + "kbd", n + "Key Track", false, [k] (SynthParams& s, float v) { s.utility.noise[k].keytrack = asBool (v); });
        b.flt (p + "tune", n + "Tune", -36, 36, 0, [k] (SynthParams& s, float v) { s.utility.noise[k].tune = v; }, "st");
        b.flt (p + "filter", n + "Filter LP/HP", -1, 1, 0, [k] (SynthParams& s, float v) { s.utility.noise[k].filter = v; });
        b.boolean (p + "loop", n + "Loop", true, [k] (SynthParams& s, float v) { s.utility.noise[k].loop = asBool (v); });
        b.flt (p + "len", n + "Length", 0, 1, 1, [k] (SynthParams& s, float v) { s.utility.noise[k].length = v; });
        b.flt (p + "fmix", n + "Filter Mix F1/F2", 0, 1, 0, [k] (SynthParams& s, float v) { s.utility.noise[k].filterMix = v; });
        b.flt (p + "vol", n + "Volume", 0, 1, 0, [k] (SynthParams& s, float v) { s.utility.noise[k].volume = v; });
    }
    b.boolean ("u_osc_on", "Utility Osc On", true, [] (SynthParams& s, float v) { s.utility.oscOn = asBool (v); });
    b.choice ("u_osc_wave", "Utility Osc Wave", kWaves, WaveSine, [] (SynthParams& s, float v) { s.utility.oscWave = asInt (v); });
    b.flt ("u_osc_width", "Utility Osc Width", 0, 1, 0.5f, [] (SynthParams& s, float v) { s.utility.oscWidth = v; });
    b.flt ("u_osc_coarse", "Utility Osc Coarse", -36, 36, -12, [] (SynthParams& s, float v) { s.utility.oscCoarse = (float) asInt (v); }, "st");
    b.flt ("u_osc_fine", "Utility Osc Fine", -1, 1, 0, [] (SynthParams& s, float v) { s.utility.oscFine = v; }, "st");
    b.boolean ("u_osc_kbd", "Utility Osc Key Track", true, [] (SynthParams& s, float v) { s.utility.oscKeytrack = asBool (v); });
    b.flt ("u_osc_fmix", "Utility Osc Filter Mix F1/F2", 0, 1, 0, [] (SynthParams& s, float v) { s.utility.oscFilterMix = v; });
    b.flt ("u_osc_vol", "Utility Osc Volume", 0, 1, 0, [] (SynthParams& s, float v) { s.utility.oscVolume = v; });
    b.choice ("u_osc_route", "Utility Osc Output", kRoutes, RouteFilters, [] (SynthParams& s, float v) { s.utility.oscRoute = asInt (v); });
}

void addFilter (Builder& b, size_t f)
{
    const int num = (int) f + 1;
    const juce::String p = "f" + juce::String (num) + "_";
    const juce::String n = "Filter" + juce::String (num) + " ";
    b.group = "Filter " + juce::String (num);

    b.boolean (p + "on", n + "On", num == 1, [f] (SynthParams& s, float v) { s.filter[f].on = asBool (v); });
    b.choice (p + "mode", n + "Type", kClassicModes, LP12, [f] (SynthParams& s, float v) { s.filter[f].mode = asInt (v); });
    b.flt (p + "cutoff", n + "Cutoff", 20, 20000, 20000, [f] (SynthParams& s, float v) { s.filter[f].cutoff = v; }, "Hz", 1000);
    b.flt (p + "reso", n + "Resonance", 0, 1, 0, [f] (SynthParams& s, float v) { s.filter[f].reso = v; });
    b.flt (p + "drive", n + "Drive", 0, 1, 0, [f] (SynthParams& s, float v) { s.filter[f].drive = v; });
    b.boolean (p + "analog", n + "Analog", true, [f] (SynthParams& s, float v) { s.filter[f].analog = asBool (v); });
    b.flt (p + "kbd", n + "Key Track", 0, 1, 0, [f] (SynthParams& s, float v) { s.filter[f].keytrack = v; });
    // 0 dB in the middle of the knob, up to +12 dB of make-up after filtering.
    b.flt (p + "vol", n + "Volume", 0, 4, 1, [f] (SynthParams& s, float v) { s.filter[f].volume = v; }, "gain", 1.0f);
    b.flt (p + "pan", n + "Pan", -1, 1, 0, [f] (SynthParams& s, float v) { s.filter[f].pan = v; });

    // --- Milestone 4: filter types (control meanings per type: see MultiFilter.h / SPEC) ---
    b.choice (p + "type", n + "Model", kFilterTypes, FilterClassicType, [f] (SynthParams& s, float v) { s.filter[f].type = asInt (v); });
    b.integer (p + "submode", n + "Model Mode", 0, 6, 0, [f] (SynthParams& s, float v) { s.filter[f].submode = asInt (v); });
    b.flt (p + "a", n + "Control A", 0, 1, 0.5f, [f] (SynthParams& s, float v) { s.filter[f].a = v; });
    b.flt (p + "b", n + "Control B", 0, 1, 0.5f, [f] (SynthParams& s, float v) { s.filter[f].b = v; });
    b.flt (p + "c", n + "Control C", 0, 1, 0.5f, [f] (SynthParams& s, float v) { s.filter[f].c = v; });
    b.flt (p + "fm", n + "FM Amount", 0, 1, 0, [f] (SynthParams& s, float v) { s.filter[f].fm = v; });
    b.choice (p + "fm_src", n + "FM Source", kFilterFmSources, FmSrcEngine1, [f] (SynthParams& s, float v) { s.filter[f].fmSource = asInt (v); });
}

void addRate (Builder& b, const juce::String& p, const juce::String& n, float defHz, int defDiv,
              std::function<RateParams& (SynthParams&)> rate)
{
    b.flt (p + "_rate", n + " Rate", 0.01f, 50, defHz, [rate] (SynthParams& s, float v) { rate (s).hz = v; }, "Hz", 2.0f);
    b.boolean (p + "_sync", n + " Sync", false, [rate] (SynthParams& s, float v) { rate (s).sync = asBool (v); });
    b.choice (p + "_div", n + " Sync Division", syncDivNames(), defDiv, [rate] (SynthParams& s, float v) { rate (s).div = asInt (v); });
}

void addEnvelope (Builder& b, const juce::String& p, const juce::String& n, std::function<EnvParams& (SynthParams&)> env,
                  std::function<int& (SynthParams&)> retrig)
{
    b.flt (p + "_a", n + " Attack", 0.0f, 20, 0.002f, [env] (SynthParams& s, float v) { env (s).attack = v; }, "s", 0.5f);
    b.flt (p + "_d", n + " Decay", 0.001f, 20, 0.5f, [env] (SynthParams& s, float v) { env (s).decay = v; }, "s", 0.8f);
    b.flt (p + "_s", n + " Sustain", 0, 1, 0, [env] (SynthParams& s, float v) { env (s).sustain = v; });
    b.flt (p + "_r", n + " Release", 0.001f, 20, 0.4f, [env] (SynthParams& s, float v) { env (s).release = v; }, "s", 0.8f);
    b.flt (p + "_acurve", n + " Att Curve", -20, 20, 0, [env] (SynthParams& s, float v) { env (s).attCurve = v; });
    b.flt (p + "_dcurve", n + " Dec Curve", -20, 20, 8, [env] (SynthParams& s, float v) { env (s).decCurve = v; });
    b.boolean (p + "_adr", n + " ADR Mode", false, [env] (SynthParams& s, float v) { env (s).adr = asBool (v); });
    b.boolean (p + "_link", n + " Release Link", false, [env] (SynthParams& s, float v) { env (s).releaseLink = asBool (v); });
    b.choice (p + "_retrig", n + " Retrig Source", kEnvRetrigNames, EnvRetrigPolyKbd, [retrig] (SynthParams& s, float v) { retrig (s) = asInt (v); });
}

void addModulation (Builder& b)
{
    b.group = "Envelopes 2 & 3";
    addEnvelope (b, "env2", "Env 2", [] (SynthParams& s) -> EnvParams& { return s.env2; }, [] (SynthParams& s) -> int& { return s.env2Retrig; });
    addEnvelope (b, "env3", "Env 3", [] (SynthParams& s) -> EnvParams& { return s.env3; }, [] (SynthParams& s) -> int& { return s.env3Retrig; });

    b.group = "LFOs";
    for (size_t k = 0; k < (size_t) kNumLfos; ++k)
    {
        const juce::String p = "lfo" + juce::String ((int) k + 1), n = "LFO " + juce::String ((int) k + 1);
        b.choice (p + "_wave", n + " Wave", kLfoWaves, LfoSine, [k] (SynthParams& s, float v) { s.lfo[k].wave = asInt (v); });
        b.flt (p + "_sym", n + " Symmetry", 0, 1, 0.5f, [k] (SynthParams& s, float v) { s.lfo[k].symmetry = v; });
        addRate (b, p, n, 1.0f, 7, [k] (SynthParams& s) -> RateParams& { return s.lfo[k].rate; });
        b.flt (p + "_phase", n + " Phase", 0, 360, 0, [k] (SynthParams& s, float v) { s.lfo[k].phase = v / 360.0f; }, "deg");
        b.flt (p + "_kbd", n + " Key Track", -2, 2, 0, [k] (SynthParams& s, float v) { s.lfo[k].keytrack = v; });
        b.flt (p + "_fade", n + " Fade", 0, 20, 0, [k] (SynthParams& s, float v) { s.lfo[k].fade = v; }, "s", 2.0f);
        b.flt (p + "_smooth", n + " Smooth", 0, 4, 0, [k] (SynthParams& s, float v) { s.lfo[k].smooth = v; }, "s", 0.5f);
        b.choice (p + "_retrig", n + " Retrig Source", kRetrigNames, RetrigPolyKbd, [k] (SynthParams& s, float v) { s.lfo[k].retrig = asInt (v); });
        b.boolean (p + "_bipolar", n + " Bipolar", true, [k] (SynthParams& s, float v) { s.lfo[k].bipolar = asBool (v); });
    }

    b.group = "Functions";
    for (size_t k = 0; k < (size_t) kNumFuncs; ++k)
    {
        const juce::String p = "func" + juce::String ((int) k + 1), n = "Function " + juce::String ((int) k + 1);
        // The last entry, "Custom", is the drawn shape kept in the plugin state (set by the processor).
        b.choice (p + "_shape", n + " Shape", functionShapeNames(),
                  (int) k == 0 ? 2 : (int) k == 1 ? 6 : 13,
                  [k] (SynthParams& s, float v) { s.func[k].shape = &functionPreset (std::min (asInt (v), kNumFunctionPresets - 1)); });
        addRate (b, p, n, 1.0f, 3, [k] (SynthParams& s) -> RateParams& { return s.func[k].rate; });
        b.choice (p + "_mode", n + " Mode", kFuncModes, FuncLoop, [k] (SynthParams& s, float v) { s.func[k].mode = asInt (v); });
        b.choice (p + "_retrig", n + " Retrig Source", kRetrigNames, RetrigPolyKbd, [k] (SynthParams& s, float v) { s.func[k].retrig = asInt (v); });
        b.boolean (p + "_bipolar", n + " Bipolar", false, [k] (SynthParams& s, float v) { s.func[k].bipolar = asBool (v); });
        b.flt (p + "_smooth", n + " Smooth", 0, 1, 0, [k] (SynthParams& s, float v) { s.func[k].smooth = v; }, "s", 0.2f);
        b.flt (p + "_scale", n + " Scale", -1, 1, 1, [k] (SynthParams& s, float v) { s.func[k].scale = v; });
    }

    b.group = "Random";
    for (size_t k = 0; k < (size_t) kNumRandoms; ++k)
    {
        const juce::String p = "rand" + juce::String ((int) k + 1), n = "Random " + juce::String ((int) k + 1);
        b.choice (p + "_type", n + " Type", kRandomTypes, RandRandom, [k] (SynthParams& s, float v) { s.random[k].type = asInt (v); });
        addRate (b, p, n, 4.0f, 10, [k] (SynthParams& s) -> RateParams& { return s.random[k].rate; });
        b.flt (p + "_smooth", n + " Smooth", 0, 1, 0, [k] (SynthParams& s, float v) { s.random[k].smooth = v; });
        b.flt (p + "_dist", n + " Distance", 0, 1, 1, [k] (SynthParams& s, float v) { s.random[k].distance = v; });
        b.flt (p + "_jitter", n + " Jitter", 0, 1, 0, [k] (SynthParams& s, float v) { s.random[k].jitter = v; });
        b.boolean (p + "_bipolar", n + " Bipolar", true, [k] (SynthParams& s, float v) { s.random[k].bipolar = asBool (v); });
        b.choice (p + "_retrig", n + " Retrig Source", kRetrigNames, RetrigFreeRun, [k] (SynthParams& s, float v) { s.random[k].retrig = asInt (v); });
        b.integer (p + "_vm_voices", n + " VM Steps", 1, 8, 8, [k] (SynthParams& s, float v) { s.random[k].vmVoices = asInt (v); });
        b.choice (p + "_vm_mode", n + " VM Mode", kVmModes, VmCycle, [k] (SynthParams& s, float v) { s.random[k].vmMode = asInt (v); });
        const float defaults[8] = { 0.6f, -0.3f, 0.9f, -0.8f, 0.2f, -0.5f, 0.4f, -0.1f };
        for (size_t st = 0; st < 8; ++st)
            b.flt (p + "_vm" + juce::String ((int) st + 1), n + " VM Step " + juce::String ((int) st + 1), -1, 1, defaults[st],
                   [k, st] (SynthParams& s, float v) { s.random[k].vmSteps[st] = v; });
        juce::StringArray shSources { "Noise" };
        shSources.addArray (sourceNames());
        b.choice (p + "_sh_src", n + " S&H Source", shSources, 0, [k] (SynthParams& s, float v) { s.random[k].shSource = asInt (v) - 1; });
        b.flt (p + "_sh_rise", n + " S&H Rise", 0, 2, 0, [k] (SynthParams& s, float v) { s.random[k].shRise = v; }, "s", 0.2f);
        b.flt (p + "_sh_fall", n + " S&H Fall", 0, 2, 0, [k] (SynthParams& s, float v) { s.random[k].shFall = v; }, "s", 0.2f);
        b.boolean (p + "_sh_link", n + " S&H Link", true, [k] (SynthParams& s, float v) { s.random[k].shLink = asBool (v); });
        b.flt (p + "_flip", n + " Turing Flip", 0, 1, 0, [k] (SynthParams& s, float v) { s.random[k].flip = v; });
        b.integer (p + "_len", n + " Turing Length", 1, 32, 8, [k] (SynthParams& s, float v) { s.random[k].length = asInt (v); });
        b.flt (p + "_proba", n + " Binary Probability", 0, 1, 0.5f, [k] (SynthParams& s, float v) { s.random[k].probability = v; });
        b.flt (p + "_correl", n + " Binary Correlation", 0, 1, 0, [k] (SynthParams& s, float v) { s.random[k].correlation = v; });
    }

    b.group = "Combinators";
    for (size_t k = 0; k < (size_t) kNumCombs; ++k)
    {
        const juce::String p = "comb" + juce::String ((int) k + 1), n = "Combinator " + juce::String ((int) k + 1);
        b.choice (p + "_type", n + " Type", kCombTypes, CombSum, [k] (SynthParams& s, float v) { s.comb[k].type = asInt (v); });
        b.choice (p + "_src", n + " Source", sourceNames(), SrcLfo1, [k] (SynthParams& s, float v) { s.comb[k].source = asInt (v); });
        b.choice (p + "_mod", n + " Mod", sourceNames(), SrcLfo2, [k] (SynthParams& s, float v) { s.comb[k].mod = asInt (v); });
        b.flt (p + "_amount", n + " Amount", 0, 1, 0.5f, [k] (SynthParams& s, float v) { s.comb[k].amount = v; });
        b.choice (p + "_remap", n + " Remap Shape", presetNames (true), 3, [k] (SynthParams& s, float v) { s.comb[k].remapShape = asInt (v); });
        b.choice (p + "_ef_input", n + " Follower Input", kFollowInputs, FollowVoice, [k] (SynthParams& s, float v) { s.comb[k].followInput = asInt (v); });
        b.flt (p + "_ef_gain", n + " Follower Gain", 0, 4, 1, [k] (SynthParams& s, float v) { s.comb[k].followGain = v; });
        b.flt (p + "_ef_thresh", n + " Follower Threshold", 0, 1, 0, [k] (SynthParams& s, float v) { s.comb[k].followThreshold = v; });
        b.flt (p + "_ef_att", n + " Follower Attack", 0.001f, 2, 0.01f, [k] (SynthParams& s, float v) { s.comb[k].followAttack = v; }, "s", 0.1f);
        b.flt (p + "_ef_rel", n + " Follower Release", 0.001f, 5, 0.2f, [k] (SynthParams& s, float v) { s.comb[k].followRelease = v; }, "s", 0.5f);
    }

    b.group = "Keyboard Curves & Quantize";
    b.flt ("curve_velo", "Velocity Curve", -1, 1, 0, [] (SynthParams& s, float v) { s.curves.velocity = v; });
    b.flt ("curve_at", "Aftertouch Curve", -1, 1, 0, [] (SynthParams& s, float v) { s.curves.aftertouch = v; });
    b.flt ("curve_kbd", "Keytrack Curve", -1, 1, 0, [] (SynthParams& s, float v) { s.curves.keytrack = v; });
    b.integer ("e1_q_mask", "Eng1 Coarse Quantize Notes", 1, 4095, 4095, [] (SynthParams& s, float v) { s.coarseQuantMask[0] = asInt (v); });
    b.integer ("e2_q_mask", "Eng2 Coarse Quantize Notes", 1, 4095, 4095, [] (SynthParams& s, float v) { s.coarseQuantMask[1] = asInt (v); });
    b.integer ("u_osc_q_mask", "Utility Osc Quantize Notes", 1, 4095, 4095, [] (SynthParams& s, float v) { s.utilityQuantMask = asInt (v); });
}

juce::StringArray fxTypeNames()
{
    juce::StringArray a;
    for (int t = 0; t < fx::FxTypeCount; ++t)
        a.add (fx::fxInfo (t).name);
    return a;
}

void addFx (Builder& b)
{
    static const char* const kSlotIds[9] = { "a1", "a2", "a3", "b1", "b2", "b3", "x1", "x2", "x3" };
    static const char* const kSlotNames[9] = { "FX A1", "FX A2", "FX A3", "FX B1", "FX B2", "FX B3", "Aux 1", "Aux 2", "Aux 3" };

    b.group = "FX Buses";
    b.choice ("fx_routing", "FX Routing", juce::StringArray { "A > B", "B > A", "A // B" }, fx::RouteAthenB,
              [] (SynthParams& s, float v) { s.fx.routing = asInt (v); });
    b.boolean ("fx_a_on", "FX Bus A On", true, [] (SynthParams& s, float v) { s.fx.busOn[0] = asBool (v); });
    b.boolean ("fx_b_on", "FX Bus B On", true, [] (SynthParams& s, float v) { s.fx.busOn[1] = asBool (v); });
    b.boolean ("fx_aux_on", "Aux Bus On", true, [] (SynthParams& s, float v) { s.fx.busOn[2] = asBool (v); });
    b.flt ("fx_a_vol", "FX Bus A Volume", 0, 1.5f, 1, [] (SynthParams& s, float v) { s.fx.busVolume[0] = v; });
    b.flt ("fx_b_vol", "FX Bus B Volume", 0, 1.5f, 1, [] (SynthParams& s, float v) { s.fx.busVolume[1] = v; });
    b.flt ("fx_aux_send", "Aux Send", 0, 1, 0, [] (SynthParams& s, float v) { s.fx.auxSend = v; });
    b.flt ("fx_aux_return", "Aux Return", 0, 1, 1, [] (SynthParams& s, float v) { s.fx.auxReturn = v; });
    b.boolean ("fx_aux_pre", "Aux Pre A/B", false, [] (SynthParams& s, float v) { s.fx.auxPre = asBool (v); });
    b.boolean ("fx_on", "FX On", true, [] (SynthParams& s, float v) { s.fx.on = asBool (v); });

    for (size_t sl = 0; sl < 9; ++sl)
    {
        b.group = sl < 3 ? "FX Bus A" : sl < 6 ? "FX Bus B" : "Aux Bus";
        const juce::String p = juce::String ("fx_") + kSlotIds[sl] + "_", n = juce::String (kSlotNames[sl]) + " ";
        b.boolean (p + "on", n + "On", true, [sl] (SynthParams& s, float v) { s.fx.slot[sl].on = asBool (v); });
        b.choice (p + "type", n + "Effect", fxTypeNames(), fx::FxNone, [sl] (SynthParams& s, float v) { s.fx.slot[sl].type = asInt (v); });
        b.boolean (p + "sync", n + "Sync", false, [sl] (SynthParams& s, float v) { s.fx.slot[sl].sync = asBool (v); });
        for (size_t k = 0; k < (size_t) fx::kFxParams; ++k)
            b.flt (p + "p" + juce::String ((int) k + 1), n + "Param " + juce::String ((int) k + 1), 0, 1, 0.5f,
                   [sl, k] (SynthParams& s, float v) { s.fx.slot[sl].p[k] = v; });
    }
}

void addSeq (Builder& b)
{
    b.group = "Seq/Arp";
    auto names = [] (const char* const* list, int n) {
        juce::StringArray a;
        for (int i = 0; i < n; ++i)
            a.add (list[i]);
        return a;
    };
    juce::StringArray rates, scales;
    for (const auto& d : kSyncDivs)
        rates.add (d.name);
    for (const auto& sc : kSeqScales)
        scales.add (sc.name);

    b.boolean ("seq_on", "Seq/Arp On", false, [] (SynthParams& s, float v) { s.seq.on = asBool (v); });
    b.choice ("seq_mode", "Seq/Arp Mode", names (kSeqModeNames, SeqModeCount), SeqModeArp,
              [] (SynthParams& s, float v) { s.seq.mode = asInt (v); });
    b.choice ("seq_rate", "Seq Rate", rates, 13, [] (SynthParams& s, float v) { s.seq.rateDiv = asInt (v); });
    b.flt ("seq_swing", "Seq Swing", 0.5f, 0.75f, 0.5f, [] (SynthParams& s, float v) { s.seq.swing = v; });
    b.choice ("seq_direction", "Seq Direction", names (kSeqDirectionNames, SeqDirectionCount), SeqForward,
              [] (SynthParams& s, float v) { s.seq.direction = asInt (v); });
    b.integer ("seq_length", "Seq Length", 1, kSeqSteps, kSeqSteps, [] (SynthParams& s, float v) { s.seq.length = asInt (v); });
    b.boolean ("seq_polymetry", "Seq Polymetry", false, [] (SynthParams& s, float v) { s.seq.polymetry = asBool (v); });
    b.integer ("seq_reset", "Seq Reset Steps", 0, 64, 0, [] (SynthParams& s, float v) { s.seq.resetSteps = asInt (v); });
    b.choice ("seq_arp_order", "Arp Order", names (kArpOrderNames, ArpOrderCount), ArpUp,
              [] (SynthParams& s, float v) { s.seq.arpOrder = asInt (v); });
    b.integer ("seq_arp_octaves", "Arp Octaves", 1, 4, 1, [] (SynthParams& s, float v) { s.seq.arpOctaves = asInt (v); });
    b.flt ("seq_vel_blend", "Seq Velocity Blend", 0, 1, 0.5f, [] (SynthParams& s, float v) { s.seq.velBlend = v; });
    b.integer ("seq_transpose", "Seq Transpose", -24, 24, 0, [] (SynthParams& s, float v) { s.seq.transpose = (float) asInt (v); });
    b.choice ("seq_scale", "Seq Scale", scales, 0, [] (SynthParams& s, float v) { s.seq.scale = asInt (v); });
    b.boolean ("seq_snap", "Seq Snap to Scale", false, [] (SynthParams& s, float v) { s.seq.snap = asBool (v); });
    b.choice ("seq_autoregen", "Seq Auto Regen", names (kAutoRegenNames, kNumAutoRegen), 0,
              [] (SynthParams& s, float v) { s.seq.autoRegen = asInt (v); });
    b.flt ("seq_bpm", "Tempo (no host)", 40, 240, 120, [] (SynthParams& s, float v) { s.seq.bpm = v; }, "BPM");
    b.boolean ("seq_lock", "Seq Lock Pattern", false, [] (SynthParams& s, float v) { s.seq.lock = asBool (v); });
}

std::vector<ParamDef> build()
{
    Builder b;

    // Macros first so they are the first host parameters (MainStage knob mapping).
    b.group = "Macros";
    for (size_t m = 0; m < 4; ++m)
        b.flt ("macro" + juce::String ((int) m + 1), "Macro " + juce::String ((int) m + 1), 0, 1, 0,
               [m] (SynthParams& s, float v) { s.macro[m] = v; });

    addEngine (b, 0);
    addEngine (b, 1);
    addUtility (b);
    addFilter (b, 0);
    addFilter (b, 1);

    b.group = "Routing & Amp";
    b.flt ("f_routing", "Filter Routing Ser/Par", 0, 1, 0, [] (SynthParams& s, float v) { s.filterRouting = v; });
    b.choice ("f_output", "Filter Output Sum/Split", kFilterOutputs, RoutingSum, [] (SynthParams& s, float v) { s.filterOutput = asInt (v); });
    b.flt ("amp_velo", "Amp Mod Amount", 0, 1, 0.5f, [] (SynthParams& s, float v) { s.ampVelocity = v; });
    b.flt ("voice_pan", "Voice Pan", -1, 1, 0, [] (SynthParams& s, float v) { s.voicePan = v; });
    b.choice ("amp_mod_src", "Amp Mod Source", sourceNames(), SrcVelocity, [] (SynthParams& s, float v) { s.ampModSource = asInt (v); });

    b.group = "Amp Envelope";
    b.flt ("env1_a", "Env Amp Attack", 0.0f, 20, 0.002f, [] (SynthParams& s, float v) { s.ampEnv.attack = v; }, "s", 0.5f);
    b.flt ("env1_d", "Env Amp Decay", 0.001f, 20, 0.4f, [] (SynthParams& s, float v) { s.ampEnv.decay = v; }, "s", 0.8f);
    b.flt ("env1_s", "Env Amp Sustain", 0, 1, 1, [] (SynthParams& s, float v) { s.ampEnv.sustain = v; });
    b.flt ("env1_r", "Env Amp Release", 0.001f, 20, 0.25f, [] (SynthParams& s, float v) { s.ampEnv.release = v; }, "s", 0.8f);
    b.flt ("env1_acurve", "Env Amp Att Curve", -20, 20, 0, [] (SynthParams& s, float v) { s.ampEnv.attCurve = v; });
    b.flt ("env1_dcurve", "Env Amp Dec Curve", -20, 20, 8, [] (SynthParams& s, float v) { s.ampEnv.decCurve = v; });
    b.boolean ("env1_adr", "Env Amp ADR Mode", false, [] (SynthParams& s, float v) { s.ampEnv.adr = asBool (v); });
    b.boolean ("env1_link", "Env Amp Release Link", false, [] (SynthParams& s, float v) { s.ampEnv.releaseLink = asBool (v); });

    b.group = "Keyboard";
    b.choice ("voicing", "Play Mode", kVoicing, VoicingPolyRotate, [] (SynthParams& s, float v) { s.voicing = asInt (v); });
    b.integer ("max_voices", "Max Voices", 1, kMaxVoices, 8, [] (SynthParams& s, float v) { s.maxVoices = asInt (v); });
    b.flt ("glide", "Glide Time", 0, 10, 0, [] (SynthParams& s, float v) { s.glide = v; }, "s", 0.5f);
    b.boolean ("glide_legato", "Glide Legato Only", false, [] (SynthParams& s, float v) { s.glideLegato = asBool (v); });
    b.integer ("bend_up", "Bend Up", 0, 36, 2, [] (SynthParams& s, float v) { s.bendUp = v; });
    b.integer ("bend_down", "Bend Down", 0, 36, 2, [] (SynthParams& s, float v) { s.bendDown = v; });
    b.integer ("transpose", "Transpose Oct", -3, 3, 0, [] (SynthParams& s, float v) { s.transposeOct = asInt (v); });
    b.flt ("fine_tune", "Fine Tune", -1, 1, 0, [] (SynthParams& s, float v) { s.transposeFine = v; }, "st");
    b.boolean ("hold", "Hold", false, [] (SynthParams& s, float v) { s.hold = asBool (v); });

    addModulation (b);
    addFx (b);
    addSeq (b);

    b.group = "Master";
    b.flt ("master_vol", "Master Volume", 0, 1, 0.8f, [] (SynthParams& s, float v) { s.masterVolume = v; });
    b.flt ("master_width", "Master Width", 0, 1, 1, [] (SynthParams& s, float v) { s.masterWidth = v; });

    return b.defs;
}
} // namespace

const std::vector<ParamDef>& parameterDefs()
{
    static const std::vector<ParamDef> defs = build();
    return defs;
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    std::unique_ptr<juce::AudioProcessorParameterGroup> current;
    juce::String currentName;

    auto flush = [&] {
        if (current != nullptr)
            layout.add (std::move (current));
    };

    for (const auto& d : parameterDefs())
    {
        if (d.group != currentName || current == nullptr)
        {
            flush();
            currentName = d.group;
            current = std::make_unique<juce::AudioProcessorParameterGroup> (
                d.group.removeCharacters (" &").toLowerCase(), d.group, " | ");
        }

        const juce::ParameterID pid { d.id, 1 };
        switch (d.kind)
        {
            case ParamDef::Float:
            {
                juce::NormalisableRange<float> range (d.min, d.max);
                if (d.centre > d.min && d.centre < d.max)
                    range.setSkewForCentre (d.centre);
                current->addChild (std::make_unique<juce::AudioParameterFloat> (
                    pid, d.name, range, d.def, juce::AudioParameterFloatAttributes().withLabel (d.unit)));
                break;
            }
            case ParamDef::Int:
                current->addChild (std::make_unique<juce::AudioParameterInt> (pid, d.name, (int) d.min, (int) d.max, (int) d.def));
                break;
            case ParamDef::Choice:
                current->addChild (std::make_unique<juce::AudioParameterChoice> (pid, d.name, d.choices, (int) d.def));
                break;
            case ParamDef::Bool:
                current->addChild (std::make_unique<juce::AudioParameterBool> (pid, d.name, d.def >= 0.5f));
                break;
        }
    }
    flush();
    return layout;
}
} // namespace cs
