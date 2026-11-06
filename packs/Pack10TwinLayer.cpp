// Preset Pack 10: Twin Layer (the late-70s two-channel polyphonic flagship with
// aftertouch, CS-80-style).
//
// Built on that synth's architecture, so the pack teaches it:
//   every voice is TWO complete layers ("channels"): Engine 1 through Filter 1 and
//   Engine 2 through Filter 2 (parallel), each with its own oscillator mix and its
//   own filter envelope (Env 2 and Env 3). The two layers are usually tuned or
//   shaped differently and blend into one instrument.
//   TOUCH: velocity sets brightness; AFTERTOUCH (pressing harder into a held key)
//   opens the filters further and adds vibrato, the famous expressive "growl".
//   No aftertouch on your keyboard? Macro 3 (Growl / Touch) does the same thing.
//   Ring modulation for bells, and a wide pitch bend on the leads (like the ribbon).
//
// Macros: M1 Brightness, M2 Space, M3 Growl / Touch (= aftertouch), M4 Attack / Decay / Release.

#include "Packs.h"

#include "plugin/PresetHelpers.h"

#include <map>

using namespace presethelpers;

namespace
{
struct Layer
{
    int wave1 = kSaw, wave2 = kSquare;
    float vol1 = 0.8f, vol2 = 0.0f;
    float coarse = 0.0f, fine = 0.0f, width = 0.5f;
    float cutoff = 1200.0f, reso = 0.15f, kbd = 0.5f;
    float envAmt = 0.3f, fa = 0.05f, fd = 0.8f, fs = 0.6f, fr = 0.6f;
    float level = 0.5f;
};

// One channel: engine `e` through filter `e` (12 dB, Analog), its own envelope (Env 2 / Env 3).
void channel (B& b, int e, const Layer& l)
{
    const juce::String E = "e" + juce::String (e + 1) + "_";
    const juce::String F = "f" + juce::String (e + 1) + "_";
    engineOn (b, e, l.level);
    osc (b, e, 0, l.wave1, l.vol1);
    osc (b, e, 1, l.wave2, l.vol2, 0.0f, 0.0f, l.width);
    b.set (E + "coarse", l.coarse).set (E + "fine", l.fine).set (E + "drift", 0.15f).set (E + "fmix", (float) e);
    filter (b, e, kLP12, l.cutoff, l.reso, l.kbd);
    b.set (F + "analog", 1.0f);
    const juce::String envName = e == 0 ? "env2" : "env3";
    env (b, envName, l.fa, l.fd, l.fs, l.fr);
    b.route (e == 0 ? "Env 2" : "Env 3", F + "cutoff", l.envAmt);
    if (e == 1)
        b.set ("env3_link", 0.0f);
    b.set ("f_routing", 1.0f); // parallel: each layer through its own filter
}

// Touch: velocity brightens; aftertouch (or Macro 3) opens both filters and adds vibrato.
void touch (B& b, float velocity, float growl, float vibrato)
{
    b.route ("Velocity", "f1_cutoff", velocity).route ("Velocity", "f2_cutoff", velocity);
    for (const char* src : { "Aftertouch", "Macro 3" })
    {
        b.route (src, "f1_cutoff", growl).route (src, "f2_cutoff", growl);
        b.route ("LFO 1", "e1_fine", vibrato, src, 1.0f).route ("LFO 1", "e2_fine", vibrato, src, 1.0f);
    }
    lfo (b, 0, LfoSine, 5.5f);
}

void twinMacros (B& b, const char* m3, const char* m4, const char* m4dest, float m4amt)
{
    macros (b, "Brightness", "Space", m3, m4);
    b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 1", "f2_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f)
        .route ("Macro 4", m4dest, m4amt);
}

