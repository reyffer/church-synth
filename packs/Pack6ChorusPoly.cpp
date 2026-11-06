// Preset Pack 6: Chorus Poly (the 80s one-oscillator chorus polysynth, Juno-style).
//
// Every sound is built the way that synth works, so the pack doubles as a lesson:
//   one oscillator per voice (saw and / or pulse with PWM) + a square sub an octave down
//   + a little noise -> a gentle highpass -> a smooth 4-pole lowpass (OTA model)
//   -> one envelope shared by filter and amp (or a simple gate for the amp)
//   -> the stereo chorus (I, II or I + II), which is half the sound.
// One LFO, with a delay (fade-in), for PWM, filter wobble or vibrato.
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

struct Poly
{
    float saw = 0.8f, pulse = 0.0f, width = 0.5f, pwm = 0.0f, sub = 0.0f, noise = 0.0f;
    int hpf = 1;            // 1 = flat, 2 = thin (240 Hz), 3 = thinner (720 Hz)
    float cutoff = 1500.0f, reso = 0.1f, envAmt = 0.3f, lfoFilter = 0.0f, kbd = 0.5f;
    float a = 0.005f, d = 0.6f, s = 0.7f, r = 0.3f;
    bool gate = false;      // amp = simple on/off gate, the envelope only moves the filter
    int chorus = 1;         // 0 off, 1 = I, 2 = II, 3 = I + II
    float chorusMix = 0.5f;
    float lfoRate = 0.6f, lfoDelay = 0.0f;
    float level = 1.0f;     // filter output level (one oscillator is quieter on single notes)
};

void poly (B& b, const Poly& p)
{
    osc (b, 0, 0, kSaw, p.saw);
    osc (b, 0, 1, kSquare, p.pulse, 0.0f, 0.0f, p.width);
    osc (b, 0, 2, kSquare, p.sub, -12.0f);
    b.set ("e1_vol", 0.7f).set ("e1_noise_vol", p.noise);

    lfo (b, 0, LfoTriangle, p.lfoRate);
    b.set ("lfo1_fade", p.lfoDelay);
    if (p.pwm > 0.0f)
        b.route ("LFO 1", "e1_o2_width", p.pwm);
    if (p.lfoFilter > 0.0f)
        b.route ("LFO 1", "f1_cutoff", p.lfoFilter);

    model (b, 0, kOta, 0, p.cutoff, p.reso, p.kbd);
    b.set ("f1_vol", p.level);
    if (p.hpf >= 2)
    {
        filter (b, 1, kHP6, p.hpf == 2 ? 240.0f : 720.0f, 0.0f);
        b.set ("f_routing", 0.0f);
    }

    // One envelope: Env 2 drives the filter with the same shape the amp uses.
    env (b, "env2", p.a, p.d, p.s, p.r);
    b.route ("Env 2", "f1_cutoff", p.envAmt);
    if (p.gate)
        env (b, "env1", 0.002f, 0.1f, 1.0f, std::max (0.02f, p.r * 0.3f));
    else
        env (b, "env1", p.a, p.d, p.s, p.r);
    b.set ("amp_velo", 0.25f); // the original had no velocity; a little helps on a modern keyboard

    if (p.chorus > 0)
    {
        vintageChorus (b, 0, p.chorus - 1, p.chorusMix);
        b.set ("fx_a1_p6", 0.7f);
    }
}

void polyMacros (B& b, const char* m3, const char* m3dest, float m3amt, const char* m4, const char* m4dest, float m4amt)
{
    macros (b, "Brightness", "Space", m3, m4);
    b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", m3dest, m3amt).route ("Macro 4", m4dest, m4amt);
}
} // namespace

