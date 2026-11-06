// Preset Pack 4: Orchestral Strings.
//
// Section strings (full ensemble, violins, violas, cellos, basses), articulations
// (legato, marcato, staccato, pizzicato, tremolo, flautando), two expressive solo
// voices and a few worship / cinematic beds. All synthesised: these are synth
// strings with an orchestral approach, not a sampled orchestra.
//
// Macros: M1 Brightness, M2 Space, M3 Vibrato (or another character control),
// M4 Attack (or Length for short notes). Several presets use the mod wheel for
// dynamics, as orchestral libraries do: wheel down = soft and dark, up = loud and bright.

#include "Packs.h"

#include "plugin/PresetHelpers.h"

#include <map>

using namespace presethelpers;

namespace
{
constexpr int kFormant = 3;

// A string section on engine `e`: the String Ensemble wavetable, `players` unison
// voices slightly out of tune with each other.
void section (B& b, int e, float pos, float coarse, int players, float detune, float vol)
{
    wavetable (b, e, "String Ensemble", pos, vol);
    b.set ("e" + juce::String (e + 1) + "_coarse", coarse);
    if (players > 1)
        uni (b, e, 0, players, detune, 0.7f);
}

// Violins on Engine 1, cellos an octave down on Engine 2, a quiet bass saw two
// octaves down on the Utility oscillator: one key = the whole section.
void fullSection (B& b, float violins = 0.55f, float cellos = 0.4f, float basses = 0.12f)
{
    section (b, 0, 0.5f, 0.0f, 4, 0.12f, violins);
    section (b, 1, 0.4f, -12.0f, 3, 0.1f, cellos);
    if (basses > 0.0f)
        b.set ("u_osc_on", 1.0f).set ("u_osc_wave", 2.0f).set ("u_osc_coarse", -24.0f).set ("u_osc_vol", basses);
}

// Body resonance: a Formant filter after the lowpass, blended in partly, gives the
// wooden "vowel" of a string instrument. `vowel` darker towards 1.
void body (B& b, float vowel = 0.45f, float blend = 0.3f)
{
    b.set ("f2_on", 1.0f).set ("f2_type", (float) kFormant).set ("f2_cutoff", 1000.0f).set ("f2_reso", 0.3f)
        .set ("f2_a", vowel).set ("f2_b", blend);
    b.set ("f_routing", 0.0f);
}

// Vibrato that fades in after the bow starts (LFO 1 on both engines' pitch), more
// with Macro 3.
void vibrato (B& b, float depth, float fade = 0.8f, float rate = 5.3f)
{
    lfo (b, 0, LfoSine, rate);
    b.set ("lfo1_fade", fade);
    b.route ("LFO 1", "e1_fine", depth).route ("LFO 1", "e2_fine", depth);
    b.route ("LFO 1", "e1_fine", 0.008f, "Macro 3", 1.0f).route ("LFO 1", "e2_fine", 0.008f, "Macro 3", 1.0f);
}

// Bow noise: a little breathy noise, high-passed, for the rosin on the strings.
void bowAir (B& b, float level) { noise (b, 0, "Breath", level, 0.5f); }

// Bowed envelope; harder notes start the bow faster.
void bowed (B& b, float attack, float release)
{
    env (b, "env1", attack, 1.0f, 1.0f, release);
    b.route ("Velocity", "env1_a", -0.15f);
    b.set ("amp_velo", 0.4f);
}

void orchHall (B& b, float send) { hall (b, send, 0.85f, 0.75f); }

void orchMacros (B& b, const char* m3, const char* m4, const char* m4dest, float m4amt)
{
    macros (b, "Brightness", "Space", m3, m4);
    b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 4", m4dest, m4amt);
}

// Mod wheel = dynamics: louder and brighter as it goes up. At zero the section
// is soft and dark but still audible.
void wheelDynamics (B& b, float e1Base, float e2Base)
{
    b.set ("e1_vol", e1Base).set ("e2_vol", e2Base);
    b.route ("Mod Wheel", "e1_vol", 0.35f).route ("Mod Wheel", "e2_vol", 0.25f).route ("Mod Wheel", "f1_cutoff", 0.25f);
}
} // namespace

