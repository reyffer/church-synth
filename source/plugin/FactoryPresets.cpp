// Factory presets, sequencer patterns and FX presets, built in code (no files
// to install). Aimed at worship: pads, keys, plucks, swells, ambient beds, arps.
// Values are in real units (Hz, seconds, choice index, 0..1 knob).
//
// Macro convention: M1 Brightness, M2 Space, M3 Motion, M4 Swell / Attack.

#include "PresetManager.h"

#include "dsp/fx/Effects.h"

using namespace cs;
using namespace cs::fx;

namespace
{
using B = PresetBuilder;
using Params = std::vector<std::pair<juce::String, float>>;

// Synced delay time knob for a sync-division index.
float syncKnob (int division) { return ((float) division + 0.5f) / (float) kNumSyncDivs; }
constexpr int kDiv4 = 7, kDiv4D = 8, kDiv8 = 10, kDiv8D = 11, kDiv8T = 12, kDiv16 = 13;
const char* const kSlot[9] = { "a1", "a2", "a3", "b1", "b2", "b3", "x1", "x2", "x3" };

// --- building blocks -----------------------------------------------------------
void hall (B& b, float send, float size = 0.7f, float decay = 0.6f)
{
    b.fx (6, FxReverb, { { 0, 0.15f }, { 1, size }, { 2, decay }, { 3, 0.45f }, { 4, 0.2f }, { 5, 0.72f } });
    b.set ("fx_aux_send", send);
}

void shimmer (B& b, float send, float amount = 0.45f, float decay = 0.7f)
{
    b.fx (6, FxShimmer, { { 1, amount }, { 2, 0.8f }, { 3, decay }, { 4, 0.35f }, { 5, 0.65f } });
    b.set ("fx_aux_send", send);
}

void delay (B& b, int slot, int division, float feedback, float mix, bool pingPong = false)
{
    b.fx (slot, FxDelay, { { 0, syncKnob (division) }, { 2, feedback }, { 3, 0.25f }, { 4, 0.6f }, { 6, pingPong ? 1.0f : 0.0f }, { 7, mix } });
    b.set (juce::String ("fx_") + kSlot[slot] + "_sync", 1.0f);
}

void chorus (B& b, int slot, float mix = 0.4f)
{
    b.fx (slot, FxChorus, { { 1, 0.35f }, { 2, 0.25f }, { 3, 0.2f }, { 6, 1.0f }, { 7, mix } });
}

void vintageChorus (B& b, int slot, int mode, float mix = 0.5f)
{
    b.fx (slot, FxVintageChorus, { { 0, ((float) mode + 0.5f) / 3.0f }, { 4, mix } });
}

void macros (B& b, const char* m1, const char* m2, const char* m3, const char* m4)
{
    b.macroName (0, m1).macroName (1, m2).macroName (2, m3).macroName (3, m4);
}

// Brightness / Space / Motion / Swell as most pads use them.
void padMacros (B& b, float cutoffAmt = 0.35f)
{
    macros (b, "Brightness", "Space", "Motion", "Swell");
    b.route ("Macro 1", "f1_cutoff", cutoffAmt);
    b.route ("Macro 2", "fx_aux_send", 0.5f);
    b.route ("Macro 4", "env1_a", 0.35f);
}

void env (B& b, const juce::String& p, float a, float d, float s, float r)
{
    b.set (p + "_a", a).set (p + "_d", d).set (p + "_s", s).set (p + "_r", r);
}

void filter (B& b, int f, int mode, float cutoff, float reso, float kbd = 0.0f, float drive = 0.0f)
{
    const juce::String p = "f" + juce::String (f + 1) + "_";
    b.set (p + "on", 1.0f).set (p + "type", 0.0f).set (p + "mode", (float) mode).set (p + "cutoff", cutoff).set (p + "reso", reso).set (p + "kbd", kbd).set (p + "drive", drive);
}

constexpr int kLP12 = 1, kLP24 = 2, kBP12 = 4, kHP12 = 8;
constexpr int kSine = 0, kTri = 1, kSaw = 2, kSquare = 4;

void osc (B& b, int e, int o, int wave, float vol, float coarse = 0.0f, float fine = 0.0f, float width = 0.5f)
{
    const juce::String p = "e" + juce::String (e + 1) + "_o" + juce::String (o + 1) + "_";
    b.set (p + "wave", (float) wave).set (p + "vol", vol).set (p + "coarse", coarse).set (p + "width", width);
    if (o > 0)
        b.set (p + "fine", fine);
}

void uni (B& b, int e, int mode, int voices, float detune, float stereo = 0.8f)
{
    const juce::String p = "e" + juce::String (e + 1) + "_";
    b.set (p + "uni_on", 1.0f).set (p + "uni_mode", (float) mode).set (p + "uni_voices", (float) voices).set (p + "uni_detune", detune).set (p + "uni_stereo", stereo);
}

void wavetable (B& b, int e, const char* table, float pos, float vol = 0.7f)
{
    const juce::String p = "e" + juce::String (e + 1) + "_";
    b.set (p + "on", 1.0f).set (p + "type", 1.0f).choice (p + "wt_table", table).set (p + "wt_pos", pos).set (p + "vol", vol);
}

void noise (B& b, int n, const char* sample, float vol, float filter = 0.0f, bool loop = true)
{
    const juce::String p = "u_n" + juce::String (n + 1) + "_";
    b.set (p + "on", 1.0f).choice (p + "type", sample).set (p + "vol", vol).set (p + "filter", filter).set (p + "loop", loop ? 1.0f : 0.0f);
    if (! loop)
        b.set (p + "phase", 0.0f); // one-shots start at the beginning
}

void lfo (B& b, int k, int wave, float hz, bool bipolar = true)
{
    const juce::String p = "lfo" + juce::String (k + 1);
    b.set (p + "_wave", (float) wave).set (p + "_rate", hz).set (p + "_bipolar", bipolar ? 1.0f : 0.0f);
}

void mono (B& b, bool legato, float glide)
{
    b.set ("voicing", legato ? 1.0f : 0.0f).set ("glide", glide).set ("glide_legato", legato ? 1.0f : 0.0f);
}

// --- sequencer patterns -------------------------------------------------------------
SeqPattern gatePattern (std::initializer_list<int> on, float gate = 0.5f)
{
    SeqPattern p;
    int i = 0;
    for (int v : on)
    {
        p.steps[(size_t) i].on = v != 0;
        p.steps[(size_t) i].gate = gate;
        p.steps[(size_t) i].velocity = (i % 4 == 0) ? 1.0f : 0.7f;
        ++i;
    }
    return p;
}
} // namespace

