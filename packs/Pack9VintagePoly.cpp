// Preset Pack 9: Vintage Poly (the late-70s five-voice classic, Prophet-style).
//
// Built on that synth's architecture, so the pack teaches it:
//   two oscillators per voice (here Osc 1 = "B", Osc 2 = "A"), saw and / or pulse,
//   with a little analog drift, + noise -> a warm, resonant 24 dB lowpass
//   -> a filter envelope (Env 2) and an amp envelope.
//   POLY-MOD: the filter envelope and Osc B can modulate Osc A's pitch (FM) and
//   pulse width, per voice. With Osc A synced to Osc B that gives the famous sync sweep.
//   WHEEL-MOD: the mod wheel brings in the LFO on pitch, pulse width or filter.
//   Unison stacks all the voices on one note.
//
// Macros: M1 Brightness, M2 Space, M3 a character control, M4 Attack / Decay / Release.

#include "Packs.h"

#include "plugin/PresetHelpers.h"

#include <map>

using namespace presethelpers;

namespace
{
struct Voice5
{
    int waveB = kSaw, waveA = kSaw;
    float volB = 0.7f, volA = 0.7f;
    float coarseA = 0.0f, detuneA = 0.07f, widthB = 0.5f, widthA = 0.5f;
    float noise = 0.0f, drift = 0.3f;
    float cutoff = 1200.0f, reso = 0.15f, kbd = 0.5f, drive = 0.05f;
    float envAmt = 0.3f;
    float fa = 0.01f, fd = 0.6f, fs = 0.5f, fr = 0.4f;
    float a = 0.005f, d = 0.6f, s = 0.85f, r = 0.4f;
    float level = 1.0f;
};

void voice5 (B& b, const Voice5& v)
{
    osc (b, 0, 0, v.waveB, v.volB, 0.0f, 0.0f, v.widthB);
    osc (b, 0, 1, v.waveA, v.volA, v.coarseA, v.detuneA, v.widthA);
    b.set ("e1_vol", 0.6f).set ("e1_drift", v.drift).set ("e1_noise_vol", v.noise);
    filter (b, 0, kLP24, v.cutoff, v.reso, v.kbd, v.drive);
    b.set ("f1_analog", 1.0f).set ("f1_vol", v.level);
    env (b, "env2", v.fa, v.fd, v.fs, v.fr);
    b.route ("Env 2", "f1_cutoff", v.envAmt);
    env (b, "env1", v.a, v.d, v.s, v.r);
    b.set ("amp_velo", 0.3f);
}

// Poly-Mod: Osc B (a silent copy on Osc 3) frequency-modulates Osc A; the filter
// envelope can add more (and sweep Osc A's pitch).
void polyModOscB (B& b, float coarseB, float amount, float fromEnv)
{
    osc (b, 0, 2, kSaw, 0.0f, coarseB);
    b.set ("e1_mod_src", 0.0f).set ("e1_o2_fm", amount);
    if (fromEnv > 0.0f)
        b.route ("Env 2", "e1_o2_fm", fromEnv);
}

// Sync sweep: Osc A synced to Osc B, the filter envelope sweeping Osc A's pitch.
void syncSweep (B& b, float sweep)
{
    b.set ("e1_sync", 1.0f);
    b.route ("Env 2", "e1_o2_coarse", sweep);
}

// Wheel-Mod: the mod wheel brings in LFO 1.
void wheelMod (B& b, float rate, const char* dest, float amount)
{
    lfo (b, 0, LfoTriangle, rate);
    b.route ("LFO 1", dest, amount, "Mod Wheel", 1.0f);
}

void vpMacros (B& b, const char* m3, const char* m3dest, float m3amt, const char* m4, const char* m4dest, float m4amt)
{
    macros (b, "Brightness", "Space", m3, m4);
    b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", m3dest, m3amt).route ("Macro 4", m4dest, m4amt);
}
} // namespace