const std::vector<FactoryPreset>& pack4Presets()
{
    static const std::vector<FactoryPreset> list {
        // ================================================================ FULL SECTION
        { "Full Strings", "Strings/Brass", "The whole string section from one key: violins, cellos an octave down and basses below.",
          [] (B& b) {
              fullSection (b);
              filter (b, 0, kLP12, 3500.0f, 0.05f, 0.3f);
              body (b);
              vibrato (b, 0.005f);
              bowAir (b, 0.03f);
              bowed (b, 0.35f, 0.9f);
              orchHall (b, 0.4f);
              orchMacros (b, "Vibrato", "Attack", "env1_a", 0.35f);
          } },
        { "Full Strings Marcato", "Strings/Brass", "Full section with an accented, biting start on every note. For strong rhythmic chords.",
          [] (B& b) {
              fullSection (b);
              filter (b, 0, kLP12, 3000.0f, 0.1f, 0.3f);
              env (b, "env2", 0.005f, 0.3f, 0.0f, 0.3f);
              b.route ("Env 2", "f1_cutoff", 0.25f, "Velocity", 1.0f);
              body (b);
              vibrato (b, 0.004f);
              bowAir (b, 0.04f);
              env (b, "env1", 0.03f, 0.5f, 0.65f, 0.6f);
              b.set ("amp_velo", 0.5f);
              orchHall (b, 0.38f);
              orchMacros (b, "Vibrato", "Accent", "env1_s", -0.3f);
          } },
        { "Full Strings Dynamic", "Strings/Brass", "Full section played with the mod wheel: down = soft and dark, up = loud and bright.",
          [] (B& b) {
              fullSection (b);
              wheelDynamics (b, 0.25f, 0.18f);
              filter (b, 0, kLP12, 1800.0f, 0.05f, 0.3f);
              body (b);
              vibrato (b, 0.005f);
              bowAir (b, 0.03f);
              bowed (b, 0.3f, 0.9f);
              orchHall (b, 0.4f);
              orchMacros (b, "Vibrato", "Attack", "env1_a", 0.35f);
          } },
        // ================================================================ SECTIONS
        { "Violins Ensemble", "Strings/Brass", "A big violin section: bright, airy, with a vibrato that blooms on long notes.",
          [] (B& b) {
              section (b, 0, 0.6f, 0.0f, 6, 0.14f, 0.55f);
              engineOn (b, 1, 0.2f);
              osc (b, 1, 0, kSaw, 0.8f, 0.0f);
              b.set ("e2_fine", 0.08f);
              filter (b, 0, kLP12, 5000.0f, 0.05f, 0.3f);
              filter (b, 1, kHP12, 250.0f, 0.0f);
              b.set ("f_routing", 0.0f);
              vibrato (b, 0.007f);
              bowAir (b, 0.03f);
              bowed (b, 0.3f, 0.8f);
              orchHall (b, 0.4f);
              orchMacros (b, "Vibrato", "Attack", "env1_a", 0.35f);
          } },
        { "Violins Con Sordino", "Strings/Brass", "Muted violins: soft, veiled and breathy. Beautiful under quiet moments.",
          [] (B& b) {
              section (b, 0, 0.3f, 0.0f, 5, 0.12f, 0.6f);
              filter (b, 0, kLP24, 1600.0f, 0.05f, 0.3f);
              filter (b, 1, kHP12, 200.0f, 0.0f);
              b.set ("f_routing", 0.0f);
              vibrato (b, 0.004f);
              bowAir (b, 0.05f);
              bowed (b, 0.5f, 1.2f);
              orchHall (b, 0.45f);
              orchMacros (b, "Vibrato", "Attack", "env1_a", 0.35f);
          } },
        { "Violins Octaves", "Strings/Brass", "First and second violins an octave apart. Soaring melody lines from one finger.",
          [] (B& b) {
              section (b, 0, 0.55f, 0.0f, 4, 0.12f, 0.5f);
              section (b, 1, 0.6f, 12.0f, 4, 0.12f, 0.35f);
              filter (b, 0, kLP12, 5000.0f, 0.05f, 0.3f);
              filter (b, 1, kHP12, 200.0f, 0.0f);
              b.set ("f_routing", 0.0f);
              vibrato (b, 0.007f);
              bowAir (b, 0.03f);
              bowed (b, 0.3f, 0.9f);
              orchHall (b, 0.4f);
              orchMacros (b, "Vibrato", "Attack", "env1_a", 0.35f);
          } },
        { "Violas Section", "Strings/Brass", "Violas: darker and warmer than violins. The inner voice of the section.",
          [] (B& b) {
              section (b, 0, 0.45f, 0.0f, 4, 0.12f, 0.6f);
              filter (b, 0, kLP12, 2500.0f, 0.05f, 0.3f);
              body (b, 0.55f, 0.35f);
              vibrato (b, 0.005f);
              bowAir (b, 0.03f);
              bowed (b, 0.35f, 0.9f);
              orchHall (b, 0.4f);
              orchMacros (b, "Vibrato", "Attack", "env1_a", 0.35f);
          } },
        { "Cellos Section", "Strings/Brass", "A warm cello section with a bit of bite. Play it in the low-mid range.",
          [] (B& b) {
              section (b, 0, 0.35f, 0.0f, 4, 0.1f, 0.6f);
              engineOn (b, 1, 0.15f);
              osc (b, 1, 0, kSaw, 0.8f);
              b.set ("e2_fine", 0.06f);
              filter (b, 0, kLP12, 2000.0f, 0.05f, 0.4f);
              body (b, 0.6f, 0.35f);
              vibrato (b, 0.006f);
              bowAir (b, 0.03f);
              bowed (b, 0.3f, 0.9f);
              orchHall (b, 0.38f);
              orchMacros (b, "Vibrato", "Attack", "env1_a", 0.35f);
          } },
        { "Celli and Basses", "Strings/Brass", "Cellos with the basses an octave below: a deep, warm low end for the left hand.",
          [] (B& b) {
              section (b, 0, 0.35f, 0.0f, 4, 0.1f, 0.55f);
              section (b, 1, 0.3f, -12.0f, 3, 0.08f, 0.4f);
              filter (b, 0, kLP24, 1200.0f, 0.05f, 0.4f);
              body (b, 0.65f, 0.3f);
              vibrato (b, 0.004f);
              bowed (b, 0.4f, 1.0f);
              orchHall (b, 0.35f);
              orchMacros (b, "Vibrato", "Attack", "env1_a", 0.35f);
          } },
        // ================================================================ ARTICULATIONS
        { "Basses Pizzicato", "Bass", "Plucked double basses: round and woody. A walking or sustained bass line under strings.",
          [] (B& b) {
              section (b, 0, 0.3f, 0.0f, 2, 0.06f, 0.8f);
              mono (b, false, 0.0f);
              noise (b, 0, "Pluck Noise", 0.15f, 0.2f, false);
              filter (b, 0, kLP12, 900.0f, 0.05f, 0.4f);
              b.set ("f1_vol", 1.8f);
              env (b, "env2", 0.001f, 0.15f, 0.0f, 0.1f);
              b.route ("Env 2", "f1_cutoff", 0.2f, "Velocity", 1.0f);
              env (b, "env1", 0.001f, 0.7f, 0.0f, 0.3f);
              b.set ("amp_velo", 0.5f);
              room (b, 0.3f);
              orchMacros (b, "Thumb", "Length", "env1_d", 0.35f);
              b.route ("Macro 3", "u_n1_vol", 0.3f);
          } },
        { "Pizzicato Section", "Pluck", "The whole section plucking: violins with cellos underneath. Light and playful.",
          [] (B& b) {
              section (b, 0, 0.5f, 0.0f, 4, 0.1f, 0.6f);
              section (b, 1, 0.4f, -12.0f, 2, 0.08f, 0.3f);
              noise (b, 0, "Pluck Noise", 0.12f, 0.3f, false);
              filter (b, 0, kLP12, 2500.0f, 0.05f, 0.4f);
              env (b, "env2", 0.001f, 0.1f, 0.0f, 0.1f);
              b.route ("Env 2", "f1_cutoff", 0.3f, "Velocity", 1.0f);
              env (b, "env1", 0.001f, 0.45f, 0.0f, 0.35f);
              b.set ("amp_velo", 0.5f);
              orchHall (b, 0.32f);
              orchMacros (b, "Snap", "Length", "env1_d", 0.35f);
              b.route ("Macro 3", "u_n1_vol", 0.3f);
          } },
        { "Staccato Section", "Strings/Brass", "Short, bouncing full-section notes for rhythmic and cinematic parts.",
          [] (B& b) {
              fullSection (b, 0.55f, 0.4f, 0.08f);
              noise (b, 0, "Pluck Noise", 0.05f, 0.4f, false);
              filter (b, 0, kLP12, 3000.0f, 0.1f, 0.3f);
              env (b, "env2", 0.003f, 0.12f, 0.0f, 0.1f);
              b.route ("Env 2", "f1_cutoff", 0.2f, "Velocity", 1.0f);
              body (b);
              env (b, "env1", 0.005f, 0.25f, 0.0f, 0.2f);
              b.set ("amp_velo", 0.55f);
              orchHall (b, 0.35f);
              orchMacros (b, "Bite", "Length", "env1_d", 0.35f);
              b.route ("Macro 3", "u_n1_vol", 0.2f);
          } },
        { "Tremolo Build", "Strings/Brass", "Bowed tremolo that grows from nothing to full over a few seconds. Tension for builds.",
          [] (B& b) {
              fullSection (b, 0.55f, 0.4f, 0.08f);
              filter (b, 0, kLP12, 1500.0f, 0.1f, 0.3f);
              env (b, "env2", 2.5f, 1.0f, 1.0f, 0.8f);
              b.route ("Env 2", "f1_cutoff", 0.25f);
              body (b);
              lfo (b, 1, LfoTriangle, 11.0f);
              b.route ("LFO 2", "e1_vol", -0.2f).route ("LFO 2", "e2_vol", -0.15f);
              env (b, "env1", 2.5f, 1.0f, 1.0f, 0.8f);
              b.set ("amp_velo", 0.3f);
              orchHall (b, 0.4f);
              orchMacros (b, "Speed", "Build Time", "env1_a", 0.35f);
              b.route ("Macro 3", "lfo2_rate", 0.2f).route ("Macro 4", "env2_a", 0.35f);
          } },
        { "Flautando Strings", "Strings/Brass", "Strings bowed lightly over the fingerboard: soft, flute-like, almost no vibrato.",
          [] (B& b) {
              section (b, 0, 0.15f, 0.0f, 4, 0.1f, 0.55f);
              engineOn (b, 1, 0.12f);
              osc (b, 1, 0, kSine, 0.8f, 12.0f);
              filter (b, 0, kLP24, 1400.0f, 0.0f, 0.3f);
              filter (b, 1, kHP12, 180.0f, 0.0f);
              b.set ("f_routing", 0.0f);
              vibrato (b, 0.002f, 1.5f);
              bowAir (b, 0.08f);
              bowed (b, 0.6f, 1.5f);
              orchHall (b, 0.45f);
              orchMacros (b, "Vibrato", "Attack", "env1_a", 0.35f);
          } },
        { "String Ostinato", "Sequence", "Short strings repeating the held chord in eighths, with accents. Hold a chord; follows the tempo.",
          [] (B& b) {
              fullSection (b, 0.55f, 0.4f, 0.0f);
              filter (b, 0, kLP12, 3000.0f, 0.1f, 0.3f);
              env (b, "env2", 0.003f, 0.12f, 0.0f, 0.1f);
              b.route ("Env 2", "f1_cutoff", 0.2f, "Velocity", 1.0f);
              body (b);
              env (b, "env1", 0.005f, 0.22f, 0.0f, 0.15f);
              b.set ("amp_velo", 0.6f).set ("f1_vol", 2.0f);
              b.set ("seq_on", 1.0f).set ("seq_mode", 0.0f).set ("seq_arp_order", (float) ArpUp).set ("seq_arp_octaves", 1.0f)
                  .set ("seq_rate", (float) kDiv8);
              b.pattern (gatePattern ({ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, 0.4f));
              orchHall (b, 0.35f);
              orchMacros (b, "Bite", "Length", "env1_d", 0.35f);
              b.route ("Macro 3", "env2_d", 0.2f);
          } },
        // ================================================================ SOLO
        { "Solo Violin", "Lead", "An expressive synth violin: legato with slides, vibrato that blooms, more with aftertouch.",
          [] (B& b) {
              osc (b, 0, 0, kSaw, 0.8f);
              osc (b, 0, 1, kSaw, 0.3f, 0.0f, 0.05f);
              b.set ("e1_vol", 0.7f).set ("e2_on", 0.0f);
              mono (b, true, 0.06f);
              filter (b, 0, kLP24, 3000.0f, 0.1f, 0.4f);
              b.route ("Velocity", "f1_cutoff", 0.15f);
              b.set ("f1_vol", 1.6f);
              body (b, 0.4f, 0.4f);
              lfo (b, 0, LfoSine, 5.6f);
              b.set ("lfo1_fade", 0.5f).set ("lfo1_retrig", 2.0f);
              b.route ("LFO 1", "e1_fine", 0.008f).route ("LFO 1", "e1_fine", 0.008f, "Aftertouch", 1.0f)
                  .route ("LFO 1", "e1_fine", 0.008f, "Macro 3", 1.0f);
              bowAir (b, 0.04f);
              env (b, "env1", 0.12f, 1.0f, 1.0f, 0.35f);
              b.set ("amp_velo", 0.4f);
              orchHall (b, 0.35f);
              orchMacros (b, "Vibrato", "Attack", "env1_a", 0.35f);
          } },
        { "Solo Cello", "Lead", "An expressive synth cello: warm, legato with slides, aftertouch adds vibrato.",
          [] (B& b) {
              osc (b, 0, 0, kSaw, 0.8f);
              osc (b, 0, 1, kTri, 0.3f, 0.0f, 0.04f);
              b.set ("e1_vol", 0.75f).set ("e2_on", 0.0f);
              mono (b, true, 0.08f);
              filter (b, 0, kLP24, 1300.0f, 0.1f, 0.4f);
              b.route ("Velocity", "f1_cutoff", 0.15f);
              b.set ("f1_vol", 1.6f);
              body (b, 0.65f, 0.4f);
              lfo (b, 0, LfoSine, 5.0f);
              b.set ("lfo1_fade", 0.6f).set ("lfo1_retrig", 2.0f);
              b.route ("LFO 1", "e1_fine", 0.008f).route ("LFO 1", "e1_fine", 0.008f, "Aftertouch", 1.0f)
                  .route ("LFO 1", "e1_fine", 0.008f, "Macro 3", 1.0f);
              bowAir (b, 0.04f);
              env (b, "env1", 0.15f, 1.0f, 1.0f, 0.4f);
              b.set ("amp_velo", 0.4f);
              orchHall (b, 0.35f);
              orchMacros (b, "Vibrato", "Attack", "env1_a", 0.35f);
          } },
        // ================================================================ BEDS
        { "Chamber Quartet", "Strings/Brass", "A small group instead of a section: two players a part, close and intimate.",
          [] (B& b) {
              section (b, 0, 0.5f, 0.0f, 2, 0.06f, 0.6f);
              section (b, 1, 0.4f, -12.0f, 2, 0.06f, 0.35f);
              filter (b, 0, kLP12, 3000.0f, 0.05f, 0.3f);
              body (b, 0.5f, 0.4f);
              vibrato (b, 0.006f, 0.6f);
              bowAir (b, 0.04f);
              bowed (b, 0.25f, 0.7f);
              hall (b, 0.3f, 0.6f, 0.55f);
              orchMacros (b, "Vibrato", "Attack", "env1_a", 0.35f);
          } },
        { "Hymn Strings", "Strings/Brass", "Gentle, warm full strings with a slow bow and a long tail. Made for hymns and worship.",
          [] (B& b) {
              fullSection (b, 0.5f, 0.4f, 0.1f);
              filter (b, 0, kLP12, 2200.0f, 0.05f, 0.3f);
              body (b, 0.5f, 0.3f);
              vibrato (b, 0.004f);
              bowAir (b, 0.03f);
              bowed (b, 0.6f, 1.5f);
              orchHall (b, 0.45f);
              orchMacros (b, "Warmth", "Attack", "env1_a", 0.35f);
              b.route ("Macro 3", "e2_vol", 0.25f);
          } },
        { "Epic Low Strings", "Strings/Brass", "Low strings in octaves with a sub, opening slowly. Big, dark, cinematic.",
          [] (B& b) {
              section (b, 0, 0.4f, -12.0f, 4, 0.1f, 0.55f);
              engineOn (b, 1, 0.3f);
              osc (b, 1, 0, kSaw, 0.8f, -24.0f);
              b.set ("u_osc_on", 1.0f).set ("u_osc_wave", 0.0f).set ("u_osc_coarse", -24.0f).set ("u_osc_vol", 0.2f);
              filter (b, 0, kLP24, 900.0f, 0.1f, 0.3f);
              env (b, "env2", 2.0f, 2.0f, 0.8f, 1.5f);
              b.route ("Env 2", "f1_cutoff", 0.3f);
              body (b, 0.65f, 0.25f);
              vibrato (b, 0.004f);
              bowed (b, 0.5f, 1.5f);
              orchHall (b, 0.45f);
              orchMacros (b, "Sub", "Attack", "env1_a", 0.35f);
              b.route ("Macro 3", "u_osc_vol", 0.25f);
          } },
    };
    return list;
}

juce::StringArray pack4Tags (const juce::String& presetName)
{
    static const std::map<juce::String, const char*> tags {
        { "Full Strings", "Strings, Orchestral, Full Section" }, { "Full Strings Marcato", "Strings, Orchestral, Full Section" },
        { "Full Strings Dynamic", "Strings, Orchestral, Full Section, Expressive" },
        { "Violins Ensemble", "Strings, Orchestral, Violins" }, { "Violins Con Sordino", "Strings, Orchestral, Violins, Worship" },
        { "Violins Octaves", "Strings, Orchestral, Violins, Cinematic" }, { "Violas Section", "Strings, Orchestral, Violas" },
        { "Cellos Section", "Strings, Orchestral, Cellos" }, { "Celli and Basses", "Strings, Orchestral, Cellos, Basses" },
        { "Basses Pizzicato", "Strings, Orchestral, Basses, Pizzicato, Bass" },
        { "Pizzicato Section", "Strings, Orchestral, Pizzicato, Short" }, { "Staccato Section", "Strings, Orchestral, Short, Cinematic" },
        { "Tremolo Build", "Strings, Orchestral, Tremolo, Cinematic, Swell" }, { "Flautando Strings", "Strings, Orchestral, Worship" },
        { "String Ostinato", "Strings, Orchestral, Short, Arp, Cinematic" },
        { "Solo Violin", "Strings, Solo, Violins, Expressive" }, { "Solo Cello", "Strings, Solo, Cellos, Expressive" },
        { "Chamber Quartet", "Strings, Orchestral, Worship" }, { "Hymn Strings", "Strings, Orchestral, Worship" },
        { "Epic Low Strings", "Strings, Orchestral, Cinematic, Cellos, Basses" },
    };
    const auto it = tags.find (presetName);
    return it == tags.end() ? juce::StringArray() : PresetManager::parseTags (it->second);
}
