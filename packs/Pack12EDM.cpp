// Preset Pack 12: EDM.
//
// Modern electronic dance sounds: plucks, stabs and leads for house, progressive,
// trance, future bass and big-room. The style's ingredients, pointed out in the
// notes: supersaws (7-voice unison), side-chain pumping, pitch rises into chords,
// chord stabs from one key, short plucks with big reverbs and echoes.
//
// Macros: M1 Brightness, M2 Space, M3 a character control, M4 Decay / Release.

#include "Packs.h"

#include "plugin/PresetHelpers.h"

#include <map>

using namespace presethelpers;

namespace
{
constexpr int kOta = 11, kLadder = 9;

struct Edm
{
    int w1 = kSaw, w2 = kSaw;
    float v1 = 0.8f, v2 = 0.0f, c2 = 0.0f, d2 = 0.08f, width = 0.5f, sub = 0.0f;
    int voices = 1, uniMode = 0; float uniDetune = 0.2f, uniStereo = 0.7f;
    int chord = -1;                               // >= 0: chord-mode unison (one key = a chord)
    int model = kOta;
    float cutoff = 2000.0f, reso = 0.1f, kbd = 0.4f, drive = 0.0f;
    float envAmt = 0.3f, velAmt = 0.1f;
    float fa = 0.001f, fd = 0.3f, fs = 0.0f, fr = 0.3f;
    float a = 0.001f, d = 0.4f, s = 0.0f, r = 0.35f;
    bool mono = false, legato = false; float glide = 0.0f;
    float level = 1.0f;
};

void edm (B& b, const Edm& x)
{
    osc (b, 0, 0, x.w1, x.v1, 0.0f, 0.0f, x.width);
    osc (b, 0, 1, x.w2, x.v2, x.c2, x.d2, x.width);
    osc (b, 0, 2, kSquare, x.sub, -12.0f);
    b.set ("e1_vol", 0.6f);
    if (x.chord >= 0)
    {
        uni (b, 0, 1, 4, 0.0f, 0.6f);
        b.set ("e1_uni_chord", (float) x.chord);
    }
    else if (x.voices > 1)
    {
        uni (b, 0, x.uniMode, x.voices, x.uniDetune, x.uniStereo);
        if (x.uniMode == 2)
            b.set ("e1_uni_mix", 0.65f);
    }
    model (b, 0, x.model, 0, x.cutoff, x.reso, x.kbd, x.drive);
    b.set ("f1_vol", x.level);
    env (b, "env2", x.fa, x.fd, x.fs, x.fr);
    b.route ("Env 2", "f1_cutoff", x.envAmt);
    if (x.velAmt > 0.0f)
        b.route ("Velocity", "f1_cutoff", x.velAmt);
    env (b, "env1", x.a, x.d, x.s, x.r);
    b.set ("amp_velo", 0.4f);
    if (x.mono)
        mono (b, x.legato, x.glide);
}

// Side-chain style pump on every beat (Function "Duck" on the amp, synced).
void pump (B& b, float depth)
{
    b.set ("func1_shape", 15.0f).set ("func1_sync", 1.0f).set ("func1_div", (float) kDiv4).set ("func1_retrig", 3.0f)
        .set ("func1_smooth", 0.08f);
    b.set ("amp_mod_src", 11.0f).set ("amp_velo", depth);
}

// Pitch rise into each note: Env 3 bends the engine up from `semis` below.
void pitchRise (B& b, float semis, float time)
{
    env (b, "env3", 0.0f, time, 0.0f, 0.05f);
    b.route ("Env 3", "e1_coarse", -semis / 120.0f);
}

void eMacros (B& b, const char* m3, const char* m3dest, float m3amt, const char* m4, const char* m4dest, float m4amt)
{
    macros (b, "Brightness", "Space", m3, m4);
    b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", m3dest, m3amt).route ("Macro 4", m4dest, m4amt);
}

constexpr int kChordMajor = 3, kChordMinor = 4, kChordMajor7 = 7, kChordMinor7 = 8;
} // namespace