const std::vector<FactoryPreset>& pack9Presets()
{
    static const std::vector<FactoryPreset> list {
        // ================================================================ PADS
        { "Vintage Poly Pad 5", "Pad", "The classic late-70s pad: saw and pulse, a warm 4-pole filter, gentle drift.",
          [] (B& b) {
              Voice5 v;
              v.waveA = kSquare; v.widthA = 0.4f; v.cutoff = 900.0f; v.envAmt = 0.25f;
              v.fa = 0.9f; v.fd = 1.5f; v.fs = 0.6f; v.fr = 1.2f; v.a = 0.6f; v.d = 1.0f; v.s = 1.0f; v.r = 1.2f;
              voice5 (b, v);
              wheelMod (b, 0.5f, "e1_o2_width", 0.3f);
              hall (b, 0.35f, 0.8f, 0.7f);
              vpMacros (b, "Detune", "e1_o2_fine", 0.05f, "Attack", "env1_a", 0.35f);
          } },
        { "Poly-Mod Pad", "Pad", "Osc B gently frequency-modulates Osc A: a pad with a glassy, slightly metallic shimmer.",
          [] (B& b) {
              Voice5 v;
              v.waveA = kSaw; v.cutoff = 1000.0f; v.envAmt = 0.2f;
              v.fa = 1.0f; v.fd = 2.0f; v.fs = 0.7f; v.fr = 1.2f; v.a = 0.7f; v.d = 1.0f; v.s = 1.0f; v.r = 1.3f;
              voice5 (b, v);
              polyModOscB (b, 7.0f, 0.03f, 0.06f);
              hall (b, 0.38f, 0.8f, 0.7f);
              vpMacros (b, "Poly-Mod", "e1_o2_fm", 0.15f, "Attack", "env1_a", 0.35f);
          } },
        { "Analog Strings 5", "Strings/Brass", "Two detuned saws, filter fairly open, a slow bow: warm analog strings.",
          [] (B& b) {
              Voice5 v;
              v.detuneA = 0.12f; v.cutoff = 2800.0f; v.reso = 0.05f; v.envAmt = 0.1f; v.drift = 0.35f;
              v.fa = 0.4f; v.fd = 1.0f; v.fs = 0.8f; v.fr = 0.9f; v.a = 0.4f; v.d = 1.0f; v.s = 1.0f; v.r = 0.9f;
              voice5 (b, v);
              filter (b, 1, kHP12, 200.0f, 0.0f);
              b.set ("f_routing", 0.0f);
              wheelMod (b, 5.2f, "e1_fine", 0.008f);
              hall (b, 0.35f, 0.8f, 0.7f);
              vpMacros (b, "Detune", "e1_o2_fine", 0.05f, "Attack", "env1_a", 0.35f);
          } },
        { "Dark Sweep Pad", "Pad", "A dark pad with a resonant filter that sweeps up slowly on every chord.",
          [] (B& b) {
              Voice5 v;
              v.waveA = kSquare; v.widthA = 0.35f; v.cutoff = 350.0f; v.reso = 0.45f; v.envAmt = 0.4f;
              v.fa = 2.5f; v.fd = 2.0f; v.fs = 0.5f; v.fr = 1.2f; v.a = 0.5f; v.d = 1.0f; v.s = 1.0f; v.r = 1.2f;
              voice5 (b, v);
              hall (b, 0.38f, 0.85f, 0.75f);
              vpMacros (b, "Resonance", "f1_reso", 0.25f, "Sweep Time", "env2_a", 0.3f);
          } },
        { "Hollow Pulse Pad", "Pad", "Two narrow pulses for a hollow, reedy pad. The mod wheel adds pulse-width movement.",
          [] (B& b) {
              Voice5 v;
              v.waveB = kSquare; v.waveA = kSquare; v.widthB = 0.2f; v.widthA = 0.25f; v.cutoff = 1800.0f; v.envAmt = 0.15f;
              v.fa = 0.5f; v.fd = 1.0f; v.fs = 0.7f; v.fr = 1.0f; v.a = 0.5f; v.d = 1.0f; v.s = 1.0f; v.r = 1.1f;
              voice5 (b, v);
              wheelMod (b, 0.6f, "e1_o2_width", 0.3f);
              hall (b, 0.35f, 0.8f, 0.7f);
              vpMacros (b, "Width", "e1_o1_width", 0.2f, "Attack", "env1_a", 0.35f);
          } },
        // ================================================================ BRASS
        { "Vintage Brass 5", "Strings/Brass", "Warm, slightly unstable polysynth brass with a filter swell. Classic rock and pop.",
          [] (B& b) {
              Voice5 v;
              v.cutoff = 650.0f; v.envAmt = 0.4f; v.fa = 0.08f; v.fd = 0.6f; v.fs = 0.55f; v.fr = 0.35f;
              v.a = 0.04f; v.d = 0.6f; v.s = 0.9f; v.r = 0.35f;
              voice5 (b, v);
              b.route ("Velocity", "f1_cutoff", 0.12f);
              room (b, 0.22f);
              vpMacros (b, "Bite", "env2_d", -0.25f, "Attack", "env1_a", 0.3f);
          } },
        { "Soft Brass Pad", "Strings/Brass", "Brass that swells slowly into a pad. Warm and noble under worship.",
          [] (B& b) {
              Voice5 v;
              v.cutoff = 450.0f; v.envAmt = 0.35f; v.fa = 1.0f; v.fd = 1.5f; v.fs = 0.7f; v.fr = 1.0f;
              v.a = 0.5f; v.d = 1.0f; v.s = 1.0f; v.r = 1.0f;
              voice5 (b, v);
              hall (b, 0.38f, 0.8f, 0.7f);
              vpMacros (b, "Swell", "env2_a", 0.3f, "Attack", "env1_a", 0.35f);
          } },
        { "Poly-Mod Brass", "Strings/Brass", "Brass with a burst of poly-mod at each note: a bright, buzzy, almost vocal attack.",
          [] (B& b) {
              Voice5 v;
              v.cutoff = 700.0f; v.envAmt = 0.4f; v.fa = 0.04f; v.fd = 0.5f; v.fs = 0.5f; v.fr = 0.3f;
              v.a = 0.03f; v.d = 0.6f; v.s = 0.9f; v.r = 0.3f;
              voice5 (b, v);
              polyModOscB (b, 0.0f, 0.0f, 0.15f);
              room (b, 0.22f);
              vpMacros (b, "Poly-Mod", "e1_o2_fm", 0.1f, "Attack", "env1_a", 0.3f);
          } },
        // ================================================================ KEYS AND PLUCKS
        { "Vintage Pluck Keys", "Keys", "Plucky saw-and-pulse keys with a quick filter. The singer-songwriter synth comp.",
          [] (B& b) {
              Voice5 v;
              v.waveA = kSquare; v.widthA = 0.3f; v.cutoff = 600.0f; v.reso = 0.2f; v.envAmt = 0.45f;
              v.fa = 0.001f; v.fd = 0.35f; v.fs = 0.15f; v.fr = 0.3f; v.a = 0.001f; v.d = 1.2f; v.s = 0.3f; v.r = 0.35f;
              voice5 (b, v);
              b.set ("amp_velo", 0.5f);
              chorus (b, 0, 0.25f);
              room (b, 0.22f);
              vpMacros (b, "Chorus", "fx_a1_p8", 0.35f, "Decay", "env1_d", 0.35f);
          } },
        { "Poly-Mod Bells", "Pluck", "Osc B a fifth-plus-octave up FMs Osc A, more at the strike: clangy, glassy bells.",
          [] (B& b) {
              Voice5 v;
              v.waveB = kTri; v.volB = 0.2f; v.waveA = kTri; v.cutoff = 6000.0f; v.envAmt = 0.0f; v.detuneA = 0.0f;
              v.fa = 0.001f; v.fd = 1.0f; v.fs = 0.0f; v.fr = 0.8f; v.a = 0.001f; v.d = 2.5f; v.s = 0.0f; v.r = 2.0f;
              v.level = 1.6f;
              voice5 (b, v);
              polyModOscB (b, 19.0f, 0.08f, 0.3f);
              delay (b, 0, kDiv4D, 0.3f, 0.18f, true);
              hall (b, 0.35f, 0.85f, 0.75f);
              vpMacros (b, "Clang", "e1_o2_fm", 0.15f, "Decay", "env1_d", 0.35f);
          } },
        { "Sync Sweep Keys", "Keys", "Osc A synced to Osc B, its pitch swept by the filter envelope: the signature poly-mod sync keys.",
          [] (B& b) {
              Voice5 v;
              v.volB = 0.0f; v.volA = 0.85f; v.cutoff = 2200.0f; v.envAmt = 0.15f;
              v.fa = 0.001f; v.fd = 0.6f; v.fs = 0.15f; v.fr = 0.4f; v.a = 0.002f; v.d = 1.5f; v.s = 0.5f; v.r = 0.4f;
              v.level = 1.5f;
              voice5 (b, v);
              syncSweep (b, 0.3f);
              chorus (b, 0, 0.25f);
              room (b, 0.22f);
              vpMacros (b, "Sweep", "e1_o2_coarse", 0.15f, "Decay", "env1_d", 0.35f);
          } },
        { "Synth Harpsichord", "Keys", "Narrow pulses with a bright, fast pluck and no sustain: a synth harpsichord.",
          [] (B& b) {
              Voice5 v;
              v.waveB = kSquare; v.waveA = kSquare; v.widthB = 0.12f; v.widthA = 0.2f; v.coarseA = 12.0f; v.volA = 0.4f;
              v.cutoff = 2500.0f; v.envAmt = 0.3f; v.fa = 0.001f; v.fd = 0.25f; v.fs = 0.1f; v.fr = 0.2f;
              v.a = 0.001f; v.d = 1.0f; v.s = 0.0f; v.r = 0.25f;
              voice5 (b, v);
              b.route ("Keytrack", "env1_d", -0.3f);
              room (b, 0.25f);
              vpMacros (b, "Octave", "e1_o2_vol", 0.3f, "Decay", "env1_d", 0.35f);
          } },
        { "Glass Keys", "Keys", "Triangle and narrow pulse with a light poly-mod burst: clear, glassy electric keys.",
          [] (B& b) {
              Voice5 v;
              v.waveB = kTri; v.waveA = kSquare; v.widthA = 0.15f; v.volA = 0.35f; v.coarseA = 12.0f; v.cutoff = 3000.0f; v.envAmt = 0.2f;
              v.fa = 0.001f; v.fd = 0.6f; v.fs = 0.2f; v.fr = 0.5f; v.a = 0.001f; v.d = 2.0f; v.s = 0.2f; v.r = 0.6f;
              v.level = 1.4f;
              voice5 (b, v);
              polyModOscB (b, 12.0f, 0.0f, 0.12f);
              b.set ("amp_velo", 0.5f);
              delay (b, 0, kDiv8D, 0.25f, 0.15f, true);
              hall (b, 0.3f, 0.75f, 0.65f);
              vpMacros (b, "Glass", "e1_o2_fm", 0.1f, "Decay", "env1_d", 0.35f);
          } },
        // ================================================================ LEADS
        { "Sync Sweep Lead", "Lead", "The screaming sync lead: Osc A synced to Osc B and swept by the envelope, plus the mod wheel.",
          [] (B& b) {
              Voice5 v;
              v.volB = 0.35f; v.volA = 0.8f; v.coarseA = 12.0f; v.cutoff = 3000.0f; v.envAmt = 0.1f;
              v.fa = 0.001f; v.fd = 0.9f; v.fs = 0.3f; v.fr = 0.3f; v.a = 0.003f; v.d = 0.5f; v.s = 0.9f; v.r = 0.3f;
              v.level = 2.4f;
              voice5 (b, v);
              syncSweep (b, 0.2f);
              b.route ("Mod Wheel", "e1_o2_coarse", 0.2f);
              mono (b, false, 0.03f);
              delay (b, 0, kDiv8D, 0.3f, 0.18f, true);
              room (b, 0.22f);
              vpMacros (b, "Sweep", "env2_d", 0.3f, "Release", "env1_r", 0.3f);
          } },
        { "Unison Saw Lead", "Lead", "Five stacked voices on one note: a fat, wide saw lead with glide.",
          [] (B& b) {
              Voice5 v;
              v.cutoff = 1600.0f; v.reso = 0.2f; v.envAmt = 0.25f; v.a = 0.005f; v.d = 0.5f; v.s = 0.9f; v.r = 0.3f;
              v.level = 1.8f;
              voice5 (b, v);
              mono (b, true, 0.05f);
              uni (b, 0, 0, 5, 0.12f, 0.8f);
              wheelMod (b, 5.5f, "e1_fine", 0.008f);
              delay (b, 0, kDiv8D, 0.3f, 0.18f, true);
              room (b, 0.25f);
              vpMacros (b, "Spread", "e1_uni_detune", 0.2f, "Release", "env1_r", 0.3f);
          } },
        { "Wheel-Mod Lead", "Lead", "Mod wheel up and the LFO wobbles the filter: a talking, wah-like lead.",
          [] (B& b) {
              Voice5 v;
              v.waveA = kSquare; v.widthA = 0.4f; v.cutoff = 900.0f; v.reso = 0.4f; v.envAmt = 0.25f;
              v.a = 0.005f; v.d = 0.5f; v.s = 0.9f; v.r = 0.3f; v.level = 2.2f;
              voice5 (b, v);
              mono (b, true, 0.04f);
              wheelMod (b, 4.0f, "f1_cutoff", 0.2f);
              delay (b, 0, kDiv8D, 0.3f, 0.18f, true);
              room (b, 0.22f);
              vpMacros (b, "Wobble Speed", "lfo1_rate", 0.2f, "Release", "env1_r", 0.3f);
          } },
        // ================================================================ BASS AND ARP
        { "Vintage Poly Bass", "Bass", "Saw and pulse an octave down, punchy filter envelope: a warm, round synth bass.",
          [] (B& b) {
              Voice5 v;
              v.waveA = kSquare; v.coarseA = -12.0f; v.cutoff = 300.0f; v.reso = 0.2f; v.kbd = 0.3f; v.envAmt = 0.4f;
              v.fa = 0.001f; v.fd = 0.3f; v.fs = 0.25f; v.fr = 0.15f; v.a = 0.002f; v.d = 0.5f; v.s = 0.8f; v.r = 0.12f;
              v.level = 2.4f;
              voice5 (b, v);
              mono (b, false, 0.0f);
              vpMacros (b, "Punch", "env2_d", 0.3f, "Release", "env1_r", 0.3f);
          } },
        { "Unison Bass 5", "Bass", "All voices stacked on one note an octave down: a huge, thick bass.",
          [] (B& b) {
              Voice5 v;
              v.coarseA = -12.0f; v.cutoff = 350.0f; v.reso = 0.15f; v.kbd = 0.3f; v.envAmt = 0.35f;
              v.fa = 0.001f; v.fd = 0.35f; v.fs = 0.3f; v.fr = 0.15f; v.a = 0.002f; v.d = 0.5f; v.s = 0.8f; v.r = 0.12f;
              v.level = 2.2f;
              voice5 (b, v);
              mono (b, false, 0.0f);
              uni (b, 0, 0, 4, 0.06f, 0.3f);
              vpMacros (b, "Spread", "e1_uni_detune", 0.15f, "Release", "env1_r", 0.3f);
          } },
        { "Vintage Arp", "Sequence", "Pluck arpeggio in 16ths over two octaves with an echo. Hold a chord; follows the tempo.",
          [] (B& b) {
              Voice5 v;
              v.waveA = kSquare; v.widthA = 0.3f; v.cutoff = 800.0f; v.reso = 0.25f; v.envAmt = 0.4f;
              v.fa = 0.001f; v.fd = 0.18f; v.fs = 0.1f; v.fr = 0.15f; v.a = 0.001f; v.d = 0.25f; v.s = 0.15f; v.r = 0.15f;
              v.level = 1.8f;
              voice5 (b, v);
              b.set ("seq_on", 1.0f).set ("seq_mode", 0.0f).set ("seq_arp_order", (float) ArpUp).set ("seq_arp_octaves", 2.0f)
                  .set ("seq_rate", (float) kDiv16);
              b.pattern (gatePattern ({ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, 0.5f));
              delay (b, 0, kDiv8D, 0.3f, 0.2f, true);
              room (b, 0.22f);
              vpMacros (b, "Resonance", "f1_reso", 0.25f, "Decay", "env1_d", 0.35f);
          } },
        { "Wheel-Mod Strings", "Strings/Brass", "Analog strings where the mod wheel adds a slow pulse-width chorus: still to shimmering.",
          [] (B& b) {
              Voice5 v;
              v.waveA = kSquare; v.widthA = 0.5f; v.cutoff = 2500.0f; v.reso = 0.05f; v.envAmt = 0.1f; v.drift = 0.3f;
              v.fa = 0.4f; v.fd = 1.0f; v.fs = 0.8f; v.fr = 0.9f; v.a = 0.4f; v.d = 1.0f; v.s = 1.0f; v.r = 1.0f;
              voice5 (b, v);
              filter (b, 1, kHP12, 220.0f, 0.0f);
              b.set ("f_routing", 0.0f);
              wheelMod (b, 0.8f, "e1_o2_width", 0.35f);
              hall (b, 0.35f, 0.8f, 0.7f);
              vpMacros (b, "Detune", "e1_o2_fine", 0.05f, "Attack", "env1_a", 0.35f);
          } },
    };
    return list;
}

juce::StringArray pack9Tags (const juce::String& presetName)
{
    static const std::map<juce::String, const char*> tags {
        { "Vintage Poly Pad 5", "Vintage Poly, Pad, Worship" }, { "Poly-Mod Pad", "Vintage Poly, Pad, Poly-Mod" },
        { "Analog Strings 5", "Vintage Poly, Strings" }, { "Dark Sweep Pad", "Vintage Poly, Pad, Evolving" },
        { "Hollow Pulse Pad", "Vintage Poly, Pad, PWM" },
        { "Vintage Brass 5", "Vintage Poly, Brass, Synth Brass" }, { "Soft Brass Pad", "Vintage Poly, Brass, Pad, Worship" },
        { "Poly-Mod Brass", "Vintage Poly, Brass, Poly-Mod" },
        { "Vintage Pluck Keys", "Vintage Poly, Synth Keys, Pluck" }, { "Poly-Mod Bells", "Vintage Poly, Bells, Poly-Mod" },
        { "Sync Sweep Keys", "Vintage Poly, Synth Keys, Sync, Poly-Mod" }, { "Synth Harpsichord", "Vintage Poly, Synth Keys" },
        { "Glass Keys", "Vintage Poly, Synth Keys, Poly-Mod" },
        { "Sync Sweep Lead", "Vintage Poly, Lead, Sync" }, { "Unison Saw Lead", "Vintage Poly, Lead, Unison" },
        { "Wheel-Mod Lead", "Vintage Poly, Lead, Mono" },
        { "Vintage Poly Bass", "Vintage Poly, Bass" }, { "Unison Bass 5", "Vintage Poly, Bass, Unison" },
        { "Vintage Arp", "Vintage Poly, Arp" }, { "Wheel-Mod Strings", "Vintage Poly, Strings, PWM" },
    };
    const auto it = tags.find (presetName);
    return it == tags.end() ? juce::StringArray() : PresetManager::parseTags (it->second);
}