// Ring-mod voice on engine `e`: a sine (Basic Shapes, position 0) ring-modulated by a sine at `ratio`.
void ringVoice (B& b, int e, float ratio, float amount, float vol)
{
    const juce::String E = "e" + juce::String (e + 1) + "_";
    wavetable (b, e, "Basic Shapes", 0.0f, vol);
    b.set (E + "wt_mod_wave", 0.0f).set (E + "wt_mod_tune_mode", 0.0f).set (E + "wt_mod_ratio", ratio)
        .set (E + "wt_fm_type", 2.0f).set (E + "wt_fm", amount).set (E + "fmix", (float) e);
}
} // namespace

const std::vector<FactoryPreset>& pack10Presets()
{
    static const std::vector<FactoryPreset> list {
        // ================================================================ BRASS
        { "Noir Brass", "Strings/Brass", "Dark, cinematic layered brass: press into the keys and it growls open with vibrato.",
          [] (B& b) {
              Layer a; a.cutoff = 380.0f; a.reso = 0.2f; a.envAmt = 0.28f; a.fa = 0.15f; a.fd = 1.0f; a.fs = 0.5f;
              Layer c; c.wave1 = kSaw; c.coarse = 0.0f; c.fine = 0.08f; c.cutoff = 280.0f; c.envAmt = 0.25f; c.fa = 0.4f; c.fd = 1.5f; c.fs = 0.6f; c.level = 0.4f;
              channel (b, 0, a);
              channel (b, 1, c);
              touch (b, 0.15f, 0.25f, 0.012f);
              env (b, "env1", 0.08f, 1.0f, 1.0f, 1.2f);
              b.set ("amp_velo", 0.4f);
              hall (b, 0.45f, 0.9f, 0.85f);
              twinMacros (b, "Growl", "Release", "env1_r", 0.3f);
          } },
        { "Touch Brass", "Strings/Brass", "Bright brass stabs that open up when you play harder or press into the keys.",
          [] (B& b) {
              Layer a; a.cutoff = 800.0f; a.envAmt = 0.4f; a.fa = 0.04f; a.fd = 0.5f; a.fs = 0.5f;
              Layer c; c.wave1 = kSquare; c.vol1 = 0.6f; c.width = 0.4f; c.coarse = 12.0f; c.cutoff = 1200.0f; c.envAmt = 0.3f; c.fa = 0.02f; c.fd = 0.3f; c.fs = 0.3f; c.level = 0.25f;
              channel (b, 0, a);
              channel (b, 1, c);
              touch (b, 0.2f, 0.2f, 0.008f);
              env (b, "env1", 0.03f, 0.6f, 0.9f, 0.4f);
              b.set ("amp_velo", 0.5f);
              room (b, 0.25f);
              twinMacros (b, "Growl", "Attack", "env1_a", 0.3f);
          } },
        { "Brass Swell 80", "Strings/Brass", "Two brass layers swelling at different speeds: the lower one first, the bright one later.",
          [] (B& b) {
              Layer a; a.cutoff = 500.0f; a.envAmt = 0.35f; a.fa = 0.6f; a.fd = 1.0f; a.fs = 0.8f;
              Layer c; c.coarse = 12.0f; c.fine = -0.06f; c.cutoff = 700.0f; c.envAmt = 0.35f; c.fa = 1.8f; c.fd = 1.5f; c.fs = 0.8f; c.level = 0.35f;
              channel (b, 0, a);
              channel (b, 1, c);
              touch (b, 0.1f, 0.2f, 0.01f);
              env (b, "env1", 0.5f, 1.0f, 1.0f, 1.0f);
              hall (b, 0.38f, 0.85f, 0.75f);
              twinMacros (b, "Growl", "Attack", "env1_a", 0.35f);
          } },
        { "Ring Mod Brass", "Strings/Brass", "Brass with a ring-modulated layer on top: a metallic, otherworldly edge.",
          [] (B& b) {
              Layer a; a.cutoff = 700.0f; a.envAmt = 0.35f; a.fa = 0.06f; a.fd = 0.8f; a.fs = 0.6f;
              channel (b, 0, a);
              ringVoice (b, 1, 2.5f, 0.8f, 0.25f);
              filter (b, 1, kLP12, 3000.0f, 0.0f, 0.4f);
              b.set ("f_routing", 1.0f);
              touch (b, 0.15f, 0.2f, 0.01f);
              env (b, "env1", 0.05f, 0.8f, 0.9f, 0.6f);
              hall (b, 0.38f, 0.85f, 0.75f);
              twinMacros (b, "Growl", "Release", "env1_r", 0.3f);
          } },
        { "Soft Horn Layer", "Strings/Brass", "Mellow horns: a soft saw layer under a sine layer. Warm and noble under worship.",
          [] (B& b) {
              Layer a; a.cutoff = 450.0f; a.reso = 0.05f; a.envAmt = 0.25f; a.fa = 0.2f; a.fd = 1.0f; a.fs = 0.7f;
              Layer c; c.wave1 = kSine; c.vol1 = 0.8f; c.cutoff = 3000.0f; c.envAmt = 0.0f; c.level = 0.3f;
              channel (b, 0, a);
              channel (b, 1, c);
              touch (b, 0.1f, 0.15f, 0.008f);
              env (b, "env1", 0.15f, 1.0f, 1.0f, 0.9f);
              b.set ("amp_velo", 0.35f);
              hall (b, 0.42f, 0.85f, 0.8f);
              twinMacros (b, "Growl", "Attack", "env1_a", 0.35f);
          } },
        // ================================================================ PADS AND STRINGS
        { "Twin Layer Strings", "Strings/Brass", "Two saw layers a few cents apart, each with its own slow filter: rich, moving strings.",
          [] (B& b) {
              Layer a; a.cutoff = 2200.0f; a.envAmt = 0.1f; a.fa = 0.5f; a.fd = 1.5f; a.fs = 0.8f;
              Layer c; c.fine = 0.1f; c.cutoff = 3000.0f; c.envAmt = 0.15f; c.fa = 1.2f; c.fd = 2.0f; c.fs = 0.7f; c.level = 0.45f;
              channel (b, 0, a);
              channel (b, 1, c);
              touch (b, 0.1f, 0.15f, 0.01f);
              env (b, "env1", 0.4f, 1.0f, 1.0f, 1.0f);
              hall (b, 0.38f, 0.85f, 0.75f);
              twinMacros (b, "Touch", "Attack", "env1_a", 0.35f);
          } },
        { "Noir Pad", "Pad", "Very dark, slow pad with a long tail: rain-on-the-window atmosphere.",
          [] (B& b) {
              Layer a; a.cutoff = 350.0f; a.reso = 0.25f; a.envAmt = 0.25f; a.fa = 2.0f; a.fd = 3.0f; a.fs = 0.5f; a.fr = 3.0f;
              Layer c; c.wave1 = kSquare; c.width = 0.3f; c.coarse = 12.0f; c.cutoff = 600.0f; c.envAmt = 0.2f; c.fa = 3.0f; c.fd = 3.0f; c.fs = 0.4f; c.fr = 3.0f; c.level = 0.25f;
              channel (b, 0, a);
              channel (b, 1, c);
              touch (b, 0.05f, 0.2f, 0.008f);
              env (b, "env1", 1.5f, 1.0f, 1.0f, 3.0f);
              hall (b, 0.5f, 0.95f, 0.9f);
              twinMacros (b, "Growl", "Attack", "env1_a", 0.35f);
          } },
        { "Crystal Layer Pad", "Pad", "A warm saw layer under a bright, narrow-pulse layer an octave up. Shimmering and clear.",
          [] (B& b) {
              Layer a; a.cutoff = 900.0f; a.envAmt = 0.15f; a.fa = 0.8f; a.fd = 1.5f; a.fs = 0.7f;
              Layer c; c.wave1 = kSquare; c.width = 0.15f; c.coarse = 12.0f; c.cutoff = 5000.0f; c.envAmt = 0.0f; c.level = 0.3f;
              channel (b, 0, a);
              channel (b, 1, c);
              touch (b, 0.1f, 0.15f, 0.008f);
              env (b, "env1", 0.6f, 1.0f, 1.0f, 1.5f);
              delay (b, 0, kDiv4D, 0.3f, 0.15f, true);
              hall (b, 0.4f, 0.85f, 0.8f);
              twinMacros (b, "Touch", "Attack", "env1_a", 0.35f);
          } },
        { "Breathing Strings", "Strings/Brass", "Strings you play with pressure: press into the chord and it swells and opens up.",
          [] (B& b) {
              Layer a; a.cutoff = 1200.0f; a.envAmt = 0.1f; a.fa = 0.4f; a.fd = 1.0f; a.fs = 0.8f;
              Layer c; c.fine = 0.09f; c.cutoff = 1500.0f; c.envAmt = 0.1f; c.fa = 0.6f; c.fd = 1.2f; c.fs = 0.8f; c.level = 0.4f;
              channel (b, 0, a);
              channel (b, 1, c);
              touch (b, 0.05f, 0.3f, 0.01f);
              b.route ("Aftertouch", "e1_vol", 0.2f).route ("Macro 3", "e1_vol", 0.2f);
              b.set ("e1_vol", 0.35f);
              env (b, "env1", 0.5f, 1.0f, 1.0f, 1.0f);
              hall (b, 0.38f, 0.85f, 0.75f);
              twinMacros (b, "Pressure", "Attack", "env1_a", 0.35f);
          } },
        { "Cathedral Pad", "Pad", "Two soft layers an octave apart in a huge shimmering space. For worship and prayer.",
          [] (B& b) {
              Layer a; a.wave1 = kSaw; a.vol1 = 0.6f; a.wave2 = kSquare; a.vol2 = 0.4f; a.cutoff = 800.0f; a.envAmt = 0.15f; a.fa = 1.0f; a.fd = 2.0f; a.fs = 0.8f;
              Layer c; c.wave1 = kSine; c.coarse = 12.0f; c.cutoff = 3000.0f; c.envAmt = 0.0f; c.level = 0.25f;
              channel (b, 0, a);
              channel (b, 1, c);
              touch (b, 0.05f, 0.15f, 0.006f);
              env (b, "env1", 1.2f, 1.0f, 1.0f, 2.5f);
              shimmer (b, 0.45f, 0.45f, 0.85f);
              twinMacros (b, "Touch", "Attack", "env1_a", 0.35f);
          } },
        // ================================================================ KEYS AND BELLS
        { "Ring Mod Bells", "Pluck", "Ring-modulated sines: bright, inharmonic, spacey bells.",
          [] (B& b) {
              ringVoice (b, 0, 3.5f, 0.9f, 0.6f);
              ringVoice (b, 1, 1.41f, 0.6f, 0.3f);
              b.set ("e2_coarse", 12.0f);
              filter (b, 0, kLP12, 8000.0f, 0.0f, 0.3f);
              filter (b, 1, kLP12, 8000.0f, 0.0f, 0.3f);
              b.set ("f_routing", 1.0f).set ("f1_vol", 1.5f).set ("f2_vol", 1.5f);
              env (b, "env1", 0.001f, 2.5f, 0.0f, 2.0f);
              b.set ("amp_velo", 0.5f);
              delay (b, 0, kDiv4D, 0.3f, 0.2f, true);
              hall (b, 0.4f, 0.85f, 0.8f);
              macros (b, "Ring", "Space", "Inharmonic", "Decay");
              b.route ("Macro 1", "e1_wt_fm", 0.1f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "e2_vol", 0.3f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "Layer Electric Piano", "Keys", "A struck sine layer plus a short, bright pulse layer: the flagship's own electric piano.",
          [] (B& b) {
              Layer a; a.wave1 = kSine; a.wave2 = kTri; a.vol2 = 0.3f; a.cutoff = 3000.0f; a.envAmt = 0.0f; a.level = 0.6f;
              Layer c; c.wave1 = kSquare; c.width = 0.2f; c.coarse = 12.0f; c.cutoff = 1500.0f; c.envAmt = 0.4f; c.fa = 0.001f; c.fd = 0.25f; c.fs = 0.0f; c.fr = 0.2f; c.level = 0.25f;
              channel (b, 0, a);
              channel (b, 1, c);
              env (b, "env1", 0.001f, 3.0f, 0.0f, 0.6f);
              b.route ("Keytrack", "env1_d", -0.3f);
              b.route ("Velocity", "f2_cutoff", 0.25f);
              b.set ("amp_velo", 0.6f);
              chorus (b, 0, 0.3f);
              room (b, 0.22f);
              macros (b, "Brightness", "Space", "Tine", "Decay");
              b.route ("Macro 1", "f2_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "e2_vol", 0.25f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "Touch Clav", "Keys", "Bright pulse clav where pressing into a key opens the filter: expressive funk.",
          [] (B& b) {
              Layer a; a.wave1 = kSquare; a.width = 0.15f; a.cutoff = 900.0f; a.reso = 0.25f; a.envAmt = 0.4f; a.fa = 0.001f; a.fd = 0.2f; a.fs = 0.2f; a.fr = 0.1f;
              Layer c; c.wave1 = kSaw; c.coarse = 12.0f; c.cutoff = 1500.0f; c.envAmt = 0.3f; c.fa = 0.001f; c.fd = 0.15f; c.fs = 0.0f; c.level = 0.2f;
              channel (b, 0, a);
              channel (b, 1, c);
              touch (b, 0.25f, 0.25f, 0.0f);
              env (b, "env1", 0.001f, 1.5f, 0.35f, 0.06f);
              b.set ("amp_velo", 0.6f).set ("f1_vol", 1.4f);
              room (b, 0.15f);
              twinMacros (b, "Touch", "Decay", "env1_d", 0.35f);
          } },
        { "Glass Harp", "Pad", "A sine layer and a soft triangle layer with a slow bow: like wet fingers on glasses.",
          [] (B& b) {
              Layer a; a.wave1 = kSine; a.cutoff = 4000.0f; a.envAmt = 0.0f; a.level = 0.6f;
              Layer c; c.wave1 = kTri; c.coarse = 12.0f; c.fine = 0.06f; c.cutoff = 4000.0f; c.envAmt = 0.0f; c.level = 0.3f;
              channel (b, 0, a);
              channel (b, 1, c);
              touch (b, 0.0f, 0.1f, 0.006f);
              env (b, "env1", 0.35f, 1.0f, 1.0f, 2.0f);
              b.set ("f1_vol", 1.5f).set ("f2_vol", 1.5f);
              hall (b, 0.45f, 0.9f, 0.85f);
              twinMacros (b, "Touch", "Attack", "env1_a", 0.35f);
          } },
        // ================================================================ LEADS
        { "Ribbon Lead", "Lead", "Saw lead with a 7-semitone pitch bend, like sliding along the ribbon, and aftertouch vibrato.",
          [] (B& b) {
              Layer a; a.cutoff = 1500.0f; a.reso = 0.2f; a.envAmt = 0.25f; a.level = 0.6f;
              Layer c; c.wave1 = kSquare; c.width = 0.4f; c.fine = 0.06f; c.cutoff = 1200.0f; c.envAmt = 0.2f; c.level = 0.4f;
              channel (b, 0, a);
              channel (b, 1, c);
              touch (b, 0.15f, 0.2f, 0.015f);
              mono (b, true, 0.05f);
              b.set ("bend_up", 7.0f).set ("bend_down", 7.0f);
              env (b, "env1", 0.01f, 0.5f, 0.9f, 0.4f);
              b.set ("f1_vol", 1.8f).set ("f2_vol", 1.8f);
              delay (b, 0, kDiv8D, 0.3f, 0.2f, true);
              hall (b, 0.32f, 0.8f, 0.7f);
              twinMacros (b, "Growl", "Release", "env1_r", 0.3f);
          } },
        { "Touch Lead", "Lead", "Expressive lead: softer when you play gently, growling and singing when you press in.",
          [] (B& b) {
              Layer a; a.cutoff = 700.0f; a.reso = 0.25f; a.envAmt = 0.2f; a.level = 0.6f;
              Layer c; c.coarse = 12.0f; c.fine = -0.05f; c.cutoff = 600.0f; c.envAmt = 0.2f; c.level = 0.3f;
              channel (b, 0, a);
              channel (b, 1, c);
              touch (b, 0.25f, 0.35f, 0.015f);
              mono (b, true, 0.04f);
              env (b, "env1", 0.02f, 0.5f, 0.9f, 0.35f);
              b.set ("f1_vol", 2.0f).set ("f2_vol", 2.0f).set ("amp_velo", 0.5f);
              delay (b, 0, kDiv8D, 0.3f, 0.18f, true);
              hall (b, 0.3f, 0.8f, 0.7f);
              twinMacros (b, "Growl", "Release", "env1_r", 0.3f);
          } },
        { "Soaring Sine Lead", "Lead", "A pure sine layer over a soft pulse, with a slow vibrato and a long echo. Floats over everything.",
          [] (B& b) {
              Layer a; a.wave1 = kSine; a.cutoff = 4000.0f; a.envAmt = 0.0f; a.level = 0.7f;
              Layer c; c.wave1 = kSquare; c.width = 0.3f; c.vol1 = 0.5f; c.cutoff = 900.0f; c.envAmt = 0.15f; c.level = 0.3f;
              channel (b, 0, a);
              channel (b, 1, c);
              touch (b, 0.1f, 0.15f, 0.012f);
              lfo (b, 0, LfoSine, 5.2f);
              b.set ("lfo1_fade", 0.6f).set ("lfo1_retrig", 2.0f);
              b.route ("LFO 1", "e1_fine", 0.006f).route ("LFO 1", "e2_fine", 0.006f);
              mono (b, true, 0.08f);
              env (b, "env1", 0.04f, 0.5f, 0.9f, 0.6f);
              b.set ("f1_vol", 2.4f).set ("f2_vol", 2.0f);
              delay (b, 0, kDiv4D, 0.35f, 0.25f, true);
              hall (b, 0.38f, 0.85f, 0.8f);
              twinMacros (b, "Growl", "Release", "env1_r", 0.3f);
          } },
        { "Sub-Osc Trill Lead", "Lead", "Hold a note and press in: a square-wave modulator makes it trill up a whole tone.",
          [] (B& b) {
              Layer a; a.cutoff = 1300.0f; a.reso = 0.2f; a.envAmt = 0.25f; a.level = 0.6f;
              Layer c; c.wave1 = kSquare; c.width = 0.35f; c.coarse = 12.0f; c.cutoff = 1500.0f; c.envAmt = 0.15f; c.level = 0.25f;
              channel (b, 0, a);
              channel (b, 1, c);
              mono (b, true, 0.03f);
              lfo (b, 1, LfoSquare, 8.0f, false);
              for (const char* src : { "Aftertouch", "Macro 3" })
                  b.route ("LFO 2", "e1_coarse", 2.0f / 120.0f, src, 1.0f).route ("LFO 2", "e2_coarse", 2.0f / 120.0f, src, 1.0f);
              env (b, "env1", 0.01f, 0.5f, 0.9f, 0.35f);
              b.set ("f1_vol", 1.8f).set ("f2_vol", 1.8f);
              delay (b, 0, kDiv8D, 0.3f, 0.18f, true);
              room (b, 0.25f);
              twinMacros (b, "Trill", "Release", "env1_r", 0.3f);
          } },
        // ================================================================ BASS AND ARP
        { "Twin Bass", "Bass", "Two bass layers: a round sine-and-saw low end and a short, bright attack layer on top.",
          [] (B& b) {
              Layer a; a.wave1 = kSaw; a.wave2 = kSine; a.vol1 = 0.5f; a.vol2 = 0.0f; a.cutoff = 300.0f; a.kbd = 0.3f; a.envAmt = 0.25f;
              a.fa = 0.001f; a.fd = 0.4f; a.fs = 0.4f; a.level = 0.7f;
              Layer c; c.wave1 = kSquare; c.width = 0.3f; c.cutoff = 800.0f; c.envAmt = 0.4f; c.fa = 0.001f; c.fd = 0.12f; c.fs = 0.0f; c.fr = 0.1f; c.level = 0.25f;
              channel (b, 0, a);
              osc (b, 0, 2, kSine, 0.6f, -12.0f);
              channel (b, 1, c);
              mono (b, false, 0.0f);
              env (b, "env1", 0.002f, 0.6f, 0.8f, 0.12f);
              b.set ("f1_vol", 2.2f).set ("f2_vol", 2.2f);
              macros (b, "Brightness", "Space", "Attack Layer", "Release");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.4f).route ("Macro 3", "e2_vol", 0.25f).route ("Macro 4", "env1_r", 0.3f);
          } },
        { "Layer Arp", "Sequence", "A two-layer pluck arpeggiated in 16ths: warm saws below, glassy pulse on top. Hold a chord.",
          [] (B& b) {
              Layer a; a.cutoff = 700.0f; a.reso = 0.2f; a.envAmt = 0.4f; a.fa = 0.001f; a.fd = 0.2f; a.fs = 0.1f; a.fr = 0.15f;
              Layer c; c.wave1 = kSquare; c.width = 0.2f; c.coarse = 12.0f; c.cutoff = 2500.0f; c.envAmt = 0.2f; c.fa = 0.001f; c.fd = 0.1f; c.fs = 0.0f; c.level = 0.25f;
              channel (b, 0, a);
              channel (b, 1, c);
              env (b, "env1", 0.001f, 0.3f, 0.15f, 0.15f);
              b.set ("f1_vol", 1.8f).set ("f2_vol", 1.8f);
              b.set ("seq_on", 1.0f).set ("seq_mode", 0.0f).set ("seq_arp_order", (float) ArpUpDownIncl).set ("seq_arp_octaves", 2.0f)
                  .set ("seq_rate", (float) kDiv16);
              b.pattern (gatePattern ({ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, 0.5f));
              delay (b, 0, kDiv8D, 0.3f, 0.2f, true);
              hall (b, 0.3f, 0.8f, 0.7f);
              macros (b, "Brightness", "Space", "Glass Layer", "Decay");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "e2_vol", 0.25f).route ("Macro 4", "env1_d", 0.35f);
          } },
    };
    return list;
}

juce::StringArray pack10Tags (const juce::String& presetName)
{
    static const std::map<juce::String, const char*> tags {
        { "Noir Brass", "Twin Layer, Brass, Cinematic, Expressive" }, { "Touch Brass", "Twin Layer, Brass, Expressive" },
        { "Brass Swell 80", "Twin Layer, Brass, Swell" }, { "Ring Mod Brass", "Twin Layer, Brass, Ring Mod" },
        { "Soft Horn Layer", "Twin Layer, Brass, Worship" },
        { "Twin Layer Strings", "Twin Layer, Strings" }, { "Noir Pad", "Twin Layer, Pad, Dark, Cinematic" },
        { "Crystal Layer Pad", "Twin Layer, Pad, Bright" }, { "Breathing Strings", "Twin Layer, Strings, Expressive" },
        { "Cathedral Pad", "Twin Layer, Pad, Worship, Shimmer" },
        { "Ring Mod Bells", "Twin Layer, Bells, Ring Mod" }, { "Layer Electric Piano", "Twin Layer, Electric Piano" },
        { "Touch Clav", "Twin Layer, Clav, Funk, Expressive" }, { "Glass Harp", "Twin Layer, Pad, Bells" },
        { "Ribbon Lead", "Twin Layer, Lead, Expressive" }, { "Touch Lead", "Twin Layer, Lead, Expressive" },
        { "Soaring Sine Lead", "Twin Layer, Lead, Worship" }, { "Sub-Osc Trill Lead", "Twin Layer, Lead" },
        { "Twin Bass", "Twin Layer, Bass" }, { "Layer Arp", "Twin Layer, Arp" },
    };
    const auto it = tags.find (presetName);
    return it == tags.end() ? juce::StringArray() : PresetManager::parseTags (it->second);
}
