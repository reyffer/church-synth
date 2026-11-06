// Preset Pack 7: Grand Poly (the 80s 8-voice flagship polysynth, Jupiter-style).
//
// Built on that synth's architecture, so the pack teaches it:
//   TWO oscillators per voice (VCO 1 and VCO 2), detuned and drifting: the width
//   comes from the oscillators themselves, not from a chorus.
//   Cross-mod (VCO 2 frequency-modulating VCO 1) and hard sync (VCO 2 locked to VCO 1).
//   A non-resonant highpass, then a 4-pole lowpass switchable to 2-pole (OTA model).
//   TWO envelopes: Env 2 for the filter / pitch / pulse width, Env Amp for the volume.
//   Unison (all voices stacked on one note) and Dual (two layered patches).
//
// Macros: M1 Brightness, M2 Space, M3 a character control, M4 Attack / Decay / Release.

#include "Packs.h"

#include "plugin/PresetHelpers.h"

#include <map>

using namespace presethelpers;

namespace
{
constexpr int kOta = 11;
constexpr int kHP6 = 7;

struct Vcos
{
    int wave1 = kSaw, wave2 = kSaw;
    float vol1 = 0.8f, vol2 = 0.7f;
    float coarse2 = 0.0f, detune = 0.08f, width2 = 0.5f;
    float drift = 0.25f;
    float hpf = 0.0f;        // Hz, 0 = off
    bool pole2 = false;      // 12 dB instead of 24 dB
    float cutoff = 1500.0f, reso = 0.1f, kbd = 0.5f;
    float envAmt = 0.3f;     // Env 2 -> cutoff
    float fa = 0.01f, fd = 0.6f, fs = 0.6f, fr = 0.4f; // Env 2 (filter)
    float a = 0.005f, d = 0.6f, s = 0.85f, r = 0.4f;   // Env Amp
};

void vcos (B& b, const Vcos& v)
{
    osc (b, 0, 0, v.wave1, v.vol1);
    osc (b, 0, 1, v.wave2, v.vol2, v.coarse2, v.detune, v.width2);
    b.set ("e1_vol", 0.6f).set ("e1_drift", v.drift);
    model (b, 0, kOta, v.pole2 ? 1 : 0, v.cutoff, v.reso, v.kbd);
    if (v.hpf > 0.0f)
    {
        filter (b, 1, kHP6, v.hpf, 0.0f);
        b.set ("f_routing", 0.0f);
    }
    env (b, "env2", v.fa, v.fd, v.fs, v.fr);
    b.route ("Env 2", "f1_cutoff", v.envAmt);
    env (b, "env1", v.a, v.d, v.s, v.r);
    b.set ("amp_velo", 0.3f);
}

// Cross-mod: VCO 2's pitch (on Osc 3, silent) frequency-modulates VCO 1.
void crossMod (B& b, float coarse2, float amount)
{
    osc (b, 0, 2, kSine, 0.0f, coarse2);
    b.set ("e1_mod_src", 0.0f).set ("e1_o1_fm", amount);
}

// Unison: all voices on one note, spread and detuned.
void stackVoices (B& b, int voices, float detune, bool legato, float glide)
{
    mono (b, legato, glide);
    uni (b, 0, 0, voices, detune, 0.8f);
}

void gpMacros (B& b, const char* m3, const char* m3dest, float m3amt, const char* m4, const char* m4dest, float m4amt)
{
    macros (b, "Brightness", "Space", m3, m4);
    b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", m3dest, m3amt).route ("Macro 4", m4dest, m4amt);
}
} // namespace