const std::vector<FactoryPreset>& pack12Presets()
{
    static const std::vector<FactoryPreset> list {
        // ================================================================ PLUCKS
        { "Progressive Pluck", "Pluck", "Bright supersaw pluck into a dotted echo and a big hall: the progressive-house lead pluck.",
          [] (B& b) {
              Edm x; x.voices = 7; x.uniMode = 2; x.cutoff = 800.0f; x.reso = 0.15f; x.envAmt = 0.45f; x.fd = 0.25f; x.d = 0.45f; x.r = 0.35f;
              x.level = 1.6f;
              edm (b, x);
              delay (b, 0, kDiv8D, 0.4f, 0.25f, true);
              hall (b, 0.38f, 0.85f, 0.75f);
              eMacros (b, "Echo", "fx_a1_p8", 0.35f, "Decay", "env1_d", 0.35f);
          } },
        { "Tropical Pluck", "Pluck", "Soft, woody marimba-like pluck for tropical house. Short and bouncy.",
          [] (B& b) {
              fmVoice (b, 0, 3.0f, 0.0f, 0.8f);
              env (b, "env2", 0.001f, 0.08f, 0.0f, 0.05f);
              b.route ("Env 2", "e1_wt_pm", 0.3f, "Velocity", 1.0f);
              filter (b, 0, kLP12, 5000.0f, 0.0f, 0.3f);
              b.set ("f1_vol", 1.4f);
              env (b, "env1", 0.001f, 0.35f, 0.0f, 0.2f);
              b.set ("amp_velo", 0.5f);
              delay (b, 0, kDiv8D, 0.3f, 0.2f, true);
              hall (b, 0.3f);
              eMacros (b, "Mallet", "e1_wt_pm", 0.2f, "Decay", "env1_d", 0.35f);
          } },
        { "Future Pluck", "Pluck", "One key = a major-seventh chord pluck with a pitch rise: the future-bass chord pluck.",
          [] (B& b) {
              Edm x; x.v2 = 0.5f; x.chord = kChordMajor7; x.cutoff = 1200.0f; x.envAmt = 0.4f; x.fd = 0.25f; x.d = 0.5f; x.r = 0.35f;
              edm (b, x);
              pitchRise (b, 1.0f, 0.06f);
              vintageChorus (b, 0, 1, 0.35f);
              b.set ("fx_a1_p6", 0.7f);
              delay (b, 1, kDiv8D, 0.3f, 0.18f, true);
              hall (b, 0.35f);
              eMacros (b, "Echo", "fx_a2_p8", 0.35f, "Decay", "env1_d", 0.35f);
          } },
        { "Trance Pluck", "Pluck", "Short, bright supersaw pluck with a quick resonant snap. For fast trance arpeggios.",
          [] (B& b) {
              Edm x; x.voices = 7; x.uniMode = 2; x.cutoff = 600.0f; x.reso = 0.3f; x.envAmt = 0.5f; x.fd = 0.15f; x.d = 0.25f; x.r = 0.2f;
              x.level = 1.6f;
              edm (b, x);
              delay (b, 0, kDiv8D, 0.35f, 0.22f, true);
              hall (b, 0.32f);
              eMacros (b, "Resonance", "f1_reso", 0.25f, "Decay", "env1_d", 0.35f);
          } },
        { "Bell Drop Pluck", "Pluck", "A glassy FM bell pluck with a long echo: the melodic drop that floats over the kick.",
          [] (B& b) {
              fmVoice (b, 0, 2.0f, 0.1f, 0.6f);
              env (b, "env2", 0.001f, 0.4f, 0.0f, 0.3f);
              b.route ("Env 2", "e1_wt_pm", 0.3f, "Velocity", 1.0f);
              fmVoice (b, 1, 7.0f, 0.3f, 0.0f);
              env (b, "env3", 0.001f, 0.2f, 0.0f, 0.1f);
              b.route ("Env 3", "e2_vol", 0.3f);
              filter (b, 0, kLP12, 10000.0f, 0.0f, 0.3f);
              b.set ("f1_vol", 1.4f);
              env (b, "env1", 0.001f, 1.2f, 0.0f, 0.8f);
              delay (b, 0, kDiv8D, 0.4f, 0.28f, true);
              hall (b, 0.38f, 0.85f, 0.8f);
              eMacros (b, "Metal", "e1_wt_pm", 0.2f, "Decay", "env1_d", 0.35f);
          } },
        { "Pumping Pluck", "Pluck", "A saw pluck that ducks on every beat, like it's side-chained to the kick.",
          [] (B& b) {
              Edm x; x.v2 = 0.5f; x.voices = 3; x.cutoff = 900.0f; x.envAmt = 0.4f; x.fd = 0.3f; x.d = 0.6f; x.s = 0.2f; x.r = 0.4f;
              x.level = 1.4f;
              edm (b, x);
              pump (b, 0.6f);
              delay (b, 0, kDiv8D, 0.3f, 0.18f, true);
              hall (b, 0.3f);
              eMacros (b, "Pump", "amp_velo", 0.3f, "Decay", "env1_d", 0.35f);
          } },
        { "Dark Techno Pluck", "Pluck", "A dark, resonant ladder pluck for melodic techno: hypnotic and moody.",
          [] (B& b) {
              Edm x; x.v2 = 0.4f; x.c2 = -12.0f; x.model = kLadder; x.cutoff = 300.0f; x.reso = 0.45f; x.drive = 0.3f; x.envAmt = 0.4f;
              x.fd = 0.2f; x.d = 0.4f; x.r = 0.3f; x.level = 2.0f;
              edm (b, x);
              delay (b, 0, kDiv8D, 0.4f, 0.25f, true);
              hall (b, 0.35f, 0.85f, 0.8f);
              eMacros (b, "Resonance", "f1_reso", 0.25f, "Decay", "env2_d", 0.3f);
          } },
        // ================================================================ STABS
        { "Big Room Stab", "Keys", "A huge, bright supersaw chord stab for the drop. Play big chords, short.",
          [] (B& b) {
              Edm x; x.voices = 7; x.uniMode = 2; x.uniDetune = 0.25f; x.sub = 0.3f; x.cutoff = 2500.0f; x.envAmt = 0.3f; x.fd = 0.3f; x.fs = 0.3f;
              x.d = 0.4f; x.s = 0.3f; x.r = 0.3f;
              x.level = 1.2f;
              edm (b, x);
              b.fx (0, FxDistortion, { { 0, (2 + 0.5f) / 16.0f }, { 1, 0.15f }, { 10, 0.3f } });
              hall (b, 0.3f, 0.8f, 0.7f);
              eMacros (b, "Drive", "fx_a1_p2", 0.3f, "Decay", "env1_d", 0.35f);
          } },
        { "Future Bass Chords", "Keys", "Wide supersaw chords that pitch-rise in, pump with the beat and wobble. The future-bass drop.",
          [] (B& b) {
              Edm x; x.voices = 7; x.uniMode = 2; x.uniDetune = 0.22f; x.cutoff = 1800.0f; x.envAmt = 0.2f; x.fd = 0.5f; x.fs = 0.6f;
              x.a = 0.005f; x.d = 1.0f; x.s = 1.0f; x.r = 0.4f;
              edm (b, x);
              pitchRise (b, 2.0f, 0.08f);
              pump (b, 0.55f);
              lfo (b, 1, LfoTriangle, 4.0f);
              b.set ("lfo2_sync", 1.0f).set ("lfo2_div", (float) kDiv8).set ("lfo2_retrig", 3.0f);
              b.route ("LFO 2", "f1_cutoff", 0.15f, "Macro 3", 1.0f); // Macro 3 = wobble depth
              vintageChorus (b, 0, 1, 0.3f);
              b.set ("fx_a1_p6", 0.7f);
              hall (b, 0.3f, 0.8f, 0.7f);
              macros (b, "Brightness", "Space", "Wobble", "Release");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 4", "env1_r", 0.3f);
          } },
        { "House Organ Stab", "Keys", "The 90s house organ: sines at octave and fifth partials with a quick, percussive decay.",
          [] (B& b) {
              osc (b, 0, 0, kSine, 0.8f);
              osc (b, 0, 1, kSine, 0.6f, 12.0f);
              osc (b, 0, 2, kSine, 0.4f, 19.0f);
              b.set ("e1_vol", 0.6f);
              filter (b, 0, kLP12, 4000.0f, 0.0f, 0.3f, 0.3f);
              b.set ("f1_vol", 1.6f);
              env (b, "env1", 0.001f, 0.3f, 0.25f, 0.12f);
              b.set ("amp_velo", 0.5f);
              chorus (b, 0, 0.25f);
              room (b, 0.25f);
              eMacros (b, "Click", "e1_o3_vol", 0.3f, "Decay", "env1_d", 0.35f);
          } },
        { "Rave Stab", "Keys", "One key = a minor chord of resonant saws with a snappy filter: the 90s rave stab.",
          [] (B& b) {
              Edm x; x.v2 = 0.5f; x.chord = kChordMinor; x.cutoff = 700.0f; x.reso = 0.4f; x.envAmt = 0.45f; x.fd = 0.25f; x.fs = 0.1f;
              x.d = 0.4f; x.s = 0.0f; x.r = 0.3f;
              x.level = 1.7f;
              edm (b, x);
              delay (b, 0, kDiv8D, 0.3f, 0.2f, true);
              hall (b, 0.3f);
              eMacros (b, "Resonance", "f1_reso", 0.25f, "Decay", "env1_d", 0.35f);
          } },
        { "Piano House Stab", "Keys", "Bright FM piano chords for piano-house: punchy, percussive, made for off-beat chords.",
          [] (B& b) {
              fmVoice (b, 0, 1.0f, 0.25f, 0.75f);
              env (b, "env2", 0.001f, 0.6f, 0.1f, 0.3f);
              b.route ("Env 2", "e1_wt_pm", 0.35f, "Velocity", 1.0f);
              fmVoice (b, 1, 14.0f, 0.5f, 0.0f);
              env (b, "env3", 0.001f, 0.15f, 0.0f, 0.1f);
              b.route ("Env 3", "e2_vol", 0.4f);
              filter (b, 0, kLP12, 12000.0f, 0.0f, 0.3f);
              env (b, "env1", 0.0f, 1.5f, 0.0f, 0.3f);
              b.set ("amp_velo", 0.6f);
              room (b, 0.25f);
              eMacros (b, "Bark", "e1_wt_pm", 0.2f, "Decay", "env1_d", 0.35f);
          } },
        { "Pumping Saw Stab", "Keys", "Sustained saw chords that duck on every beat: the side-chained house chord.",
          [] (B& b) {
              Edm x; x.v2 = 0.6f; x.voices = 3; x.cutoff = 1500.0f; x.envAmt = 0.15f; x.fd = 0.5f; x.fs = 0.6f; x.a = 0.005f; x.d = 1.0f; x.s = 1.0f; x.r = 0.3f;
              edm (b, x);
              pump (b, 0.7f);
              vintageChorus (b, 0, 1, 0.3f);
              b.set ("fx_a1_p6", 0.7f);
              hall (b, 0.28f);
              eMacros (b, "Pump", "amp_velo", 0.25f, "Release", "env1_r", 0.3f);
          } },
        { "Brass Drop Stab", "Strings/Brass", "Bright synth brass with a pitch rise into each stab. Big-room horns.",
          [] (B& b) {
              Edm x; x.v2 = 0.7f; x.voices = 3; x.cutoff = 900.0f; x.envAmt = 0.4f; x.fa = 0.02f; x.fd = 0.3f; x.fs = 0.4f;
              x.a = 0.01f; x.d = 0.4f; x.s = 0.6f; x.r = 0.25f;
              edm (b, x);
              pitchRise (b, 1.0f, 0.08f);
              b.set ("f1_drive", 0.3f);
              room (b, 0.25f);
              eMacros (b, "Bite", "env2_d", -0.2f, "Release", "env1_r", 0.3f);
          } },
        // ================================================================ LEADS
        { "Trance Supersaw Lead", "Lead", "The anthemic trance lead: 7-voice supersaw, open filter, long echo and reverb.",
          [] (B& b) {
              Edm x; x.voices = 7; x.uniMode = 2; x.uniDetune = 0.22f; x.cutoff = 3500.0f; x.envAmt = 0.1f; x.a = 0.005f; x.d = 0.5f; x.s = 0.9f;
              x.r = 0.35f; x.mono = true; x.legato = true; x.glide = 0.04f; x.level = 1.8f;
              edm (b, x);
              lfo (b, 0, LfoSine, 5.5f);
              b.route ("LFO 1", "e1_fine", 0.008f, "Mod Wheel", 1.0f);
              delay (b, 0, kDiv8D, 0.4f, 0.25f, true);
              hall (b, 0.38f, 0.85f, 0.8f);
              eMacros (b, "Width", "e1_uni_detune", 0.15f, "Release", "env1_r", 0.3f);
          } },
        { "Festival Lead", "Lead", "Big-room saw lead with a sub and drive: loud, simple, made for the drop.",
          [] (B& b) {
              Edm x; x.v2 = 0.7f; x.d2 = 0.15f; x.sub = 0.4f; x.voices = 3; x.cutoff = 2500.0f; x.drive = 0.3f; x.envAmt = 0.15f;
              x.a = 0.003f; x.d = 0.4f; x.s = 0.9f; x.r = 0.25f; x.mono = true; x.legato = true; x.glide = 0.03f; x.level = 1.8f;
              edm (b, x);
              b.set ("bend_up", 12.0f).set ("bend_down", 12.0f);
              room (b, 0.25f);
              eMacros (b, "Drive", "f1_drive", 0.3f, "Release", "env1_r", 0.3f);
          } },
        { "Future Bass Lead", "Lead", "A bright square-saw lead with a pitch rise, glide and a quick vibrato. Playful and wide.",
          [] (B& b) {
              Edm x; x.w1 = kSquare; x.v2 = 0.5f; x.width = 0.45f; x.voices = 3; x.cutoff = 2500.0f; x.envAmt = 0.15f;
              x.a = 0.003f; x.d = 0.5f; x.s = 0.9f; x.r = 0.3f; x.mono = true; x.legato = true; x.glide = 0.08f; x.level = 1.8f;
              edm (b, x);
              pitchRise (b, 1.0f, 0.05f);
              lfo (b, 0, LfoSine, 6.5f);
              b.set ("lfo1_fade", 0.3f).set ("lfo1_retrig", 2.0f);
              b.route ("LFO 1", "e1_fine", 0.01f);
              vintageChorus (b, 0, 1, 0.35f);
              b.set ("fx_a1_p6", 0.7f);
              delay (b, 1, kDiv8D, 0.3f, 0.2f, true);
              hall (b, 0.3f);
              eMacros (b, "Vibrato", "lfo1_rate", 0.1f, "Release", "env1_r", 0.3f);
          } },
        { "Sync Screech Lead", "Lead", "Aggressive hard-sync screech with drive: a cutting, edgy lead.",
          [] (B& b) {
              Edm x; x.v1 = 0.3f; x.v2 = 0.8f; x.c2 = 12.0f; x.cutoff = 4000.0f; x.drive = 0.5f; x.envAmt = 0.1f; x.fd = 0.5f; x.fs = 0.4f;
              x.a = 0.002f; x.d = 0.4f; x.s = 0.9f; x.r = 0.2f; x.mono = true; x.legato = true; x.glide = 0.03f; x.level = 2.8f;
              edm (b, x);
              b.set ("e1_sync", 1.0f);
              b.route ("Env 2", "e1_o2_coarse", 0.2f).route ("Mod Wheel", "e1_o2_coarse", 0.25f);
              room (b, 0.22f);
              eMacros (b, "Screech", "e1_o2_coarse", 0.15f, "Release", "env1_r", 0.3f);
          } },
        { "Vocal Chop Lead", "Lead", "A saw through a vowel filter that changes on every note: like chopped vocal samples.",
          [] (B& b) {
              Edm x; x.v2 = 0.5f; x.cutoff = 4000.0f; x.envAmt = 0.0f; x.a = 0.005f; x.d = 0.3f; x.s = 0.7f; x.r = 0.2f;
              x.mono = true; x.legato = false; x.glide = 0.02f; x.level = 1.6f;
              edm (b, x);
              b.set ("f2_on", 1.0f).set ("f2_type", 3.0f).set ("f2_cutoff", 1000.0f).set ("f2_reso", 0.35f).set ("f2_a", 0.3f).set ("f2_b", 0.75f)
                  .set ("f_routing", 0.0f).set ("f2_vol", 1.6f);
              b.set ("rand1_type", 1.0f).set ("rand1_vm_mode", 0.0f);
              b.route ("Random 1", "f2_a", 0.25f);
              delay (b, 0, kDiv8D, 0.3f, 0.2f, true);
              hall (b, 0.32f);
              eMacros (b, "Vowel", "f2_a", 0.3f, "Release", "env1_r", 0.3f);
          } },
        { "Drop Pluck Lead", "Lead", "A bright saw lead that plucks on every note but sustains a little: melodic drops.",
          [] (B& b) {
              Edm x; x.v2 = 0.6f; x.voices = 3; x.cutoff = 900.0f; x.reso = 0.2f; x.envAmt = 0.45f; x.fd = 0.25f; x.fs = 0.25f;
              x.a = 0.002f; x.d = 0.4f; x.s = 0.5f; x.r = 0.3f; x.mono = true; x.legato = false; x.glide = 0.03f; x.level = 2.8f;
              edm (b, x);
              delay (b, 0, kDiv8D, 0.35f, 0.22f, true);
              hall (b, 0.32f);
              eMacros (b, "Resonance", "f1_reso", 0.25f, "Release", "env1_r", 0.3f);
          } },
    };
    return list;
}

