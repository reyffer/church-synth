// Preset Pack 8: Ladder (the classic three-oscillator mono synth and its ladder
// filter, Moog-style), plus a few ladder polysynths and a bass-pedal sound.
//
// Built on that synth's architecture, so the pack teaches it:
//   THREE oscillators (saw / triangle / square / pulse), Osc 3 often an octave down,
//   + noise -> the mixer pushed into the filter (filter Drive = the famous overdriven
//   mixer) -> the 24 dB ladder lowpass -> a filter contour (Env 2) and a loudness
//   contour (Env Amp), where the release usually equals the decay (the "decay switch").
//   Mono, with glide.
//
// Macros: M1 Brightness (cutoff), M2 Space, M3 a character control
// (Resonance, Contour, Drive, Glide...), M4 Decay / Release.

#include "Packs.h"

#include "plugin/PresetHelpers.h"

#include <map>

using namespace presethelpers;

namespace
{
constexpr int kLadder = 9;

struct Mono
{
    int w1 = kSaw, w2 = kSaw, w3 = kSaw;
    float o1 = 0.8f, o2 = 0.7f, o3 = 0.6f;
    float c2 = 0.0f, c3 = -12.0f, d2 = 0.07f, d3 = -0.05f;
    float width = 0.5f;   // pulse width when a square is used
    float noise = 0.0f;
    float cutoff = 400.0f, reso = 0.2f, kbd = 0.5f, drive = 0.2f;
    float contour = 0.4f;                         // how far Env 2 opens the filter
    float fa = 0.001f, fd = 0.4f, fs = 0.3f;      // filter contour
    float a = 0.002f, d = 0.6f, s = 0.85f;        // loudness contour
    bool decaySwitch = true;                      // release = decay (else a short release)
    bool mono = true, legato = false;
    float glide = 0.0f;
    float level = 2.0f;                           // ladder output level (mono sounds need more)
    float velocity = 0.3f;
};

void ladder (B& b, const Mono& m)
{
    osc (b, 0, 0, m.w1, m.o1, 0.0f, 0.0f, m.width);
    osc (b, 0, 1, m.w2, m.o2, m.c2, m.d2, m.width);
    osc (b, 0, 2, m.w3, m.o3, m.c3, m.d3, m.width);
    b.set ("e1_vol", 0.6f).set ("e1_noise_vol", m.noise).set ("e1_drift", 0.15f);
    model (b, 0, kLadder, 0, m.cutoff, m.reso, m.kbd, m.drive);
    b.set ("f1_vol", m.level);
    const float rel = m.decaySwitch ? m.d : 0.04f;
    env (b, "env2", m.fa, m.fd, m.fs, m.decaySwitch ? m.fd : 0.04f);
    b.route ("Env 2", "f1_cutoff", m.contour);
    env (b, "env1", m.a, m.d, m.s, rel);
    b.set ("amp_velo", m.velocity);
    if (m.mono)
        mono (b, m.legato, m.glide);
}

void ladderMacros (B& b, const char* m3, const char* m3dest, float m3amt, const char* m4, const char* m4dest, float m4amt)
{
    macros (b, "Brightness", "Space", m3, m4);
    b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", m3dest, m3amt).route ("Macro 4", m4dest, m4amt);
}

// Mod wheel = vibrato from LFO 1, as on the original (where Osc 3 or the LFO did it).
void wheelVibrato (B& b, float rate = 5.5f, float depth = 0.01f)
{
    lfo (b, 0, LfoTriangle, rate);
    b.route ("LFO 1", "e1_fine", depth, "Mod Wheel", 1.0f);
}
} // namespace