const std::vector<FactoryPreset>& pack7Presets()
{
    static const std::vector<FactoryPreset> list {
        // ================================================================ PADS AND STRINGS
        { "Grand Strings", "Strings/Brass", "Two detuned saws per voice and no chorus: wide, rich strings from the oscillators alone.",
          [] (B& b) {
              Vcos v;
              v.detune = 0.12f; v.drift = 0.3f; v.hpf = 200.0f; v.cutoff = 3500.0f; v.envAmt = 0.1f;
              v.fa = 0.4f; v.fd = 1.0f; v.fs = 0.8f; v.fr = 0.8f; v.a = 0.4f; v.d = 1.0f; v.s = 1.0f; v.r = 0.9f;
              vcos (b, v);
              hall (b, 0.35f, 0.8f, 0.7f);
              gpMacros (b, "Detune", "e1_o2_fine", 0.05f, "Attack", "env1_a", 0.35f);
          } },
        { "Grand Warm Pad", "Pad", "Saw and pulse through the gentler 12 dB filter. Smooth and warm.",
          [] (B& b) {
              Vcos v;
              v.wave2 = kSquare; v.width2 = 0.4f; v.detune = 0.09f; v.pole2 = true; v.cutoff = 900.0f; v.envAmt = 0.25f;
              v.fa = 1.0f; v.fd = 2.0f; v.fs = 0.6f; v.fr = 1.2f; v.a = 0.7f; v.d = 1.0f; v.s = 1.0f; v.r = 1.3f;
              vcos (b, v);
              hall (b, 0.38f, 0.8f, 0.7f);
              gpMacros (b, "Filter Swell", "env2_a", 0.3f, "Attack", "env1_a", 0.35f);
          } },
        { "Envelope PWM Pad", "Pad", "The pulse width follows the filter envelope: each chord's tone thins out as it settles.",
          [] (B& b) {
              Vcos v;
              v.wave1 = kSquare; v.wave2 = kSquare; v.width2 = 0.5f; v.detune = 0.1f; v.cutoff = 1400.0f; v.envAmt = 0.2f;
              v.fa = 0.8f; v.fd = 2.5f; v.fs = 0.3f; v.fr = 1.0f; v.a = 0.5f; v.d = 1.0f; v.s = 1.0f; v.r = 1.2f;
              vcos (b, v);
              b.set ("e1_o1_width", 0.5f);
              b.route ("Env 2", "e1_o1_width", 0.3f).route ("Env 2", "e1_o2_width", -0.3f);
              hall (b, 0.35f, 0.8f, 0.7f);
              gpMacros (b, "PWM Depth", "e1_o2_width", 0.2f, "Attack", "env1_a", 0.35f);
          } },
        { "Dual Strings and Bells", "Pad", "Two patches layered (Dual): soft strings underneath, a cross-mod bell on top.",
          [] (B& b) {
              Vcos v;
              v.detune = 0.1f; v.cutoff = 2500.0f; v.envAmt = 0.0f; v.a = 0.4f; v.d = 1.0f; v.s = 1.0f; v.r = 1.0f;
              vcos (b, v);
              // Upper layer on Engine 2: a sine cross-modulated by a sine 19 semitones up, with an FM burst.
              engineOn (b, 1, 0.35f);
              osc (b, 1, 0, kSine, 0.8f, 12.0f);
              osc (b, 1, 2, kSine, 0.0f, 19.0f);
              b.set ("e2_mod_src", 0.0f).set ("e2_o1_fm", 0.15f);
              env (b, "env3", 0.001f, 1.5f, 0.0f, 1.0f);
              b.route ("Env 3", "e2_o1_fm", 0.25f).route ("Env 3", "e2_vol", 0.2f);
              b.set ("e2_fmix", 1.0f);
              filter (b, 1, kLP12, 9000.0f, 0.0f, 0.3f);
              b.set ("f_routing", 1.0f);
              hall (b, 0.38f, 0.85f, 0.75f);
              gpMacros (b, "Bells", "e2_vol", 0.3f, "Attack", "env1_a", 0.35f);
          } },
        { "Resonant Sweep Strings", "Strings/Brass", "Strings with a slow, resonant filter envelope that sweeps up through every chord.",
          [] (B& b) {
              Vcos v;
              v.detune = 0.1f; v.cutoff = 500.0f; v.reso = 0.45f; v.envAmt = 0.4f;
              v.fa = 2.0f; v.fd = 2.0f; v.fs = 0.5f; v.fr = 1.0f; v.a = 0.3f; v.d = 1.0f; v.s = 1.0f; v.r = 1.0f;
              vcos (b, v);
              hall (b, 0.35f, 0.8f, 0.7f);
              gpMacros (b, "Resonance", "f1_reso", 0.25f, "Sweep Time", "env2_a", 0.3f);
          } },
        { "Glass Pulse Pad", "Pad", "Two narrow pulses and a high highpass: thin, glassy and bright, sits above the band.",
          [] (B& b) {
              Vcos v;
              v.wave1 = kSquare; v.wave2 = kSquare; v.width2 = 0.2f; v.detune = 0.07f; v.hpf = 600.0f; v.cutoff = 6000.0f; v.envAmt = 0.0f;
              v.a = 0.5f; v.d = 1.0f; v.s = 1.0f; v.r = 1.2f;
              vcos (b, v);
              b.set ("e1_o1_width", 0.25f);
              delay (b, 0, kDiv4D, 0.3f, 0.15f, true);
              hall (b, 0.38f, 0.85f, 0.75f);
              gpMacros (b, "Thin", "f2_cutoff", 0.15f, "Attack", "env1_a", 0.35f);
          } },
        // ================================================================ BRASS
        { "Grand Brass", "Strings/Brass", "The big flagship brass: two saws, filter envelope punch, a hint of drift. Bold stabs.",
          [] (B& b) {
              Vcos v;
              v.detune = 0.09f; v.cutoff = 700.0f; v.reso = 0.1f; v.envAmt = 0.4f;
              v.fa = 0.05f; v.fd = 0.5f; v.fs = 0.55f; v.fr = 0.3f; v.a = 0.03f; v.d = 0.5f; v.s = 0.85f; v.r = 0.3f;
              vcos (b, v);
              b.route ("Velocity", "f1_cutoff", 0.12f);
              room (b, 0.22f);
              gpMacros (b, "Bite", "env2_d", -0.25f, "Attack", "env1_a", 0.3f);
          } },
        { "Brass Swell Stack", "Strings/Brass", "Slow-blooming stacked brass: volume and brightness rise together. Majestic.",
          [] (B& b) {
              Vcos v;
              v.detune = 0.1f; v.coarse2 = -12.0f; v.vol2 = 0.5f; v.cutoff = 500.0f; v.envAmt = 0.45f;
              v.fa = 0.9f; v.fd = 1.0f; v.fs = 0.8f; v.fr = 0.6f; v.a = 0.5f; v.d = 1.0f; v.s = 1.0f; v.r = 0.6f;
              vcos (b, v);
              hall (b, 0.32f, 0.8f, 0.7f);
              gpMacros (b, "Swell", "env2_a", 0.3f, "Attack", "env1_a", 0.35f);
          } },
        { "Unison Brass Stab", "Strings/Brass", "All voices stacked on one note: a huge, thick monophonic brass stab.",
          [] (B& b) {
              Vcos v;
              v.detune = 0.1f; v.cutoff = 650.0f; v.envAmt = 0.45f;
              v.fa = 0.02f; v.fd = 0.35f; v.fs = 0.4f; v.fr = 0.25f; v.a = 0.01f; v.d = 0.5f; v.s = 0.8f; v.r = 0.25f;
              vcos (b, v);
              stackVoices (b, 5, 0.15f, false, 0.0f);
              b.set ("f1_vol", 1.8f);
              room (b, 0.2f);
              gpMacros (b, "Spread", "e1_uni_detune", 0.2f, "Release", "env1_r", 0.3f);
          } },
        // ================================================================ KEYS AND BELLS
        { "Cross-Mod Bells", "Pluck", "VCO 2 frequency-modulates VCO 1: metallic, glassy bells with a long tail.",
          [] (B& b) {
              Vcos v;
              v.wave1 = kSine; v.wave2 = kSine; v.vol2 = 0.2f; v.coarse2 = 12.0f; v.detune = 0.03f; v.cutoff = 8000.0f; v.envAmt = 0.0f;
              v.a = 0.001f; v.d = 2.5f; v.s = 0.0f; v.r = 2.0f;
              vcos (b, v);
              crossMod (b, 19.0f, 0.12f);
              b.set ("f1_vol", 1.5f);
              env (b, "env2", 0.001f, 1.2f, 0.0f, 0.8f);
              b.route ("Env 2", "e1_o1_fm", 0.3f, "Velocity", 1.0f);
              delay (b, 0, kDiv4D, 0.3f, 0.18f, true);
              hall (b, 0.35f, 0.85f, 0.75f);
              gpMacros (b, "Metal", "e1_o1_fm", 0.2f, "Decay", "env1_d", 0.35f);
          } },
        { "Sync Keys", "Keys", "Hard-sync keys: VCO 2 is locked to VCO 1 and its pitch swept by the envelope. A biting, vocal attack.",
          [] (B& b) {
              Vcos v;
              v.vol1 = 0.0f; v.vol2 = 0.8f; v.cutoff = 2500.0f; v.envAmt = 0.15f;
              v.fa = 0.001f; v.fd = 0.4f; v.fs = 0.2f; v.fr = 0.3f; v.a = 0.002f; v.d = 1.2f; v.s = 0.4f; v.r = 0.35f;
              vcos (b, v);
              b.set ("e1_sync", 1.0f).set ("f1_vol", 1.6f);
              b.route ("Env 2", "e1_o2_coarse", 0.25f);
              room (b, 0.22f);
              gpMacros (b, "Sync Depth", "e1_o2_coarse", 0.15f, "Decay", "env1_d", 0.35f);
          } },
        { "Bright Poly Pluck", "Pluck", "Two pulses and a snappy filter: a crisp, bright pluck for arpeggios and rhythms.",
          [] (B& b) {
              Vcos v;
              v.wave1 = kSquare; v.wave2 = kSaw; v.detune = 0.06f; v.cutoff = 600.0f; v.reso = 0.2f; v.envAmt = 0.5f;
              v.fa = 0.001f; v.fd = 0.22f; v.fs = 0.0f; v.fr = 0.2f; v.a = 0.001f; v.d = 0.35f; v.s = 0.0f; v.r = 0.3f;
              vcos (b, v);
              b.set ("e1_o1_width", 0.35f);
              delay (b, 0, kDiv8D, 0.3f, 0.2f, true);
              room (b, 0.2f);
              gpMacros (b, "Resonance", "f1_reso", 0.3f, "Decay", "env1_d", 0.35f);
          } },
        { "Synth Piano 80s", "Keys", "The 80s 'synth piano': a struck, decaying two-oscillator tone. Pop ballad keys.",
          [] (B& b) {
              Vcos v;
              v.wave2 = kSquare; v.width2 = 0.3f; v.vol2 = 0.5f; v.coarse2 = 12.0f; v.detune = 0.05f; v.cutoff = 1200.0f; v.envAmt = 0.35f;
              v.fa = 0.001f; v.fd = 0.8f; v.fs = 0.2f; v.fr = 0.5f; v.a = 0.001f; v.d = 2.5f; v.s = 0.0f; v.r = 0.5f;
              vcos (b, v);
              b.route ("Velocity", "f1_cutoff", 0.15f);
              b.set ("amp_velo", 0.6f);
              b.route ("Keytrack", "env1_d", -0.3f);
              chorus (b, 0, 0.25f);
              room (b, 0.22f);
              gpMacros (b, "Chorus", "fx_a1_p8", 0.35f, "Decay", "env1_d", 0.35f);
          } },
        { "Grand Arpeggio", "Sequence", "Up-and-down 16th arpeggio over three octaves with an echo: the flagship's arpeggiator. Hold a chord.",
          [] (B& b) {
              Vcos v;
              v.wave2 = kSquare; v.width2 = 0.35f; v.detune = 0.06f; v.cutoff = 900.0f; v.reso = 0.25f; v.envAmt = 0.35f;
              v.fa = 0.001f; v.fd = 0.18f; v.fs = 0.1f; v.fr = 0.15f; v.a = 0.001f; v.d = 0.25f; v.s = 0.2f; v.r = 0.15f;
              vcos (b, v);
              b.set ("f1_vol", 1.8f);
              b.set ("seq_on", 1.0f).set ("seq_mode", 0.0f).set ("seq_arp_order", (float) ArpUpDownIncl).set ("seq_arp_octaves", 3.0f)
                  .set ("seq_rate", (float) kDiv16);
              b.pattern (gatePattern ({ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, 0.5f));
              delay (b, 0, kDiv8D, 0.3f, 0.2f, true);
              room (b, 0.22f);
              gpMacros (b, "Echo", "fx_a1_p8", 0.35f, "Decay", "env1_d", 0.35f);
          } },
        // ================================================================ LEADS
        { "Unison Lead", "Lead", "Every voice stacked and detuned on one note: an enormous saw lead with glide.",
          [] (B& b) {
              Vcos v;
              v.detune = 0.1f; v.cutoff = 1800.0f; v.reso = 0.15f; v.envAmt = 0.25f;
              v.fa = 0.01f; v.fd = 0.6f; v.fs = 0.6f; v.fr = 0.3f; v.a = 0.005f; v.d = 0.5f; v.s = 0.9f; v.r = 0.3f;
              vcos (b, v);
              stackVoices (b, 5, 0.12f, true, 0.06f);
              b.set ("f1_vol", 1.6f);
              lfo (b, 0, LfoSine, 5.5f);
              b.set ("lfo1_fade", 0.5f).set ("lfo1_retrig", 2.0f);
              b.route ("LFO 1", "e1_fine", 0.006f, "Mod Wheel", 1.0f);
              delay (b, 0, kDiv8D, 0.3f, 0.2f, true);
              room (b, 0.25f);
              gpMacros (b, "Spread", "e1_uni_detune", 0.2f, "Release", "env1_r", 0.3f);
          } },
        { "Sync Lead", "Lead", "The screaming sync lead: VCO 2 locked to VCO 1, swept by the envelope and the mod wheel.",
          [] (B& b) {
              Vcos v;
              v.vol1 = 0.3f; v.vol2 = 0.8f; v.coarse2 = 7.0f; v.cutoff = 3000.0f; v.reso = 0.1f; v.envAmt = 0.1f;
              v.fa = 0.001f; v.fd = 0.8f; v.fs = 0.3f; v.fr = 0.3f; v.a = 0.005f; v.d = 0.5f; v.s = 0.9f; v.r = 0.3f;
              vcos (b, v);
              b.set ("e1_sync", 1.0f);
              b.route ("Env 2", "e1_o2_coarse", 0.2f).route ("Mod Wheel", "e1_o2_coarse", 0.2f);
              mono (b, true, 0.04f);
              b.set ("f1_vol", 2.6f);
              delay (b, 0, kDiv8D, 0.3f, 0.18f, true);
              room (b, 0.22f);
              gpMacros (b, "Sync Sweep", "env2_d", 0.3f, "Release", "env1_r", 0.3f);
          } },
        { "Soft Square Lead", "Lead", "Two squares an octave apart, a soft filter and a delayed vibrato. Gentle melodies.",
          [] (B& b) {
              Vcos v;
              v.wave1 = kSquare; v.wave2 = kSquare; v.coarse2 = 12.0f; v.vol2 = 0.4f; v.detune = 0.04f; v.pole2 = true;
              v.cutoff = 1300.0f; v.envAmt = 0.15f; v.a = 0.03f; v.d = 0.5f; v.s = 0.9f; v.r = 0.35f;
              vcos (b, v);
              mono (b, true, 0.05f);
              b.set ("f1_vol", 1.6f);
              lfo (b, 0, LfoSine, 5.2f);
              b.set ("lfo1_fade", 0.7f).set ("lfo1_retrig", 2.0f);
              b.route ("LFO 1", "e1_fine", 0.006f);
              delay (b, 0, kDiv4D, 0.3f, 0.18f, true);
              hall (b, 0.28f);
              gpMacros (b, "Octave", "e1_o2_vol", 0.3f, "Release", "env1_r", 0.3f);
          } },
        // ================================================================ BASS
        { "Unison Bass", "Bass", "Stacked, detuned saws on one note: a fat, wide-but-solid synth bass.",
          [] (B& b) {
              Vcos v;
              v.coarse2 = -12.0f; v.detune = 0.06f; v.cutoff = 350.0f; v.reso = 0.15f; v.envAmt = 0.35f; v.kbd = 0.3f;
              v.fa = 0.001f; v.fd = 0.3f; v.fs = 0.3f; v.fr = 0.15f; v.a = 0.002f; v.d = 0.5f; v.s = 0.8f; v.r = 0.12f;
              vcos (b, v);
              stackVoices (b, 3, 0.06f, false, 0.0f);
              b.set ("e1_uni_stereo", 0.3f).set ("f1_vol", 2.4f);
              gpMacros (b, "Punch", "env2_d", 0.3f, "Release", "env1_r", 0.3f);
          } },
        { "Sync Bass", "Bass", "Hard-sync bass with an envelope sweep on every note: growly and funky.",
          [] (B& b) {
              Vcos v;
              v.vol1 = 0.4f; v.vol2 = 0.8f; v.coarse2 = 5.0f; v.cutoff = 900.0f; v.envAmt = 0.25f; v.kbd = 0.3f;
              v.fa = 0.001f; v.fd = 0.3f; v.fs = 0.2f; v.fr = 0.15f; v.a = 0.002f; v.d = 0.4f; v.s = 0.7f; v.r = 0.1f;
              vcos (b, v);
              b.set ("e1_sync", 1.0f);
              b.route ("Env 2", "e1_o2_coarse", 0.18f);
              mono (b, false, 0.0f);
              b.set ("f1_vol", 2.4f);
              gpMacros (b, "Sync Depth", "e1_o2_coarse", 0.12f, "Release", "env1_r", 0.3f);
          } },
        // ================================================================ STAB
        { "Dual Brass and Strings", "Strings/Brass", "Brass and strings layered (Dual): a punchy brass attack over sustained strings.",
          [] (B& b) {
              Vcos v;
              v.detune = 0.09f; v.cutoff = 700.0f; v.envAmt = 0.4f;
              v.fa = 0.04f; v.fd = 0.5f; v.fs = 0.5f; v.fr = 0.3f; v.a = 0.02f; v.d = 0.5f; v.s = 0.85f; v.r = 0.5f;
              vcos (b, v);
              // Strings layer on Engine 2 through its own filter (parallel).
              engineOn (b, 1, 0.35f);
              osc (b, 1, 0, kSaw, 0.8f);
              osc (b, 1, 1, kSaw, 0.6f, 0.0f, 0.12f);
              b.set ("e2_fmix", 1.0f);
              filter (b, 1, kLP12, 3000.0f, 0.0f, 0.3f);
              b.set ("f_routing", 1.0f);
              hall (b, 0.3f, 0.8f, 0.7f);
              gpMacros (b, "Strings", "e2_vol", 0.3f, "Attack", "env1_a", 0.3f);
          } },
    };
    return list;
}

juce::StringArray pack7Tags (const juce::String& presetName)
{
    static const std::map<juce::String, const char*> tags {
        { "Grand Strings", "Grand Poly, Strings" }, { "Grand Warm Pad", "Grand Poly, Pad, Worship" },
        { "Envelope PWM Pad", "Grand Poly, Pad, PWM" }, { "Dual Strings and Bells", "Grand Poly, Pad, Bells, Dual" },
        { "Resonant Sweep Strings", "Grand Poly, Strings, Evolving" }, { "Glass Pulse Pad", "Grand Poly, Pad, Bright" },
        { "Grand Brass", "Grand Poly, Brass, Synth Brass" }, { "Brass Swell Stack", "Grand Poly, Brass, Synth Brass, Swell" },
        { "Unison Brass Stab", "Grand Poly, Brass, Stab, Unison" },
        { "Cross-Mod Bells", "Grand Poly, Bells, Cross-Mod" }, { "Sync Keys", "Grand Poly, Synth Keys, Sync" },
        { "Bright Poly Pluck", "Grand Poly, Pluck" }, { "Synth Piano 80s", "Grand Poly, Synth Keys, Piano" },
        { "Grand Arpeggio", "Grand Poly, Arp" },
        { "Unison Lead", "Grand Poly, Lead, Unison" }, { "Sync Lead", "Grand Poly, Lead, Sync" },
        { "Soft Square Lead", "Grand Poly, Lead, Mono" },
        { "Unison Bass", "Grand Poly, Bass, Unison" }, { "Sync Bass", "Grand Poly, Bass, Sync" },
        { "Dual Brass and Strings", "Grand Poly, Brass, Strings, Dual" },
    };
    const auto it = tags.find (presetName);
    return it == tags.end() ? juce::StringArray() : PresetManager::parseTags (it->second);
}
