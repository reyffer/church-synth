// Preset Pack 5: Brass.
//
// R&B and soul synth brass, sharp stabs and falls, and horns (trumpets, trombones,
// a horn section, French horns, solo / muted trumpet, flugelhorn). All synthesised.
//
// What makes synth brass sound like brass: the tone gets brighter as it gets louder
// (a filter envelope that follows the amp, scaled by velocity), a small pitch scoop
// up into each note, a little breath, and a section of slightly out-of-tune players.
//
// Macros: M1 Brightness, M2 Space, M3 a character control (Bite, Scoop, Fall...),
// M4 Attack / Release.

#include "Packs.h"

#include "plugin/PresetHelpers.h"

#include <map>

using namespace presethelpers;

namespace
{
constexpr int kOta = 11;

struct Brass
{
    int wave2 = kSaw;      // second oscillator (saw = brassy, square = reedy, triangle = mellow)
    float osc2 = 0.5f;     // its level
    float detune = 0.08f;  // osc 2 detune (semitones)
    int players = 2;       // unison voices
    float cutoff = 700.0f; // closed filter
    float reso = 0.12f;
    float envAmt = 0.35f;  // how far the filter opens (half always, half with velocity)
    float attack = 0.03f;  // amp attack
    float decay = 0.4f, sustain = 0.85f, release = 0.25f;
    float filterSustain = 0.55f;
    float scoop = 0.004f;  // pitch scoop into the note (knob units: 0.004 = half a semitone)
    float breath = 0.03f;
};

void brass (B& b, const Brass& s)
{
    osc (b, 0, 0, kSaw, 0.8f);
    osc (b, 0, 1, s.wave2, s.osc2, 0.0f, s.detune);
    b.set ("e1_vol", 0.6f);
    if (s.players > 1)
        uni (b, 0, 0, s.players, 0.08f, 0.6f);
    filter (b, 0, kLP24, s.cutoff, s.reso, 0.4f);
    env (b, "env2", s.attack * 0.8f + 0.005f, s.decay, s.filterSustain, s.release);
    b.route ("Env 2", "f1_cutoff", s.envAmt * 0.5f).route ("Env 2", "f1_cutoff", s.envAmt * 0.5f, "Velocity", 1.0f);
    env (b, "env3", 0.0f, 0.07f, 0.0f, 0.05f);
    b.route ("Env 3", "e1_coarse", -s.scoop);
    if (s.breath > 0.0f)
        noise (b, 0, "Breath", s.breath, 0.3f);
    env (b, "env1", s.attack, s.decay, s.sustain, s.release);
    b.set ("amp_velo", 0.5f);
}

void brassMacros (B& b, const char* m3, const char* m3dest, float m3amt, const char* m4 = "Attack", const char* m4dest = "env1_a", float m4amt = 0.3f)
{
    macros (b, "Brightness", "Space", m3, m4);
    b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", m3dest, m3amt).route ("Macro 4", m4dest, m4amt);
}

// Solo horn: mono legato with a short slide, vibrato that fades in, more with aftertouch.
void soloHorn (B& b, float glide, float vibRate)
{
    mono (b, true, glide);
    lfo (b, 0, LfoSine, vibRate);
    b.set ("lfo1_fade", 0.4f).set ("lfo1_retrig", 2.0f);
    b.route ("LFO 1", "e1_fine", 0.005f).route ("LFO 1", "e1_fine", 0.01f, "Aftertouch", 1.0f);
}
} // namespace

