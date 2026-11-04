#include "Parameters.h"

#include "dsp/Content.h"

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
    b.flt (p + "vol", n + "Volume", 0, 1, 1, [f] (SynthParams& s, float v) { s.filter[f].volume = v; });
    b.flt (p + "pan", n + "Pan", -1, 1, 0, [f] (SynthParams& s, float v) { s.filter[f].pan = v; });
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
    b.flt ("amp_velo", "Amp Velocity", 0, 1, 0.5f, [] (SynthParams& s, float v) { s.ampVelocity = v; });
    b.flt ("voice_pan", "Voice Pan", -1, 1, 0, [] (SynthParams& s, float v) { s.voicePan = v; });

    b.group = "Amp Envelope";
    b.flt ("env1_a", "Env Amp Attack", 0.001f, 20, 0.002f, [] (SynthParams& s, float v) { s.ampEnv.attack = v; }, "s", 0.5f);
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

    b.group = "Master";
    b.flt ("master_vol", "Master Volume", 0, 1, 0.8f, [] (SynthParams& s, float v) { s.masterVolume = v; });

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
