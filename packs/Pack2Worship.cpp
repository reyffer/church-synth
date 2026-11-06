// Preset Pack 2: Worship Elements.
//
// Pads, drones, swells, bells and rhythmic beds for worship sets. Same rules as
// Pack 1: built in code, exported and level-matched by `cs_state_test
// --export-pack <dir>`; install by copying the folder into Documents/Church Synth/Presets.
//
// Macro convention: M1 Brightness / Tone, M2 Space, M3 a per-sound character
// control, M4 Swell / Attack / Decay / Length. Most sounds are kept light on CPU
// (unison 3-4) because they're meant to run several at once in a live rig.

#include "Packs.h"

#include "plugin/PresetHelpers.h"

#include <map>

using namespace presethelpers;

namespace
{
constexpr int kFormant = 3, kComb = 6, kOta = 11;
constexpr int kDiv4Bars = 1, kDiv2Bars = 2, kDiv1Bar = 3;

// A slow free-running LFO on `dest`: a little always, more with Macro 3 ("Motion").
void motion (B& b, int lfoIndex, int wave, float hz, const char* dest, float base, float withMacro)
{
    lfo (b, lfoIndex, wave, hz);
    const juce::String src = "LFO " + juce::String (lfoIndex + 1);
    b.set ("lfo" + juce::String (lfoIndex + 1) + "_retrig", 3.0f);
    b.route (src, dest, base);
    b.route (src, dest, withMacro, "Macro 3", 1.0f);
}

// Rhythmic amp shape from Function 1, synced to the host tempo.
void ampFunction (B& b, int shape, int division, float depth, float smooth = 0.0f)
{
    b.set ("func1_shape", (float) shape).set ("func1_sync", 1.0f).set ("func1_div", (float) division)
        .set ("func1_retrig", 3.0f).set ("func1_smooth", smooth);
    b.set ("amp_mod_src", 11.0f).set ("amp_velo", depth); // Function 1
}

// One-shot rise (or fall) over `division`, started by each key.
void oneShot (B& b, int shape, int division)
{
    b.set ("func1_shape", (float) shape).set ("func1_sync", 1.0f).set ("func1_div", (float) division)
        .set ("func1_mode", 1.0f).set ("func1_retrig", 0.0f);
}

void arp (B& b, int order, int octaves, int rate, float gate)
{
    b.set ("seq_on", 1.0f).set ("seq_mode", 0.0f).set ("seq_arp_order", (float) order)
        .set ("seq_arp_octaves", (float) octaves).set ("seq_rate", (float) rate);
    b.pattern (gatePattern ({ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, gate));
}

// Vintage chorus with the width held at 70 % so pads stay solid when the PA sums to mono.
void vChorus (B& b, int slot, int mode, float mix)
{
    vintageChorus (b, slot, mode, mix);
    b.set (juce::String ("fx_") + kSlot[slot] + "_p6", 0.7f);
}

void tapeEcho (B& b, int slot, int division, float intensity, float wow, float mix)
{
    b.fx (slot, FxTapeEcho, { { 0, syncKnob (division) }, { 2, intensity }, { 4, wow }, { 7, mix } });
    b.set (juce::String ("fx_") + kSlot[slot] + "_sync", 1.0f);
}
} // namespace

const std::vector<FactoryPreset>& pack2Presets()
{
    static const std::vector<FactoryPreset> list {
        // ================================================================ FOUNDATION PADS
        { "Sanctuary Pad", "Pad", "The everyday worship pad: warm, wide and soft, sits under any band. Macro 3 adds slow movement.",
          [] (B& b) {
              wavetable (b, 0, "Warm Pad", 0.4f, 0.55f);
              uni (b, 0, 0, 4, 0.14f, 0.7f);
              engineOn (b, 1, 0.3f);
              osc (b, 1, 0, kSaw, 0.7f);
              b.set ("e2_fine", 0.06f);
              filter (b, 0, kLP24, 1800.0f, 0.1f, 0.3f);
              env (b, "env1", 1.2f, 1.0f, 1.0f, 3.0f);
              motion (b, 0, LfoSine, 0.09f, "e1_wt_pos", 0.06f, 0.25f);
              chorus (b, 0, 0.3f);
              hall (b, 0.4f, 0.8f, 0.7f);
              padMacros (b, 0.3f);
          } },
        { "Warm Blanket", "Pad", "Very dark, round pad with a sub underneath. For quiet prayer and spoken moments.",
          [] (B& b) {
              osc (b, 0, 0, kTri, 0.8f);
              osc (b, 0, 1, kSaw, 0.25f, 0.0f, 0.06f);
              osc (b, 0, 2, kSine, 0.4f, -12.0f);
              b.set ("e1_vol", 0.6f);
              uni (b, 0, 0, 3, 0.1f, 0.6f);
              filter (b, 0, kLP24, 650.0f, 0.05f, 0.35f);
              b.set ("f1_analog", 1.0f);
              env (b, "env1", 1.5f, 1.0f, 1.0f, 3.5f);
              hall (b, 0.35f, 0.75f, 0.65f);
              padMacros (b, 0.35f);
              b.macroName (2, "Sub");
              b.route ("Macro 3", "e1_o3_vol", 0.4f);
          } },
        { "Morning Light", "Pad", "Bright, airy pad with no low end and a shimmer tail. Floats above the band.",
          [] (B& b) {
              wavetable (b, 0, "Airy Pad", 0.5f, 0.5f);
              uni (b, 0, 0, 4, 0.12f, 0.8f);
              wavetable (b, 1, "Glass Pad", 0.4f, 0.0f);
              b.set ("e2_coarse", 12.0f).set ("e2_vol", 0.25f);
              filter (b, 0, kLP12, 7000.0f, 0.05f, 0.2f);
              filter (b, 1, kHP12, 250.0f, 0.0f);
              b.set ("f_routing", 0.0f);
              env (b, "env1", 1.0f, 1.0f, 1.0f, 3.0f);
              shimmer (b, 0.45f, 0.5f, 0.8f);
              padMacros (b, 0.3f);
              b.macroName (2, "Air");
              b.route ("Macro 3", "e2_vol", 0.3f);
          } },
        { "Open Fifths", "Pad", "Every key plays root, fifth and octave. No third, so one finger fits major and minor songs.",
          [] (B& b) {
              osc (b, 0, 0, kSaw, 0.8f);
              osc (b, 0, 1, kSaw, 0.6f, 0.0f, 0.08f);
              b.set ("e1_vol", 0.55f);
              uni (b, 0, 1, 3, 0.0f, 0.6f);
              b.set ("e1_uni_chord", 2.0f); // Fifth + Oct
              filter (b, 0, kLP24, 1400.0f, 0.1f, 0.3f);
              env (b, "env1", 1.2f, 1.0f, 1.0f, 3.0f);
              motion (b, 0, LfoTriangle, 0.12f, "f1_cutoff", 0.04f, 0.12f);
              vChorus (b, 0, 1, 0.4f);
              hall (b, 0.4f, 0.8f, 0.75f);
              padMacros (b, 0.3f);
          } },
        { "Octave Air", "Pad", "Thin, high octave pad with a sparkle of air. Stays out of the singers' way.",
          [] (B& b) {
              osc (b, 0, 0, kSaw, 0.7f);
              osc (b, 0, 1, kSquare, 0.3f, 0.0f, 0.05f, 0.3f);
              b.set ("e1_vol", 0.55f).set ("e1_coarse", 12.0f);
              uni (b, 0, 1, 2, 0.0f, 0.7f);
              b.set ("e1_uni_chord", 0.0f); // Octave
              noise (b, 0, "Sparkle Air", 0.05f, 0.3f);
              filter (b, 0, kLP12, 5000.0f, 0.05f, 0.2f);
              filter (b, 1, kHP12, 400.0f, 0.0f);
              b.set ("f_routing", 0.0f);
              env (b, "env1", 0.8f, 1.0f, 1.0f, 2.5f);
              chorus (b, 0, 0.35f);
              shimmer (b, 0.4f, 0.45f, 0.8f);
              padMacros (b, 0.3f);
              b.macroName (2, "Sparkle");
              b.route ("Macro 3", "u_n1_vol", 0.2f);
          } },
        { "Analog Worship Pad", "Pad", "Two-oscillator analog pad with slow PWM and vintage chorus. Warm, a little wobbly, alive.",
          [] (B& b) {
              osc (b, 0, 0, kSaw, 0.7f);
              osc (b, 0, 1, kSquare, 0.55f, 0.0f, 0.1f, 0.4f);
              b.set ("e1_vol", 0.55f).set ("e1_drift", 0.3f);
              uni (b, 0, 0, 2, 0.1f, 0.6f);
              model (b, 0, kOta, 0, 1500.0f, 0.15f, 0.3f);
              env (b, "env2", 1.5f, 2.0f, 0.6f, 3.0f);
              b.route ("Env 2", "f1_cutoff", 0.2f);
              env (b, "env1", 0.9f, 1.0f, 1.0f, 2.8f);
              motion (b, 0, LfoTriangle, 0.3f, "e1_o2_width", 0.1f, 0.15f);
              vChorus (b, 0, 1, 0.45f);
              hall (b, 0.38f, 0.8f, 0.7f);
              padMacros (b, 0.3f);
          } },
        { "Choir Pad", "Pad", "Soft 'ah' choir pad with breath. Macro 3 moves the vowel from 'ah' to 'oo'.",
          [] (B& b) {
              wavetable (b, 0, "Choir Ah to Oo", 0.2f, 0.55f);
              uni (b, 0, 0, 3, 0.1f, 0.7f);
              wavetable (b, 1, "Breathy Vox", 0.4f, 0.25f);
              b.set ("e2_fine", 0.05f);
              noise (b, 0, "Breath", 0.05f, 0.2f);
              filter (b, 0, kLP12, 4000.0f, 0.05f, 0.3f);
              env (b, "env1", 1.0f, 1.0f, 1.0f, 3.0f);
              lfo (b, 0, LfoSine, 0.08f);
              b.set ("lfo1_retrig", 3.0f);
              b.route ("LFO 1", "e1_wt_pos", 0.12f);
              hall (b, 0.45f, 0.85f, 0.75f);
              macros (b, "Brightness", "Space", "Vowel", "Swell");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "e1_wt_pos", 0.6f).route ("Macro 4", "env1_a", 0.35f);
          } },
        { "Glass Worship Pad", "Pad", "Clear, glassy pad with a sine underneath and a soft echo. Beautiful with piano.",
          [] (B& b) {
              wavetable (b, 0, "Glass Pad", 0.4f, 0.55f);
              uni (b, 0, 0, 3, 0.12f, 0.7f);
              engineOn (b, 1, 0.25f);
              osc (b, 1, 0, kSine, 0.8f, -12.0f);
              filter (b, 0, kLP12, 6000.0f, 0.05f, 0.2f);
              env (b, "env1", 1.2f, 1.0f, 1.0f, 3.5f);
              motion (b, 0, LfoSine, 0.06f, "e1_wt_pos", 0.05f, 0.3f);
              delay (b, 0, kDiv4D, 0.3f, 0.15f, true);
              shimmer (b, 0.4f, 0.45f, 0.8f);
              padMacros (b, 0.3f);
          } },
        { "Felt Pad", "Pad", "Soft, intimate pad with tape wobble and a little hiss. Like an old recording of a pad.",
          [] (B& b) {
              osc (b, 0, 0, kTri, 0.8f);
              osc (b, 0, 1, kSine, 0.3f, 12.0f);
              b.set ("e1_vol", 0.65f);
              uni (b, 0, 0, 2, 0.06f, 0.5f);
              noise (b, 0, "Tape Hiss", 0.03f, 0.0f);
              filter (b, 0, kLP12, 1100.0f, 0.05f, 0.35f);
              env (b, "env1", 0.9f, 1.0f, 1.0f, 2.5f);
              tapeEcho (b, 0, kDiv4, 0.25f, 0.5f, 0.15f);
              hall (b, 0.35f, 0.7f, 0.65f);
              padMacros (b, 0.35f);
              b.macroName (2, "Hiss");
              b.route ("Macro 3", "u_n1_vol", 0.12f);
          } },
        { "Slow Rise Pad", "Pad", "Every chord rises slowly from nothing, dark to bright. For the moment before the chorus.",
          [] (B& b) {
              osc (b, 0, 0, kSaw, 0.8f);
              osc (b, 0, 1, kSaw, 0.6f, -12.0f, 0.05f);
              b.set ("e1_vol", 0.55f);
              uni (b, 0, 0, 4, 0.16f, 0.7f);
              filter (b, 0, kLP24, 400.0f, 0.15f, 0.3f);
              env (b, "env1", 2.5f, 1.0f, 1.0f, 3.5f);
              env (b, "env2", 3.0f, 2.0f, 0.9f, 3.5f);
              b.route ("Env 2", "f1_cutoff", 0.4f);
              chorus (b, 0, 0.3f);
              hall (b, 0.42f, 0.85f, 0.8f);
              padMacros (b, 0.3f);
              b.macroName (2, "Rise");
              b.route ("Macro 3", "env2_a", -0.3f);
          } },
        { "Worship Strings", "Strings/Brass", "Soft string-section pad blended with a warm pad. Slow bow, gentle vibrato on the mod wheel.",
          [] (B& b) {
              wavetable (b, 0, "String Ensemble", 0.5f, 0.55f);
              uni (b, 0, 0, 4, 0.12f, 0.7f);
              wavetable (b, 1, "Warm Pad", 0.4f, 0.3f);
              filter (b, 0, kLP12, 3000.0f, 0.05f, 0.3f);
              filter (b, 1, kHP12, 160.0f, 0.0f);
              b.set ("f_routing", 0.0f);
              b.route ("Velocity", "f1_cutoff", 0.1f);
              env (b, "env1", 0.7f, 1.0f, 1.0f, 2.5f);
              lfo (b, 0, LfoSine, 5.2f);
              b.route ("LFO 1", "e1_fine", 0.01f, "Mod Wheel", 1.0f);
              hall (b, 0.42f, 0.85f, 0.75f);
              macros (b, "Brightness", "Space", "Pad", "Attack");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "e2_vol", 0.3f).route ("Macro 4", "env1_a", 0.35f);
          } },
        // ================================================================ EVOLVING PADS
        { "Tidal Pad", "Pad", "Pad that slowly opens and closes like a tide, over about twenty seconds.",
          [] (B& b) {
              wavetable (b, 0, "Warm Saw Sweep", 0.3f, 0.55f);
              uni (b, 0, 0, 4, 0.15f, 0.7f);
              filter (b, 0, kLP24, 1600.0f, 0.2f, 0.3f);
              env (b, "env1", 1.2f, 1.0f, 1.0f, 3.5f);
              lfo (b, 0, LfoSine, 0.05f);
              b.set ("lfo1_retrig", 3.0f);
              b.route ("LFO 1", "e1_wt_pos", 0.35f).route ("LFO 1", "f1_cutoff", 0.1f);
              hall (b, 0.42f, 0.85f, 0.75f);
              padMacros (b, 0.3f);
              b.route ("Macro 3", "lfo1_rate", 0.3f);
          } },
        { "Heaven Wash", "Pad", "Huge supersaw wash with an airy octave and a deep shimmer. For the big worship moment.",
          [] (B& b) {
              osc (b, 0, 0, kSaw, 0.9f);
              uni (b, 0, 2, 7, 0.25f, 0.9f);
              b.set ("e1_uni_mix", 0.65f).set ("e1_vol", 0.5f);
              wavetable (b, 1, "Airy Pad", 0.5f, 0.2f);
              b.set ("e2_coarse", 12.0f);
              filter (b, 0, kLP24, 2400.0f, 0.1f, 0.2f);
              filter (b, 1, kHP12, 150.0f, 0.0f);
              b.set ("f_routing", 0.0f);
              env (b, "env1", 1.8f, 1.0f, 1.0f, 4.5f);
              chorus (b, 0, 0.25f);
              shimmer (b, 0.55f, 0.6f, 0.9f);
              padMacros (b, 0.3f);
              b.macroName (2, "Shimmer");
              b.route ("Macro 3", "fx_x1_p2", 0.4f);
          } },
        { "Distant Choir", "Pad", "A choir far away: saws through a vowel filter that slowly changes its 'mouth shape'.",
          [] (B& b) {
              osc (b, 0, 0, kSaw, 0.8f);
              osc (b, 0, 1, kSaw, 0.5f, 0.0f, 0.1f);
              b.set ("e1_vol", 0.55f);
              uni (b, 0, 0, 3, 0.12f, 0.7f);
              model (b, 0, kFormant, 0, 1000.0f, 0.5f, 0.0f);
              b.set ("f1_a", 0.3f).set ("f1_b", 1.0f);
              filter (b, 1, kLP12, 3500.0f, 0.0f, 0.2f);
              b.set ("f_routing", 0.0f);
              env (b, "env1", 1.4f, 1.0f, 1.0f, 3.5f);
              lfo (b, 0, LfoTriangle, 0.07f);
              b.set ("lfo1_retrig", 3.0f);
              b.route ("LFO 1", "f1_a", 0.3f);
              hall (b, 0.5f, 0.9f, 0.8f);
              macros (b, "Brightness", "Space", "Vowel", "Swell");
              b.route ("Macro 1", "f2_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "f1_a", 0.5f).route ("Macro 4", "env1_a", 0.35f);
          } },
        { "Shimmer Cloud", "Pad", "Glittering cloud of partials that never repeats, with a ping-pong echo and shimmer.",
          [] (B& b) {
              wavetable (b, 0, "Shimmer Evolve", 0.5f, 0.5f);
              uni (b, 0, 0, 4, 0.14f, 0.8f);
              wavetable (b, 1, "Fibonacci Partials", 0.3f, 0.2f);
              b.set ("e2_coarse", 12.0f);
              filter (b, 0, kLP12, 5000.0f, 0.05f, 0.2f);
              env (b, "env1", 1.5f, 1.0f, 1.0f, 4.0f);
              b.set ("rand1_type", 0.0f).set ("rand1_rate", 0.3f).set ("rand1_smooth", 1.0f).set ("rand1_retrig", 3.0f);
              b.route ("Random 1", "e1_wt_pos", 0.3f).route ("Random 1", "e2_wt_pos", -0.3f);
              delay (b, 0, kDiv4D, 0.4f, 0.25f, true);
              shimmer (b, 0.55f, 0.65f, 0.9f);
              padMacros (b, 0.3f);
              b.route ("Macro 3", "rand1_rate", 0.4f);
          } },
        { "Moving Fifths", "Pad", "Warm saw pad where a fifth above slowly fades in and out. Changes colour under a long chord.",
          [] (B& b) {
              osc (b, 0, 0, kSaw, 0.8f);
              b.set ("e1_vol", 0.5f);
              uni (b, 0, 0, 2, 0.1f, 0.6f);
              engineOn (b, 1, 0.08f);
              osc (b, 1, 0, kSaw, 0.8f);
              b.set ("e2_coarse", 7.0f);
              filter (b, 0, kLP24, 1300.0f, 0.1f, 0.3f);
              env (b, "env1", 1.2f, 1.0f, 1.0f, 3.0f);
              lfo (b, 0, LfoTriangle, 0.08f, false);
              b.set ("lfo1_retrig", 3.0f);
              b.route ("LFO 1", "e2_vol", 0.3f);
              vChorus (b, 0, 0, 0.45f);
              hall (b, 0.4f, 0.8f, 0.75f);
              padMacros (b, 0.3f);
              b.route ("Macro 3", "lfo1_rate", 0.35f);
          } },
        { "Tape Memory", "Pad", "Old-tape pad: gentle pitch wobble, hiss and dust. Nostalgic, lo-fi.",
          [] (B& b) {
              wavetable (b, 0, "Vintage Saw", 0.3f, 0.55f);
              uni (b, 0, 0, 3, 0.12f, 0.6f);
              noise (b, 0, "Tape Hiss", 0.04f, 0.0f);
              noise (b, 1, "Vinyl Dust", 0.03f, 0.0f);
              filter (b, 0, kLP12, 2000.0f, 0.05f, 0.3f);
              env (b, "env1", 0.9f, 1.0f, 1.0f, 3.0f);
              lfo (b, 0, LfoSine, 0.4f);
              b.set ("lfo1_retrig", 3.0f);
              b.route ("LFO 1", "e1_fine", 0.012f);
              tapeEcho (b, 0, kDiv4, 0.25f, 0.7f, 0.2f);
              hall (b, 0.35f, 0.75f, 0.65f);
              padMacros (b, 0.3f);
              b.macroName (2, "Age");
              b.route ("Macro 3", "u_n1_vol", 0.15f).route ("Macro 3", "u_n2_vol", 0.15f);
          } },
        { "Sparkle Pad", "Pad", "Warm pad with a glittering top of high noise, a ping-pong echo and shimmer.",
          [] (B& b) {
              wavetable (b, 0, "Warm Pad", 0.5f, 0.55f);
              uni (b, 0, 0, 4, 0.13f, 0.7f);
              noise (b, 0, "Sparkle Air", 0.1f, 0.6f);
              filter (b, 0, kLP12, 4500.0f, 0.05f, 0.25f);
              env (b, "env1", 1.0f, 1.0f, 1.0f, 3.0f);
              delay (b, 0, kDiv8D, 0.35f, 0.2f, true);
              shimmer (b, 0.45f, 0.5f, 0.85f);
              padMacros (b, 0.3f);
              b.macroName (2, "Sparkle");
              b.route ("Macro 3", "u_n1_vol", 0.25f);
          } },
        // ================================================================ DRONES
        { "Root Drone", "Texture", "Pure, steady root note with a soft octave. Hold the song's key note through a whole set.",
          [] (B& b) {
              osc (b, 0, 0, kSine, 0.8f);
              osc (b, 0, 1, kTri, 0.3f, 12.0f);
              osc (b, 0, 2, kSine, 0.5f, -12.0f);
              b.set ("e1_vol", 0.7f);
              filter (b, 0, kLP12, 1500.0f, 0.0f, 0.2f);
              env (b, "env1", 2.0f, 1.0f, 1.0f, 4.0f);
              lfo (b, 0, LfoSine, 0.1f);
              b.set ("lfo1_retrig", 3.0f);
              b.route ("LFO 1", "f1_cutoff", 0.03f);
              hall (b, 0.45f, 0.9f, 0.85f);
              macros (b, "Brightness", "Space", "Octave", "Swell");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "e1_o2_vol", 0.4f).route ("Macro 4", "env1_a", 0.35f);
          } },
        { "Fifth Drone", "Texture", "Root, fifth and octave from one key, very dark, into shimmer. A bed for a whole song.",
          [] (B& b) {
              osc (b, 0, 0, kSaw, 0.6f);
              osc (b, 0, 1, kTri, 0.6f, 0.0f, 0.06f);
              b.set ("e1_vol", 0.55f);
              uni (b, 0, 1, 3, 0.0f, 0.6f);
              b.set ("e1_uni_chord", 2.0f); // Fifth + Oct
              filter (b, 0, kLP24, 700.0f, 0.1f, 0.2f);
              env (b, "env1", 2.5f, 1.0f, 1.0f, 4.5f);
              motion (b, 0, LfoSine, 0.06f, "f1_cutoff", 0.04f, 0.15f);
              shimmer (b, 0.5f, 0.5f, 0.85f);
              padMacros (b, 0.3f);
          } },
        { "Cello Drone", "Texture", "Low bowed-string drone with a slow vibrato. Hold low notes under a reading or a prayer.",
          [] (B& b) {
              wavetable (b, 0, "String Ensemble", 0.4f, 0.6f);
              b.set ("e1_coarse", -12.0f);
              uni (b, 0, 0, 4, 0.1f, 0.6f);
              filter (b, 0, kLP24, 900.0f, 0.1f, 0.3f);
              env (b, "env1", 2.0f, 1.0f, 1.0f, 4.0f);
              lfo (b, 0, LfoSine, 4.8f);
              b.set ("lfo1_fade", 1.5f);
              b.route ("LFO 1", "e1_fine", 0.004f);
              b.route ("LFO 1", "e1_fine", 0.008f, "Macro 3", 1.0f);
              hall (b, 0.42f, 0.85f, 0.8f);
              macros (b, "Brightness", "Space", "Vibrato", "Swell");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 4", "env1_a", 0.35f);
          } },
        { "Organ Drone", "Texture", "Soft drawbar organ that swells in, with a slow rotary. Holds a hymn's key note.",
          [] (B& b) {
              drawbars (b, { 8, 0, 8, 4, 0, 3, 0, 0, 0 });
              b.set ("u_n1_vol", 0.0f); // no key click on a drone
              env (b, "env1", 1.5f, 1.0f, 1.0f, 3.0f);
              filter (b, 0, kLP12, 3000.0f, 0.0f, 0.2f);
              rotary (b, 0.1f, 0.5f);
              hall (b, 0.45f, 0.9f, 0.8f);
              macros (b, "Tone", "Space", "Rotary Fast", "Swell");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "fx_a1_p1", 1.0f).route ("Macro 4", "env1_a", 0.35f);
          } },
        { "Dark Drone", "Texture", "Deep, rumbling low drone with a slow filter wave. Tension under a dramatic moment.",
          [] (B& b) {
              osc (b, 0, 0, kSaw, 0.6f, -12.0f);
              osc (b, 0, 1, kSquare, 0.4f, -24.0f);
              b.set ("e1_vol", 0.6f);
              noise (b, 0, "Rumble Drone", 0.35f, 0.0f);
              filter (b, 0, kLP24, 300.0f, 0.3f, 0.3f);
              env (b, "env1", 2.5f, 1.0f, 1.0f, 4.0f);
              lfo (b, 0, LfoSine, 0.05f);
              b.set ("lfo1_retrig", 3.0f);
              b.route ("LFO 1", "f1_cutoff", 0.12f);
              hall (b, 0.4f, 0.85f, 0.8f);
              macros (b, "Brightness", "Space", "Rumble", "Swell");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "u_n1_vol", 0.3f).route ("Macro 4", "env1_a", 0.35f);
          } },
        { "Air Drone", "Texture", "A soft, pure tone wrapped in breath that's tuned to the note. Calm and spacious.",
          [] (B& b) {
              osc (b, 0, 0, kSine, 0.8f);
              osc (b, 0, 1, kTri, 0.25f, 12.0f, 0.04f);
              b.set ("e1_vol", 0.5f);
              noise (b, 0, "Pink", 0.3f, 0.0f);
              model (b, 0, kComb, 0, 261.63f, 0.97f, 1.0f);
              b.set ("f1_a", 0.5f).set ("f1_vol", 2.6f);
              filter (b, 1, kLP12, 3000.0f, 0.0f, 0.3f);
              b.set ("f_routing", 0.0f);
              env (b, "env1", 2.0f, 1.0f, 1.0f, 4.0f);
              hall (b, 0.45f, 0.9f, 0.8f);
              macros (b, "Brightness", "Space", "Air", "Swell");
              b.route ("Macro 1", "f2_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "u_n1_vol", 0.3f).route ("Macro 4", "env1_a", 0.35f);
          } },
        { "Shimmer Drone", "Texture", "Simple sine-and-triangle drone fed into a big octave shimmer. Glows above the root.",
          [] (B& b) {
              osc (b, 0, 0, kSine, 0.8f);
              osc (b, 0, 1, kTri, 0.35f, 12.0f, 0.04f);
              b.set ("e1_vol", 0.65f);
              filter (b, 0, kLP12, 2500.0f, 0.0f, 0.2f);
              env (b, "env1", 2.0f, 1.0f, 1.0f, 4.5f);
              shimmer (b, 0.6f, 0.7f, 0.9f);
              macros (b, "Brightness", "Space", "Shimmer", "Swell");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.4f).route ("Macro 3", "fx_x1_p2", 0.3f).route ("Macro 4", "env1_a", 0.35f);
          } },
        { "Pulsing Drone", "Texture", "Warm drone that breathes in quarter notes with the song's tempo. Movement without notes.",
          [] (B& b) {
              osc (b, 0, 0, kSaw, 0.6f);
              osc (b, 0, 1, kSine, 0.6f, -12.0f);
              b.set ("e1_vol", 0.6f);
              uni (b, 0, 0, 3, 0.1f, 0.6f);
              filter (b, 0, kLP24, 1000.0f, 0.1f, 0.3f);
              env (b, "env1", 1.5f, 1.0f, 1.0f, 3.5f);
              lfo (b, 0, LfoSine, 2.0f, false);
              b.set ("lfo1_sync", 1.0f).set ("lfo1_div", (float) kDiv4).set ("lfo1_retrig", 3.0f);
              b.set ("amp_mod_src", 8.0f).set ("amp_velo", 0.45f); // LFO 1
              hall (b, 0.4f, 0.8f, 0.75f);
              macros (b, "Brightness", "Space", "Pulse", "Swell");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "amp_velo", 0.4f).route ("Macro 4", "env1_a", 0.35f);
          } },
        // ================================================================ SWELLS AND TRANSITIONS
        { "Riser 2 Bars", "FX", "Noise and saws that rise in pitch, brightness and volume over two bars. Press on the downbeat.",
          [] (B& b) {
              osc (b, 0, 0, kSaw, 0.8f);
              b.set ("e1_vol", 0.5f);
              uni (b, 0, 0, 4, 0.2f, 0.8f);
              noise (b, 0, "White Wide", 0.5f, 0.0f);
              filter (b, 0, kLP24, 300.0f, 0.25f, 0.0f);
              env (b, "env1", 0.01f, 1.0f, 1.0f, 1.5f);
              oneShot (b, 0, kDiv2Bars);
              b.route ("Function 1", "f1_cutoff", 0.55f).route ("Function 1", "e1_coarse", 0.05f)
                  .route ("Function 1", "e1_coarse", 0.05f, "Macro 4", 1.0f);
              b.set ("amp_mod_src", 11.0f).set ("amp_velo", 0.9f);
              delay (b, 0, kDiv8, 0.4f, 0.25f, true);
              hall (b, 0.5f, 0.85f, 0.8f);
              macros (b, "Brightness", "Space", "Noise", "Pitch Rise");
              b.route ("Macro 1", "f1_cutoff", 0.2f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "u_n1_vol", 0.3f);
          } },
        { "Riser 4 Bars", "FX", "A longer, slower build over four bars: noise and saws climbing into a big shimmer.",
          [] (B& b) {
              osc (b, 0, 0, kSaw, 0.8f);
              b.set ("e1_vol", 0.5f);
              uni (b, 0, 0, 4, 0.2f, 0.8f);
              noise (b, 0, "Pink Wide", 0.5f, 0.0f);
              filter (b, 0, kLP24, 250.0f, 0.3f, 0.0f);
              env (b, "env1", 0.01f, 1.0f, 1.0f, 2.0f);
              oneShot (b, 0, kDiv4Bars);
              b.route ("Function 1", "f1_cutoff", 0.6f).route ("Function 1", "e1_coarse", 0.05f)
                  .route ("Function 1", "e1_coarse", 0.05f, "Macro 4", 1.0f);
              b.set ("amp_mod_src", 11.0f).set ("amp_velo", 0.9f);
              shimmer (b, 0.5f, 0.55f, 0.85f);
              macros (b, "Brightness", "Space", "Noise", "Pitch Rise");
              b.route ("Macro 1", "f1_cutoff", 0.2f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "u_n1_vol", 0.3f);
          } },
        { "Reverse Swell", "FX", "A reversed-cymbal swell into a chord that cuts off. Press a beat and a half before the downbeat.",
          [] (B& b) {
              wavetable (b, 0, "Warm Pad", 0.5f, 0.5f);
              uni (b, 0, 0, 3, 0.12f, 0.7f);
              noise (b, 0, "Swell", 0.55f, 0.0f, false);
              filter (b, 0, kLP24, 2500.0f, 0.1f, 0.2f);
              env (b, "env1", 1.5f, 0.1f, 0.0f, 0.25f);
              b.set ("env1_acurve", -10.0f);
              hall (b, 0.5f, 0.9f, 0.8f);
              macros (b, "Brightness", "Space", "Noise", "Length");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "u_n1_vol", 0.3f).route ("Macro 4", "env1_a", 0.3f);
          } },
        { "Down Sweep", "FX", "Falling noise and saws for the bar after a big moment: everything drops away over one bar.",
          [] (B& b) {
              osc (b, 0, 0, kSaw, 0.8f);
              b.set ("e1_vol", 0.45f);
              uni (b, 0, 0, 4, 0.2f, 0.8f);
              noise (b, 0, "White Wide", 0.45f, 0.0f);
              filter (b, 0, kLP24, 250.0f, 0.2f, 0.0f);
              env (b, "env1", 0.005f, 4.0f, 0.0f, 2.0f);
              oneShot (b, 1, kDiv1Bar);
              b.route ("Function 1", "f1_cutoff", 0.55f).route ("Function 1", "e1_coarse", 0.1f);
              hall (b, 0.5f, 0.9f, 0.85f);
              macros (b, "Brightness", "Space", "Noise", "Decay");
              b.route ("Macro 1", "f1_cutoff", 0.2f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "u_n1_vol", 0.3f).route ("Macro 4", "env1_d", 0.3f);
          } },
        { "Wind Swell", "FX", "A pitched gust: saws and noise tuned to the chord swell up together and fade when you let go.",
          [] (B& b) {
              osc (b, 0, 0, kSaw, 0.8f);
              b.set ("e1_vol", 0.45f);
              uni (b, 0, 0, 3, 0.12f, 0.8f);
              noise (b, 0, "White Wide", 0.3f, 0.0f);
              model (b, 0, kComb, 0, 261.63f, 0.95f, 1.0f);
              b.set ("f1_a", 0.4f).set ("f1_vol", 1.5f);
              filter (b, 1, kLP12, 400.0f, 0.2f, 0.3f);
              b.set ("f_routing", 0.0f);
              env (b, "env2", 3.5f, 1.0f, 1.0f, 2.0f);
              b.route ("Env 2", "f2_cutoff", 0.45f);
              env (b, "env1", 3.5f, 0.5f, 1.0f, 1.5f);
              hall (b, 0.45f, 0.85f, 0.8f);
              macros (b, "Brightness", "Space", "Wind", "Length");
              b.route ("Macro 1", "f2_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "u_n1_vol", 0.3f)
                  .route ("Macro 4", "env1_a", 0.3f).route ("Macro 4", "env2_a", 0.3f);
          } },
        { "Sub Swell", "Texture", "A deep sine swell, felt more than heard. Low notes under the last chorus. Needs subs on the PA.",
          [] (B& b) {
              osc (b, 0, 0, kSine, 0.9f);
              osc (b, 0, 1, kTri, 0.15f, 12.0f);
              b.set ("e1_vol", 0.8f);
              filter (b, 0, kLP12, 400.0f, 0.0f, 0.0f);
              env (b, "env1", 2.5f, 0.5f, 1.0f, 1.2f);
              macros (b, "Brightness", "Space", "Overtone", "Swell");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.3f).route ("Macro 3", "e1_o2_vol", 0.4f).route ("Macro 4", "env1_a", 0.35f);
              room (b, 0.0f);
          } },
        // ================================================================ BELLS AND MALLETS
        { "Worship Bells", "Pluck", "Sweet FM bells with a tine on top, a dotted echo and shimmer. Play simple melodies.",
          [] (B& b) {
              fmVoice (b, 0, 2.0f, 0.15f, 0.7f);
              env (b, "env2", 0.001f, 1.5f, 0.0f, 0.8f);
              b.route ("Env 2", "e1_wt_pm", 0.3f, "Velocity", 1.0f);
              fmVoice (b, 1, 7.0f, 0.3f, 0.0f);
              env (b, "env3", 0.001f, 0.15f, 0.0f, 0.1f);
              b.route ("Env 3", "e2_vol", 0.35f, "Velocity", 0.8f);
              filter (b, 0, kLP12, 8000.0f, 0.0f, 0.3f);
              env (b, "env1", 0.001f, 3.5f, 0.0f, 1.8f);
              delay (b, 0, kDiv4D, 0.3f, 0.18f, true);
              shimmer (b, 0.35f, 0.4f, 0.8f);
              macros (b, "Brightness", "Space", "Echo", "Decay");
              b.route ("Macro 1", "e1_wt_pm", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "fx_a1_p8", 0.4f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "Soft Chimes", "Pluck", "High glass chimes, each note in a different place in the stereo field, like wind chimes.",
          [] (B& b) {
              wavetable (b, 0, "Glass", 0.3f, 1.0f);
              b.set ("e1_coarse", 12.0f);
              filter (b, 0, kLP12, 9000.0f, 0.0f, 0.2f);
              b.set ("f1_vol", 1.6f);
              env (b, "env1", 0.004f, 4.5f, 0.0f, 3.0f);
              b.set ("rand1_type", 1.0f).set ("rand1_vm_mode", 2.0f);
              b.route ("Random 1", "voice_pan", 0.35f);
              hall (b, 0.5f, 0.9f, 0.85f);
              macros (b, "Brightness", "Space", "Colour", "Decay");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "e1_wt_pos", 0.4f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "Kalimba", "Pluck", "Thumb piano: a bright 'tink' that quickly settles into a pure, woody tone.",
          [] (B& b) {
              fmVoice (b, 0, 5.9f, 0.0f, 0.8f);
              env (b, "env2", 0.001f, 0.12f, 0.0f, 0.1f);
              b.route ("Env 2", "e1_wt_pm", 0.35f, "Velocity", 1.0f);
              noise (b, 0, "Pluck Noise", 0.15f, 0.2f, false);
              filter (b, 0, kLP12, 6000.0f, 0.0f, 0.3f);
              env (b, "env1", 0.001f, 1.4f, 0.0f, 0.6f);
              room (b, 0.25f);
              macros (b, "Brightness", "Space", "Thumb", "Decay");
              b.route ("Macro 1", "e1_wt_pm", 0.25f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "u_n1_vol", 0.3f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "Soft Vibes", "Pluck", "Mellow vibraphone with its motor tremolo. Jazzy, warm, gentle.",
          [] (B& b) {
              osc (b, 0, 0, kSine, 0.9f);
              osc (b, 0, 1, kSine, 0.0f, 24.0f);
              b.set ("e1_vol", 0.75f);
              env (b, "env3", 0.001f, 0.3f, 0.0f, 0.2f);
              b.route ("Env 3", "e1_o2_vol", 0.35f, "Velocity", 1.0f);
              filter (b, 0, kLP12, 6000.0f, 0.0f, 0.3f);
              env (b, "env1", 0.002f, 3.5f, 0.0f, 1.2f);
              lfo (b, 0, LfoSine, 5.2f);
              b.set ("lfo1_retrig", 3.0f);
              b.set ("amp_mod_src", 8.0f).set ("amp_velo", 0.25f); // LFO 1 = the motor
              room (b, 0.3f);
              macros (b, "Brightness", "Space", "Motor", "Decay");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "amp_velo", 0.4f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "Wooden Marimba", "Pluck", "Soft, woody marimba with a mallet thump. Lovely for gentle rhythmic parts.",
          [] (B& b) {
              osc (b, 0, 0, kTri, 0.8f);
              osc (b, 0, 1, kSine, 0.0f, 24.0f);
              b.set ("e1_vol", 0.8f);
              env (b, "env3", 0.001f, 0.12f, 0.0f, 0.1f);
              b.route ("Env 3", "e1_o2_vol", 0.4f, "Velocity", 1.0f);
              noise (b, 0, "Thump", 0.2f, 0.0f, false);
              filter (b, 0, kLP12, 3500.0f, 0.0f, 0.4f);
              env (b, "env1", 0.001f, 0.7f, 0.0f, 0.35f);
              room (b, 0.2f);
              macros (b, "Brightness", "Space", "Mallet", "Decay");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "u_n1_vol", 0.3f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "Hand Bells", "Pluck", "Church handbells: a slightly inharmonic ring with a long, wide tail.",
          [] (B& b) {
              fmVoice (b, 0, 1.4f, 0.12f, 0.75f);
              env (b, "env2", 0.001f, 2.5f, 0.0f, 1.0f);
              b.route ("Env 2", "e1_wt_pm", 0.25f, "Velocity", 1.0f);
              engineOn (b, 1, 0.2f);
              osc (b, 1, 0, kSine, 0.8f, 12.0f);
              filter (b, 0, kLP12, 7000.0f, 0.0f, 0.3f);
              env (b, "env1", 0.002f, 4.5f, 0.0f, 2.5f);
              hall (b, 0.45f, 0.9f, 0.8f);
              macros (b, "Brightness", "Space", "Ring", "Decay");
              b.route ("Macro 1", "e1_wt_pm", 0.25f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "e2_vol", 0.3f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "Bowed Glass", "Pad", "Glass played with a bow: a soft, pure tone that fades in, with a slow vibrato.",
          [] (B& b) {
              wavetable (b, 0, "Glass", 0.5f, 0.6f);
              uni (b, 0, 0, 2, 0.05f, 0.5f);
              filter (b, 0, kLP12, 5000.0f, 0.0f, 0.3f);
              env (b, "env1", 0.45f, 1.0f, 1.0f, 2.0f);
              lfo (b, 0, LfoSine, 5.0f);
              b.set ("lfo1_fade", 1.0f);
              b.route ("LFO 1", "e1_fine", 0.008f);
              hall (b, 0.45f, 0.85f, 0.8f);
              macros (b, "Brightness", "Space", "Colour", "Attack");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "e1_wt_pos", 0.4f).route ("Macro 4", "env1_a", 0.35f);
          } },
        // ================================================================ KEYS, PLUCKS AND RHYTHM
        { "Ambient Piano", "Keys", "Soft felt piano drowned in shimmer and echo. Play slow, open voicings.",
          [] (B& b) {
              osc (b, 0, 0, kTri, 0.8f);
              osc (b, 0, 1, kSine, 0.25f, 12.0f);
              b.set ("e1_vol", 0.8f);
              uni (b, 0, 0, 2, 0.03f, 0.4f);
              noise (b, 0, "Thump", 0.25f, 0.0f, false);
              filter (b, 0, kLP12, 1100.0f, 0.05f, 0.4f);
              env (b, "env2", 0.001f, 0.8f, 0.0f, 0.5f);
              b.route ("Env 2", "f1_cutoff", 0.2f, "Velocity", 1.0f);
              env (b, "env1", 0.002f, 5.0f, 0.0f, 1.5f);
              b.set ("amp_velo", 0.6f);
              delay (b, 0, kDiv4, 0.3f, 0.12f, true);
              shimmer (b, 0.45f, 0.45f, 0.85f);
              macros (b, "Tone", "Space", "Shimmer", "Decay");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "fx_x1_p2", 0.4f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "Dotted Pluck", "Pluck", "Short synth pluck into a dotted-eighth echo: play quarter notes and the echo fills the gaps.",
          [] (B& b) {
              osc (b, 0, 0, kSaw, 0.7f);
              osc (b, 0, 1, kSquare, 0.4f, 0.0f, 0.08f);
              b.set ("e1_vol", 0.7f);
              filter (b, 0, kLP24, 900.0f, 0.15f, 0.4f);
              env (b, "env2", 0.001f, 0.25f, 0.0f, 0.2f);
              b.route ("Env 2", "f1_cutoff", 0.4f, "Velocity", 0.6f);
              env (b, "env1", 0.0f, 0.6f, 0.0f, 0.4f);
              delay (b, 0, kDiv8D, 0.45f, 0.35f, false);
              hall (b, 0.3f);
              macros (b, "Brightness", "Space", "Echo", "Decay");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "fx_a1_p8", 0.35f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "Soft Synth Keys", "Keys", "Gentle pulse-wave synth keys with chorus. A synth alternative to the electric piano.",
          [] (B& b) {
              osc (b, 0, 0, kSquare, 0.7f, 0.0f, 0.0f, 0.35f);
              osc (b, 0, 1, kTri, 0.3f, 12.0f);
              b.set ("e1_vol", 0.7f);
              filter (b, 0, kLP24, 1500.0f, 0.1f, 0.5f);
              env (b, "env2", 0.001f, 0.6f, 0.3f, 0.4f);
              b.route ("Env 2", "f1_cutoff", 0.25f, "Velocity", 1.0f);
              env (b, "env1", 0.002f, 1.8f, 0.45f, 0.5f);
              vChorus (b, 0, 1, 0.4f);
              room (b, 0.25f);
              macros (b, "Brightness", "Space", "Chorus", "Decay");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "fx_a1_p5", 0.4f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "Worship Arp", "Sequence", "Soft eighth-note arpeggio over two octaves with an echo. Hold a chord and let it run.",
          [] (B& b) {
              osc (b, 0, 0, kTri, 0.8f);
              osc (b, 0, 1, kSaw, 0.25f);
              b.set ("e1_vol", 1.0f);
              filter (b, 0, kLP12, 2500.0f, 0.05f, 0.3f);
              b.set ("f1_vol", 1.8f);
              env (b, "env2", 0.001f, 0.3f, 0.0f, 0.2f);
              b.route ("Env 2", "f1_cutoff", 0.25f);
              env (b, "env1", 0.002f, 0.5f, 0.0f, 0.4f);
              arp (b, ArpUp, 2, kDiv8, 0.6f);
              delay (b, 1, kDiv8D, 0.35f, 0.25f, true);
              hall (b, 0.35f);
              macros (b, "Brightness", "Space", "Echo", "Gate");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "fx_a2_p8", 0.4f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "Sixteenth Shimmer", "Sequence", "Fast, quiet bell arpeggio up and down three octaves. A glittering bed for builds.",
          [] (B& b) {
              fmVoice (b, 0, 4.0f, 0.15f, 0.8f);
              filter (b, 0, kLP12, 7000.0f, 0.0f, 0.2f);
              b.set ("f1_vol", 1.5f);
              env (b, "env1", 0.001f, 0.3f, 0.0f, 0.3f);
              arp (b, ArpUpDownIncl, 3, kDiv16, 0.5f);
              delay (b, 0, kDiv8D, 0.4f, 0.3f, true);
              shimmer (b, 0.4f, 0.45f, 0.8f);
              macros (b, "Brightness", "Space", "Echo", "Gate");
              b.route ("Macro 1", "e1_wt_pm", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "fx_a1_p8", 0.4f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "Eighth Pulse Pad", "Pad", "Pad chopped into eighth notes in time with the song. Instant drive without playing rhythm.",
          [] (B& b) {
              osc (b, 0, 0, kSaw, 0.8f);
              osc (b, 0, 1, kSquare, 0.3f, 0.0f, 0.06f);
              b.set ("e1_vol", 0.55f);
              uni (b, 0, 0, 4, 0.15f, 0.7f);
              filter (b, 0, kLP24, 1600.0f, 0.1f, 0.3f);
              env (b, "env1", 0.6f, 1.0f, 1.0f, 2.5f);
              ampFunction (b, 4, kDiv8, 0.85f, 0.15f); // Square
              vChorus (b, 0, 1, 0.4f);
              delay (b, 1, kDiv8D, 0.3f, 0.15f, true);
              hall (b, 0.35f);
              macros (b, "Brightness", "Space", "Steady", "Swell");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "amp_velo", -0.7f).route ("Macro 4", "env1_a", 0.35f);
          } },
        { "Pumping Pad", "Pad", "Pad that ducks on every beat, like it's side-chained to a kick drum. For upbeat songs.",
          [] (B& b) {
              wavetable (b, 0, "Warm Pad", 0.6f, 0.55f);
              uni (b, 0, 0, 4, 0.15f, 0.7f);
              engineOn (b, 1, 0.25f);
              osc (b, 1, 0, kSaw, 0.7f, -12.0f);
              filter (b, 0, kLP24, 2200.0f, 0.1f, 0.3f);
              env (b, "env1", 0.3f, 1.0f, 1.0f, 2.0f);
              ampFunction (b, 15, kDiv4, 0.55f, 0.08f); // Duck, smoothed so the dip doesn't click
              chorus (b, 0, 0.3f);
              hall (b, 0.35f);
              macros (b, "Brightness", "Space", "Pump", "Swell");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "amp_velo", 0.4f).route ("Macro 4", "env1_a", 0.35f);
          } },
        { "Heartbeat Sub", "Bass", "A sub that beats like a heart, two hits per bar. Hold one note under a quiet moment.",
          [] (B& b) {
              osc (b, 0, 0, kSine, 0.9f);
              osc (b, 0, 1, kTri, 0.2f, 12.0f);
              b.set ("e1_vol", 1.0f);
              mono (b, false, 0.0f);
              filter (b, 0, kLP12, 600.0f, 0.0f, 0.0f);
              env (b, "env1", 0.005f, 0.35f, 0.0f, 0.2f);
              bassSeq (b, kDiv16, { 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1.5f);
              macros (b, "Brightness", "Space", "Overtone", "Length");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 3", "e1_o2_vol", 0.4f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "Soft Sub Bass", "Bass", "Round, simple sub bass for when there's no bass player. Mono with a little glide.",
          [] (B& b) {
              osc (b, 0, 0, kSine, 0.9f);
              osc (b, 0, 1, kTri, 0.2f, 12.0f);
              b.set ("e1_vol", 1.0f);
              mono (b, true, 0.04f);
              b.set ("amp_velo", 0.1f); // even level: a sub shouldn't jump with velocity
              filter (b, 0, kLP12, 500.0f, 0.0f, 0.3f);
              env (b, "env1", 0.01f, 0.5f, 0.9f, 0.25f);
              macros (b, "Brightness", "Space", "Overtone", "Release");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 3", "e1_o2_vol", 0.4f).route ("Macro 4", "env1_r", 0.35f);
          } },
        // ================================================================ ATMOSPHERE
        { "Prayer Atmosphere", "Texture", "Quiet, airy pad with a breath of wind around it. For prayer, readings and communion.",
          [] (B& b) {
              wavetable (b, 0, "Airy Pad", 0.3f, 0.5f);
              uni (b, 0, 0, 3, 0.1f, 0.7f);
              wavetable (b, 1, "Warm Pad", 0.4f, 0.3f);
              noise (b, 0, "Wind", 0.12f, 0.0f);
              noise (b, 1, "Air", 0.08f, 0.2f);
              filter (b, 0, kLP12, 2500.0f, 0.05f, 0.2f);
              env (b, "env1", 2.5f, 1.0f, 1.0f, 4.0f);
              lfo (b, 0, LfoSine, 0.05f);
              b.set ("lfo1_retrig", 3.0f);
              b.route ("LFO 1", "f1_cutoff", 0.1f);
              hall (b, 0.5f, 0.9f, 0.85f);
              macros (b, "Brightness", "Space", "Wind", "Swell");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "u_n1_vol", 0.3f).route ("Macro 4", "env1_a", 0.35f);
          } },
        { "Final Chorus Stack", "Pad", "Everything at once: supersaw, strings, a sub and shimmer. For the last chorus.",
          [] (B& b) {
              osc (b, 0, 0, kSaw, 0.9f);
              uni (b, 0, 2, 7, 0.25f, 0.9f);
              b.set ("e1_uni_mix", 0.6f).set ("e1_vol", 0.45f);
              wavetable (b, 1, "String Ensemble", 0.5f, 0.3f);
              uni (b, 1, 0, 3, 0.1f, 0.7f);
              b.set ("u_osc_on", 1.0f).set ("u_osc_wave", 0.0f).set ("u_osc_coarse", -12.0f).set ("u_osc_vol", 0.25f);
              filter (b, 0, kLP24, 3000.0f, 0.1f, 0.3f);
              b.route ("Velocity", "f1_cutoff", 0.15f);
              env (b, "env1", 0.3f, 1.0f, 1.0f, 3.0f);
              chorus (b, 0, 0.25f);
              shimmer (b, 0.45f, 0.5f, 0.85f);
              padMacros (b, 0.3f);
              b.macroName (2, "Sub");
              b.route ("Macro 3", "u_osc_vol", 0.3f);
          } },
    };
    return list;
}

// Tags use the pack's own neutral words (see Pack 1 for the synth-family names).
juce::StringArray pack2Tags (const juce::String& presetName)
{
    static const std::map<juce::String, const char*> tags {
        { "Sanctuary Pad", "Worship, Pad, Warm" }, { "Warm Blanket", "Worship, Pad, Warm, Dark" },
        { "Morning Light", "Worship, Pad, Bright, Shimmer" }, { "Open Fifths", "Worship, Pad, Fifths" },
        { "Octave Air", "Worship, Pad, Bright" }, { "Analog Worship Pad", "Worship, Pad, Polysynth, Warm" },
        { "Choir Pad", "Worship, Pad, Choir" }, { "Glass Worship Pad", "Worship, Pad, Bright" },
        { "Felt Pad", "Worship, Pad, Lo-Fi, Warm" }, { "Slow Rise Pad", "Worship, Pad, Swell" },
        { "Worship Strings", "Worship, Strings, Pad" },
        { "Tidal Pad", "Worship, Pad, Evolving" }, { "Heaven Wash", "Worship, Pad, Shimmer, Big" },
        { "Distant Choir", "Worship, Pad, Choir, Evolving" }, { "Shimmer Cloud", "Worship, Pad, Evolving, Shimmer" },
        { "Moving Fifths", "Worship, Pad, Evolving, Fifths" }, { "Tape Memory", "Worship, Pad, Lo-Fi" },
        { "Sparkle Pad", "Worship, Pad, Bright, Shimmer" },
        { "Root Drone", "Worship, Drone" }, { "Fifth Drone", "Worship, Drone, Fifths, Shimmer" },
        { "Cello Drone", "Worship, Drone, Strings" }, { "Organ Drone", "Worship, Drone, Organ, Tonewheel" },
        { "Dark Drone", "Worship, Drone, Dark, Cinematic" }, { "Air Drone", "Worship, Drone, Texture" },
        { "Shimmer Drone", "Worship, Drone, Shimmer" }, { "Pulsing Drone", "Worship, Drone, Pulsing" },
        { "Riser 2 Bars", "Worship, Transition, Riser" }, { "Riser 4 Bars", "Worship, Transition, Riser" },
        { "Reverse Swell", "Worship, Transition, Swell" }, { "Down Sweep", "Worship, Transition" },
        { "Wind Swell", "Worship, Transition, Swell, Texture" }, { "Sub Swell", "Worship, Transition, Swell, Sub" },
        { "Worship Bells", "Worship, Bells" }, { "Soft Chimes", "Worship, Bells, Ambient" }, { "Kalimba", "Worship, Mallet" },
        { "Soft Vibes", "Worship, Mallet" }, { "Wooden Marimba", "Worship, Mallet" }, { "Hand Bells", "Worship, Bells" },
        { "Bowed Glass", "Worship, Pad, Bells" },
        { "Ambient Piano", "Worship, Piano, Felt, Shimmer" }, { "Dotted Pluck", "Worship, Pluck, Delay" },
        { "Soft Synth Keys", "Worship, Synth Keys" }, { "Worship Arp", "Worship, Arp" }, { "Sixteenth Shimmer", "Worship, Arp, Bells" },
        { "Eighth Pulse Pad", "Worship, Pad, Pulsing" }, { "Pumping Pad", "Worship, Pad, Pulsing" },
        { "Heartbeat Sub", "Worship, Bass, Sub, Pulsing" }, { "Soft Sub Bass", "Worship, Bass, Sub" },
        { "Prayer Atmosphere", "Worship, Texture, Ambient" }, { "Final Chorus Stack", "Worship, Pad, Big, Shimmer" },
    };
    const auto it = tags.find (presetName);
    return it == tags.end() ? juce::StringArray() : PresetManager::parseTags (it->second);
}