const std::vector<FactoryPreset>& pack5Presets()
{
    static const std::vector<FactoryPreset> list {
        // ================================================================ R&B AND SOUL SYNTH BRASS
        { "RnB Brass Stab", "Strings/Brass", "Punchy, warm synth brass for R&B and soul stabs. Short chords, played with feeling.",
          [] (B& b) {
              Brass s;
              s.cutoff = 600.0f; s.envAmt = 0.4f; s.attack = 0.01f; s.decay = 0.35f; s.sustain = 0.6f; s.release = 0.2f; s.filterSustain = 0.35f;
              brass (b, s);
              b.set ("e1_drift", 0.2f);
              vintageChorus (b, 0, 1, 0.35f);
              b.set ("fx_a1_p6", 0.5f); // brass stays focused, mono-safe
              room (b, 0.2f);
              brassMacros (b, "Bite", "env2_d", -0.25f, "Release", "env1_r", 0.3f);
          } },
        { "Velvet RnB Brass", "Strings/Brass", "Soft, swelling synth brass for 90s R&B ballads. Lush under a vocal.",
          [] (B& b) {
              Brass s;
              s.players = 3; s.cutoff = 500.0f; s.envAmt = 0.3f; s.attack = 0.18f; s.decay = 0.8f; s.sustain = 0.9f; s.release = 0.6f;
              s.filterSustain = 0.7f; s.breath = 0.02f;
              brass (b, s);
              b.set ("e1_drift", 0.3f);
              vintageChorus (b, 0, 2, 0.45f);
              b.set ("fx_a1_p6", 0.5f); // brass stays focused, mono-safe
              hall (b, 0.3f, 0.75f, 0.65f);
              brassMacros (b, "Swell", "env2_a", 0.3f);
          } },
        { "Soul Brass Swell", "Strings/Brass", "Brass that swells up slowly while you hold the chord, then opens wide. For soulful builds.",
          [] (B& b) {
              Brass s;
              s.players = 3; s.cutoff = 400.0f; s.envAmt = 0.45f; s.attack = 0.6f; s.decay = 1.0f; s.sustain = 1.0f; s.release = 0.5f;
              s.filterSustain = 0.8f;
              brass (b, s);
              env (b, "env2", 1.2f, 1.0f, 0.85f, 0.5f);
              chorus (b, 0, 0.3f);
              hall (b, 0.32f, 0.75f, 0.65f);
              brassMacros (b, "Swell", "env2_a", 0.3f);
          } },
        { "Funk Brass Hits", "Strings/Brass", "Very short, bright brass hits for funk and gospel punches.",
          [] (B& b) {
              Brass s;
              s.cutoff = 900.0f; s.envAmt = 0.45f; s.attack = 0.003f; s.decay = 0.18f; s.sustain = 0.0f; s.release = 0.1f; s.filterSustain = 0.1f;
              s.scoop = 0.003f;
              brass (b, s);
              room (b, 0.18f);
              brassMacros (b, "Bite", "f1_reso", 0.3f, "Length", "env1_d", 0.35f);
          } },
        { "Bright Synth Brass", "Strings/Brass", "Bright, wide 80s synth brass with chorus. Bold stabs and pads alike.",
          [] (B& b) {
              Brass s;
              s.players = 3; s.cutoff = 1200.0f; s.envAmt = 0.35f; s.attack = 0.02f; s.decay = 0.5f; s.sustain = 0.8f; s.release = 0.3f;
              s.filterSustain = 0.6f; s.breath = 0.0f;
              brass (b, s);
              model (b, 0, kOta, 0, 1200.0f, 0.15f, 0.4f);
              chorus (b, 0, 0.4f);
              room (b, 0.2f);
              brassMacros (b, "Bite", "env2_d", -0.25f);
          } },
        { "Octave Synth Brass", "Strings/Brass", "Synth brass with an octave below: one hand sounds like a full horn line.",
          [] (B& b) {
              Brass s;
              s.players = 2; s.cutoff = 800.0f; s.envAmt = 0.38f; s.attack = 0.02f;
              brass (b, s);
              osc (b, 0, 2, kSaw, 0.45f, -12.0f);
              vintageChorus (b, 0, 0, 0.35f);
              b.set ("fx_a1_p6", 0.5f); // brass stays focused, mono-safe
              room (b, 0.2f);
              brassMacros (b, "Octave", "e1_o3_vol", 0.3f);
          } },
        { "Gospel Brass", "Strings/Brass", "Big, bright brass with a strong scoop and a touch of grit. For gospel shouts.",
          [] (B& b) {
              Brass s;
              s.players = 3; s.cutoff = 900.0f; s.envAmt = 0.45f; s.attack = 0.015f; s.scoop = 0.008f;
              brass (b, s);
              b.set ("f1_drive", 0.3f);
              room (b, 0.22f);
              brassMacros (b, "Scoop", "env3_d", 0.3f);
          } },
        // ================================================================ SHARP BRASS
        { "Sharp Brass Stab", "Strings/Brass", "Very bright, cutting brass stab with a resonant 'blat'. Cuts through a loud band.",
          [] (B& b) {
              Brass s;
              s.cutoff = 1000.0f; s.reso = 0.3f; s.envAmt = 0.5f; s.attack = 0.003f; s.decay = 0.25f; s.sustain = 0.55f; s.release = 0.15f;
              s.filterSustain = 0.3f;
              brass (b, s);
              room (b, 0.18f);
              brassMacros (b, "Blat", "f1_reso", 0.3f, "Release", "env1_r", 0.3f);
          } },
        { "Brass Fall", "Strings/Brass", "Brass that falls off in pitch when you let go of the note. The big-band 'fall'.",
          [] (B& b) {
              Brass s;
              s.cutoff = 900.0f; s.envAmt = 0.4f; s.attack = 0.01f; s.release = 0.45f;
              s.scoop = 0.0f; // Env 3 is used for the fall instead
              brass (b, s);
              // Pitch sits 5 semitones low; Env 3 lifts it back while the key is held, so on release it falls.
              b.set ("e1_coarse", -5.0f);
              env (b, "env3", 0.0f, 0.07f, 1.0f, 0.4f);
              b.route ("Env 3", "e1_coarse", 5.0f / 120.0f);
              room (b, 0.22f);
              brassMacros (b, "Fall", "env3_r", 0.3f, "Release", "env1_r", 0.3f);
          } },
        { "Brass Sfz Swell", "Strings/Brass", "Hit hard, drop back, then swell up again: the dramatic 'sforzando-crescendo'.",
          [] (B& b) {
              Brass s;
              s.players = 3; s.cutoff = 700.0f; s.envAmt = 0.45f; s.attack = 0.005f; s.decay = 0.2f; s.sustain = 0.3f; s.release = 0.4f;
              s.filterSustain = 0.2f;
              s.scoop = 0.0f; // Env 3 is used for the swell instead
              brass (b, s);
              env (b, "env3", 1.8f, 1.0f, 1.0f, 0.4f);
              b.route ("Env 3", "f1_cutoff", 0.25f).route ("Env 3", "e1_vol", 0.35f);
              b.set ("e1_vol", 0.4f);
              hall (b, 0.3f, 0.8f, 0.7f);
              brassMacros (b, "Swell Time", "env3_a", 0.3f, "Release", "env1_r", 0.3f);
          } },
        { "Staccato Brass", "Strings/Brass", "Tight, short brass section notes for rhythmic figures.",
          [] (B& b) {
              Brass s;
              s.players = 3; s.cutoff = 800.0f; s.envAmt = 0.4f; s.attack = 0.004f; s.decay = 0.22f; s.sustain = 0.0f; s.release = 0.12f;
              s.filterSustain = 0.0f;
              brass (b, s);
              hall (b, 0.25f, 0.7f, 0.6f);
              brassMacros (b, "Bite", "f1_reso", 0.3f, "Length", "env1_d", 0.35f);
          } },
        { "Brass Blast", "Strings/Brass", "A huge, slightly overdriven brass section. Final chords and big endings.",
          [] (B& b) {
              Brass s;
              s.players = 4; s.cutoff = 1100.0f; s.envAmt = 0.4f; s.attack = 0.01f; s.scoop = 0.006f;
              brass (b, s);
              osc (b, 0, 2, kSaw, 0.4f, -12.0f);
              b.fx (0, FxDistortion, { { 0, (2 + 0.5f) / 16.0f }, { 1, 0.2f }, { 10, 0.35f } });
              hall (b, 0.3f, 0.8f, 0.7f);
              brassMacros (b, "Drive", "fx_a1_p2", 0.3f);
          } },
        // ================================================================ HORNS
        { "Trumpet Section", "Strings/Brass", "A bright trumpet section with a scoop and breath. Lines and stabs.",
          [] (B& b) {
              Brass s;
              s.players = 3; s.cutoff = 1100.0f; s.envAmt = 0.45f; s.attack = 0.02f; s.scoop = 0.005f; s.breath = 0.04f;
              brass (b, s);
              b.set ("f2_on", 1.0f).set ("f2_type", 0.0f).set ("f2_mode", 8.0f).set ("f2_cutoff", 300.0f).set ("f_routing", 0.0f);
              room (b, 0.25f);
              brassMacros (b, "Scoop", "env3_d", 0.3f);
          } },
        { "Trombone Section", "Strings/Brass", "Warm, buzzy trombones. Play in the low-mid range.",
          [] (B& b) {
              Brass s;
              s.wave2 = kSquare; s.osc2 = 0.45f; s.players = 3; s.cutoff = 500.0f; s.envAmt = 0.4f; s.attack = 0.04f; s.scoop = 0.006f;
              s.breath = 0.03f;
              brass (b, s);
              room (b, 0.25f);
              brassMacros (b, "Buzz", "f1_drive", 0.4f);
          } },
        { "Horn Section", "Strings/Brass", "Trumpet, sax and trombone together: the soul horn section in one patch.",
          [] (B& b) {
              Brass s;
              s.wave2 = kSquare; s.osc2 = 0.35f; s.players = 2; s.cutoff = 900.0f; s.envAmt = 0.42f; s.attack = 0.015f; s.scoop = 0.005f;
              brass (b, s);
              osc (b, 0, 2, kSaw, 0.35f, -12.0f);
              b.set ("e1_o3_fine", -0.06f);
              room (b, 0.22f);
              brassMacros (b, "Low Horns", "e1_o3_vol", 0.3f);
          } },
        { "French Horns", "Strings/Brass", "Round, noble French horns with a soft attack. Film and hymn moments.",
          [] (B& b) {
              Brass s;
              s.wave2 = kTri; s.osc2 = 0.7f; s.players = 3; s.cutoff = 450.0f; s.reso = 0.05f; s.envAmt = 0.3f; s.attack = 0.08f;
              s.decay = 0.6f; s.sustain = 0.9f; s.release = 0.5f; s.filterSustain = 0.6f; s.scoop = 0.003f; s.breath = 0.02f;
              brass (b, s);
              hall (b, 0.38f, 0.85f, 0.75f);
              brassMacros (b, "Breath", "u_n1_vol", 0.1f);
          } },
        { "French Horns Soft", "Strings/Brass", "Very soft, distant French horns. A warm bed under worship.",
          [] (B& b) {
              Brass s;
              s.wave2 = kTri; s.osc2 = 0.8f; s.players = 3; s.cutoff = 350.0f; s.reso = 0.0f; s.envAmt = 0.22f; s.attack = 0.25f;
              s.decay = 1.0f; s.sustain = 1.0f; s.release = 0.9f; s.filterSustain = 0.7f; s.scoop = 0.0f; s.breath = 0.02f;
              brass (b, s);
              b.set ("amp_velo", 0.35f);
              hall (b, 0.45f, 0.9f, 0.8f);
              brassMacros (b, "Breath", "u_n1_vol", 0.1f);
          } },
        { "Solo Trumpet", "Lead", "Expressive synth trumpet: scoops, legato slides, vibrato with aftertouch.",
          [] (B& b) {
              Brass s;
              s.players = 1; s.cutoff = 1000.0f; s.envAmt = 0.45f; s.attack = 0.025f; s.scoop = 0.006f; s.breath = 0.04f;
              brass (b, s);
              b.set ("f1_vol", 1.5f);
              soloHorn (b, 0.04f, 5.5f);
              room (b, 0.28f);
              brassMacros (b, "Scoop", "env3_d", 0.3f);
          } },
        { "Muted Trumpet", "Lead", "Nasal, intimate muted trumpet for late-night jazz phrases.",
          [] (B& b) {
              Brass s;
              s.players = 1; s.cutoff = 1400.0f; s.envAmt = 0.3f; s.attack = 0.03f; s.scoop = 0.004f; s.breath = 0.05f;
              brass (b, s);
              filter (b, 1, kBP12, 1200.0f, 0.25f, 0.5f);
              b.set ("f_routing", 0.0f).set ("f2_vol", 3.5f);
              soloHorn (b, 0.04f, 5.2f);
              hall (b, 0.3f, 0.7f, 0.6f);
              brassMacros (b, "Mute", "f2_reso", 0.3f);
          } },
        { "Flugelhorn", "Lead", "Warm, dark, breathy flugelhorn: a softer cousin of the trumpet for ballads.",
          [] (B& b) {
              Brass s;
              s.wave2 = kTri; s.osc2 = 0.6f; s.players = 1; s.cutoff = 600.0f; s.reso = 0.05f; s.envAmt = 0.3f; s.attack = 0.05f;
              s.release = 0.35f; s.scoop = 0.004f; s.breath = 0.06f;
              brass (b, s);
              b.set ("f1_vol", 1.6f);
              soloHorn (b, 0.05f, 5.0f);
              hall (b, 0.32f, 0.75f, 0.65f);
              brassMacros (b, "Breath", "u_n1_vol", 0.1f);
          } },
    };
    return list;
}