const std::vector<FactoryPreset>& pack8Presets()
{
    static const std::vector<FactoryPreset> list {
        // ================================================================ BASS
        { "Classic Ladder Bass", "Bass", "Two saws and a square an octave down through the ladder filter: the definitive synth bass.",
          [] (B& b) {
              Mono m;
              m.w3 = kSquare; m.cutoff = 300.0f; m.contour = 0.35f; m.fd = 0.35f; m.fs = 0.25f; m.d = 0.5f; m.s = 0.8f;
              ladder (b, m);
              ladderMacros (b, "Contour", "env2_d", 0.3f, "Decay", "env1_d", 0.3f);
          } },
        { "Round Soul Bass", "Bass", "Triangles and a little square, a low filter, no edge: warm, round soul and gospel bass.",
          [] (B& b) {
              Mono m;
              m.w1 = kTri; m.w2 = kTri; m.w3 = kSquare; m.o3 = 0.35f; m.c3 = -12.0f; m.cutoff = 260.0f; m.reso = 0.05f; m.contour = 0.2f;
              m.fd = 0.5f; m.fs = 0.5f; m.d = 0.8f; m.s = 0.9f; m.drive = 0.1f; m.level = 2.6f;
              ladder (b, m);
              ladderMacros (b, "Thump", "env2_d", -0.2f, "Decay", "env1_d", 0.3f);
          } },
        { "Pedal Bass", "Bass", "Huge, deep bass-pedal sound: hold the root note under a whole worship song. Needs subs.",
          [] (B& b) {
              Mono m;
              m.w3 = kSquare; m.c2 = -12.0f; m.c3 = -12.0f; m.o3 = 0.5f; m.cutoff = 180.0f; m.reso = 0.25f; m.kbd = 0.3f; m.contour = 0.3f;
              m.fa = 0.02f; m.fd = 1.5f; m.fs = 0.5f; m.a = 0.02f; m.d = 2.0f; m.s = 0.9f; m.glide = 0.08f; m.legato = true; m.level = 2.6f;
              ladder (b, m);
              room (b, 0.1f);
              ladderMacros (b, "Growl", "f1_reso", 0.25f, "Release", "env1_r", 0.3f);
          } },
        { "Funk Pluck Bass", "Bass", "Short, plucky square bass with resonance. Slap-like attack, tight decay.",
          [] (B& b) {
              Mono m;
              m.w1 = kSquare; m.w2 = kSaw; m.o2 = 0.4f; m.o3 = 0.4f; m.width = 0.4f; m.cutoff = 250.0f; m.reso = 0.35f; m.contour = 0.45f;
              m.fd = 0.18f; m.fs = 0.1f; m.d = 0.3f; m.s = 0.5f; m.decaySwitch = false; m.velocity = 0.5f;
              ladder (b, m);
              ladderMacros (b, "Resonance", "f1_reso", 0.25f, "Decay", "env2_d", 0.3f);
          } },
        { "Overdriven Bass", "Bass", "The mixer pushed hard into the filter: thick, gritty, rock bass.",
          [] (B& b) {
              Mono m;
              m.cutoff = 350.0f; m.reso = 0.15f; m.drive = 0.7f; m.contour = 0.35f; m.fd = 0.4f; m.fs = 0.35f;
              ladder (b, m);
              ladderMacros (b, "Drive", "f1_drive", 0.3f, "Decay", "env1_d", 0.3f);
          } },
        { "Glide Bass", "Bass", "Legato bass with a long glide: overlap the notes and it slides between them.",
          [] (B& b) {
              Mono m;
              m.w3 = kSquare; m.cutoff = 320.0f; m.contour = 0.3f; m.legato = true; m.glide = 0.12f; m.d = 0.8f; m.s = 0.9f;
              ladder (b, m);
              ladderMacros (b, "Resonance", "f1_reso", 0.25f, "Decay", "env1_d", 0.3f);
          } },
        { "Squelch Bass", "Bass", "High resonance and a snappy contour: a squelchy, talking bass.",
          [] (B& b) {
              Mono m;
              m.o2 = 0.0f; m.o3 = 0.5f; m.cutoff = 220.0f; m.reso = 0.6f; m.contour = 0.5f; m.fd = 0.22f; m.fs = 0.15f;
              m.d = 0.4f; m.s = 0.8f; m.velocity = 0.5f; m.level = 2.8f;
              ladder (b, m);
              b.route ("Velocity", "f1_cutoff", 0.15f);
              ladderMacros (b, "Resonance", "f1_reso", 0.25f, "Contour", "env2_d", 0.3f);
          } },
        { "Ladder Sequence 16ths", "Bass", "A sixteenth-note bass sequence with accents and an octave jump. Hold one note; follows the tempo.",
          [] (B& b) {
              Mono m;
              m.w3 = kSquare; m.cutoff = 280.0f; m.reso = 0.3f; m.contour = 0.4f; m.fd = 0.15f; m.fs = 0.1f; m.d = 0.25f; m.s = 0.4f;
              m.mono = false;
              ladder (b, m);
              mono (b, false, 0.0f);
              b.set ("seq_on", 1.0f).set ("seq_mode", 1.0f).set ("seq_rate", (float) kDiv16).set ("seq_vel_blend", 0.6f).set ("seq_length", 16.0f);
              auto pat = gatePattern ({ 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1 }, 0.5f);
              pat.steps[7].octave = 1;
              pat.steps[15].octave = 1;
              b.pattern (pat);
              ladderMacros (b, "Resonance", "f1_reso", 0.25f, "Gate", "env1_s", 0.3f);
          } },
        // ================================================================ LEADS
        { "Classic Ladder Lead", "Lead", "Three detuned saws, a touch of glide and mod-wheel vibrato. The prog-rock lead.",
          [] (B& b) {
              Mono m;
              m.c3 = 0.0f; m.d3 = -0.1f; m.cutoff = 900.0f; m.reso = 0.2f; m.contour = 0.35f; m.fd = 0.6f; m.fs = 0.5f;
              m.d = 0.5f; m.s = 0.9f; m.legato = true; m.glide = 0.05f; m.level = 1.8f;
              ladder (b, m);
              wheelVibrato (b);
              delay (b, 0, kDiv8D, 0.3f, 0.18f, true);
              room (b, 0.25f);
              ladderMacros (b, "Resonance", "f1_reso", 0.3f, "Release", "env1_r", 0.3f);
          } },
        { "Portamento Lead", "Lead", "Square lead with a long, singing portamento on every note. Expressive and smooth.",
          [] (B& b) {
              Mono m;
              m.w1 = kSquare; m.w2 = kSaw; m.o2 = 0.5f; m.c3 = 12.0f; m.w3 = kTri; m.o3 = 0.3f; m.cutoff = 1100.0f; m.reso = 0.15f;
              m.contour = 0.25f; m.glide = 0.18f; m.legato = false; m.level = 1.8f;
              ladder (b, m);
              wheelVibrato (b);
              delay (b, 0, kDiv4D, 0.3f, 0.2f, true);
              room (b, 0.25f);
              ladderMacros (b, "Resonance", "f1_reso", 0.2f, "Release", "env1_r", 0.3f);
          } },
        { "Octave Saw Lead", "Lead", "Saws an octave apart with drive: big, bold and cutting.",
          [] (B& b) {
              Mono m;
              m.c2 = 12.0f; m.o2 = 0.5f; m.c3 = -12.0f; m.cutoff = 1200.0f; m.reso = 0.2f; m.drive = 0.45f; m.contour = 0.3f;
              m.legato = true; m.glide = 0.04f; m.level = 1.7f;
              ladder (b, m);
              wheelVibrato (b);
              delay (b, 0, kDiv8D, 0.3f, 0.18f, true);
              room (b, 0.22f);
              ladderMacros (b, "Drive", "f1_drive", 0.3f, "Release", "env1_r", 0.3f);
          } },
        { "Flute Lead", "Lead", "Triangles, a breath of noise and a gentle delayed vibrato: a soft synth flute.",
          [] (B& b) {
              Mono m;
              m.w1 = kTri; m.w2 = kTri; m.o2 = 0.3f; m.c2 = 12.0f; m.o3 = 0.0f; m.noise = 0.04f; m.cutoff = 1500.0f; m.reso = 0.05f;
              m.drive = 0.0f; m.contour = 0.15f; m.a = 0.05f; m.legato = true; m.glide = 0.03f; m.level = 2.6f;
              ladder (b, m);
              lfo (b, 0, LfoSine, 5.0f);
              b.set ("lfo1_fade", 0.6f).set ("lfo1_retrig", 2.0f);
              b.route ("LFO 1", "e1_fine", 0.006f);
              hall (b, 0.3f, 0.75f, 0.65f);
              ladderMacros (b, "Breath", "e1_noise_vol", 0.08f, "Release", "env1_r", 0.3f);
          } },
        { "Funky Square Lead", "Lead", "Short, bouncy square lead with a snappy filter. Funk and disco lines.",
          [] (B& b) {
              Mono m;
              m.w1 = kSquare; m.w2 = kSquare; m.width = 0.35f; m.o3 = 0.0f; m.cutoff = 500.0f; m.reso = 0.35f; m.contour = 0.45f;
              m.fd = 0.25f; m.fs = 0.2f; m.d = 0.4f; m.s = 0.6f; m.decaySwitch = false; m.velocity = 0.5f; m.level = 2.2f;
              ladder (b, m);
              delay (b, 0, kDiv8, 0.25f, 0.15f, true);
              room (b, 0.2f);
              ladderMacros (b, "Resonance", "f1_reso", 0.25f, "Contour", "env2_d", 0.3f);
          } },
        { "Trill Lead", "Lead", "Mod wheel up and the lead trills a semitone, like a fast square-wave LFO on the pitch.",
          [] (B& b) {
              Mono m;
              m.c3 = 0.0f; m.d3 = 0.1f; m.cutoff = 1000.0f; m.reso = 0.2f; m.contour = 0.3f; m.legato = true; m.glide = 0.03f; m.level = 1.8f;
              ladder (b, m);
              lfo (b, 0, LfoSquare, 9.0f, false);
              b.route ("LFO 1", "e1_coarse", 1.0f / 120.0f, "Mod Wheel", 1.0f);
              delay (b, 0, kDiv8D, 0.3f, 0.18f, true);
              room (b, 0.22f);
              ladderMacros (b, "Trill Speed", "lfo1_rate", 0.15f, "Release", "env1_r", 0.3f);
          } },
        { "Screaming Lead", "Lead", "Driven saws and high resonance: an aggressive, singing rock lead.",
          [] (B& b) {
              Mono m;
              m.c3 = 7.0f; m.o3 = 0.4f; m.cutoff = 1300.0f; m.reso = 0.5f; m.drive = 0.6f; m.contour = 0.3f; m.legato = true; m.glide = 0.05f;
              m.level = 1.6f;
              ladder (b, m);
              wheelVibrato (b, 5.8f, 0.012f);
              delay (b, 0, kDiv8D, 0.35f, 0.2f, true);
              room (b, 0.25f);
              ladderMacros (b, "Scream", "f1_reso", 0.25f, "Release", "env1_r", 0.3f);
          } },
        { "Soft Mono Lead", "Lead", "Mellow triangle-and-saw lead with a low filter. Gentle melodies over worship.",
          [] (B& b) {
              Mono m;
              m.w1 = kTri; m.o2 = 0.3f; m.w3 = kTri; m.c3 = 12.0f; m.o3 = 0.25f; m.cutoff = 700.0f; m.reso = 0.1f; m.drive = 0.0f;
              m.contour = 0.2f; m.a = 0.03f; m.legato = true; m.glide = 0.05f; m.level = 2.4f;
              ladder (b, m);
              lfo (b, 0, LfoSine, 5.2f);
              b.set ("lfo1_fade", 0.7f).set ("lfo1_retrig", 2.0f);
              b.route ("LFO 1", "e1_fine", 0.005f);
              delay (b, 0, kDiv4D, 0.3f, 0.2f, true);
              hall (b, 0.3f, 0.75f, 0.65f);
              ladderMacros (b, "Resonance", "f1_reso", 0.25f, "Release", "env1_r", 0.3f);
          } },
        // ================================================================ POLY
        { "Ladder Poly Brass", "Strings/Brass", "Polyphonic ladder brass, the big American poly: thick, warm, punchy.",
          [] (B& b) {
              Mono m;
              m.mono = false; m.o3 = 0.0f; m.cutoff = 600.0f; m.reso = 0.1f; m.drive = 0.3f; m.contour = 0.4f;
              m.fa = 0.05f; m.fd = 0.6f; m.fs = 0.5f; m.a = 0.03f; m.d = 0.6f; m.s = 0.9f; m.level = 1.0f;
              ladder (b, m);
              b.set ("e1_drift", 0.3f);
              room (b, 0.22f);
              ladderMacros (b, "Bite", "env2_d", -0.25f, "Release", "env1_r", 0.3f);
          } },
        { "Ladder Poly Pad", "Pad", "Warm polyphonic ladder pad with drift and a slow filter. Rich without being bright.",
          [] (B& b) {
              Mono m;
              m.mono = false; m.w2 = kSquare; m.width = 0.4f; m.o3 = 0.0f; m.cutoff = 500.0f; m.reso = 0.15f; m.drive = 0.2f; m.contour = 0.3f;
              m.fa = 1.2f; m.fd = 2.0f; m.fs = 0.6f; m.a = 0.6f; m.d = 1.5f; m.s = 1.0f; m.level = 1.0f;
              ladder (b, m);
              b.set ("e1_drift", 0.35f).set ("env1_r", 1.5f).set ("env2_r", 1.5f);
              hall (b, 0.38f, 0.8f, 0.7f);
              ladderMacros (b, "Swell", "env2_a", 0.3f, "Release", "env1_r", 0.3f);
          } },
        { "Ensemble Poly Strings", "Strings/Brass", "70s polyphonic strings with a slow phaser: the classic swirling string ensemble.",
          [] (B& b) {
              Mono m;
              m.mono = false; m.o3 = 0.0f; m.d2 = 0.12f; m.cutoff = 2500.0f; m.reso = 0.05f; m.drive = 0.0f; m.contour = 0.0f;
              m.a = 0.35f; m.d = 1.0f; m.s = 1.0f; m.level = 1.0f;
              ladder (b, m);
              b.set ("env1_r", 0.9f);
              filter (b, 1, 8, 250.0f, 0.0f);
              b.set ("f_routing", 0.0f);
              b.fx (0, FxPhaser, { { 3, 0.5f }, { 4, 0.1f }, { 7, 0.4f } });
              hall (b, 0.32f, 0.75f, 0.65f);
              ladderMacros (b, "Phase", "fx_a1_p8", 0.3f, "Attack", "env1_a", 0.35f);
          } },
        { "Ladder Pluck Keys", "Keys", "Polyphonic ladder pluck: woody, round, a little resonant. Rhythmic keys.",
          [] (B& b) {
              Mono m;
              m.mono = false; m.w2 = kSquare; m.o3 = 0.0f; m.cutoff = 400.0f; m.reso = 0.3f; m.contour = 0.45f;
              m.fd = 0.3f; m.fs = 0.0f; m.d = 0.9f; m.s = 0.0f; m.level = 1.2f; m.velocity = 0.5f;
              ladder (b, m);
              delay (b, 0, kDiv8D, 0.25f, 0.15f, true);
              room (b, 0.22f);
              ladderMacros (b, "Resonance", "f1_reso", 0.25f, "Decay", "env1_d", 0.3f);
          } },
    };
    return list;
}