juce::StringArray pack12Tags (const juce::String& presetName)
{
    static const std::map<juce::String, const char*> tags {
        { "Progressive Pluck", "EDM, Pluck, Supersaw" }, { "Tropical Pluck", "EDM, Pluck, FM, Mallet" },
        { "Future Pluck", "EDM, Pluck, Chord, Future Bass" }, { "Trance Pluck", "EDM, Pluck, Supersaw, Trance" },
        { "Bell Drop Pluck", "EDM, Pluck, Bells, FM" }, { "Pumping Pluck", "EDM, Pluck, Pulsing" },
        { "Dark Techno Pluck", "EDM, Pluck, Dark, Techno" },
        { "Big Room Stab", "EDM, Stab, Supersaw, Big" }, { "Future Bass Chords", "EDM, Stab, Supersaw, Future Bass, Pulsing" },
        { "House Organ Stab", "EDM, Stab, Organ, House" }, { "Rave Stab", "EDM, Stab, Chord" },
        { "Piano House Stab", "EDM, Stab, Piano, FM, House" }, { "Pumping Saw Stab", "EDM, Stab, Pulsing, House" },
        { "Brass Drop Stab", "EDM, Stab, Brass" },
        { "Trance Supersaw Lead", "EDM, Lead, Supersaw, Trance" }, { "Festival Lead", "EDM, Lead, Big" },
        { "Future Bass Lead", "EDM, Lead, Future Bass" }, { "Sync Screech Lead", "EDM, Lead, Sync, Driven" },
        { "Vocal Chop Lead", "EDM, Lead" }, { "Drop Pluck Lead", "EDM, Lead, Pluck" },
    };
    const auto it = tags.find (presetName);
    return it == tags.end() ? juce::StringArray() : PresetManager::parseTags (it->second);
}