juce::StringArray pack5Tags (const juce::String& presetName)
{
    static const std::map<juce::String, const char*> tags {
        { "RnB Brass Stab", "Brass, Synth Brass, RnB, Stab" }, { "Velvet RnB Brass", "Brass, Synth Brass, RnB, Swell" },
        { "Soul Brass Swell", "Brass, Synth Brass, Soul, Swell" }, { "Funk Brass Hits", "Brass, Synth Brass, Funk, Stab, Short" },
        { "Bright Synth Brass", "Brass, Synth Brass, Polysynth" }, { "Octave Synth Brass", "Brass, Synth Brass" },
        { "Gospel Brass", "Brass, Synth Brass, Gospel, Stab" },
        { "Sharp Brass Stab", "Brass, Sharp, Stab" }, { "Brass Fall", "Brass, Sharp, Fall" },
        { "Brass Sfz Swell", "Brass, Sharp, Swell, Cinematic" }, { "Staccato Brass", "Brass, Sharp, Short" },
        { "Brass Blast", "Brass, Sharp, Big, Cinematic" },
        { "Trumpet Section", "Brass, Horns, Trumpet" }, { "Trombone Section", "Brass, Horns, Trombone" },
        { "Horn Section", "Brass, Horns, Soul" }, { "French Horns", "Brass, Horns, French Horn, Cinematic" },
        { "French Horns Soft", "Brass, Horns, French Horn, Worship" },
        { "Solo Trumpet", "Brass, Horns, Trumpet, Solo, Expressive" }, { "Muted Trumpet", "Brass, Horns, Trumpet, Solo" },
        { "Flugelhorn", "Brass, Horns, Solo, Expressive" },
    };
    const auto it = tags.find (presetName);
    return it == tags.end() ? juce::StringArray() : PresetManager::parseTags (it->second);
}
