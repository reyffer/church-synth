// Preset Pack 13: Famous Sounds.
//
// Timbres inspired by famous synth sounds from records and films, with nod names.
// These are our own recipes built from scratch to get close to the character of
// each sound. No factory patch data was copied, and nothing plays a song's riff
// or melody: arps and sequences use generic patterns.
//
// Macros: M1 Brightness, M2 Space, M3 a character control, M4 Decay / Attack / Release.

#include "Packs.h"

#include "plugin/PresetHelpers.h"

#include <map>

using namespace presethelpers;

namespace
{
constexpr int kOta = 11, kLadder = 9, kClassic = 0;

struct Snd
{
    int w1 = kSaw, w2 = kSaw;
    float v1 = 0.8f, v2 = 0.0f, c2 = 0.0f, d2 = 0.08f, width = 0.5f, sub = 0.0f, noise = 0.0f;
    int voices = 1, uniMode = 0; float uniDetune = 0.15f, uniStereo = 0.6f;
    int chord = -1;
    int model = kOta;
    float cutoff = 1500.0f, reso = 0.1f, kbd = 0.4f, drive = 0.0f;
    float envAmt = 0.3f, velAmt = 0.1f;
    float fa = 0.001f, fd = 0.5f, fs = 0.5f, fr = 0.3f;
    float a = 0.002f, d = 0.5f, s = 0.85f, r = 0.3f;
    bool mono = false, legato = false; float glide = 0.0f;
    float level = 1.0f, velo = 0.4f, drift = 0.2f;
};

void snd (B& b, const Snd& x)
{
    osc (b, 0, 0, x.w1, x.v1, 0.0f, 0.0f, x.width);
    osc (b, 0, 1, x.w2, x.v2, x.c2, x.d2, x.width);
    osc (b, 0, 2, kSquare, x.sub, -12.0f);
    b.set ("e1_vol", 0.6f).set ("e1_drift", x.drift).set ("e1_noise_vol", x.noise);
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
    if (x.model == kClassic)
    {
        filter (b, 0, kLP24, x.cutoff, x.reso, x.kbd, x.drive);
        b.set ("f1_analog", 1.0f);
    }
    else
        model (b, 0, x.model, 0, x.cutoff, x.reso, x.kbd, x.drive);
    b.set ("f1_vol", x.level);
    env (b, "env2", x.fa, x.fd, x.fs, x.fr);
    b.route ("Env 2", "f1_cutoff", x.envAmt);
    if (x.velAmt > 0.0f)
        b.route ("Velocity", "f1_cutoff", x.velAmt);
    env (b, "env1", x.a, x.d, x.s, x.r);
    b.set ("amp_velo", x.velo);
    if (x.mono)
        mono (b, x.legato, x.glide);
}

void vibratoWheel (B& b, float rate = 5.5f, float depth = 0.008f)
{
    lfo (b, 0, LfoSine, rate);
    b.route ("LFO 1", "e1_fine", depth, "Mod Wheel", 1.0f);
}

void delayedVibrato (B& b, float rate, float depth, float fade)
{
    lfo (b, 0, LfoSine, rate);
    b.set ("lfo1_fade", fade).set ("lfo1_retrig", 2.0f);
    b.route ("LFO 1", "e1_fine", depth);
}

void vChorus (B& b, int slot, int mode, float mix, float width = 0.7f)
{
    vintageChorus (b, slot, mode, mix);
    b.set (juce::String ("fx_") + kSlot[slot] + "_p6", width);
}

void arp (B& b, int order, int octaves, int rate, float gate)
{
    b.set ("seq_on", 1.0f).set ("seq_mode", 0.0f).set ("seq_arp_order", (float) order).set ("seq_arp_octaves", (float) octaves)
        .set ("seq_rate", (float) rate);
    b.pattern (gatePattern ({ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, gate));
}

void seq (B& b, int rate, std::initializer_list<int> steps, std::initializer_list<int> octaveUp, float gate)
{
    bassSeq (b, rate, steps, gate);
    SeqPattern p = gatePattern (steps, gate);
    for (int i : octaveUp)
        p.steps[(size_t) i].octave = 1;
    b.pattern (p);
}

void fMacros (B& b, const char* m3, const char* m3dest, float m3amt, const char* m4, const char* m4dest, float m4amt)
{
    macros (b, "Brightness", "Space", m3, m4);
    b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", m3dest, m3amt).route ("Macro 4", m4dest, m4amt);
}
} // namespace

const std::vector<FactoryPreset>& pack13Presets()
{
    static const std::vector<FactoryPreset> list {
        // ================================================================ BRASS AND POLY STABS
        { "Countdown Brass", "Strings/Brass", "Bright, triumphant stacked synth brass: saws plus a square an octave up, with chorus. Arena rock.",
          [] (B& b) {
              Snd x; x.v2 = 0.5f; x.w2 = kSquare; x.c2 = 12.0f; x.voices = 3; x.cutoff = 1400.0f; x.envAmt = 0.3f; x.fa = 0.02f; x.fd = 0.6f; x.fs = 0.6f;
              x.a = 0.01f; x.d = 0.6f; x.s = 0.9f; x.r = 0.3f;
              snd (b, x);
              vChorus (b, 0, 1, 0.35f);
              hall (b, 0.3f, 0.8f, 0.7f);
              fMacros (b, "Bite", "env2_d", -0.2f, "Release", "env1_r", 0.3f);
          } },
        { "Jump Poly", "Strings/Brass", "Big, bright two-saw polysynth brass with a punchy filter: the 80s rock keyboard stab.",
          [] (B& b) {
              Snd x; x.v2 = 0.7f; x.d2 = 0.1f; x.cutoff = 1100.0f; x.reso = 0.15f; x.envAmt = 0.35f; x.fa = 0.01f; x.fd = 0.4f; x.fs = 0.55f;
              x.a = 0.004f; x.d = 0.5f; x.s = 0.9f; x.r = 0.25f; x.drift = 0.3f;
              snd (b, x);
              room (b, 0.25f);
              fMacros (b, "Bite", "env2_d", -0.2f, "Release", "env1_r", 0.3f);
          } },
        { "Separate Brass", "Strings/Brass", "Driving, edgy synth brass with a hard attack and a little grit. Stadium rock.",
          [] (B& b) {
              Snd x; x.v2 = 0.7f; x.w2 = kSquare; x.width = 0.4f; x.cutoff = 1200.0f; x.reso = 0.2f; x.drive = 0.35f; x.envAmt = 0.35f;
              x.fa = 0.005f; x.fd = 0.3f; x.fs = 0.5f; x.a = 0.002f; x.d = 0.4f; x.s = 0.85f; x.r = 0.2f;
              snd (b, x);
              room (b, 0.25f);
              fMacros (b, "Drive", "f1_drive", 0.3f, "Release", "env1_r", 0.3f);
          } },
        { "Runaway Poly", "Keys", "Bright, resonant poly stab that bites and decays: the 80s rock synth hook sound.",
          [] (B& b) {
              Snd x; x.w1 = kSquare; x.width = 0.35f; x.v2 = 0.5f; x.cutoff = 900.0f; x.reso = 0.3f; x.envAmt = 0.45f; x.fd = 0.3f; x.fs = 0.25f;
              x.a = 0.002f; x.d = 0.6f; x.s = 0.5f; x.r = 0.25f;
              snd (b, x);
              vChorus (b, 0, 0, 0.3f);
              room (b, 0.25f);
              fMacros (b, "Resonance", "f1_reso", 0.25f, "Decay", "env1_d", 0.35f);
          } },
        { "Party Stab 99", "Strings/Brass", "Short, fat, funky polysynth brass stab: the Minneapolis sound.",
          [] (B& b) {
              Snd x; x.v2 = 0.7f; x.d2 = 0.12f; x.cutoff = 800.0f; x.reso = 0.15f; x.envAmt = 0.45f; x.fd = 0.25f; x.fs = 0.3f;
              x.a = 0.003f; x.d = 0.35f; x.s = 0.4f; x.r = 0.2f; x.drift = 0.3f;
              x.level = 1.6f;
              snd (b, x);
              room (b, 0.22f);
              fMacros (b, "Bite", "env2_d", -0.2f, "Decay", "env1_d", 0.35f);
          } },
        { "Want Me Brass", "Strings/Brass", "Cold, bright synth-pop brass: thin, punchy, very 1981.",
          [] (B& b) {
              Snd x; x.v2 = 0.5f; x.w2 = kSquare; x.width = 0.3f; x.cutoff = 1300.0f; x.envAmt = 0.3f; x.fd = 0.3f; x.fs = 0.5f;
              x.a = 0.005f; x.d = 0.5f; x.s = 0.8f; x.r = 0.2f; x.drift = 0.1f;
              snd (b, x);
              filter (b, 1, kHP12, 300.0f, 0.0f);
              b.set ("f_routing", 0.0f);
              room (b, 0.22f);
              fMacros (b, "Bite", "env2_d", -0.2f, "Release", "env1_r", 0.3f);
          } },
        { "Chariot Brass", "Strings/Brass", "Majestic, slow, expressive brass with aftertouch growl and a vast reverb. Victory-lap cinema.",
          [] (B& b) {
              Snd x; x.v2 = 0.6f; x.d2 = 0.1f; x.model = kClassic; x.cutoff = 600.0f; x.reso = 0.2f; x.envAmt = 0.3f; x.fa = 0.3f; x.fd = 1.0f; x.fs = 0.6f;
              x.a = 0.15f; x.d = 1.0f; x.s = 1.0f; x.r = 1.2f;
              snd (b, x);
              for (const char* src : { "Aftertouch", "Macro 3" })
                  b.route (src, "f1_cutoff", 0.25f);
              lfo (b, 0, LfoSine, 5.5f);
              b.route ("LFO 1", "e1_fine", 0.01f, "Aftertouch", 1.0f).route ("LFO 1", "e1_fine", 0.01f, "Macro 3", 1.0f);
              hall (b, 0.45f, 0.95f, 0.85f);
              macros (b, "Brightness", "Space", "Growl", "Attack");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 4", "env1_a", 0.3f);
          } },
        { "Replicant Brass", "Strings/Brass", "Dark, smoky, rain-soaked brass with a long tail and slow vibrato. Neo-noir.",
          [] (B& b) {
              Snd x; x.v2 = 0.6f; x.d2 = 0.12f; x.model = kClassic; x.cutoff = 400.0f; x.reso = 0.25f; x.envAmt = 0.3f; x.fa = 0.4f; x.fd = 1.5f; x.fs = 0.5f;
              x.a = 0.2f; x.d = 1.0f; x.s = 1.0f; x.r = 2.5f;
              snd (b, x);
              for (const char* src : { "Aftertouch", "Macro 3" })
                  b.route (src, "f1_cutoff", 0.25f);
              lfo (b, 0, LfoSine, 4.5f);
              b.set ("lfo1_fade", 1.0f);
              b.route ("LFO 1", "e1_fine", 0.006f);
              hall (b, 0.5f, 0.95f, 0.9f);
              macros (b, "Brightness", "Space", "Growl", "Release");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 4", "env1_r", 0.3f);
          } },
        { "Orchestra Hit 80s", "Keys", "The 80s sampler orchestra hit, synthesised: a short, huge octave-and-fifth stab with a noise burst.",
          [] (B& b) {
              Snd x; x.v2 = 0.6f; x.sub = 0.4f; x.chord = 2; x.noise = 0.15f; x.cutoff = 2500.0f; x.envAmt = 0.3f; x.fd = 0.2f; x.fs = 0.2f;
              x.a = 0.001f; x.d = 0.35f; x.s = 0.0f; x.r = 0.35f; x.velo = 0.5f;
              snd (b, x);
              hall (b, 0.4f, 0.85f, 0.75f);
              fMacros (b, "Noise", "e1_noise_vol", 0.2f, "Decay", "env1_d", 0.35f);
          } },
        // ================================================================ LEADS
        { "Take On Lead", "Lead", "Bright, plucky chorus lead that decays to a soft sustain. Fast, catchy 80s melodies.",
          [] (B& b) {
              Snd x; x.w1 = kSquare; x.width = 0.45f; x.v2 = 0.5f; x.cutoff = 1500.0f; x.envAmt = 0.35f; x.fd = 0.2f; x.fs = 0.4f;
              x.a = 0.002f; x.d = 0.3f; x.s = 0.6f; x.r = 0.15f; x.mono = true; x.level = 2.2f;
              snd (b, x);
              vChorus (b, 0, 1, 0.45f);
              room (b, 0.25f);
              fMacros (b, "Chorus", "fx_a1_p5", 0.3f, "Decay", "env1_d", 0.35f);
          } },
        { "Axel Lead", "Lead", "Punchy square lead with a snappy filter and a touch of glide: the 80s movie-theme lead.",
          [] (B& b) {
              Snd x; x.w1 = kSquare; x.width = 0.5f; x.v2 = 0.4f; x.cutoff = 900.0f; x.reso = 0.2f; x.envAmt = 0.4f; x.fd = 0.25f; x.fs = 0.4f;
              x.a = 0.002f; x.d = 0.3f; x.s = 0.8f; x.r = 0.15f; x.mono = true; x.legato = true; x.glide = 0.03f; x.level = 2.2f;
              snd (b, x);
              delay (b, 0, kDiv8, 0.2f, 0.12f, true);
              room (b, 0.22f);
              fMacros (b, "Resonance", "f1_reso", 0.25f, "Release", "env1_r", 0.3f);
          } },
        { "Vice Lead", "Lead", "Expressive, bending saw lead with vibrato, echo and reverb: the neon-cop-show solo.",
          [] (B& b) {
              Snd x; x.v2 = 0.6f; x.d2 = 0.1f; x.cutoff = 1800.0f; x.reso = 0.15f; x.envAmt = 0.2f; x.mono = true; x.legato = true; x.glide = 0.05f;
              x.level = 2.0f;
              snd (b, x);
              b.set ("bend_up", 5.0f).set ("bend_down", 5.0f);
              delayedVibrato (b, 5.5f, 0.008f, 0.5f);
              delay (b, 0, kDiv8D, 0.35f, 0.22f, true);
              hall (b, 0.35f, 0.85f, 0.75f);
              fMacros (b, "Resonance", "f1_reso", 0.25f, "Release", "env1_r", 0.3f);
          } },
        { "G-Funk Whistle", "Lead", "High, pure sine lead with a long portamento and vibrato: the West Coast whistle.",
          [] (B& b) {
              Snd x; x.w1 = kSine; x.w2 = kTri; x.v2 = 0.1f; x.cutoff = 4000.0f; x.envAmt = 0.0f; x.a = 0.03f; x.r = 0.3f;
              x.mono = true; x.legato = false; x.glide = 0.12f; x.level = 2.6f;
              snd (b, x);
              b.set ("transpose", 1.0f);
              delayedVibrato (b, 5.8f, 0.01f, 0.3f);
              room (b, 0.25f);
              fMacros (b, "Vibrato", "lfo1_rate", 0.1f, "Release", "env1_r", 0.3f);
          } },
        { "Talk Box Lead", "Lead", "Funky talking lead: a saw through a vowel filter that 'speaks' with every note and the mod wheel.",
          [] (B& b) {
              Snd x; x.v2 = 0.5f; x.cutoff = 3000.0f; x.envAmt = 0.0f; x.a = 0.005f; x.d = 0.3f; x.s = 0.8f; x.r = 0.15f;
              x.mono = true; x.legato = true; x.glide = 0.06f; x.level = 1.8f;
              snd (b, x);
              b.set ("f2_on", 1.0f).set ("f2_type", 3.0f).set ("f2_cutoff", 1000.0f).set ("f2_reso", 0.35f).set ("f2_a", 0.05f).set ("f2_b", 0.8f)
                  .set ("f_routing", 0.0f).set ("f2_vol", 1.6f);
              env (b, "env3", 0.08f, 0.35f, 0.5f, 0.2f);
              b.route ("Env 3", "f2_a", 0.45f).route ("Mod Wheel", "f2_a", 0.5f);
              room (b, 0.2f);
              fMacros (b, "Vowel", "f2_a", 0.4f, "Release", "env1_r", 0.3f);
          } },
        { "Popcorn Pluck", "Lead", "A bouncy, resonant square pluck: the bubbly early-70s synth novelty sound.",
          [] (B& b) {
              Snd x; x.w1 = kSquare; x.width = 0.4f; x.model = kLadder; x.cutoff = 600.0f; x.reso = 0.45f; x.envAmt = 0.45f; x.fd = 0.15f; x.fs = 0.0f;
              x.a = 0.001f; x.d = 0.25f; x.s = 0.0f; x.r = 0.1f; x.mono = true; x.level = 2.4f;
              snd (b, x);
              room (b, 0.2f);
              fMacros (b, "Resonance", "f1_reso", 0.2f, "Decay", "env1_d", 0.35f);
          } },
        { "Enough Lead", "Lead", "Bright, bouncy synth-pop lead: a short square pluck with a little sustain.",
          [] (B& b) {
              Snd x; x.w1 = kSquare; x.width = 0.3f; x.v2 = 0.4f; x.c2 = 12.0f; x.w2 = kSquare; x.cutoff = 2000.0f; x.envAmt = 0.3f; x.fd = 0.15f; x.fs = 0.3f;
              x.a = 0.001f; x.d = 0.2f; x.s = 0.5f; x.r = 0.1f; x.mono = true; x.level = 2.2f;
              snd (b, x);
              delay (b, 0, kDiv8, 0.2f, 0.12f, true);
              room (b, 0.2f);
              fMacros (b, "Octave", "e1_o2_vol", 0.3f, "Decay", "env1_d", 0.35f);
          } },
        { "Sandstorm Lead", "Lead", "Bright, buzzy detuned saw lead with short, chopped notes: late-90s trance energy.",
          [] (B& b) {
              Snd x; x.v2 = 0.8f; x.d2 = 0.15f; x.voices = 3; x.uniDetune = 0.2f; x.cutoff = 3000.0f; x.envAmt = 0.15f; x.fd = 0.15f; x.fs = 0.7f;
              x.a = 0.001f; x.d = 0.15f; x.s = 0.7f; x.r = 0.08f; x.mono = true; x.level = 1.8f;
              snd (b, x);
              delay (b, 0, kDiv8D, 0.3f, 0.18f, true);
              hall (b, 0.28f);
              fMacros (b, "Width", "e1_uni_detune", 0.15f, "Release", "env1_r", 0.3f);
          } },
        { "Levels Lead", "Lead", "Huge supersaw lead with a pitch drop into each note and big echo: 2010s progressive house.",
          [] (B& b) {
              Snd x; x.voices = 7; x.uniMode = 2; x.uniDetune = 0.22f; x.cutoff = 2500.0f; x.envAmt = 0.2f; x.fd = 0.3f; x.fs = 0.6f;
              x.a = 0.002f; x.d = 0.4f; x.s = 0.8f; x.r = 0.25f; x.mono = true; x.legato = false; x.level = 1.8f;
              snd (b, x);
              env (b, "env3", 0.0f, 0.06f, 0.0f, 0.05f);
              b.route ("Env 3", "e1_coarse", 1.0f / 120.0f);
              delay (b, 0, kDiv8D, 0.35f, 0.22f, true);
              hall (b, 0.35f, 0.85f, 0.75f);
              fMacros (b, "Width", "e1_uni_detune", 0.15f, "Release", "env1_r", 0.3f);
          } },
        { "Animal Drop", "Lead", "Big, distorted, hollow-sounding big-room lead with a sub. Made for one-note drops.",
          [] (B& b) {
              Snd x; x.w1 = kSquare; x.width = 0.35f; x.v2 = 0.6f; x.d2 = 0.2f; x.sub = 0.5f; x.cutoff = 2200.0f; x.drive = 0.5f; x.envAmt = 0.15f;
              x.a = 0.002f; x.d = 0.3f; x.s = 0.9f; x.r = 0.15f; x.mono = true; x.level = 1.6f;
              snd (b, x);
              b.fx (0, FxDistortion, { { 0, (4 + 0.5f) / 16.0f }, { 1, 0.3f }, { 10, 0.5f } });
              room (b, 0.22f);
              fMacros (b, "Drive", "fx_a1_p2", 0.3f, "Release", "env1_r", 0.3f);
          } },
        { "Town Riff Lead", "Lead", "A snappy, resonant disco synth lead: short notes that pop.",
          [] (B& b) {
              Snd x; x.w1 = kSaw; x.w2 = kSquare; x.v2 = 0.4f; x.width = 0.3f; x.cutoff = 700.0f; x.reso = 0.35f; x.envAmt = 0.45f; x.fd = 0.15f; x.fs = 0.2f;
              x.a = 0.001f; x.d = 0.2f; x.s = 0.5f; x.r = 0.08f; x.mono = true; x.level = 3.2f;
              snd (b, x);
              room (b, 0.2f);
              fMacros (b, "Resonance", "f1_reso", 0.25f, "Decay", "env2_d", 0.3f);
          } },
        { "Robot Love Lead", "Lead", "A clean, simple, slightly hollow lead with an echo: precise, robotic electronic pop.",
          [] (B& b) {
              Snd x; x.w1 = kSquare; x.width = 0.5f; x.w2 = kTri; x.v2 = 0.4f; x.c2 = 12.0f; x.cutoff = 2500.0f; x.envAmt = 0.1f;
              x.a = 0.003f; x.d = 0.4f; x.s = 0.8f; x.r = 0.1f; x.mono = true; x.drift = 0.0f; x.level = 2.2f;
              snd (b, x);
              delay (b, 0, kDiv8D, 0.3f, 0.2f, false);
              room (b, 0.15f);
              fMacros (b, "Octave", "e1_o2_vol", 0.3f, "Release", "env1_r", 0.3f);
          } },
        { "Sawyer Growl", "Lead", "A resonant saw that growls as the filter sweeps down on every note. Prog-rock power.",
          [] (B& b) {
              Snd x; x.v2 = 0.7f; x.d2 = 0.1f; x.sub = 0.4f; x.model = kLadder; x.cutoff = 400.0f; x.reso = 0.5f; x.drive = 0.3f; x.envAmt = 0.45f;
              x.fa = 0.01f; x.fd = 1.2f; x.fs = 0.1f; x.a = 0.003f; x.d = 1.0f; x.s = 0.9f; x.r = 0.3f; x.mono = true; x.level = 2.0f;
              snd (b, x);
              room (b, 0.25f);
              fMacros (b, "Growl", "f1_reso", 0.25f, "Sweep Time", "env2_d", 0.3f);
          } },
        // ================================================================ KEYS, BELLS AND PLUCKS
        { "Mirror Bells", "Keys", "Glassy FM bell-piano: a soft body with a clear bell on top. Late-80s pop ballad keys.",
          [] (B& b) {
              fmVoice (b, 0, 1.0f, 0.1f, 0.7f);
              env (b, "env2", 0.001f, 1.0f, 0.0f, 0.5f);
              b.route ("Env 2", "e1_wt_pm", 0.25f, "Velocity", 1.0f);
              fmVoice (b, 1, 3.5f, 0.25f, 0.0f);
              env (b, "env3", 0.001f, 1.2f, 0.0f, 0.8f);
              b.set ("env3_link", 0.0f);
              b.route ("Env 3", "e2_vol", 0.4f, "Velocity", 0.8f);
              filter (b, 0, kLP12, 10000.0f, 0.0f, 0.3f);
              env (b, "env1", 0.0f, 4.0f, 0.0f, 1.0f);
              b.route ("Keytrack", "env1_d", -0.3f);
              b.set ("amp_velo", 0.6f);
              chorus (b, 0, 0.3f);
              hall (b, 0.3f);
              fMacros (b, "Bell", "e2_wt_pm", 0.25f, "Decay", "env1_d", 0.35f);
          } },
        { "Tainted Bink", "Pluck", "A tiny, percussive 'bink': a short, high synth blip with a little pitch drop.",
          [] (B& b) {
              Snd x; x.w1 = kSquare; x.width = 0.5f; x.cutoff = 3000.0f; x.envAmt = 0.0f; x.a = 0.001f; x.d = 0.08f; x.s = 0.0f; x.r = 0.06f;
              x.level = 1.6f; x.velo = 0.5f;
              snd (b, x);
              env (b, "env3", 0.0f, 0.04f, 0.0f, 0.02f);
              b.route ("Env 3", "e1_coarse", 2.0f / 120.0f);
              room (b, 0.25f);
              fMacros (b, "Length", "env1_d", 0.2f, "Release", "env1_r", 0.2f);
          } },
        { "Rains Pluck", "Pluck", "A woody, kalimba-like digital pluck: bright tink, warm body. Late-70s/early-80s pop percussion keys.",
          [] (B& b) {
              fmVoice (b, 0, 5.0f, 0.0f, 0.8f);
              env (b, "env2", 0.001f, 0.1f, 0.0f, 0.05f);
              b.route ("Env 2", "e1_wt_pm", 0.35f, "Velocity", 1.0f);
              filter (b, 0, kLP12, 6000.0f, 0.0f, 0.3f);
              b.set ("f1_vol", 1.4f);
              env (b, "env1", 0.001f, 0.8f, 0.0f, 0.4f);
              b.set ("amp_velo", 0.5f);
              room (b, 0.25f);
              fMacros (b, "Tink", "e1_wt_pm", 0.2f, "Decay", "env1_d", 0.35f);
          } },
        { "Rule the World Keys", "Keys", "Shimmering, plucky poly keys with chorus and a dotted echo: mid-80s sophisti-pop.",
          [] (B& b) {
              Snd x; x.w1 = kSaw; x.w2 = kSquare; x.v2 = 0.4f; x.c2 = 12.0f; x.width = 0.25f; x.cutoff = 1500.0f; x.envAmt = 0.3f; x.fd = 0.4f; x.fs = 0.2f;
              x.a = 0.001f; x.d = 1.2f; x.s = 0.25f; x.r = 0.5f;
              snd (b, x);
              vChorus (b, 0, 1, 0.45f);
              delay (b, 1, kDiv8D, 0.3f, 0.2f, true);
              hall (b, 0.28f);
              fMacros (b, "Echo", "fx_a2_p8", 0.35f, "Decay", "env1_d", 0.35f);
          } },
        { "Riley Organ Arp", "Sequence", "A bright, percussive organ played by a fast repeating arpeggio: the hypnotic 70s organ pulse.",
          [] (B& b) {
              osc (b, 0, 0, kSine, 0.8f);
              osc (b, 0, 1, kSquare, 0.3f, 12.0f, 0.0f, 0.5f);
              osc (b, 0, 2, kSine, 0.3f, 19.0f);
              b.set ("e1_vol", 0.6f);
              filter (b, 0, kLP12, 5000.0f, 0.0f, 0.3f);
              b.set ("f1_vol", 2.0f);
              env (b, "env1", 0.001f, 0.12f, 0.2f, 0.06f);
              arp (b, ArpAsPlayed, 1, kDiv16, 0.6f);
              room (b, 0.3f);
              fMacros (b, "Brightness 2", "e1_o2_vol", 0.3f, "Decay", "env1_d", 0.35f);
          } },
        { "Insomnia Pluck", "Pluck", "A bright, hypnotic saw pluck with resonance and a long echo: 90s anthem dance.",
          [] (B& b) {
              Snd x; x.v2 = 0.6f; x.d2 = 0.1f; x.cutoff = 700.0f; x.reso = 0.3f; x.envAmt = 0.45f; x.fd = 0.2f; x.fs = 0.0f;
              x.a = 0.001f; x.d = 0.35f; x.s = 0.0f; x.r = 0.25f; x.level = 1.8f;
              snd (b, x);
              delay (b, 0, kDiv8D, 0.4f, 0.28f, true);
              hall (b, 0.32f);
              fMacros (b, "Resonance", "f1_reso", 0.25f, "Decay", "env1_d", 0.35f);
          } },
        { "Dream Piano 96", "Keys", "A soft, synth-built dream-house piano in a huge reverb. Mid-90s euphoria (not a real piano).",
          [] (B& b) {
              osc (b, 0, 0, kTri, 0.8f);
              osc (b, 0, 1, kSine, 0.3f, 12.0f);
              osc (b, 0, 2, kSaw, 0.12f, 0.0f, 0.04f);
              b.set ("e1_vol", 0.75f);
              noise (b, 0, "Thump", 0.2f, 0.0f, false);
              filter (b, 0, kLP12, 1800.0f, 0.0f, 0.4f);
              env (b, "env2", 0.001f, 0.8f, 0.0f, 0.5f);
              b.route ("Env 2", "f1_cutoff", 0.2f, "Velocity", 1.0f);
              env (b, "env1", 0.001f, 4.0f, 0.0f, 1.0f);
              b.route ("Keytrack", "env1_d", -0.3f);
              b.set ("amp_velo", 0.6f);
              delay (b, 0, kDiv4D, 0.3f, 0.15f, true);
              hall (b, 0.5f, 0.95f, 0.9f);
              fMacros (b, "Hammer", "u_n1_vol", 0.3f, "Decay", "env1_d", 0.35f);
          } },
        { "Strobe Pluck", "Pluck", "A soft, round saw pluck with a resonant filter and a long, slow echo: patient progressive house.",
          [] (B& b) {
              Snd x; x.v2 = 0.5f; x.cutoff = 500.0f; x.reso = 0.35f; x.envAmt = 0.35f; x.fa = 0.01f; x.fd = 0.4f; x.fs = 0.0f;
              x.a = 0.005f; x.d = 0.6f; x.s = 0.0f; x.r = 0.4f; x.level = 1.8f;
              snd (b, x);
              delay (b, 0, kDiv4D, 0.45f, 0.3f, true);
              hall (b, 0.38f, 0.9f, 0.85f);
              fMacros (b, "Resonance", "f1_reso", 0.25f, "Decay", "env1_d", 0.35f);
          } },
        { "Beat Gong", "FX", "A deep, dark synthetic gong: inharmonic and slowly decaying. A dramatic intro hit.",
          [] (B& b) {
              wavetable (b, 0, "Basic Shapes", 0.0f, 0.7f);
              b.set ("e1_wt_mod_wave", 0.0f).set ("e1_wt_mod_tune_mode", 0.0f).set ("e1_wt_mod_ratio", 1.41f).set ("e1_wt_fm_type", 2.0f).set ("e1_wt_fm", 0.7f);
              b.set ("e1_coarse", -12.0f);
              env (b, "env2", 0.001f, 3.0f, 0.0f, 2.0f);
              b.route ("Env 2", "e1_wt_fm", 0.3f);
              noise (b, 0, "Thump", 0.3f, 0.0f, false);
              filter (b, 0, kLP12, 3000.0f, 0.0f, 0.2f);
              b.set ("f1_vol", 1.6f);
              env (b, "env1", 0.001f, 6.0f, 0.0f, 4.0f);
              hall (b, 0.45f, 0.95f, 0.9f);
              fMacros (b, "Ring", "e1_wt_fm", 0.2f, "Decay", "env1_d", 0.35f);
          } },
        // ================================================================ PADS AND STRINGS
        { "Tonight Air Pad", "Pad", "A dark, ominous, slowly breathing polysynth pad. Tension before the big drum moment.",
          [] (B& b) {
              Snd x; x.v2 = 0.6f; x.w2 = kSquare; x.width = 0.4f; x.model = kClassic; x.cutoff = 500.0f; x.reso = 0.2f; x.envAmt = 0.2f;
              x.fa = 1.5f; x.fd = 2.0f; x.fs = 0.6f; x.a = 1.0f; x.d = 1.0f; x.s = 1.0f; x.r = 2.0f; x.drift = 0.3f;
              snd (b, x);
              lfo (b, 0, LfoSine, 0.15f);
              b.set ("lfo1_retrig", 3.0f);
              b.route ("LFO 1", "f1_cutoff", 0.06f);
              hall (b, 0.42f, 0.9f, 0.85f);
              fMacros (b, "Motion", "lfo1_rate", 0.3f, "Attack", "env1_a", 0.35f);
          } },
        { "Oxygen Strings", "Strings/Brass", "Bright string-machine strings through a slow, deep phaser. Spacey 70s electronic.",
          [] (B& b) {
              Snd x; x.v2 = 0.7f; x.c2 = 12.0f; x.d2 = 0.1f; x.voices = 2; x.cutoff = 3500.0f; x.envAmt = 0.0f; x.a = 0.4f; x.d = 1.0f; x.s = 1.0f; x.r = 1.0f;
              snd (b, x);
              filter (b, 1, kHP12, 250.0f, 0.0f);
              b.set ("f_routing", 0.0f);
              b.fx (0, FxPhaser, { { 3, 0.7f }, { 4, 0.08f }, { 7, 0.55f } });
              hall (b, 0.38f, 0.85f, 0.8f);
              fMacros (b, "Phase", "fx_a1_p8", 0.3f, "Attack", "env1_a", 0.35f);
          } },
        { "Silence Pad", "Pad", "A dark, melancholy stringy pad with chorus: late-80s synth-pop gloom.",
          [] (B& b) {
              Snd x; x.v2 = 0.6f; x.voices = 2; x.cutoff = 900.0f; x.reso = 0.15f; x.envAmt = 0.15f; x.fa = 0.6f; x.fd = 1.5f; x.fs = 0.7f;
              x.a = 0.5f; x.d = 1.0f; x.s = 1.0f; x.r = 1.2f;
              snd (b, x);
              vChorus (b, 0, 0, 0.4f);
              hall (b, 0.38f, 0.85f, 0.8f);
              fMacros (b, "Chorus", "fx_a1_p5", 0.3f, "Attack", "env1_a", 0.35f);
          } },
        { "Cars Vox Pad", "Pad", "A buzzy, vocal-ish polyphonic synth pad with phaser: cold, new-wave 1979.",
          [] (B& b) {
              Snd x; x.w1 = kSquare; x.width = 0.3f; x.v2 = 0.6f; x.c2 = 12.0f; x.cutoff = 1800.0f; x.reso = 0.15f; x.envAmt = 0.1f;
              x.a = 0.15f; x.d = 1.0f; x.s = 1.0f; x.r = 0.8f;
              x.level = 2.0f;
              snd (b, x);
              b.set ("f2_on", 1.0f).set ("f2_type", 3.0f).set ("f2_cutoff", 1000.0f).set ("f2_reso", 0.3f).set ("f2_a", 0.5f).set ("f2_b", 0.5f)
                  .set ("f_routing", 0.0f);
              b.fx (0, FxPhaser, { { 3, 0.5f }, { 4, 0.12f }, { 7, 0.4f } });
              hall (b, 0.3f);
              fMacros (b, "Vox", "f2_b", 0.4f, "Attack", "env1_a", 0.35f);
          } },
        { "Detroit Strings", "Strings/Brass", "Bright, urgent string-synth chords with a short release: classic Detroit techno strings.",
          [] (B& b) {
              Snd x; x.v2 = 0.7f; x.d2 = 0.12f; x.voices = 2; x.cutoff = 3000.0f; x.envAmt = 0.1f; x.a = 0.05f; x.d = 1.0f; x.s = 1.0f; x.r = 0.3f;
              snd (b, x);
              filter (b, 1, kHP12, 300.0f, 0.0f);
              b.set ("f_routing", 0.0f);
              vChorus (b, 0, 2, 0.4f);
              hall (b, 0.3f);
              fMacros (b, "Thin", "f2_cutoff", 0.15f, "Release", "env1_r", 0.3f);
          } },
        { "Billie Pad", "Pad", "A warm, soft, slightly hollow pad that sits quietly under a tight groove. Early-80s pop-funk.",
          [] (B& b) {
              Snd x; x.w1 = kSquare; x.width = 0.35f; x.v2 = 0.5f; x.cutoff = 1000.0f; x.envAmt = 0.15f; x.fa = 0.3f; x.fd = 1.0f; x.fs = 0.7f;
              x.a = 0.25f; x.d = 1.0f; x.s = 1.0f; x.r = 0.6f;
              snd (b, x);
              vChorus (b, 0, 0, 0.35f);
              room (b, 0.25f);
              fMacros (b, "Chorus", "fx_a1_p5", 0.3f, "Attack", "env1_a", 0.35f);
          } },
        { "Vienna Strings", "Strings/Brass", "Grand, slow synth strings with a slight phaser: dramatic early-80s new romantic.",
          [] (B& b) {
              Snd x; x.v2 = 0.7f; x.d2 = 0.1f; x.voices = 2; x.cutoff = 2200.0f; x.envAmt = 0.1f; x.a = 0.6f; x.d = 1.0f; x.s = 1.0f; x.r = 1.5f;
              snd (b, x);
              b.fx (0, FxPhaser, { { 3, 0.4f }, { 4, 0.06f }, { 7, 0.3f } });
              hall (b, 0.42f, 0.9f, 0.85f);
              fMacros (b, "Phase", "fx_a1_p8", 0.3f, "Attack", "env1_a", 0.35f);
          } },
        // ================================================================ BASS AND SEQUENCES
        { "Monday Bass", "Bass", "A punchy, slightly hollow sequenced synth bass in eighths with octave jumps. Hold one note.",
          [] (B& b) {
              Snd x; x.w1 = kSquare; x.width = 0.4f; x.v2 = 0.5f; x.sub = 0.4f; x.cutoff = 450.0f; x.reso = 0.2f; x.envAmt = 0.35f; x.fd = 0.15f; x.fs = 0.2f;
              x.a = 0.002f; x.d = 0.2f; x.s = 0.4f; x.r = 0.08f; x.mono = true; x.level = 2.4f;
              snd (b, x);
              seq (b, kDiv8, { 1, 1, 1, 1, 1, 1, 1, 1 }, { 1, 5 }, 0.6f);
              fMacros (b, "Punch", "env2_d", 0.3f, "Gate", "env1_s", 0.3f);
          } },
        { "Sweet Dream Seq", "Bass", "A smooth, round, slightly resonant sequenced synth bass in sixteenths. Cool and hypnotic.",
          [] (B& b) {
              Snd x; x.v2 = 0.5f; x.w2 = kSquare; x.sub = 0.3f; x.cutoff = 500.0f; x.reso = 0.3f; x.envAmt = 0.25f; x.fd = 0.2f; x.fs = 0.3f;
              x.a = 0.002f; x.d = 0.2f; x.s = 0.4f; x.r = 0.08f; x.mono = true; x.level = 2.4f;
              snd (b, x);
              seq (b, kDiv16, { 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1 }, { 3, 11 }, 0.5f);
              vChorus (b, 0, 0, 0.2f, 0.3f);
              fMacros (b, "Resonance", "f1_reso", 0.25f, "Gate", "env1_s", 0.3f);
          } },
        { "Feel Love Seq", "Bass", "The pulsing late-70s modular sequence bass: note and octave in sixteenths, with echo. Hold one note.",
          [] (B& b) {
              Snd x; x.v2 = 0.5f; x.sub = 0.3f; x.model = kLadder; x.cutoff = 450.0f; x.reso = 0.25f; x.envAmt = 0.35f; x.fd = 0.12f; x.fs = 0.15f;
              x.a = 0.002f; x.d = 0.15f; x.s = 0.3f; x.r = 0.06f; x.mono = true; x.level = 2.4f;
              snd (b, x);
              seq (b, kDiv16, { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, { 2, 6, 10, 14 }, 0.5f);
              delay (b, 0, kDiv8D, 0.3f, 0.2f, true);
              fMacros (b, "Echo", "fx_a1_p8", 0.3f, "Gate", "env1_s", 0.3f);
          } },
        { "Thrill Bass", "Bass", "A round, punchy analog bass for tight, syncopated pop-funk lines. Played by hand.",
          [] (B& b) {
              Snd x; x.w1 = kSaw; x.v2 = 0.5f; x.w2 = kSquare; x.sub = 0.5f; x.model = kLadder; x.cutoff = 300.0f; x.reso = 0.15f; x.envAmt = 0.35f;
              x.fd = 0.2f; x.fs = 0.3f; x.a = 0.002f; x.d = 0.4f; x.s = 0.7f; x.r = 0.08f; x.mono = true; x.level = 2.4f; x.velo = 0.5f;
              snd (b, x);
              fMacros (b, "Punch", "env2_d", 0.3f, "Release", "env1_r", 0.3f);
          } },
        { "Jean Bass", "Bass", "A tight, dry, round analog bass with a short filter blip: the steady pop-funk pulse.",
          [] (B& b) {
              Snd x; x.w1 = kSquare; x.width = 0.45f; x.sub = 0.5f; x.model = kLadder; x.cutoff = 350.0f; x.reso = 0.1f; x.envAmt = 0.3f;
              x.fd = 0.12f; x.fs = 0.2f; x.a = 0.002f; x.d = 0.25f; x.s = 0.5f; x.r = 0.06f; x.mono = true; x.level = 2.4f;
              snd (b, x);
              fMacros (b, "Punch", "env2_d", 0.3f, "Decay", "env1_d", 0.3f);
          } },
        { "Chameleon Bass", "Bass", "A squelchy, resonant, funky analog bass: jazz-funk at its fattest.",
          [] (B& b) {
              Snd x; x.v2 = 0.5f; x.w2 = kSquare; x.sub = 0.3f; x.model = kClassic; x.cutoff = 300.0f; x.reso = 0.55f; x.envAmt = 0.45f;
              x.fd = 0.25f; x.fs = 0.2f; x.a = 0.002f; x.d = 0.4f; x.s = 0.7f; x.r = 0.08f; x.mono = true; x.legato = true; x.glide = 0.04f;
              x.level = 2.4f; x.velo = 0.5f;
              snd (b, x);
              b.route ("Velocity", "f1_cutoff", 0.15f);
              fMacros (b, "Squelch", "f1_reso", 0.2f, "Decay", "env2_d", 0.3f);
          } },
        { "Boogie Moog Bass", "Bass", "A fat, round, wah-ish synth bass with glide: 70s funk-soul on the synth.",
          [] (B& b) {
              Snd x; x.v2 = 0.6f; x.sub = 0.5f; x.model = kLadder; x.cutoff = 280.0f; x.reso = 0.3f; x.drive = 0.2f; x.envAmt = 0.4f;
              x.fa = 0.03f; x.fd = 0.35f; x.fs = 0.3f; x.a = 0.003f; x.d = 0.5f; x.s = 0.8f; x.r = 0.1f; x.mono = true; x.legato = true; x.glide = 0.06f;
              x.level = 2.4f;
              snd (b, x);
              fMacros (b, "Wah", "f1_reso", 0.25f, "Decay", "env2_d", 0.3f);
          } },
        { "Acid Line", "Bass", "The squelchy acid-house bass: one saw, a screaming resonant filter, accents and slides. Hold one note.",
          [] (B& b) {
              Snd x; x.v1 = 0.9f; x.model = kLadder; x.cutoff = 350.0f; x.reso = 0.7f; x.drive = 0.3f; x.envAmt = 0.45f; x.velAmt = 0.25f;
              x.fd = 0.18f; x.fs = 0.0f; x.a = 0.002f; x.d = 0.3f; x.s = 0.5f; x.r = 0.05f; x.mono = true; x.level = 2.6f; x.velo = 0.3f;
              snd (b, x);
              bassSeq (b, kDiv16, { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, 0.5f);
              SeqPattern p = gatePattern ({ 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1 }, 0.5f);
              for (int i : { 3, 6, 11 }) p.steps[(size_t) i].octave = 1;
              for (int i : { 0, 5, 9, 13 }) p.steps[(size_t) i].velocity = 1.0f;
              for (int i : { 4, 12 }) p.steps[(size_t) i].slide = 0.08f;
              b.pattern (p);
              delay (b, 0, kDiv8D, 0.25f, 0.15f, true);
              fMacros (b, "Resonance", "f1_reso", 0.2f, "Decay", "env2_d", 0.3f);
          } },
        { "FM Slap Bass 80s", "Bass", "The ubiquitous 80s digital FM bass: a sharp, slappy attack and a round body.",
          [] (B& b) {
              fmVoice (b, 0, 1.0f, 0.15f, 0.85f);
              env (b, "env2", 0.001f, 0.25f, 0.1f, 0.1f);
              b.route ("Env 2", "e1_wt_pm", 0.4f, "Velocity", 1.0f);
              osc (b, 1, 0, kSine, 0.8f, -12.0f);
              engineOn (b, 1, 0.3f);
              filter (b, 0, kLP12, 5000.0f, 0.0f, 0.3f);
              b.set ("f1_vol", 2.0f);
              env (b, "env1", 0.001f, 0.8f, 0.4f, 0.08f);
              b.set ("amp_velo", 0.6f);
              mono (b, false, 0.0f);
              fMacros (b, "Slap", "e1_wt_pm", 0.2f, "Decay", "env1_d", 0.3f);
          } },
        // ================================================================ ARPS AND SPECIAL
        { "Rio Arp", "Sequence", "Bright, sparkling polysynth arpeggio up and down in sixteenths: 80s new-wave glamour. Hold a chord.",
          [] (B& b) {
              Snd x; x.v2 = 0.5f; x.w2 = kSquare; x.width = 0.35f; x.cutoff = 1500.0f; x.reso = 0.15f; x.envAmt = 0.3f; x.fd = 0.15f; x.fs = 0.1f;
              x.a = 0.001f; x.d = 0.2f; x.s = 0.15f; x.r = 0.12f; x.level = 2.2f;
              snd (b, x);
              arp (b, ArpUpDownIncl, 3, kDiv16, 0.5f);
              delay (b, 0, kDiv8D, 0.25f, 0.15f, true);
              hall (b, 0.25f);
              fMacros (b, "Echo", "fx_a1_p8", 0.3f, "Decay", "env1_d", 0.35f);
          } },
        { "Upside Arp", "Sequence", "A dark, resonant analog arpeggio climbing in eighths, drenched in reverb: 80s-horror-series synth.",
          [] (B& b) {
              Snd x; x.v2 = 0.6f; x.d2 = 0.1f; x.model = kLadder; x.cutoff = 600.0f; x.reso = 0.35f; x.drive = 0.2f; x.envAmt = 0.3f; x.fd = 0.3f; x.fs = 0.3f;
              x.a = 0.002f; x.d = 0.4f; x.s = 0.4f; x.r = 0.2f; x.level = 2.2f;
              snd (b, x);
              lfo (b, 0, LfoSine, 0.08f);
              b.set ("lfo1_retrig", 3.0f);
              b.route ("LFO 1", "f1_cutoff", 0.1f);
              arp (b, ArpUp, 2, kDiv8, 0.6f);
              delay (b, 0, kDiv4D, 0.3f, 0.2f, true);
              hall (b, 0.42f, 0.9f, 0.85f);
              fMacros (b, "Resonance", "f1_reso", 0.25f, "Decay", "env1_d", 0.35f);
          } },
        { "Hoover Rave", "Lead", "The early-90s rave 'hoover': detuned pulse saws that swoop down in pitch, with chorus. Huge and menacing.",
          [] (B& b) {
              Snd x; x.w1 = kSaw; x.w2 = kSquare; x.v2 = 0.7f; x.width = 0.3f; x.voices = 4; x.uniDetune = 0.3f; x.uniStereo = 0.8f;
              x.cutoff = 3000.0f; x.envAmt = 0.0f; x.a = 0.002f; x.d = 0.5f; x.s = 0.9f; x.r = 0.4f; x.mono = true; x.legato = true; x.glide = 0.12f;
              x.level = 1.6f;
              snd (b, x);
              env (b, "env3", 0.0f, 0.25f, 0.0f, 0.1f);
              b.route ("Env 3", "e1_coarse", 3.0f / 120.0f);
              lfo (b, 0, LfoTriangle, 0.8f);
              b.route ("LFO 1", "e1_o2_width", 0.2f);
              vChorus (b, 0, 2, 0.5f);
              hall (b, 0.3f);
              fMacros (b, "Swoop", "env3_d", 0.3f, "Release", "env1_r", 0.3f);
          } },
    };
    return list;
}

juce::StringArray pack13Tags (const juce::String& presetName)
{
    static const std::map<juce::String, const char*> tags {
        { "Countdown Brass", "Famous, Brass, Synth Brass, 80s" }, { "Jump Poly", "Famous, Brass, Synth Brass, 80s" },
        { "Separate Brass", "Famous, Brass, Synth Brass, 80s" }, { "Runaway Poly", "Famous, Stab, 80s" },
        { "Party Stab 99", "Famous, Brass, Stab, 80s" }, { "Want Me Brass", "Famous, Brass, 80s" },
        { "Chariot Brass", "Famous, Brass, Cinematic, Expressive" }, { "Replicant Brass", "Famous, Brass, Cinematic, Dark" },
        { "Orchestra Hit 80s", "Famous, Stab, 80s" },
        { "Take On Lead", "Famous, Lead, 80s" }, { "Axel Lead", "Famous, Lead, 80s" }, { "Vice Lead", "Famous, Lead, 80s" },
        { "G-Funk Whistle", "Famous, Lead, 90s" }, { "Talk Box Lead", "Famous, Lead, Funk" }, { "Popcorn Pluck", "Famous, Lead, 70s" },
        { "Enough Lead", "Famous, Lead, 80s" }, { "Sandstorm Lead", "Famous, Lead, 90s, Trance" }, { "Levels Lead", "Famous, Lead, EDM" },
        { "Animal Drop", "Famous, Lead, EDM" }, { "Town Riff Lead", "Famous, Lead, 80s, Funk" }, { "Robot Love Lead", "Famous, Lead, 80s" },
        { "Sawyer Growl", "Famous, Lead, 80s" },
        { "Mirror Bells", "Famous, Electric Piano, Bells, FM, 80s" }, { "Tainted Bink", "Famous, Pluck, 80s" },
        { "Rains Pluck", "Famous, Pluck, Mallet, 80s" }, { "Rule the World Keys", "Famous, Synth Keys, 80s" },
        { "Riley Organ Arp", "Famous, Organ, Arp, 70s" }, { "Insomnia Pluck", "Famous, Pluck, 90s" },
        { "Dream Piano 96", "Famous, Piano, 90s" }, { "Strobe Pluck", "Famous, Pluck, EDM" }, { "Beat Gong", "Famous, Bells, 80s" },
        { "Tonight Air Pad", "Famous, Pad, Dark, 80s" }, { "Oxygen Strings", "Famous, Strings, 70s" }, { "Silence Pad", "Famous, Pad, 80s" },
        { "Cars Vox Pad", "Famous, Pad, 70s" }, { "Detroit Strings", "Famous, Strings, Techno" }, { "Billie Pad", "Famous, Pad, 80s" },
        { "Vienna Strings", "Famous, Strings, 80s" },
        { "Monday Bass", "Famous, Bass, Pulsing, 80s" }, { "Sweet Dream Seq", "Famous, Bass, Pulsing, 80s" },
        { "Feel Love Seq", "Famous, Bass, Pulsing, 70s" }, { "Thrill Bass", "Famous, Bass, 80s" }, { "Jean Bass", "Famous, Bass, 80s" },
        { "Chameleon Bass", "Famous, Bass, Funk, 70s" }, { "Boogie Moog Bass", "Famous, Bass, Funk, 70s" },
        { "Acid Line", "Famous, Bass, Pulsing, Acid" }, { "FM Slap Bass 80s", "Famous, Bass, FM, 80s" },
        { "Rio Arp", "Famous, Arp, 80s" }, { "Upside Arp", "Famous, Arp, Dark" }, { "Hoover Rave", "Famous, Lead, 90s" },
    };
    const auto it = tags.find (presetName);
    return it == tags.end() ? juce::StringArray() : PresetManager::parseTags (it->second);
}