// ===========================================================================
const std::vector<FactoryPreset>& factoryPresets()
{
    static const std::vector<FactoryPreset> list {
        // ------------------------------------------------------------------ PADS
        { "Sanctuary Pad", "Pad", "Warm two-engine saw pad with gentle movement and a hall. The everyday worship pad.",
          [] (B& b) {
              b.set ("master_vol", 0.85f); // level-matched across the factory set
              for (int e = 0; e < 2; ++e)
              {
                  b.set ("e" + juce::String (e + 1) + "_on", 1.0f).set ("e" + juce::String (e + 1) + "_vol", 0.55f).set ("e" + juce::String (e + 1) + "_drift", 0.35f);
                  osc (b, e, 0, kSaw, 0.8f);
                  osc (b, e, 1, e == 0 ? kSquare : kSaw, 0.45f, e == 0 ? -12.0f : 0.0f, 0.07f, 0.35f);
                  uni (b, e, 0, 4, 0.3f);
              }
              b.set ("e2_coarse", 0.0f).set ("e2_fine", 0.06f);
              filter (b, 0, kLP24, 1600.0f, 0.18f, 0.3f);
              env (b, "env1", 0.9f, 1.0f, 0.9f, 2.6f);
              lfo (b, 0, LfoTriangle, 0.12f);
              b.route ("LFO 1", "f1_cutoff", 0.06f);
              b.set ("amp_velo", 0.35f);
              vintageChorus (b, 0, 0, 0.35f);
              hall (b, 0.35f);
              padMacros (b);
              b.route ("Macro 3", "lfo1_rate", 0.3f);
          } },
        { "Warm Bed", "Pad", "Soft wavetable bed with a sine sub. Sits under a band without taking space.",
          [] (B& b) {
              b.set ("master_vol", 0.67f); // level-matched across the factory set
              wavetable (b, 0, "Warm Pad", 0.35f, 0.6f);
              uni (b, 0, 0, 4, 0.22f);
              b.set ("e2_on", 1.0f).set ("e2_vol", 0.35f);
              osc (b, 1, 0, kSine, 0.9f, -12.0f);
              filter (b, 0, kLP12, 2400.0f, 0.1f, 0.2f);
              env (b, "env1", 1.2f, 1.0f, 1.0f, 3.0f);
              lfo (b, 0, LfoSine, 0.08f);
              b.route ("LFO 1", "e1_wt_pos", 0.12f);
              hall (b, 0.3f, 0.75f, 0.65f);
              padMacros (b);
              b.route ("Macro 3", "e1_wt_pos", 0.4f);
          } },
        { "Glass Cathedral", "Pad", "Glassy wavetable pad with slow shimmer. Lovely for quiet moments and prayer.",
          [] (B& b) {
              b.set ("master_vol", 0.82f); // level-matched across the factory set
              wavetable (b, 0, "Glass Pad", 0.3f, 0.55f);
              uni (b, 0, 0, 3, 0.18f);
              wavetable (b, 1, "Airy Pad", 0.5f, 0.35f);
              b.set ("e2_coarse", 12.0f);
              filter (b, 0, kLP12, 5200.0f, 0.1f, 0.2f);
              env (b, "env1", 1.5f, 1.5f, 0.9f, 4.0f);
              lfo (b, 0, LfoSine, 0.06f);
              lfo (b, 1, LfoTriangle, 0.11f);
              b.route ("LFO 1", "e1_wt_pos", 0.2f).route ("LFO 2", "e2_wt_pos", 0.25f);
              shimmer (b, 0.4f);
              padMacros (b, 0.25f);
              b.route ("Macro 3", "lfo1_rate", 0.4f);
          } },
        { "Choir Air", "Pad", "Vocal 'ah to oo' pad with a breath layer. The mod wheel opens the vowel.",
          [] (B& b) {
              b.set ("master_vol", 1.00f); // level-matched across the factory set
              wavetable (b, 0, "Choir Ah to Oo", 0.2f, 0.65f);
              uni (b, 0, 0, 4, 0.2f, 0.9f);
              noise (b, 0, "Breath", 0.12f, 0.3f);
              filter (b, 0, kLP12, 3200.0f, 0.12f, 0.3f);
              env (b, "env1", 0.7f, 1.0f, 1.0f, 2.2f);
              lfo (b, 0, LfoSine, 0.2f);
              b.route ("Mod Wheel", "e1_wt_pos", 0.6f).route ("LFO 1", "e1_wt_pos", 0.05f);
              hall (b, 0.38f, 0.8f, 0.7f);
              padMacros (b, 0.3f);
              b.route ("Macro 3", "e1_wt_pos", 0.5f);
          } },
        { "Strings Pad", "Pad", "Ensemble strings pad with a slow bow. Velocity shapes the brightness.",
          [] (B& b) {
              b.set ("master_vol", 0.69f); // level-matched across the factory set
              wavetable (b, 0, "String Ensemble", 0.4f, 0.6f);
              uni (b, 0, 0, 5, 0.28f);
              b.set ("e2_on", 1.0f).set ("e2_vol", 0.3f);
              osc (b, 1, 0, kSaw, 0.8f);
              uni (b, 1, 0, 3, 0.35f);
              filter (b, 0, kLP24, 2800.0f, 0.1f, 0.4f);
              env (b, "env1", 0.6f, 1.0f, 1.0f, 1.6f);
              b.route ("Velocity", "f1_cutoff", 0.2f);
              chorus (b, 0, 0.3f);
              hall (b, 0.3f);
              padMacros (b);
          } },
        { "Shimmer Swell", "Pad", "Slow supersaw swell into an octave shimmer. Hold a chord and let it bloom.",
          [] (B& b) {
              b.set ("master_vol", 1.00f); // level-matched across the factory set
              osc (b, 0, 0, kSaw, 0.9f);
              uni (b, 0, 2, 7, 0.3f, 1.0f);
              b.set ("e1_uni_mix", 0.65f).set ("e1_vol", 0.55f);
              filter (b, 0, kLP24, 900.0f, 0.15f, 0.2f);
              env (b, "env1", 2.5f, 1.0f, 1.0f, 4.0f);
              env (b, "env2", 3.5f, 2.0f, 0.7f, 4.0f);
              b.route ("Env 2", "f1_cutoff", 0.35f);
              shimmer (b, 0.5f, 0.55f, 0.8f);
              padMacros (b, 0.3f);
          } },
        { "Evolving Light", "Pad", "Two wavetables drifting on random and function modulation. Never quite the same twice.",
          [] (B& b) {
              b.set ("master_vol", 1.00f); // level-matched across the factory set
              wavetable (b, 0, "Shimmer Evolve", 0.2f, 0.5f);
              wavetable (b, 1, "Airy Pad", 0.6f, 0.4f);
              uni (b, 0, 0, 3, 0.2f);
              b.set ("e2_fine", 0.08f);
              filter (b, 0, kLP12, 4200.0f, 0.15f, 0.2f);
              env (b, "env1", 1.6f, 1.0f, 1.0f, 3.5f);
              b.set ("rand1_type", 0.0f).set ("rand1_rate", 0.3f).set ("rand1_smooth", 1.0f).set ("rand1_retrig", 3.0f);
              b.set ("func1_shape", 3.0f).set ("func1_rate", 0.07f).set ("func1_retrig", 3.0f);
              b.route ("Random 1", "e1_wt_pos", 0.25f).route ("Function 1", "e2_wt_pos", 0.35f).route ("Random 1", "f1_cutoff", 0.08f);
              hall (b, 0.42f, 0.85f, 0.75f);
              padMacros (b, 0.3f);
              b.route ("Macro 3", "rand1_rate", 0.4f);
          } },
        { "Soft Organ Pad", "Pad", "Drawbar organ pad with a vintage chorus. Warm under a congregation.",
          [] (B& b) {
              b.set ("master_vol", 1.00f); // level-matched across the factory set
              wavetable (b, 0, "Drawbars", 0.35f, 0.6f);
              uni (b, 0, 0, 2, 0.12f);
              filter (b, 0, kLP12, 3500.0f, 0.05f, 0.2f);
              env (b, "env1", 0.25f, 1.0f, 1.0f, 1.2f);
              vintageChorus (b, 0, 2, 0.5f);
              hall (b, 0.25f, 0.6f, 0.5f);
              padMacros (b, 0.25f);
              b.route ("Macro 3", "e1_wt_pos", 0.5f);
          } },
        { "Ambient Fifths", "Pad", "Root and fifth on every key (Unison Chord), very long and wide. One finger fills the room.",
          [] (B& b) {
              b.set ("master_vol", 0.97f); // level-matched across the factory set
              osc (b, 0, 0, kTri, 0.9f);
              osc (b, 0, 1, kSine, 0.5f, 12.0f, 0.04f);
              uni (b, 0, 1, 4, 0.0f, 0.9f);
              b.set ("e1_uni_chord", 2.0f).set ("e1_vol", 0.5f).set ("e1_drift", 0.3f);
              filter (b, 0, kLP12, 2600.0f, 0.1f, 0.3f);
              env (b, "env1", 2.0f, 1.0f, 1.0f, 5.0f);
              shimmer (b, 0.45f, 0.4f, 0.8f);
              padMacros (b, 0.3f);
          } },
        { "Dark Drone", "Pad", "Low saw drone with rumble underneath. For transitions and long held notes.",
          [] (B& b) {
              b.set ("master_vol", 0.79f); // level-matched across the factory set
              osc (b, 0, 0, kSaw, 0.8f);
              osc (b, 0, 1, kSaw, 0.6f, -12.0f, 0.05f);
              uni (b, 0, 0, 3, 0.2f);
              noise (b, 0, "Rumble Drone", 0.18f, -0.5f);
              filter (b, 0, kLP24, 520.0f, 0.25f, 0.1f, 0.2f);
              env (b, "env1", 2.0f, 1.0f, 1.0f, 4.0f);
              lfo (b, 0, LfoSine, 0.05f);
              b.route ("LFO 1", "f1_cutoff", 0.08f);
              hall (b, 0.35f, 0.85f, 0.7f);
              padMacros (b, 0.4f);
          } },
        // ------------------------------------------------------------------ KEYS
        { "Electric Tines", "Keys", "Electric-piano style keys: sine with a little FM bark and a soft tremolo.",
          [] (B& b) {
              b.set ("master_vol", 1.00f); // level-matched across the factory set
              osc (b, 0, 0, kSine, 0.9f);
              osc (b, 0, 2, kSine, 0.0f, 14.0f);
              b.set ("e1_mod_src", 0.0f).set ("e1_o1_fm", 0.18f).set ("e1_vol", 0.65f);
              filter (b, 0, kLP12, 6000.0f, 0.0f, 0.3f);
              env (b, "env1", 0.003f, 2.2f, 0.25f, 0.5f);
              env (b, "env2", 0.001f, 0.4f, 0.0f, 0.3f);
              b.route ("Env 2", "e1_o1_fm", 0.3f).route ("Velocity", "e1_o1_fm", 0.25f);
              b.set ("amp_velo", 0.7f);
              b.fx (0, FxPanner, { { 0, 0.25f }, { 1, 0.45f } });
              hall (b, 0.2f, 0.5f, 0.45f);
              macros (b, "Bark", "Space", "Tremolo", "Decay");
              b.route ("Macro 1", "e1_o1_fm", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "fx_a1_p1", 0.6f).route ("Macro 4", "env1_d", 0.4f);
          } },
        { "Soft Keys", "Keys", "Gentle triangle keys that decay into a soft sustain. Good for verses.",
          [] (B& b) {
              b.set ("master_vol", 0.80f); // level-matched across the factory set
              osc (b, 0, 0, kTri, 0.9f);
              osc (b, 0, 1, kSine, 0.4f, 12.0f, 0.03f);
              b.set ("e1_vol", 0.7f);
              filter (b, 0, kLP24, 2600.0f, 0.1f, 0.5f);
              env (b, "env1", 0.004f, 1.6f, 0.35f, 0.7f);
              env (b, "env2", 0.002f, 0.9f, 0.2f, 0.6f);
              b.route ("Env 2", "f1_cutoff", 0.25f).route ("Velocity", "f1_cutoff", 0.2f);
              b.set ("amp_velo", 0.6f);
              chorus (b, 0, 0.25f);
              hall (b, 0.25f, 0.55f, 0.5f);
              macros (b, "Brightness", "Space", "Chorus", "Decay");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "fx_a1_p8", 0.5f).route ("Macro 4", "env1_d", 0.4f);
          } },
        { "Stage Keys", "Keys", "Brighter filtered keys with a little key click. Cuts through a full band.",
          [] (B& b) {
              b.set ("master_vol", 0.76f); // level-matched across the factory set
              osc (b, 0, 0, kSaw, 0.7f);
              osc (b, 0, 1, kSquare, 0.4f, 12.0f, 0.0f, 0.3f);
              b.set ("e1_vol", 0.6f);
              noise (b, 0, "Key Click", 0.25f, 0.2f, false);
              filter (b, 0, kLP24, 1800.0f, 0.15f, 0.6f);
              env (b, "env1", 0.003f, 1.2f, 0.4f, 0.4f);
              env (b, "env2", 0.001f, 0.6f, 0.15f, 0.4f);
              b.route ("Env 2", "f1_cutoff", 0.35f).route ("Velocity", "f1_cutoff", 0.25f);
              b.set ("amp_velo", 0.6f);
              hall (b, 0.18f, 0.5f, 0.45f);
              macros (b, "Brightness", "Space", "Click", "Decay");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "u_n1_vol", 0.5f).route ("Macro 4", "env1_d", 0.4f);
          } },
        { "Gospel Organ", "Keys", "Drawbar organ with vintage chorus and a fast attack. Mod wheel brings in the top drawbars.",
          [] (B& b) {
              b.set ("master_vol", 0.94f); // level-matched across the factory set
              wavetable (b, 0, "Drawbars", 0.55f, 0.6f);
              b.set ("e2_on", 1.0f).set ("e2_vol", 0.25f);
              osc (b, 1, 0, kSine, 0.8f, -12.0f);
              filter (b, 0, kLP12, 7000.0f, 0.0f, 0.2f);
              env (b, "env1", 0.006f, 0.5f, 1.0f, 0.12f);
              b.set ("amp_velo", 0.15f);
              b.route ("Mod Wheel", "e1_wt_pos", 0.4f);
              vintageChorus (b, 0, 2, 0.6f);
              hall (b, 0.15f, 0.5f, 0.4f);
              macros (b, "Drawbars", "Space", "Chorus", "Release");
              b.route ("Macro 1", "e1_wt_pos", 0.4f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "fx_a1_p5", 0.5f).route ("Macro 4", "env1_r", 0.4f);
          } },
        { "Pipe Organ", "Keys", "Church pipe organ with a flute rank and a big stone room.",
          [] (B& b) {
              b.set ("master_vol", 0.33f); // level-matched across the factory set
              wavetable (b, 0, "Pipe Organ", 0.4f, 0.55f);
              wavetable (b, 1, "Flute Pipe", 0.3f, 0.35f);
              b.set ("e2_coarse", 12.0f);
              filter (b, 0, kLP12, 6500.0f, 0.0f, 0.3f);
              env (b, "env1", 0.03f, 0.5f, 1.0f, 0.6f);
              b.set ("amp_velo", 0.0f);
              b.fx (6, FxReverb, { { 0, 0.25f }, { 1, 0.95f }, { 2, 0.85f }, { 3, 0.4f }, { 5, 0.7f } });
              b.set ("fx_aux_send", 0.4f);
              macros (b, "Brightness", "Room", "Flute", "Release");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "e2_vol", 0.4f).route ("Macro 4", "env1_r", 0.5f);
          } },
        { "Glass Keys", "Keys", "Bell-like glass keys with a quarter-note echo.",
          [] (B& b) {
              b.set ("master_vol", 1.00f); // level-matched across the factory set
              wavetable (b, 0, "Glass", 0.45f, 1.0f);
              filter (b, 0, kLP12, 5000.0f, 0.05f, 0.4f);
              env (b, "env1", 0.002f, 2.4f, 0.0f, 1.2f);
              b.set ("amp_velo", 0.6f);
              b.route ("Velocity", "e1_wt_pos", 0.25f);
              delay (b, 0, kDiv4, 0.3f, 0.22f);
              hall (b, 0.3f, 0.65f, 0.6f);
              macros (b, "Brightness", "Space", "Echo", "Decay");
              b.route ("Macro 1", "e1_wt_pos", 0.4f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "fx_a1_p8", 0.4f).route ("Macro 4", "env1_d", 0.4f);
          } },
        // ------------------------------------------------------------------ PLUCKS
        { "Worship Pluck", "Pluck", "Classic saw pluck with a dotted-eighth echo. The one for instrumentals and builds.",
          [] (B& b) {
              b.set ("master_vol", 1.00f); // level-matched across the factory set
              osc (b, 0, 0, kSaw, 0.85f);
              osc (b, 0, 1, kSaw, 0.5f, 0.0f, 0.08f);
              uni (b, 0, 0, 2, 0.15f);
              b.set ("e1_vol", 1.0f);
              filter (b, 0, kLP24, 400.0f, 0.2f, 0.4f);
              env (b, "env1", 0.002f, 0.6f, 0.0f, 0.4f);
              env (b, "env2", 0.001f, 0.35f, 0.0f, 0.3f);
              b.route ("Env 2", "f1_cutoff", 0.55f).route ("Velocity", "f1_cutoff", 0.15f);
              delay (b, 0, kDiv8D, 0.35f, 0.3f, true);
              hall (b, 0.25f, 0.6f, 0.55f);
              macros (b, "Brightness", "Space", "Echo", "Decay");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "fx_a1_p3", 0.4f).route ("Macro 4", "env2_d", 0.4f);
          } },
        { "Bell Pluck", "Pluck", "FM bell pluck with a long tail.",
          [] (B& b) {
              b.set ("master_vol", 1.00f); // level-matched across the factory set
              wavetable (b, 0, "FM Ratio 3", 0.5f, 0.6f);
              filter (b, 0, kLP12, 7000.0f, 0.0f, 0.3f);
              env (b, "env1", 0.001f, 1.8f, 0.0f, 1.6f);
              env (b, "env2", 0.001f, 0.8f, 0.0f, 0.5f);
              b.route ("Env 2", "e1_wt_pos", 0.4f);
              b.set ("amp_velo", 0.6f);
              delay (b, 0, kDiv8D, 0.3f, 0.2f);
              hall (b, 0.3f, 0.7f, 0.6f);
              macros (b, "Brightness", "Space", "Echo", "Decay");
              b.route ("Macro 1", "e1_wt_pos", 0.4f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "fx_a1_p8", 0.4f).route ("Macro 4", "env1_d", 0.4f);
          } },
        { "Soft Mallet", "Pluck", "Rounded mallet through a lowpass gate.",
          [] (B& b) {
              b.set ("master_vol", 0.79f); // level-matched across the factory set
              osc (b, 0, 0, kTri, 0.9f);
              osc (b, 0, 1, kSine, 0.5f, 19.0f, 0.0f);
              noise (b, 0, "Soft Click", 0.2f, 0.0f, false);
              b.set ("e1_vol", 0.7f);
              b.set ("f1_on", 1.0f).set ("f1_type", (float) FilterLpg).set ("f1_submode", 2.0f).set ("f1_cutoff", 3500.0f).set ("f1_a", 0.0f).set ("f1_b", 0.35f);
              env (b, "env1", 0.001f, 1.0f, 0.0f, 0.8f);
              env (b, "env2", 0.001f, 0.25f, 0.0f, 0.2f);
              b.route ("Env 2", "f1_a", 1.0f).route ("Velocity", "f1_a", 0.3f);
              hall (b, 0.3f, 0.6f, 0.55f);
              macros (b, "Brightness", "Space", "Tone", "Decay");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "e1_o2_vol", 0.4f).route ("Macro 4", "f1_b", 0.4f);
          } },
        { "Thumb Piano", "Pluck", "Kalimba-like pluck: a short tone with a woody click.",
          [] (B& b) {
              b.set ("master_vol", 0.87f); // level-matched across the factory set
              osc (b, 0, 0, kSine, 0.9f);
              osc (b, 0, 1, kSine, 0.25f, 24.0f, 0.1f);
              noise (b, 0, "Pluck Noise", 0.3f, 0.3f, false);
              b.set ("e1_vol", 0.75f);
              filter (b, 0, kLP12, 4500.0f, 0.1f, 0.5f);
              env (b, "env1", 0.001f, 0.9f, 0.0f, 0.5f);
              b.set ("amp_velo", 0.6f);
              hall (b, 0.25f, 0.5f, 0.5f);
              macros (b, "Brightness", "Space", "Overtone", "Decay");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "e1_o2_vol", 0.4f).route ("Macro 4", "env1_d", 0.4f);
          } },
        { "Ambient Pluck", "Pluck", "Soft triangle pluck into a ping-pong echo and a big room.",
          [] (B& b) {
              b.set ("master_vol", 1.00f); // level-matched across the factory set
              osc (b, 0, 0, kTri, 0.9f);
              osc (b, 0, 1, kSaw, 0.25f, 12.0f, 0.05f);
              b.set ("e1_vol", 0.7f);
              filter (b, 0, kLP24, 1500.0f, 0.15f, 0.4f);
              env (b, "env1", 0.002f, 1.2f, 0.0f, 1.0f);
              env (b, "env2", 0.001f, 0.5f, 0.0f, 0.5f);
              b.route ("Env 2", "f1_cutoff", 0.35f);
              delay (b, 0, kDiv8, 0.45f, 0.35f, true);
              hall (b, 0.4f, 0.8f, 0.7f);
              macros (b, "Brightness", "Space", "Echo", "Decay");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "fx_a1_p3", 0.4f).route ("Macro 4", "env1_d", 0.4f);
          } },
        // ------------------------------------------------------------------ LEADS
        { "Soft Lead", "Lead", "Mellow square lead with glide; the mod wheel adds vibrato.",
          [] (B& b) {
              b.set ("master_vol", 0.97f); // level-matched across the factory set
              osc (b, 0, 0, kSquare, 0.8f, 0.0f, 0.0f, 0.4f);
              osc (b, 0, 1, kSaw, 0.3f, 0.0f, 0.06f);
              b.set ("e1_vol", 0.95f);
              mono (b, true, 0.08f);
              filter (b, 0, kLP24, 2200.0f, 0.15f, 0.5f);
              env (b, "env1", 0.02f, 0.5f, 0.9f, 0.3f);
              lfo (b, 0, LfoSine, 5.5f);
              b.route ("LFO 1", "e1_coarse", 0.004f, "Mod Wheel", 1.0f);
              delay (b, 0, kDiv4, 0.3f, 0.2f);
              hall (b, 0.2f);
              macros (b, "Brightness", "Space", "Vibrato", "Glide");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "lfo1_rate", 0.2f);
          } },
        { "Saw Lead", "Lead", "Bright supersaw lead with an eighth-note echo.",
          [] (B& b) {
              b.set ("master_vol", 1.00f); // level-matched across the factory set
              osc (b, 0, 0, kSaw, 0.9f);
              uni (b, 0, 2, 7, 0.18f, 0.8f);
              b.set ("e1_uni_mix", 0.5f).set ("e1_vol", 1.0f);
              mono (b, false, 0.04f);
              filter (b, 0, kLP24, 3800.0f, 0.2f, 0.5f);
              env (b, "env1", 0.005f, 0.5f, 0.85f, 0.3f);
              delay (b, 0, kDiv8, 0.3f, 0.22f, true);
              hall (b, 0.2f);
              macros (b, "Brightness", "Space", "Width", "Glide");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "e1_uni_detune", 0.3f);
          } },
        { "Breath Flute", "Lead", "Airy flute with breath noise and a delayed vibrato.",
          [] (B& b) {
              b.set ("master_vol", 1.00f); // level-matched across the factory set
              wavetable (b, 0, "Flute Pipe", 0.25f, 1.0f);
              noise (b, 0, "Breath", 0.25f, 0.4f);
              mono (b, true, 0.06f);
              filter (b, 0, kLP12, 4200.0f, 0.05f, 0.5f);
              env (b, "env1", 0.06f, 0.5f, 1.0f, 0.3f);
              lfo (b, 0, LfoSine, 5.0f);
              b.set ("lfo1_fade", 0.8f);
              b.route ("LFO 1", "e1_coarse", 0.003f);
              hall (b, 0.3f, 0.65f, 0.6f);
              macros (b, "Brightness", "Space", "Breath", "Vibrato");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "u_n1_vol", 0.4f).route ("Macro 4", "lfo1_rate", 0.2f);
          } },
        { "Sync Lead", "Lead", "Hard-sync lead that sweeps with the envelope and the mod wheel.",
          [] (B& b) {
              b.set ("master_vol", 1.00f); // level-matched across the factory set
              osc (b, 0, 0, kSaw, 0.0f);
              osc (b, 0, 1, kSaw, 0.85f, 7.0f);
              b.set ("e1_sync", 1.0f).set ("e1_vol", 1.0f);
              mono (b, false, 0.03f);
              filter (b, 0, kLP24, 5000.0f, 0.1f, 0.4f);
              env (b, "env1", 0.005f, 0.6f, 0.8f, 0.3f);
              env (b, "env2", 0.001f, 0.8f, 0.2f, 0.4f);
              b.route ("Env 2", "e1_o2_coarse", 0.15f).route ("Mod Wheel", "e1_o2_coarse", 0.2f);
              delay (b, 0, kDiv8D, 0.3f, 0.2f);
              hall (b, 0.18f);
              macros (b, "Brightness", "Space", "Sync", "Glide");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "e1_o2_coarse", 0.25f);
          } },
        // ------------------------------------------------------------------ BASS
        { "Sub Bass", "Bass", "Clean sine sub with a hint of triangle. Felt more than heard.",
          [] (B& b) {
              b.set ("master_vol", 1.00f); // level-matched across the factory set
              osc (b, 0, 0, kSine, 0.9f);
              osc (b, 0, 1, kTri, 0.2f, 12.0f);
              b.set ("e1_vol", 1.0f);
              mono (b, true, 0.02f);
              filter (b, 0, kLP12, 900.0f, 0.0f, 0.0f);
              env (b, "env1", 0.005f, 0.5f, 1.0f, 0.15f);
              b.set ("amp_velo", 0.3f);
              macros (b, "Brightness", "Space", "Overtone", "Release");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 3", "e1_o2_vol", 0.5f).route ("Macro 4", "env1_r", 0.4f);
          } },
        { "Warm Bass", "Bass", "Round analog bass: saw and square through a 24 dB filter.",
          [] (B& b) {
              b.set ("master_vol", 1.00f); // level-matched across the factory set
              osc (b, 0, 0, kSaw, 0.8f);
              osc (b, 0, 1, kSquare, 0.6f, -12.0f, 0.0f, 0.5f);
              b.set ("e1_vol", 1.0f);
              mono (b, true, 0.03f);
              filter (b, 0, kLP24, 500.0f, 0.2f, 0.3f, 0.15f);
              env (b, "env1", 0.003f, 0.6f, 0.8f, 0.15f);
              env (b, "env2", 0.001f, 0.35f, 0.1f, 0.2f);
              b.route ("Env 2", "f1_cutoff", 0.3f).route ("Velocity", "f1_cutoff", 0.15f);
              macros (b, "Brightness", "Space", "Punch", "Release");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 3", "env2_d", 0.3f).route ("Macro 4", "env1_r", 0.4f);
          } },
        { "Pluck Bass", "Bass", "Short filtered bass pluck, polyphonic, for octave lines.",
          [] (B& b) {
              b.set ("master_vol", 0.81f); // level-matched across the factory set
              osc (b, 0, 0, kSaw, 0.9f);
              osc (b, 0, 1, kSine, 0.6f, -12.0f);
              b.set ("e1_vol", 0.7f);
              filter (b, 0, kLP24, 300.0f, 0.25f, 0.3f);
              env (b, "env1", 0.002f, 0.45f, 0.0f, 0.2f);
              env (b, "env2", 0.001f, 0.2f, 0.0f, 0.2f);
              b.route ("Env 2", "f1_cutoff", 0.5f);
              macros (b, "Brightness", "Space", "Punch", "Decay");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 3", "env2_d", 0.3f).route ("Macro 4", "env1_d", 0.4f);
          } },
        { "Drone Bass", "Bass", "Deep sustained bass with slow filter movement, for long pedal notes.",
          [] (B& b) {
              b.set ("master_vol", 0.87f); // level-matched across the factory set
              osc (b, 0, 0, kSaw, 0.6f);
              osc (b, 0, 1, kSine, 0.8f, -12.0f);
              b.set ("e1_vol", 0.7f);
              filter (b, 0, kLP24, 260.0f, 0.2f, 0.1f, 0.3f);
              env (b, "env1", 0.4f, 1.0f, 1.0f, 2.0f);
              lfo (b, 0, LfoSine, 0.07f);
              b.route ("LFO 1", "f1_cutoff", 0.1f);
              hall (b, 0.15f);
              padMacros (b, 0.3f);
          } },
        // ------------------------------------------------------------------ STRINGS / BRASS
        { "Synth Strings", "Strings/Brass", "Classic string machine: detuned saws, slow bow and chorus.",
          [] (B& b) {
              b.set ("master_vol", 1.00f); // level-matched across the factory set
              for (int e = 0; e < 2; ++e)
              {
                  b.set ("e" + juce::String (e + 1) + "_on", 1.0f).set ("e" + juce::String (e + 1) + "_vol", 0.5f);
                  osc (b, e, 0, kSaw, 0.8f, e == 0 ? 0.0f : 12.0f);
                  uni (b, e, 0, 4, 0.25f);
              }
              filter (b, 0, kLP12, 3200.0f, 0.05f, 0.4f);
              filter (b, 1, kHP12, 180.0f, 0.0f);
              b.set ("f_routing", 0.0f);
              env (b, "env1", 0.45f, 1.0f, 1.0f, 1.2f);
              vintageChorus (b, 0, 1, 0.55f);
              hall (b, 0.3f);
              padMacros (b);
          } },
        { "Soft Brass", "Strings/Brass", "Warm brass section with an envelope swell on the filter.",
          [] (B& b) {
              b.set ("master_vol", 1.00f); // level-matched across the factory set
              osc (b, 0, 0, kSaw, 0.85f);
              osc (b, 0, 1, kSaw, 0.6f, 0.0f, 0.1f);
              uni (b, 0, 0, 3, 0.15f);
              b.set ("e1_vol", 0.6f);
              filter (b, 0, kLP24, 700.0f, 0.15f, 0.5f);
              env (b, "env1", 0.06f, 0.8f, 0.85f, 0.4f);
              env (b, "env2", 0.12f, 0.6f, 0.5f, 0.4f);
              b.route ("Env 2", "f1_cutoff", 0.4f).route ("Velocity", "f1_cutoff", 0.2f);
              hall (b, 0.22f);
              macros (b, "Brightness", "Space", "Bite", "Attack");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "env2_a", -0.3f).route ("Macro 4", "env1_a", 0.4f);
          } },
        { "Cello Swell", "Strings/Brass", "Expressive mono cello-like swell; aftertouch and mod wheel add vibrato.",
          [] (B& b) {
              b.set ("master_vol", 1.00f); // level-matched across the factory set
              wavetable (b, 0, "String Ensemble", 0.6f, 0.85f);
              b.set ("e2_on", 1.0f).set ("e2_vol", 0.6f);
              osc (b, 1, 0, kSaw, 0.8f);
              mono (b, true, 0.1f);
              filter (b, 0, kLP24, 1800.0f, 0.15f, 0.5f);
              env (b, "env1", 0.35f, 1.0f, 1.0f, 0.8f);
              lfo (b, 0, LfoSine, 5.2f);
              b.route ("LFO 1", "e1_coarse", 0.004f, "Mod Wheel", 1.0f).route ("LFO 1", "e2_coarse", 0.004f, "Aftertouch", 1.0f);
              hall (b, 0.3f);
              macros (b, "Brightness", "Space", "Vibrato", "Attack");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "lfo1_rate", 0.2f).route ("Macro 4", "env1_a", 0.3f);
          } },
        // ------------------------------------------------------------------ TEXTURES
        { "Rain Texture", "Texture", "Rain and wind with slow filter movement. Plays the same on every key.",
          [] (B& b) {
              b.set ("master_vol", 1.00f); // level-matched across the factory set
              b.set ("e1_on", 0.0f);
              noise (b, 0, "Rain", 1.0f, 0.1f);
              noise (b, 1, "Wind", 0.6f, -0.2f);
              filter (b, 0, kBP12, 2200.0f, 0.2f);
              env (b, "env1", 1.5f, 1.0f, 1.0f, 3.0f);
              lfo (b, 0, LfoSine, 0.06f);
              b.route ("LFO 1", "f1_cutoff", 0.15f);
              hall (b, 0.3f, 0.8f, 0.6f);
              padMacros (b, 0.3f);
          } },
        { "Ocean Swell", "Texture", "Ocean waves over a soft low sine pad.",
          [] (B& b) {
              b.set ("master_vol", 1.00f); // level-matched across the factory set
              osc (b, 0, 0, kSine, 0.8f);
              uni (b, 0, 0, 3, 0.1f);
              b.set ("e1_vol", 0.35f);
              noise (b, 0, "Ocean", 0.55f, 0.0f);
              filter (b, 0, kLP12, 3000.0f, 0.1f);
              env (b, "env1", 2.0f, 1.0f, 1.0f, 4.0f);
              hall (b, 0.35f, 0.85f, 0.7f);
              padMacros (b, 0.3f);
          } },
        { "Vinyl Air", "Texture", "Dusty vinyl crackle on an airy pad: an instant lo-fi bed.",
          [] (B& b) {
              b.set ("master_vol", 1.00f); // level-matched across the factory set
              wavetable (b, 0, "Airy Pad", 0.4f, 0.5f);
              uni (b, 0, 0, 3, 0.2f);
              noise (b, 0, "Vinyl Crackle", 0.35f, 0.2f);
              filter (b, 0, kLP12, 2800.0f, 0.1f, 0.2f);
              env (b, "env1", 1.0f, 1.0f, 1.0f, 2.5f);
              b.fx (0, FxBitcrusher, { { 0, 0.7f }, { 1, 0.1f }, { 5, 0.3f } });
              hall (b, 0.3f);
              padMacros (b, 0.3f);
          } },
        // ------------------------------------------------------------------ SEQUENCES
        { "Pulse Arp", "Sequence", "Up arpeggio over two octaves in 16ths with a dotted echo. Hold a chord.",
          [] (B& b) {
              b.set ("master_vol", 1.00f); // level-matched across the factory set
              osc (b, 0, 0, kSaw, 0.8f);
              osc (b, 0, 1, kSquare, 0.4f, 12.0f, 0.0f, 0.3f);
              b.set ("e1_vol", 0.95f);
              filter (b, 0, kLP24, 1500.0f, 0.2f, 0.4f);
              env (b, "env1", 0.002f, 0.3f, 0.0f, 0.2f);
              env (b, "env2", 0.001f, 0.2f, 0.0f, 0.2f);
              b.route ("Env 2", "f1_cutoff", 0.35f);
              b.set ("seq_on", 1.0f).set ("seq_mode", 0.0f).set ("seq_arp_order", (float) ArpUp).set ("seq_arp_octaves", 2.0f).set ("seq_rate", (float) kDiv16);
              b.pattern (gatePattern ({ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, 0.5f));
              delay (b, 0, kDiv8D, 0.35f, 0.25f, true);
              hall (b, 0.25f);
              macros (b, "Brightness", "Space", "Echo", "Gate");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "fx_a1_p8", 0.4f).route ("Macro 4", "env1_d", 0.4f);
          } },
        { "Eighth Note Gate", "Sequence", "Seq mode: the held chord plays a rhythmic eighth-note pattern. A pad with a pulse.",
          [] (B& b) {
              b.set ("master_vol", 0.91f); // level-matched across the factory set
              for (int e = 0; e < 2; ++e)
              {
                  b.set ("e" + juce::String (e + 1) + "_on", 1.0f).set ("e" + juce::String (e + 1) + "_vol", 0.5f);
                  osc (b, e, 0, kSaw, 0.8f);
                  uni (b, e, 0, 3, 0.25f);
              }
              b.set ("e2_fine", 0.07f);
              filter (b, 0, kLP24, 1800.0f, 0.15f, 0.3f);
              env (b, "env1", 0.01f, 0.4f, 0.6f, 0.15f);
              b.set ("seq_on", 1.0f).set ("seq_mode", 1.0f).set ("seq_rate", (float) kDiv8).set ("seq_length", 8.0f);
              auto p = gatePattern ({ 1, 1, 1, 0, 1, 1, 0, 1 }, 0.7f);
              b.pattern (p);
              hall (b, 0.3f);
              padMacros (b);
          } },
        { "Ambient Arp", "Sequence", "Up and down arpeggio of glass tones in eighths, into shimmer.",
          [] (B& b) {
              b.set ("master_vol", 0.97f); // level-matched across the factory set
              wavetable (b, 0, "Glass Pad", 0.35f, 0.95f);
              filter (b, 0, kLP12, 4500.0f, 0.1f, 0.3f);
              env (b, "env1", 0.005f, 1.0f, 0.2f, 1.2f);
              b.set ("seq_on", 1.0f).set ("seq_mode", 0.0f).set ("seq_arp_order", (float) ArpUpDownExcl).set ("seq_arp_octaves", 2.0f).set ("seq_rate", (float) kDiv8);
              b.pattern (gatePattern ({ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, 0.8f));
              delay (b, 0, kDiv4D, 0.3f, 0.25f);
              shimmer (b, 0.4f);
              macros (b, "Brightness", "Space", "Echo", "Decay");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "fx_a1_p8", 0.4f).route ("Macro 4", "env1_d", 0.4f);
          } },
        { "Generative Bells", "Sequence", "Random pentatonic bells that rewrite themselves every two bars. Hold one note.",
          [] (B& b) {
              b.set ("master_vol", 0.91f); // level-matched across the factory set
              wavetable (b, 0, "FM Ratio 2", 0.4f, 0.6f);
              filter (b, 0, kLP12, 6000.0f, 0.0f, 0.3f);
              env (b, "env1", 0.001f, 1.2f, 0.0f, 1.0f);
              b.set ("seq_on", 1.0f).set ("seq_mode", 1.0f).set ("seq_rate", (float) kDiv8).set ("seq_scale", 10.0f).set ("seq_snap", 1.0f).set ("seq_autoregen", 6.0f);
              auto p = gatePattern ({ 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1 }, 0.6f);
              p.tracks[TrackPitch].random = 0.8f;
              p.tracks[TrackOctave].random = 0.25f;
              p.tracks[TrackSteps].random = 0.2f;
              p.seed = 2024;
              b.pattern (p);
              delay (b, 0, kDiv8D, 0.35f, 0.25f, true);
              shimmer (b, 0.35f);
              macros (b, "Brightness", "Space", "Echo", "Decay");
              b.route ("Macro 1", "e1_wt_pos", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "fx_a1_p8", 0.4f).route ("Macro 4", "env1_d", 0.4f);
          } },
        { "Rhythmic Pad 16ths", "Sequence", "Swung 16th-note pad with accents and an octave jump.",
          [] (B& b) {
              b.set ("master_vol", 0.87f); // level-matched across the factory set
              wavetable (b, 0, "Warm Pad", 0.4f, 0.6f);
              uni (b, 0, 0, 3, 0.2f);
              filter (b, 0, kLP24, 2400.0f, 0.15f, 0.3f);
              env (b, "env1", 0.005f, 0.3f, 0.4f, 0.12f);
              b.set ("seq_on", 1.0f).set ("seq_mode", 1.0f).set ("seq_rate", (float) kDiv16).set ("seq_swing", 0.58f);
              auto p = gatePattern ({ 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1 }, 0.45f);
              p.steps[6].octave = 1;
              p.steps[14].octave = 1;
              b.pattern (p);
              hall (b, 0.3f);
              padMacros (b);
          } },
        // ------------------------------------------------------------------ FX
        { "Riser", "FX", "Eight-second noise and saw riser (one-shot function on filter and pitch).",
          [] (B& b) {
              b.set ("master_vol", 0.95f); // level-matched across the factory set
              osc (b, 0, 0, kSaw, 0.6f);
              uni (b, 0, 2, 7, 0.4f, 1.0f);
              b.set ("e1_vol", 0.6f);
              noise (b, 0, "White", 0.35f, 0.3f);
              filter (b, 0, kLP24, 300.0f, 0.3f);
              env (b, "env1", 0.5f, 1.0f, 1.0f, 2.0f);
              b.set ("func1_shape", 0.0f).set ("func1_mode", 1.0f).set ("func1_rate", 0.125f);
              b.route ("Function 1", "f1_cutoff", 0.6f).route ("Function 1", "e1_coarse", 0.1f);
              shimmer (b, 0.4f);
              macros (b, "Brightness", "Space", "Noise", "Release");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "u_n1_vol", 0.5f).route ("Macro 4", "env1_r", 0.4f);
          } },
        { "Transition Swell", "FX", "Reverse-sounding swell into a long shimmer tail. Great between songs.",
          [] (B& b) {
              b.set ("master_vol", 0.86f); // level-matched across the factory set
              wavetable (b, 0, "Airy Pad", 0.5f, 0.55f);
              uni (b, 0, 0, 4, 0.25f);
              filter (b, 0, kLP12, 2000.0f, 0.15f);
              env (b, "env1", 3.0f, 0.5f, 0.0f, 0.8f);
              b.set ("env1_adr", 1.0f);
              env (b, "env2", 3.0f, 0.5f, 0.0f, 0.5f);
              b.route ("Env 2", "f1_cutoff", 0.4f);
              shimmer (b, 0.6f, 0.5f, 0.85f);
              padMacros (b, 0.3f);
          } },
    };
    return list;
}

