#include "Parameters.h"

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