const std::vector<FactoryPreset>& pack6Presets()
{
    static const std::vector<FactoryPreset> list {
        // ================================================================ PADS
        { "PWM Strings", "Strings/Brass", "The famous pulse-width strings: one pulse wave, slowly modulated, into chorus II.",
          [] (B& b) {
              Poly p;
              p.saw = 0.0f; p.pulse = 0.9f; p.width = 0.5f; p.pwm = 0.35f; p.lfoRate = 0.7f; p.hpf = 2;
              p.cutoff = 3500.0f; p.envAmt = 0.0f; p.a = 0.4f; p.d = 1.0f; p.s = 1.0f; p.r = 0.8f; p.chorus = 2;
              poly (b, p);
              hall (b, 0.3f, 0.75f, 0.65f);
              polyMacros (b, "PWM Speed", "lfo1_rate", 0.3f, "Attack", "env1_a", 0.35f);
          } },
        { "Saw Strings", "Strings/Brass", "Bright saw strings with chorus I + II and a thin highpass. The 80s string pad.",
          [] (B& b) {
              Poly p;
              p.saw = 0.9f; p.hpf = 3; p.cutoff = 4000.0f; p.envAmt = 0.0f; p.a = 0.35f; p.d = 1.0f; p.s = 1.0f; p.r = 0.7f; p.chorus = 3;
              poly (b, p);
              hall (b, 0.3f, 0.75f, 0.65f);
              polyMacros (b, "Thin", "f2_cutoff", 0.15f, "Attack", "env1_a", 0.35f);
          } },
        { "Dream Pad", "Pad", "Saw, pulse and sub with a slow filter wave and chorus I. Soft and warm.",
          [] (B& b) {
              Poly p;
              p.saw = 0.6f; p.pulse = 0.5f; p.pwm = 0.2f; p.sub = 0.3f; p.cutoff = 900.0f; p.reso = 0.15f; p.envAmt = 0.15f;
              p.lfoFilter = 0.06f; p.lfoRate = 0.25f; p.a = 0.8f; p.d = 1.5f; p.s = 0.9f; p.r = 1.5f; p.chorus = 1;
              poly (b, p);
              hall (b, 0.38f, 0.8f, 0.7f);
              polyMacros (b, "Motion", "lfo1_rate", 0.3f, "Attack", "env1_a", 0.35f);
          } },
        { "Night Drive Pad", "Pad", "Dark pad with a sub and a slow filter opening on every chord. Moody.",
          [] (B& b) {
              Poly p;
              p.saw = 0.8f; p.sub = 0.5f; p.cutoff = 400.0f; p.reso = 0.25f; p.envAmt = 0.3f; p.a = 1.2f; p.d = 2.0f; p.s = 0.6f; p.r = 1.2f;
              p.chorus = 1;
              poly (b, p);
              hall (b, 0.35f, 0.8f, 0.7f);
              polyMacros (b, "Sub", "e1_o3_vol", 0.3f, "Attack", "env1_a", 0.35f);
          } },
        { "Breathing Pad", "Pad", "PWM pad whose filter breathes in and out with the LFO. Delayed vibrato on long notes.",
          [] (B& b) {
              Poly p;
              p.saw = 0.3f; p.pulse = 0.8f; p.pwm = 0.3f; p.cutoff = 1200.0f; p.reso = 0.2f; p.envAmt = 0.1f; p.lfoFilter = 0.1f;
              p.lfoRate = 0.35f; p.a = 0.6f; p.d = 1.0f; p.s = 1.0f; p.r = 1.2f; p.chorus = 2;
              poly (b, p);
              hall (b, 0.35f, 0.8f, 0.7f);
              polyMacros (b, "Breath Speed", "lfo1_rate", 0.3f, "Attack", "env1_a", 0.35f);
          } },
        { "Sweep Pad", "Pad", "A resonant filter sweeping slowly up and down across the chord. Classic and hypnotic.",
          [] (B& b) {
              Poly p;
              p.saw = 0.8f; p.sub = 0.25f; p.cutoff = 900.0f; p.reso = 0.5f; p.envAmt = 0.0f; p.lfoFilter = 0.18f; p.lfoRate = 0.12f;
              p.a = 0.5f; p.d = 1.0f; p.s = 1.0f; p.r = 1.0f; p.chorus = 3;
              poly (b, p);
              b.set ("lfo1_retrig", 3.0f);
              hall (b, 0.32f, 0.75f, 0.65f);
              polyMacros (b, "Sweep Speed", "lfo1_rate", 0.3f, "Resonance", "f1_reso", 0.3f);
          } },
        // ================================================================ KEYS, STABS, PLUCKS
        { "Chorus Brass", "Strings/Brass", "One-envelope brass: the same envelope opens the filter and shapes the volume.",
          [] (B& b) {
              Poly p;
              p.saw = 0.9f; p.sub = 0.2f; p.cutoff = 600.0f; p.reso = 0.1f; p.envAmt = 0.4f; p.a = 0.06f; p.d = 0.5f; p.s = 0.6f; p.r = 0.3f;
              p.chorus = 1;
              poly (b, p);
              room (b, 0.2f);
              polyMacros (b, "Bite", "env2_d", -0.25f, "Attack", "env1_a", 0.3f);
          } },
        { "Poly Stab", "Keys", "Short, punchy saw-and-pulse stab for dance and pop chords.",
          [] (B& b) {
              Poly p;
              p.saw = 0.7f; p.pulse = 0.5f; p.width = 0.35f; p.cutoff = 700.0f; p.reso = 0.2f; p.envAmt = 0.45f;
              p.a = 0.002f; p.d = 0.3f; p.s = 0.0f; p.r = 0.2f; p.chorus = 2;
              poly (b, p);
              delay (b, 1, kDiv8D, 0.25f, 0.15f, true);
              room (b, 0.2f);
              polyMacros (b, "Echo", "fx_a2_p8", 0.35f, "Decay", "env1_d", 0.35f);
          } },
        { "Chorus Keys", "Keys", "Soft PWM keys that decay to a gentle sustain. The bread-and-butter 80s comp.",
          [] (B& b) {
              Poly p;
              p.saw = 0.2f; p.pulse = 0.8f; p.pwm = 0.2f; p.cutoff = 1000.0f; p.envAmt = 0.3f; p.a = 0.003f; p.d = 0.8f; p.s = 0.4f; p.r = 0.4f;
              p.chorus = 2;
              poly (b, p);
              room (b, 0.22f);
              polyMacros (b, "Chorus", "fx_a1_p5", 0.35f, "Decay", "env1_d", 0.35f);
          } },
        { "Pulse Organ", "Keys", "Square wave and sub through a gate: the polysynth playing organ. Chorus I + II.",
          [] (B& b) {
              Poly p;
              p.saw = 0.0f; p.pulse = 0.8f; p.sub = 0.45f; p.cutoff = 2500.0f; p.envAmt = 0.0f; p.a = 0.002f; p.d = 0.1f; p.s = 1.0f; p.r = 0.08f;
              p.gate = true; p.chorus = 3; p.lfoRate = 5.5f; p.lfoDelay = 0.6f;
              poly (b, p);
              b.route ("LFO 1", "e1_fine", 0.004f);
              room (b, 0.25f);
              polyMacros (b, "Sub", "e1_o3_vol", 0.3f, "Release", "env1_r", 0.3f);
          } },
        { "Poly Pluck", "Pluck", "Bright, short pluck with a quick filter snap. Great with the arpeggiator or for rhythmic parts.",
          [] (B& b) {
              Poly p;
              p.saw = 0.8f; p.pulse = 0.3f; p.width = 0.3f; p.cutoff = 500.0f; p.reso = 0.25f; p.envAmt = 0.5f;
              p.a = 0.001f; p.d = 0.25f; p.s = 0.0f; p.r = 0.25f; p.chorus = 1;
              poly (b, p);
              delay (b, 1, kDiv8D, 0.3f, 0.2f, true);
              room (b, 0.2f);
              polyMacros (b, "Resonance", "f1_reso", 0.3f, "Decay", "env1_d", 0.35f);
          } },
        { "Resonant Bell", "Pluck", "A narrow pulse and high resonance ring like a bell on each note.",
          [] (B& b) {
              Poly p;
              p.saw = 0.0f; p.pulse = 0.8f; p.width = 0.15f; p.hpf = 2; p.cutoff = 1800.0f; p.reso = 0.7f; p.envAmt = 0.3f; p.kbd = 1.0f;
              p.a = 0.001f; p.d = 1.2f; p.s = 0.0f; p.r = 1.2f; p.chorus = 2;
              poly (b, p);
              delay (b, 1, kDiv4D, 0.3f, 0.18f, true);
              hall (b, 0.3f, 0.75f, 0.65f);
              polyMacros (b, "Ring", "f1_reso", 0.2f, "Decay", "env1_d", 0.35f);
          } },
        { "Chord Memory Stab", "Keys", "One key plays a whole minor-seventh chord: the classic house and garage stab.",
          [] (B& b) {
              Poly p;
              p.saw = 0.7f; p.pulse = 0.4f; p.cutoff = 900.0f; p.reso = 0.15f; p.envAmt = 0.4f; p.a = 0.002f; p.d = 0.35f; p.s = 0.0f; p.r = 0.25f;
              p.chorus = 2;
              poly (b, p);
              uni (b, 0, 1, 4, 0.0f, 0.5f);
              b.set ("e1_uni_chord", 8.0f); // Minor 7
              delay (b, 1, kDiv8D, 0.3f, 0.18f, true);
              room (b, 0.2f);
              polyMacros (b, "Echo", "fx_a2_p8", 0.35f, "Decay", "env1_d", 0.35f);
          } },
        // ================================================================ BASS
        { "Square Sub Bass", "Bass", "Pulse plus sub, a short filter blip: the round, solid house bass.",
          [] (B& b) {
              Poly p;
              p.saw = 0.0f; p.pulse = 0.7f; p.sub = 0.8f; p.cutoff = 350.0f; p.reso = 0.1f; p.envAmt = 0.25f; p.kbd = 0.3f;
              p.a = 0.002f; p.d = 0.4f; p.s = 0.6f; p.r = 0.12f; p.chorus = 1; p.chorusMix = 0.25f;
              p.level = 2.6f;
              poly (b, p);
              mono (b, false, 0.0f);
              b.set ("fx_a1_p6", 0.3f);
              polyMacros (b, "Sub", "e1_o3_vol", 0.2f, "Decay", "env2_d", 0.3f);
          } },
        { "Saw Bass Punch", "Bass", "Saw and sub with a punchy filter envelope. Funky, solid, cuts on small speakers.",
          [] (B& b) {
              Poly p;
              p.saw = 0.8f; p.sub = 0.6f; p.cutoff = 300.0f; p.reso = 0.2f; p.envAmt = 0.4f; p.kbd = 0.3f;
              p.a = 0.001f; p.d = 0.25f; p.s = 0.3f; p.r = 0.1f; p.chorus = 0;
              p.level = 3.6f;
              poly (b, p);
              mono (b, false, 0.0f);
              polyMacros (b, "Resonance", "f1_reso", 0.3f, "Decay", "env2_d", 0.3f);
          } },
        { "Rubber Bass", "Bass", "High resonance and a fast envelope: a bouncy, rubbery bass.",
          [] (B& b) {
              Poly p;
              p.saw = 0.0f; p.pulse = 0.8f; p.width = 0.4f; p.sub = 0.5f; p.cutoff = 250.0f; p.reso = 0.55f; p.envAmt = 0.4f; p.kbd = 0.4f;
              p.a = 0.001f; p.d = 0.18f; p.s = 0.2f; p.r = 0.1f; p.chorus = 0;
              p.level = 3.4f;
              poly (b, p);
              mono (b, false, 0.0f);
              polyMacros (b, "Rubber", "f1_reso", 0.25f, "Decay", "env2_d", 0.3f);
          } },
        { "Arp Bass 16ths", "Bass", "Saw-and-sub bass arpeggiated in 16ths over two octaves. Hold one note; follows the tempo.",
          [] (B& b) {
              Poly p;
              p.saw = 0.7f; p.sub = 0.6f; p.cutoff = 400.0f; p.reso = 0.25f; p.envAmt = 0.35f; p.kbd = 0.3f;
              p.a = 0.001f; p.d = 0.15f; p.s = 0.2f; p.r = 0.08f; p.chorus = 1; p.chorusMix = 0.25f;
              p.level = 3.2f;
              poly (b, p);
              mono (b, false, 0.0f);
              b.set ("fx_a1_p6", 0.3f);
              b.set ("seq_on", 1.0f).set ("seq_mode", 0.0f).set ("seq_arp_order", (float) ArpUp).set ("seq_arp_octaves", 2.0f)
                  .set ("seq_rate", (float) kDiv16);
              b.pattern (gatePattern ({ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, 0.5f));
              polyMacros (b, "Resonance", "f1_reso", 0.3f, "Decay", "env2_d", 0.3f);
          } },
        // ================================================================ LEADS AND ARPS
        { "Chorus Lead", "Lead", "Mono saw lead with a delayed vibrato and chorus. Smooth 80s melody lines.",
          [] (B& b) {
              Poly p;
              p.saw = 0.8f; p.pulse = 0.4f; p.width = 0.4f; p.cutoff = 1800.0f; p.reso = 0.15f; p.envAmt = 0.2f;
              p.a = 0.01f; p.d = 0.5f; p.s = 0.85f; p.r = 0.25f; p.chorus = 1; p.lfoRate = 5.5f; p.lfoDelay = 0.5f;
              p.level = 2.6f;
              poly (b, p);
              mono (b, true, 0.03f);
              b.route ("LFO 1", "e1_fine", 0.006f);
              delay (b, 1, kDiv8D, 0.3f, 0.18f, true);
              room (b, 0.25f);
              polyMacros (b, "Vibrato Speed", "lfo1_rate", 0.1f, "Release", "env1_r", 0.3f);
          } },
        { "Resonant Whistle", "Lead", "The filter itself sings: high resonance, key-tracked, with a little saw underneath. Eerie and pure.",
          [] (B& b) {
              Poly p;
              p.saw = 0.25f; p.cutoff = 523.0f; p.reso = 0.92f; p.envAmt = 0.05f; p.kbd = 1.0f;
              p.a = 0.03f; p.d = 0.5f; p.s = 1.0f; p.r = 0.4f; p.chorus = 1; p.lfoRate = 5.0f; p.lfoDelay = 0.6f;
              p.level = 1.0f;
              poly (b, p);
              mono (b, true, 0.06f);
              b.route ("LFO 1", "f1_cutoff", 0.006f);
              b.set ("f1_vol", 4.0f);
              delay (b, 1, kDiv4D, 0.3f, 0.2f, true);
              hall (b, 0.3f, 0.75f, 0.65f);
              polyMacros (b, "Body", "e1_o1_vol", 0.3f, "Release", "env1_r", 0.3f);
          } },
        { "Chorus Arp", "Sequence", "The built-in-arpeggiator sound: up and down in 16ths with PWM, chorus and an echo. Hold a chord.",
          [] (B& b) {
              Poly p;
              p.saw = 0.4f; p.pulse = 0.7f; p.pwm = 0.25f; p.cutoff = 900.0f; p.reso = 0.2f; p.envAmt = 0.35f;
              p.a = 0.001f; p.d = 0.2f; p.s = 0.1f; p.r = 0.15f; p.chorus = 2;
              p.level = 2.4f;
              poly (b, p);
              b.set ("seq_on", 1.0f).set ("seq_mode", 0.0f).set ("seq_arp_order", (float) ArpUpDownIncl).set ("seq_arp_octaves", 2.0f)
                  .set ("seq_rate", (float) kDiv16);
              b.pattern (gatePattern ({ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, 0.5f));
              delay (b, 1, kDiv8D, 0.3f, 0.2f, true);
              room (b, 0.22f);
              polyMacros (b, "Echo", "fx_a2_p8", 0.35f, "Decay", "env1_d", 0.35f);
          } },
    };
    return list;
}

juce::StringArray pack6Tags (const juce::String& presetName)
{
    static const std::map<juce::String, const char*> tags {
        { "PWM Strings", "Chorus Poly, Strings, PWM" }, { "Saw Strings", "Chorus Poly, Strings" },
        { "Dream Pad", "Chorus Poly, Pad, Worship" }, { "Night Drive Pad", "Chorus Poly, Pad, Dark" },
        { "Breathing Pad", "Chorus Poly, Pad, PWM, Evolving" }, { "Sweep Pad", "Chorus Poly, Pad, Evolving" },
        { "Chorus Brass", "Chorus Poly, Brass, Synth Brass" }, { "Poly Stab", "Chorus Poly, Stab, Synth Keys" },
        { "Chorus Keys", "Chorus Poly, Synth Keys, PWM" }, { "Pulse Organ", "Chorus Poly, Organ" },
        { "Poly Pluck", "Chorus Poly, Pluck" }, { "Resonant Bell", "Chorus Poly, Bells" },
        { "Chord Memory Stab", "Chorus Poly, Stab, Chord" },
        { "Square Sub Bass", "Chorus Poly, Bass, Sub" }, { "Saw Bass Punch", "Chorus Poly, Bass" },
        { "Rubber Bass", "Chorus Poly, Bass" }, { "Arp Bass 16ths", "Chorus Poly, Bass, Arp, Pulsing" },
        { "Chorus Lead", "Chorus Poly, Lead, Mono" }, { "Resonant Whistle", "Chorus Poly, Lead, Mono" },
        { "Chorus Arp", "Chorus Poly, Arp, PWM" },
    };
    const auto it = tags.find (presetName);
    return it == tags.end() ? juce::StringArray() : PresetManager::parseTags (it->second);
}