juce::StringArray pack8Tags (const juce::String& presetName)
{
    static const std::map<juce::String, const char*> tags {
        { "Classic Ladder Bass", "Ladder, Bass, Mono" }, { "Round Soul Bass", "Ladder, Bass, Mono, Soul" },
        { "Pedal Bass", "Ladder, Bass, Mono, Sub, Worship" }, { "Funk Pluck Bass", "Ladder, Bass, Mono, Funk" },
        { "Overdriven Bass", "Ladder, Bass, Mono, Driven" }, { "Glide Bass", "Ladder, Bass, Mono" },
        { "Squelch Bass", "Ladder, Bass, Mono" }, { "Ladder Sequence 16ths", "Ladder, Bass, Pulsing, Sequence" },
        { "Classic Ladder Lead", "Ladder, Lead, Mono" }, { "Portamento Lead", "Ladder, Lead, Mono" },
        { "Octave Saw Lead", "Ladder, Lead, Mono, Driven" }, { "Flute Lead", "Ladder, Lead, Mono, Worship" },
        { "Funky Square Lead", "Ladder, Lead, Mono, Funk" }, { "Trill Lead", "Ladder, Lead, Mono" },
        { "Screaming Lead", "Ladder, Lead, Mono, Driven" }, { "Soft Mono Lead", "Ladder, Lead, Mono, Worship" },
        { "Ladder Poly Brass", "Ladder, Brass, Synth Brass, Polysynth" }, { "Ladder Poly Pad", "Ladder, Pad, Polysynth" },
        { "Ensemble Poly Strings", "Ladder, Strings, Polysynth" }, { "Ladder Pluck Keys", "Ladder, Pluck, Synth Keys, Polysynth" },
    };
    const auto it = tags.find (presetName);
    return it == tags.end() ? juce::StringArray() : PresetManager::parseTags (it->second);
}