// ===========================================================================
const std::vector<FactoryPattern>& factoryPatterns()
{
    static const std::vector<FactoryPattern> list {
        { "Straight 16ths", [] (SeqPattern& p, Params&) { p = gatePattern ({ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }); } },
        { "Eighths", [] (SeqPattern& p, Params& v) {
              p = gatePattern ({ 1, 1, 1, 1, 1, 1, 1, 1 }, 0.6f);
              v.push_back ({ "seq_rate", (float) kDiv8 });
              v.push_back ({ "seq_length", 8.0f });
          } },
        { "Offbeat Pulse", [] (SeqPattern& p, Params& v) {
              p = gatePattern ({ 0, 1, 0, 1, 0, 1, 0, 1 }, 0.5f);
              v.push_back ({ "seq_rate", (float) kDiv8 });
              v.push_back ({ "seq_length", 8.0f });
          } },
        { "Three-Three-Two", [] (SeqPattern& p, Params& v) {
              p = gatePattern ({ 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0 }, 0.8f);
              v.push_back ({ "seq_rate", (float) kDiv16 });
          } },
        { "Gated Chords", [] (SeqPattern& p, Params& v) {
              p = gatePattern ({ 1, 1, 1, 0, 1, 1, 0, 1 }, 0.7f);
              v.push_back ({ "seq_mode", 1.0f });
              v.push_back ({ "seq_rate", (float) kDiv8 });
              v.push_back ({ "seq_length", 8.0f });
          } },
        { "Swung Sixteenths", [] (SeqPattern& p, Params& v) {
              p = gatePattern ({ 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1 }, 0.45f);
              v.push_back ({ "seq_swing", 0.6f });
          } },
        { "Octave Bounce", [] (SeqPattern& p, Params&) {
              p = gatePattern ({ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 });
              for (int i = 1; i < kSeqSteps; i += 2)
                  p.steps[(size_t) i].octave = 1;
          } },
        { "Arp Melody (Seq)", [] (SeqPattern& p, Params& v) {
              p = gatePattern ({ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, 0.6f);
              const int notes[16] = { 0, 7, 12, 7, 4, 7, 12, 7, 0, 7, 12, 7, 5, 9, 12, 9 };
              for (int i = 0; i < kSeqSteps; ++i)
                  p.steps[(size_t) i].pitch = notes[i];
              v.push_back ({ "seq_mode", 1.0f });
          } },
        { "Pentatonic Wander", [] (SeqPattern& p, Params& v) {
              p = gatePattern ({ 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1 }, 0.6f);
              p.tracks[TrackPitch].random = 0.8f;
              p.tracks[TrackOctave].random = 0.2f;
              v.push_back ({ "seq_mode", 1.0f });
              v.push_back ({ "seq_scale", 10.0f });
              v.push_back ({ "seq_snap", 1.0f });
              v.push_back ({ "seq_autoregen", 6.0f });
          } },
        { "Slide Line", [] (SeqPattern& p, Params& v) {
              p = gatePattern ({ 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1 }, 0.55f);
              const int notes[16] = { 0, 0, 0, 12, 7, 0, 3, 5, 0, 0, 0, 12, 10, 0, 7, 5 };
              for (int i = 0; i < kSeqSteps; ++i)
                  p.steps[(size_t) i].pitch = notes[i];
              p.steps[4].slide = 0.4f;
              p.steps[12].slide = 0.4f;
              v.push_back ({ "seq_mode", 1.0f });
          } },
    };
    return list;
}

// ===========================================================================
const std::vector<FactoryFxPreset>& factoryFxPresets()
{
    static const std::vector<FactoryFxPreset> list {
        // Reverb: Pre-Delay, Size, Decay, Damping, Low Cut, High Cut, Width, Dry/Wet
        { FxReverb, "Room", { { 0, 0.05f }, { 1, 0.3f }, { 2, 0.3f }, { 3, 0.5f } } },
        { FxReverb, "Hall", { { 0, 0.15f }, { 1, 0.7f }, { 2, 0.6f }, { 3, 0.45f } } },
        { FxReverb, "Cathedral", { { 0, 0.25f }, { 1, 0.95f }, { 2, 0.88f }, { 3, 0.4f }, { 5, 0.7f } } },
        { FxReverb, "Slap", { { 0, 0.4f }, { 1, 0.15f }, { 2, 0.15f }, { 3, 0.6f } } },
        { FxReverb, "Echoes", { { 0, 0.7f }, { 1, 0.6f }, { 2, 0.7f }, { 3, 0.3f } } },
        // Delay: Time, Fine, Feedback, Low Cut, High Cut, Width, Ping-Pong, Dry/Wet (sync it for note values)
        { FxDelay, "Slapback", { { 0, 0.42f }, { 2, 0.1f }, { 7, 0.25f } } },
        { FxDelay, "Long Echo", { { 0, 0.85f }, { 2, 0.45f }, { 4, 0.55f }, { 7, 0.3f } } },
        { FxDelay, "Wide Ping-Pong", { { 0, 0.75f }, { 2, 0.4f }, { 6, 1.0f }, { 7, 0.3f } } },
        // Shimmer: Pitch, Shimmer, Size, Decay, Modulation, High Cut, Ducking, Width, Dry/Wet
        { FxShimmer, "Heaven", { { 1, 0.5f }, { 2, 0.85f }, { 3, 0.8f }, { 4, 0.35f } } },
        { FxShimmer, "Octave Bloom", { { 1, 0.7f }, { 2, 0.7f }, { 3, 0.65f } } },
        { FxShimmer, "Ducked Shimmer", { { 1, 0.45f }, { 3, 0.7f }, { 6, 0.6f } } },
        // Chorus: Delay, Depth, Rate, Feedback, Voices, Shape, Stereo, Dry/Wet
        { FxChorus, "Subtle Wide", { { 1, 0.2f }, { 2, 0.2f }, { 7, 0.3f } } },
        { FxChorus, "Lush", { { 1, 0.5f }, { 2, 0.3f }, { 4, 1.0f }, { 7, 0.5f } } },
        // Vintage Chorus: Mode, Rate, Depth, Phase, Dry/Wet
        { FxVintageChorus, "Mode I", { { 0, 0.15f } } },
        { FxVintageChorus, "Mode II", { { 0, 0.5f } } },
        { FxVintageChorus, "Mode I + II", { { 0, 0.85f } } },
        // Compressor: Threshold, Ratio, Attack, Release, Makeup, Auto Makeup, Output, Dry/Wet
        { FxCompressor, "Gentle Glue", { { 0, 0.7f }, { 1, 0.2f }, { 2, 0.4f }, { 3, 0.5f } } },
        { FxCompressor, "Pad Leveller", { { 0, 0.55f }, { 1, 0.4f }, { 2, 0.5f }, { 3, 0.6f } } },
        // Tape Echo: Time, Fine, Intensity, Input Drive, Wow & Flutter, Width, Ping-Pong, Dry/Wet
        { FxTapeEcho, "Worn Tape", { { 2, 0.5f }, { 3, 0.4f }, { 4, 0.5f }, { 7, 0.3f } } },
        // Phaser: Frequency, Feedback, LFO Wave, Depth, Rate, Poles, Stereo, Dry/Wet
        { FxPhaser, "Slow Sweep", { { 3, 0.6f }, { 4, 0.15f }, { 7, 0.5f } } },
        // Panner: Amount, Rate, Invert, Mono Bass, Bass Cutoff, Natural Law, Shape
        { FxPanner, "Gentle Tremolo", { { 0, 0.3f }, { 1, 0.45f } } },
        // Param EQ: LS Freq, LS Gain, P1 Freq, P1 Gain, P2 Freq, P2 Gain, P3 Freq, P3 Gain, HS Freq, HS Gain, Peak Q, Scale
        { FxParamEq, "Warmth", { { 1, 0.6f }, { 9, 0.42f } } },
        { FxParamEq, "Air", { { 9, 0.62f }, { 8, 0.7f } } },
        // Distortion
        { FxDistortion, "Warm Drive", { { 0, 0.15f }, { 1, 0.3f }, { 10, 0.6f } } },
    };
    return list;
}
