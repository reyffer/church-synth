// Preset Pack 11: Synthwave.
//
// 80s-retro sounds for synthwave / retrowave / outrun: sequenced basses, arps,
// plucks, wide pads, hero leads and FM keys. The style's ingredients, used on
// purpose so the notes can point at them: chorus everywhere, big reverbs, dotted
// echoes, tape wobble, sidechain-style pumping, unison saws, octave basses.
//
// Macros: M1 Brightness, M2 Space, M3 a character control, M4 Decay / Attack / Release / Gate.

#include "Packs.h"

#include "plugin/PresetHelpers.h"

#include <map>

using namespace presethelpers;

namespace
{
constexpr int kOta = 11, kLadder = 9;

struct Neon
{
    int w1 = kSaw, w2 = kSaw;
    float v1 = 0.8f, v2 = 0.0f, c2 = 0.0f, d2 = 0.07f, width = 0.5f, sub = 0.0f;
    int voices = 1, uniMode = 0; float uniDetune = 0.12f, uniStereo = 0.6f;
    int model = kOta; int submode = 0;          // filter model (OTA = smooth 80s poly, Ladder = mono)
    float cutoff = 1200.0f, reso = 0.15f, kbd = 0.4f, drive = 0.0f;
    float envAmt = 0.3f, velAmt = 0.1f;
    float fa = 0.001f, fd = 0.4f, fs = 0.3f, fr = 0.3f;
    float a = 0.002f, d = 0.5f, s = 0.8f, r = 0.3f;
    bool mono = false, legato = false; float glide = 0.0f;
    float level = 1.0f, velo = 0.4f, drift = 0.2f;
};

void neon (B& b, const Neon& n)
{
    osc (b, 0, 0, n.w1, n.v1, 0.0f, 0.0f, n.width);
    osc (b, 0, 1, n.w2, n.v2, n.c2, n.d2, n.width);
    osc (b, 0, 2, kSquare, n.sub, -12.0f);
    b.set ("e1_vol", 0.6f).set ("e1_drift", n.drift);
    if (n.voices > 1)
    {
        uni (b, 0, n.uniMode, n.voices, n.uniDetune, n.uniStereo);
        if (n.uniMode == 2)
            b.set ("e1_uni_mix", 0.6f);
    }
    model (b, 0, n.model, n.submode, n.cutoff, n.reso, n.kbd, n.drive);
    b.set ("f1_vol", n.level);
    env (b, "env2", n.fa, n.fd, n.fs, n.fr);
    b.route ("Env 2", "f1_cutoff", n.envAmt);
    if (n.velAmt > 0.0f)
        b.route ("Velocity", "f1_cutoff", n.velAmt);
    env (b, "env1", n.a, n.d, n.s, n.r);
    b.set ("amp_velo", n.velo);
    if (n.mono)
        mono (b, n.legato, n.glide);
}

void vChorus (B& b, int slot, int mode, float mix, float width = 0.7f)
{
    vintageChorus (b, slot, mode, mix);
    b.set (juce::String ("fx_") + kSlot[slot] + "_p6", width);
}

void tape (B& b, int slot, int division, float intensity, float wow, float mix)
{
    b.fx (slot, FxTapeEcho, { { 0, syncKnob (division) }, { 2, intensity }, { 4, wow }, { 7, mix } });
    b.set (juce::String ("fx_") + kSlot[slot] + "_sync", 1.0f);
}

// Sidechain-style pump: Function 1 "Duck" synced to quarter notes on the amp.
void pump (B& b, float depth)
{
    b.set ("func1_shape", 15.0f).set ("func1_sync", 1.0f).set ("func1_div", (float) kDiv4).set ("func1_retrig", 3.0f)
        .set ("func1_smooth", 0.08f);
    b.set ("amp_mod_src", 11.0f).set ("amp_velo", depth);
}

void arp (B& b, int order, int octaves, int rate, float gate)
{
    b.set ("seq_on", 1.0f).set ("seq_mode", 0.0f).set ("seq_arp_order", (float) order).set ("seq_arp_octaves", (float) octaves)
        .set ("seq_rate", (float) rate);
    b.pattern (gatePattern ({ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, gate));
}

// Sequenced bass: steps (1 = note, 0 = rest), with optional octave-up steps.
void seqBass (B& b, int rate, std::initializer_list<int> steps, std::initializer_list<int> octaveUp, float gate)
{
    bassSeq (b, rate, steps, gate);
    SeqPattern p = gatePattern (steps, gate);
    for (int i : octaveUp)
        p.steps[(size_t) i].octave = 1;
    b.pattern (p);
}

void nMacros (B& b, const char* m3, const char* m3dest, float m3amt, const char* m4, const char* m4dest, float m4amt)
{
    macros (b, "Brightness", "Space", m3, m4);
    b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", m3dest, m3amt).route ("Macro 4", m4dest, m4amt);
}

constexpr int kDiv16T = 15;
} // namespace

const std::vector<FactoryPreset>& pack11Presets()
{
    static const std::vector<FactoryPreset> list {
        // ================================================================ BASS
        { "Outrun Bass", "Bass", "The driving sixteenth-note synthwave bass. Hold one note; it follows the tempo.",
          [] (B& b) {
              Neon n; n.v2 = 0.6f; n.sub = 0.5f; n.cutoff = 380.0f; n.reso = 0.2f; n.envAmt = 0.35f; n.fd = 0.15f; n.fs = 0.15f;
              n.d = 0.2f; n.s = 0.4f; n.r = 0.08f; n.mono = true; n.level = 2.4f;
              neon (b, n);
              seqBass (b, kDiv16, { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, {}, 0.6f);
              nMacros (b, "Punch", "env2_d", 0.3f, "Gate", "env1_s", 0.3f);
          } },
        { "Octave Runner Bass", "Bass", "Sixteenths bouncing between the note and its octave. Instant 80s chase scene.",
          [] (B& b) {
              Neon n; n.v2 = 0.5f; n.sub = 0.4f; n.cutoff = 420.0f; n.reso = 0.25f; n.envAmt = 0.35f; n.fd = 0.12f; n.fs = 0.1f;
              n.d = 0.18f; n.s = 0.3f; n.r = 0.08f; n.mono = true; n.level = 2.4f;
              neon (b, n);
              seqBass (b, kDiv16, { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, { 1, 3, 5, 7, 9, 11, 13, 15 }, 0.5f);
              vChorus (b, 0, 0, 0.25f, 0.3f);
              nMacros (b, "Resonance", "f1_reso", 0.25f, "Gate", "env1_s", 0.3f);
          } },
        { "Galloping Bass", "Bass", "The 'gallop' rhythm (one-and-a, one-and-a) on a punchy saw bass.",
          [] (B& b) {
              Neon n; n.v2 = 0.6f; n.sub = 0.5f; n.cutoff = 350.0f; n.reso = 0.2f; n.envAmt = 0.4f; n.fd = 0.12f; n.fs = 0.1f;
              n.d = 0.15f; n.s = 0.3f; n.r = 0.06f; n.mono = true; n.level = 2.4f;
              neon (b, n);
              seqBass (b, kDiv16, { 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1 }, {}, 0.6f);
              nMacros (b, "Punch", "env2_d", 0.3f, "Gate", "env1_s", 0.3f);
          } },
        { "Analog Saw Bass", "Bass", "Fat ladder saw bass with a sub, played by hand. Warm and driving.",
          [] (B& b) {
              Neon n; n.v2 = 0.6f; n.sub = 0.5f; n.model = kLadder; n.cutoff = 300.0f; n.reso = 0.15f; n.drive = 0.3f; n.envAmt = 0.35f;
              n.fd = 0.3f; n.fs = 0.3f; n.d = 0.5f; n.s = 0.8f; n.r = 0.1f; n.mono = true; n.level = 2.4f;
              neon (b, n);
              nMacros (b, "Drive", "f1_drive", 0.3f, "Release", "env1_r", 0.3f);
          } },
        { "Sub Pulse Bass", "Bass", "Round pulse and sub in eighths with long gates: deep and pulsing under pads.",
          [] (B& b) {
              Neon n; n.w1 = kSquare; n.width = 0.4f; n.sub = 0.7f; n.cutoff = 300.0f; n.envAmt = 0.25f; n.fd = 0.2f; n.fs = 0.3f;
              n.d = 0.3f; n.s = 0.6f; n.r = 0.08f; n.mono = true; n.level = 2.4f;
              neon (b, n);
              seqBass (b, kDiv8, { 1, 1, 1, 1, 1, 1, 1, 1 }, {}, 0.8f);
              nMacros (b, "Sub", "e1_o3_vol", 0.25f, "Gate", "env1_s", 0.3f);
          } },
        { "Dark Synth Bass", "Bass", "Dark, driven bass in sixteenths with a slow filter wave over several bars. For the darker side.",
          [] (B& b) {
              Neon n; n.v2 = 0.6f; n.c2 = -12.0f; n.sub = 0.0f; n.model = kLadder; n.cutoff = 250.0f; n.reso = 0.3f; n.drive = 0.5f;
              n.envAmt = 0.3f; n.fd = 0.12f; n.fs = 0.1f; n.d = 0.2f; n.s = 0.3f; n.r = 0.06f; n.mono = true; n.level = 2.4f;
              neon (b, n);
              lfo (b, 0, LfoSine, 0.08f);
              b.set ("lfo1_retrig", 3.0f);
              b.route ("LFO 1", "f1_cutoff", 0.1f);
              seqBass (b, kDiv16, { 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1 }, {}, 0.5f);
              nMacros (b, "Drive", "f1_drive", 0.3f, "Gate", "env1_s", 0.3f);
          } },
        { "Funk Bass 80s", "Bass", "Snappy, resonant pulse bass for funky 80s lines, played by hand.",
          [] (B& b) {
              Neon n; n.w1 = kSquare; n.width = 0.35f; n.v2 = 0.4f; n.sub = 0.4f; n.cutoff = 300.0f; n.reso = 0.35f; n.envAmt = 0.45f;
              n.fd = 0.15f; n.fs = 0.15f; n.d = 0.35f; n.s = 0.5f; n.r = 0.06f; n.mono = true; n.level = 2.4f; n.velo = 0.5f;
              neon (b, n);
              nMacros (b, "Resonance", "f1_reso", 0.25f, "Decay", "env2_d", 0.3f);
          } },
        { "Triplet Bass", "Bass", "Bass in sixteenth-note triplets: a rolling, cinematic pulse.",
          [] (B& b) {
              Neon n; n.v2 = 0.6f; n.sub = 0.5f; n.cutoff = 380.0f; n.reso = 0.2f; n.envAmt = 0.35f; n.fd = 0.12f; n.fs = 0.1f;
              n.d = 0.15f; n.s = 0.3f; n.r = 0.06f; n.mono = true; n.level = 2.4f;
              neon (b, n);
              seqBass (b, kDiv16T, { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, { 2, 5, 8, 11 }, 0.5f);
              nMacros (b, "Punch", "env2_d", 0.3f, "Gate", "env1_s", 0.3f);
          } },
        // ================================================================ SEQUENCES AND ARPS
        { "Night City Arp", "Sequence", "Sixteenth arpeggio up two octaves, PWM and chorus, a dotted echo. The synthwave arp.",
          [] (B& b) {
              Neon n; n.w1 = kSaw; n.w2 = kSquare; n.v1 = 0.5f; n.v2 = 0.6f; n.cutoff = 900.0f; n.reso = 0.2f; n.envAmt = 0.35f;
              n.fd = 0.18f; n.fs = 0.1f; n.d = 0.25f; n.s = 0.2f; n.r = 0.15f; n.level = 2.6f;
              neon (b, n);
              lfo (b, 0, LfoTriangle, 0.6f);
              b.route ("LFO 1", "e1_o2_width", 0.25f);
              arp (b, ArpUp, 2, kDiv16, 0.5f);
              vChorus (b, 0, 1, 0.45f);
              delay (b, 1, kDiv8D, 0.35f, 0.22f, true);
              hall (b, 0.3f, 0.8f, 0.7f);
              nMacros (b, "Echo", "fx_a2_p8", 0.35f, "Decay", "env1_d", 0.35f);
          } },
        { "Neon Up Arp", "Sequence", "Bright saw arpeggio climbing three octaves in sixteenths. Energetic.",
          [] (B& b) {
              Neon n; n.v2 = 0.5f; n.cutoff = 1100.0f; n.reso = 0.15f; n.envAmt = 0.35f; n.fd = 0.15f; n.fs = 0.1f;
              n.d = 0.2f; n.s = 0.15f; n.r = 0.12f; n.level = 2.6f;
              neon (b, n);
              arp (b, ArpUp, 3, kDiv16, 0.5f);
              vChorus (b, 0, 1, 0.4f);
              delay (b, 1, kDiv8D, 0.3f, 0.2f, true);
              hall (b, 0.28f);
              nMacros (b, "Echo", "fx_a2_p8", 0.35f, "Decay", "env1_d", 0.35f);
          } },
        { "Arpeggio Bells", "Sequence", "FM bells arpeggiated up and down in eighths, with a long echo. Dreamy and bright.",
          [] (B& b) {
              fmVoice (b, 0, 3.5f, 0.15f, 0.7f);
              env (b, "env2", 0.001f, 0.6f, 0.0f, 0.4f);
              b.route ("Env 2", "e1_wt_pm", 0.25f);
              filter (b, 0, kLP12, 8000.0f, 0.0f, 0.3f);
              b.set ("f1_vol", 1.6f);
              env (b, "env1", 0.001f, 1.0f, 0.0f, 0.8f);
              arp (b, ArpUpDownIncl, 2, kDiv8, 0.5f);
              delay (b, 0, kDiv8D, 0.4f, 0.3f, true);
              hall (b, 0.38f, 0.85f, 0.75f);
              nMacros (b, "Metal", "e1_wt_pm", 0.2f, "Decay", "env1_d", 0.35f);
          } },
        { "Pluck Sequence 16ths", "Sequence", "A plucky saw played by the sequencer: a fixed 16-step rhythm with octave jumps. Hold one note.",
          [] (B& b) {
              Neon n; n.v2 = 0.4f; n.cutoff = 700.0f; n.reso = 0.25f; n.envAmt = 0.4f; n.fd = 0.15f; n.fs = 0.0f;
              n.d = 0.2f; n.s = 0.0f; n.r = 0.15f; n.mono = true; n.level = 2.6f;
              neon (b, n);
              seqBass (b, kDiv16, { 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 1 }, { 3, 6, 11, 14 }, 0.5f);
              vChorus (b, 0, 1, 0.4f);
              delay (b, 1, kDiv8D, 0.3f, 0.2f, true);
              hall (b, 0.28f);
              nMacros (b, "Echo", "fx_a2_p8", 0.35f, "Decay", "env1_d", 0.35f);
          } },
        { "Gated Chord Seq", "Sequence", "A pad chopped into a sixteenth-note rhythm: hold a chord and it pulses in time.",
          [] (B& b) {
              Neon n; n.v2 = 0.6f; n.voices = 3; n.cutoff = 1500.0f; n.envAmt = 0.1f; n.a = 0.005f; n.d = 1.0f; n.s = 1.0f; n.r = 0.3f;
              neon (b, n);
              b.set ("func1_shape", 13.0f).set ("func1_sync", 1.0f).set ("func1_div", (float) 3).set ("func1_retrig", 3.0f).set ("func1_smooth", 0.1f);
              b.set ("amp_mod_src", 11.0f).set ("amp_velo", 0.9f);
              vChorus (b, 0, 1, 0.4f);
              delay (b, 1, kDiv8D, 0.25f, 0.15f, true);
              hall (b, 0.3f);
              nMacros (b, "Gate Depth", "amp_velo", -0.5f, "Release", "env1_r", 0.3f);
          } },
        { "Octave Arp 8ths", "Sequence", "A warm square arpeggio jumping octaves in eighths. Slower and more spacious.",
          [] (B& b) {
              Neon n; n.w1 = kSquare; n.width = 0.4f; n.cutoff = 900.0f; n.envAmt = 0.3f; n.fd = 0.25f; n.fs = 0.1f;
              n.d = 0.4f; n.s = 0.2f; n.r = 0.2f; n.level = 2.4f;
              neon (b, n);
              arp (b, ArpUp, 2, kDiv8, 0.6f);
              vChorus (b, 0, 0, 0.4f);
              delay (b, 1, kDiv4D, 0.35f, 0.22f, true);
              hall (b, 0.32f, 0.8f, 0.7f);
              nMacros (b, "Echo", "fx_a2_p8", 0.35f, "Decay", "env1_d", 0.35f);
          } },
        { "Triplet Arp", "Sequence", "Arpeggio in sixteenth triplets: a swirling, cinematic rhythm against straight beats.",
          [] (B& b) {
              Neon n; n.w2 = kSquare; n.v2 = 0.5f; n.cutoff = 1000.0f; n.reso = 0.2f; n.envAmt = 0.35f; n.fd = 0.15f; n.fs = 0.1f;
              n.d = 0.2f; n.s = 0.15f; n.r = 0.12f; n.level = 2.4f;
              neon (b, n);
              arp (b, ArpUpDownIncl, 2, kDiv16T, 0.5f);
              vChorus (b, 0, 1, 0.4f);
              delay (b, 1, kDiv8D, 0.3f, 0.2f, true);
              hall (b, 0.3f);
              nMacros (b, "Echo", "fx_a2_p8", 0.35f, "Decay", "env1_d", 0.35f);
          } },
        { "Dark Arp", "Sequence", "A dark, resonant ladder arpeggio with a slow filter wave: tension and night.",
          [] (B& b) {
              Neon n; n.v2 = 0.5f; n.c2 = -12.0f; n.model = kLadder; n.cutoff = 500.0f; n.reso = 0.4f; n.drive = 0.3f; n.envAmt = 0.3f;
              n.fd = 0.15f; n.fs = 0.1f; n.d = 0.2f; n.s = 0.15f; n.r = 0.12f; n.level = 2.6f;
              neon (b, n);
              lfo (b, 0, LfoSine, 0.1f);
              b.set ("lfo1_retrig", 3.0f);
              b.route ("LFO 1", "f1_cutoff", 0.12f);
              arp (b, ArpDown, 2, kDiv16, 0.5f);
              delay (b, 0, kDiv8D, 0.35f, 0.22f, true);
              hall (b, 0.32f, 0.85f, 0.75f);
              nMacros (b, "Resonance", "f1_reso", 0.25f, "Decay", "env1_d", 0.35f);
          } },
        { "Shimmer Arp", "Sequence", "Soft triangle arpeggio into a shimmer reverb: glittering background motion.",
          [] (B& b) {
              Neon n; n.w1 = kTri; n.w2 = kSquare; n.v2 = 0.25f; n.c2 = 12.0f; n.width = 0.3f; n.cutoff = 3000.0f; n.envAmt = 0.15f;
              n.d = 0.4f; n.s = 0.1f; n.r = 0.3f; n.level = 2.2f;
              neon (b, n);
              arp (b, ArpUpDownIncl, 3, kDiv16, 0.5f);
              delay (b, 0, kDiv8D, 0.35f, 0.25f, true);
              shimmer (b, 0.45f, 0.5f, 0.85f);
              nMacros (b, "Echo", "fx_a1_p8", 0.35f, "Decay", "env1_d", 0.35f);
          } },
        { "Pulse Pad Seq", "Pad", "A pad that pumps on every beat, like it's side-chained to the kick. The synthwave groove.",
          [] (B& b) {
              Neon n; n.v2 = 0.6f; n.voices = 3; n.cutoff = 1400.0f; n.envAmt = 0.1f; n.a = 0.05f; n.d = 1.0f; n.s = 1.0f; n.r = 0.5f;
              neon (b, n);
              pump (b, 0.6f);
              vChorus (b, 0, 1, 0.45f);
              hall (b, 0.32f);
              nMacros (b, "Pump", "amp_velo", 0.35f, "Release", "env1_r", 0.3f);
          } },
        // ================================================================ PLUCKS
        { "Neon Pluck", "Pluck", "Bright, snappy saw pluck with chorus and a dotted echo: the go-to synthwave pluck.",
          [] (B& b) {
              Neon n; n.v2 = 0.5f; n.cutoff = 700.0f; n.reso = 0.2f; n.envAmt = 0.45f; n.fd = 0.2f; n.fs = 0.0f;
              n.d = 0.4f; n.s = 0.0f; n.r = 0.3f;
              n.level = 1.9f;
              neon (b, n);
              vChorus (b, 0, 1, 0.45f);
              delay (b, 1, kDiv8D, 0.35f, 0.22f, true);
              hall (b, 0.3f);
              nMacros (b, "Echo", "fx_a2_p8", 0.35f, "Decay", "env1_d", 0.35f);
          } },
        { "Glass Pluck 80s", "Pluck", "FM glass pluck: a bright 'tink' that settles into a pure tone.",
          [] (B& b) {
              fmVoice (b, 0, 4.0f, 0.05f, 0.75f);
              env (b, "env2", 0.001f, 0.15f, 0.0f, 0.1f);
              b.route ("Env 2", "e1_wt_pm", 0.35f, "Velocity", 1.0f);
              filter (b, 0, kLP12, 9000.0f, 0.0f, 0.3f);
              b.set ("f1_vol", 1.4f);
              env (b, "env1", 0.001f, 0.9f, 0.0f, 0.5f);
              b.set ("amp_velo", 0.5f);
              chorus (b, 0, 0.3f);
              delay (b, 1, kDiv8D, 0.3f, 0.2f, true);
              hall (b, 0.3f);
              nMacros (b, "Tink", "e1_wt_pm", 0.2f, "Decay", "env1_d", 0.35f);
          } },
        { "Retro Pluck", "Pluck", "Square-and-saw pluck through a tape echo: warm and slightly wobbly.",
          [] (B& b) {
              Neon n; n.w1 = kSquare; n.width = 0.35f; n.v2 = 0.4f; n.cutoff = 800.0f; n.reso = 0.15f; n.envAmt = 0.4f; n.fd = 0.22f; n.fs = 0.0f;
              n.d = 0.5f; n.s = 0.0f; n.r = 0.3f;
              n.level = 1.5f;
              neon (b, n);
              tape (b, 0, kDiv8D, 0.35f, 0.5f, 0.25f);
              hall (b, 0.28f);
              nMacros (b, "Wobble", "fx_a1_p5", 0.3f, "Decay", "env1_d", 0.35f);
          } },
        { "Echo Pluck", "Pluck", "A short pluck where the echo does half the work: play quarter notes, hear sixteenths.",
          [] (B& b) {
              Neon n; n.v2 = 0.4f; n.cutoff = 900.0f; n.reso = 0.2f; n.envAmt = 0.4f; n.fd = 0.15f; n.fs = 0.0f;
              n.d = 0.25f; n.s = 0.0f; n.r = 0.2f;
              n.level = 2.0f;
              neon (b, n);
              delay (b, 0, kDiv8D, 0.5f, 0.38f, true);
              hall (b, 0.28f);
              nMacros (b, "Echo", "fx_a1_p8", 0.3f, "Decay", "env1_d", 0.35f);
          } },
        { "Square Pluck", "Pluck", "Hollow square-wave pluck: a retro video-game edge, but warm.",
          [] (B& b) {
              Neon n; n.w1 = kSquare; n.width = 0.5f; n.cutoff = 1200.0f; n.envAmt = 0.35f; n.fd = 0.18f; n.fs = 0.0f;
              n.d = 0.35f; n.s = 0.0f; n.r = 0.2f;
              n.level = 1.6f;
              neon (b, n);
              vChorus (b, 0, 0, 0.35f);
              delay (b, 1, kDiv8D, 0.3f, 0.2f, true);
              hall (b, 0.25f);
              nMacros (b, "Width", "e1_o1_width", -0.3f, "Decay", "env1_d", 0.35f);
          } },
        { "Chorus Pluck", "Pluck", "A soft pluck drenched in chorus I + II: wide and dreamy.",
          [] (B& b) {
              Neon n; n.w1 = kSaw; n.w2 = kSquare; n.v2 = 0.4f; n.cutoff = 600.0f; n.envAmt = 0.4f; n.fd = 0.3f; n.fs = 0.0f;
              n.d = 0.6f; n.s = 0.0f; n.r = 0.4f;
              n.level = 1.9f;
              neon (b, n);
              vChorus (b, 0, 2, 0.55f);
              hall (b, 0.32f, 0.8f, 0.7f);
              nMacros (b, "Chorus", "fx_a1_p5", 0.3f, "Decay", "env1_d", 0.35f);
          } },
        { "Muted Pluck", "Pluck", "Very short, muted pluck: percussive and tight, for fast rhythmic parts.",
          [] (B& b) {
              Neon n; n.v2 = 0.4f; n.cutoff = 500.0f; n.reso = 0.2f; n.envAmt = 0.35f; n.fd = 0.08f; n.fs = 0.0f;
              n.d = 0.12f; n.s = 0.0f; n.r = 0.08f; n.level = 1.8f;
              neon (b, n);
              delay (b, 0, kDiv8D, 0.3f, 0.18f, true);
              room (b, 0.2f);
              nMacros (b, "Resonance", "f1_reso", 0.25f, "Length", "env1_d", 0.35f);
          } },
        { "Synth Marimba", "Pluck", "A woody FM marimba, the 80s digital kind. Playful and bright.",
          [] (B& b) {
              fmVoice (b, 0, 4.0f, 0.0f, 0.8f);
              env (b, "env2", 0.001f, 0.08f, 0.0f, 0.05f);
              b.route ("Env 2", "e1_wt_pm", 0.3f, "Velocity", 1.0f);
              filter (b, 0, kLP12, 6000.0f, 0.0f, 0.3f);
              b.set ("f1_vol", 1.4f);
              env (b, "env1", 0.001f, 0.5f, 0.0f, 0.3f);
              b.set ("amp_velo", 0.5f);
              delay (b, 0, kDiv8D, 0.25f, 0.15f, true);
              room (b, 0.25f);
              nMacros (b, "Mallet", "e1_wt_pm", 0.2f, "Decay", "env1_d", 0.35f);
          } },
        // ================================================================ PADS
        { "Sunset Pad", "Pad", "Warm, wide supersaw pad with chorus and a big hall: golden-hour synthwave.",
          [] (B& b) {
              Neon n; n.voices = 7; n.uniMode = 2; n.uniDetune = 0.22f; n.cutoff = 1500.0f; n.envAmt = 0.15f; n.fa = 1.0f; n.fd = 2.0f; n.fs = 0.7f;
              n.a = 0.8f; n.d = 1.0f; n.s = 1.0f; n.r = 2.0f; n.velAmt = 0.0f;
              neon (b, n);
              vChorus (b, 0, 0, 0.35f);
              hall (b, 0.42f, 0.85f, 0.8f);
              nMacros (b, "Width", "e1_uni_detune", 0.2f, "Attack", "env1_a", 0.35f);
          } },
        { "Midnight Highway Pad", "Pad", "Dark saw-and-pulse pad with a slow filter wave. Driving at night.",
          [] (B& b) {
              Neon n; n.v2 = 0.6f; n.w2 = kSquare; n.sub = 0.3f; n.voices = 2; n.cutoff = 600.0f; n.reso = 0.2f; n.envAmt = 0.2f;
              n.fa = 1.0f; n.fd = 2.0f; n.fs = 0.6f; n.a = 0.8f; n.d = 1.0f; n.s = 1.0f; n.r = 1.8f; n.velAmt = 0.0f;
              neon (b, n);
              lfo (b, 0, LfoSine, 0.12f);
              b.set ("lfo1_retrig", 3.0f);
              b.route ("LFO 1", "f1_cutoff", 0.08f);
              vChorus (b, 0, 0, 0.4f);
              hall (b, 0.4f, 0.85f, 0.8f);
              nMacros (b, "Motion", "lfo1_rate", 0.3f, "Attack", "env1_a", 0.35f);
          } },
        { "Neon Haze", "Pad", "Hazy PWM pad with tape wobble: soft-focus, nostalgic.",
          [] (B& b) {
              Neon n; n.w1 = kSquare; n.w2 = kSquare; n.v2 = 0.6f; n.d2 = 0.1f; n.cutoff = 1200.0f; n.envAmt = 0.1f;
              n.a = 0.7f; n.d = 1.0f; n.s = 1.0f; n.r = 1.8f; n.velAmt = 0.0f;
              neon (b, n);
              lfo (b, 0, LfoTriangle, 0.5f);
              b.route ("LFO 1", "e1_o1_width", 0.25f).route ("LFO 1", "e1_o2_width", -0.25f);
              tape (b, 0, kDiv4, 0.2f, 0.6f, 0.2f);
              hall (b, 0.4f, 0.85f, 0.8f);
              nMacros (b, "PWM Speed", "lfo1_rate", 0.3f, "Attack", "env1_a", 0.35f);
          } },
        { "Retro Strings", "Strings/Brass", "Bright 80s polysynth strings with chorus I + II and a highpass.",
          [] (B& b) {
              Neon n; n.v2 = 0.7f; n.d2 = 0.1f; n.voices = 2; n.cutoff = 3500.0f; n.envAmt = 0.0f; n.a = 0.4f; n.d = 1.0f; n.s = 1.0f; n.r = 1.0f;
              n.velAmt = 0.0f;
              neon (b, n);
              filter (b, 1, kHP12, 300.0f, 0.0f);
              b.set ("f_routing", 0.0f);
              vChorus (b, 0, 2, 0.45f);
              hall (b, 0.35f, 0.8f, 0.7f);
              nMacros (b, "Thin", "f2_cutoff", 0.15f, "Attack", "env1_a", 0.35f);
          } },
        { "VHS Pad", "Pad", "A pad on a worn tape: wobbly pitch, hiss and a dark top. Lo-fi synthwave.",
          [] (B& b) {
              Neon n; n.v2 = 0.6f; n.voices = 2; n.cutoff = 900.0f; n.envAmt = 0.1f; n.a = 0.6f; n.d = 1.0f; n.s = 1.0f; n.r = 1.5f; n.velAmt = 0.0f;
              neon (b, n);
              noise (b, 0, "Tape Hiss", 0.03f, 0.0f);
              lfo (b, 0, LfoSine, 0.35f);
              b.set ("lfo1_retrig", 3.0f);
              b.route ("LFO 1", "e1_fine", 0.015f);
              tape (b, 0, kDiv4, 0.25f, 0.7f, 0.25f);
              hall (b, 0.35f, 0.8f, 0.7f);
              nMacros (b, "Hiss", "u_n1_vol", 0.1f, "Attack", "env1_a", 0.35f);
          } },
        { "Analog Dream", "Pad", "Soft two-oscillator pad with drift and a slow filter swell. Gentle and warm.",
          [] (B& b) {
              Neon n; n.v2 = 0.6f; n.w2 = kTri; n.c2 = 12.0f; n.drift = 0.4f; n.cutoff = 800.0f; n.envAmt = 0.25f; n.fa = 1.5f; n.fd = 2.0f; n.fs = 0.7f;
              n.a = 0.8f; n.d = 1.0f; n.s = 1.0f; n.r = 1.8f; n.velAmt = 0.0f;
              neon (b, n);
              vChorus (b, 0, 0, 0.35f);
              hall (b, 0.42f, 0.85f, 0.8f);
              nMacros (b, "Swell", "env2_a", 0.3f, "Attack", "env1_a", 0.35f);
          } },
        { "Starfield Pad", "Pad", "Glassy wavetable pad with a sparkle of noise and a shimmer: a sky full of stars.",
          [] (B& b) {
              wavetable (b, 0, "Glass Pad", 0.5f, 0.55f);
              uni (b, 0, 0, 4, 0.14f, 0.8f);
              noise (b, 0, "Sparkle Air", 0.06f, 0.6f);
              filter (b, 0, kLP12, 6000.0f, 0.05f, 0.2f);
              env (b, "env1", 1.0f, 1.0f, 1.0f, 2.5f);
              lfo (b, 0, LfoSine, 0.07f);
              b.set ("lfo1_retrig", 3.0f);
              b.route ("LFO 1", "e1_wt_pos", 0.2f);
              delay (b, 0, kDiv4D, 0.35f, 0.18f, true);
              shimmer (b, 0.45f, 0.55f, 0.85f);
              nMacros (b, "Sparkle", "u_n1_vol", 0.15f, "Attack", "env1_a", 0.35f);
          } },
        { "Choir Haze", "Pad", "A breathy vocal pad behind a chorus and a long hall: the 80s 'ahh'.",
          [] (B& b) {
              wavetable (b, 0, "Choir Ah to Oo", 0.15f, 0.55f);
              uni (b, 0, 0, 3, 0.1f, 0.7f);
              wavetable (b, 1, "Breathy Vox", 0.4f, 0.25f);
              filter (b, 0, kLP12, 4000.0f, 0.05f, 0.3f);
              env (b, "env1", 0.8f, 1.0f, 1.0f, 2.0f);
              vintageChorus (b, 0, 1, 0.4f);
              b.set ("fx_a1_p6", 0.7f);
              hall (b, 0.45f, 0.9f, 0.85f);
              nMacros (b, "Vowel", "e1_wt_pos", 0.5f, "Attack", "env1_a", 0.35f);
          } },
        { "Warm Saw Bed", "Pad", "A simple, warm saw pad that sits under everything without getting in the way.",
          [] (B& b) {
              Neon n; n.v2 = 0.6f; n.sub = 0.2f; n.voices = 2; n.cutoff = 700.0f; n.envAmt = 0.1f; n.a = 0.5f; n.d = 1.0f; n.s = 1.0f; n.r = 1.5f;
              n.velAmt = 0.0f;
              neon (b, n);
              vChorus (b, 0, 0, 0.35f);
              hall (b, 0.35f, 0.8f, 0.7f);
              nMacros (b, "Sub", "e1_o3_vol", 0.2f, "Attack", "env1_a", 0.35f);
          } },
        { "Ocean Drive Pad", "Pad", "A slow resonant sweep that rolls in and out like waves.",
          [] (B& b) {
              Neon n; n.v2 = 0.6f; n.voices = 2; n.cutoff = 700.0f; n.reso = 0.4f; n.envAmt = 0.0f; n.a = 0.7f; n.d = 1.0f; n.s = 1.0f; n.r = 1.8f;
              n.velAmt = 0.0f;
              neon (b, n);
              lfo (b, 0, LfoSine, 0.1f);
              b.set ("lfo1_retrig", 3.0f);
              b.route ("LFO 1", "f1_cutoff", 0.18f);
              vChorus (b, 0, 2, 0.4f);
              hall (b, 0.4f, 0.85f, 0.8f);
              nMacros (b, "Wave Speed", "lfo1_rate", 0.3f, "Resonance", "f1_reso", 0.25f);
          } },
        // ================================================================ LEADS
        { "Hero Lead", "Lead", "Huge supersaw lead with glide, vibrato on the wheel and a big echo. The chorus melody.",
          [] (B& b) {
              Neon n; n.voices = 7; n.uniMode = 2; n.uniDetune = 0.18f; n.cutoff = 2000.0f; n.reso = 0.1f; n.envAmt = 0.2f;
              n.a = 0.005f; n.d = 0.5f; n.s = 0.9f; n.r = 0.35f; n.mono = true; n.legato = true; n.glide = 0.06f; n.level = 2.2f;
              neon (b, n);
              lfo (b, 0, LfoSine, 5.5f);
              b.route ("LFO 1", "e1_fine", 0.008f, "Mod Wheel", 1.0f);
              delay (b, 0, kDiv8D, 0.35f, 0.22f, true);
              hall (b, 0.35f, 0.85f, 0.75f);
              nMacros (b, "Width", "e1_uni_detune", 0.15f, "Release", "env1_r", 0.3f);
          } },
        { "Sync Lead 84", "Lead", "Screaming sync lead swept by the envelope and the mod wheel. Pure 1984.",
          [] (B& b) {
              Neon n; n.v1 = 0.3f; n.v2 = 0.8f; n.c2 = 7.0f; n.cutoff = 3000.0f; n.envAmt = 0.1f; n.fd = 0.8f; n.fs = 0.3f;
              n.mono = true; n.legato = true; n.glide = 0.04f; n.level = 2.2f;
              neon (b, n);
              b.set ("e1_sync", 1.0f);
              b.route ("Env 2", "e1_o2_coarse", 0.2f).route ("Mod Wheel", "e1_o2_coarse", 0.2f);
              delay (b, 0, kDiv8D, 0.3f, 0.2f, true);
              hall (b, 0.3f);
              nMacros (b, "Sweep", "env2_d", 0.3f, "Release", "env1_r", 0.3f);
          } },
        { "Square Lead 80s", "Lead", "Bright square lead with a delayed vibrato and chorus: a classic 80s melody voice.",
          [] (B& b) {
              Neon n; n.w1 = kSquare; n.w2 = kSquare; n.v2 = 0.4f; n.c2 = 12.0f; n.width = 0.45f; n.cutoff = 2200.0f; n.envAmt = 0.15f;
              n.mono = true; n.legato = true; n.glide = 0.04f; n.level = 2.2f;
              neon (b, n);
              lfo (b, 0, LfoSine, 5.5f);
              b.set ("lfo1_fade", 0.5f).set ("lfo1_retrig", 2.0f);
              b.route ("LFO 1", "e1_fine", 0.007f);
              vChorus (b, 0, 0, 0.35f);
              delay (b, 1, kDiv8D, 0.3f, 0.2f, true);
              hall (b, 0.3f);
              nMacros (b, "Octave", "e1_o2_vol", 0.3f, "Release", "env1_r", 0.3f);
          } },
        { "Brass Lead 80s", "Lead", "A brassy saw lead with a filter swell on each note: the synth-brass solo.",
          [] (B& b) {
              Neon n; n.v2 = 0.7f; n.cutoff = 700.0f; n.envAmt = 0.4f; n.fa = 0.06f; n.fd = 0.6f; n.fs = 0.5f;
              n.a = 0.03f; n.mono = true; n.legato = false; n.glide = 0.03f; n.level = 2.2f;
              neon (b, n);
              lfo (b, 0, LfoSine, 5.5f);
              b.route ("LFO 1", "e1_fine", 0.008f, "Mod Wheel", 1.0f);
              vChorus (b, 0, 0, 0.3f);
              delay (b, 1, kDiv8D, 0.3f, 0.18f, true);
              hall (b, 0.3f);
              nMacros (b, "Bite", "env2_d", -0.2f, "Release", "env1_r", 0.3f);
          } },
        { "Whistle Lead", "Lead", "A pure, whistling sine lead with a long portamento. Lonely and cinematic.",
          [] (B& b) {
              Neon n; n.w1 = kSine; n.w2 = kTri; n.v2 = 0.15f; n.c2 = 12.0f; n.cutoff = 4000.0f; n.envAmt = 0.0f;
              n.a = 0.04f; n.r = 0.5f; n.mono = true; n.legato = false; n.glide = 0.15f; n.level = 2.4f;
              neon (b, n);
              lfo (b, 0, LfoSine, 5.0f);
              b.set ("lfo1_fade", 0.6f).set ("lfo1_retrig", 2.0f);
              b.route ("LFO 1", "e1_fine", 0.008f);
              delay (b, 0, kDiv4D, 0.4f, 0.28f, true);
              hall (b, 0.4f, 0.85f, 0.8f);
              nMacros (b, "Breath", "e1_noise_vol", 0.05f, "Release", "env1_r", 0.3f);
          } },
        { "Chorus Saw Lead", "Lead", "A single saw lead through deep chorus: smooth, wide and singing.",
          [] (B& b) {
              Neon n; n.v2 = 0.4f; n.d2 = 0.1f; n.cutoff = 1800.0f; n.reso = 0.15f; n.envAmt = 0.2f;
              n.mono = true; n.legato = true; n.glide = 0.05f; n.level = 2.8f;
              neon (b, n);
              lfo (b, 0, LfoSine, 5.5f);
              b.route ("LFO 1", "e1_fine", 0.008f, "Mod Wheel", 1.0f);
              vChorus (b, 0, 2, 0.5f);
              delay (b, 1, kDiv8D, 0.3f, 0.2f, true);
              hall (b, 0.3f);
              nMacros (b, "Chorus", "fx_a1_p5", 0.3f, "Release", "env1_r", 0.3f);
          } },
        { "Octave Lead 80s", "Lead", "Saws an octave apart with drive: a bold lead that cuts through a dense mix.",
          [] (B& b) {
              Neon n; n.v2 = 0.5f; n.c2 = 12.0f; n.sub = 0.3f; n.model = kLadder; n.cutoff = 1500.0f; n.reso = 0.15f; n.drive = 0.4f; n.envAmt = 0.2f;
              n.mono = true; n.legato = true; n.glide = 0.04f; n.level = 2.0f;
              neon (b, n);
              lfo (b, 0, LfoSine, 5.5f);
              b.route ("LFO 1", "e1_fine", 0.008f, "Mod Wheel", 1.0f);
              delay (b, 0, kDiv8D, 0.3f, 0.2f, true);
              hall (b, 0.3f);
              nMacros (b, "Drive", "f1_drive", 0.3f, "Release", "env1_r", 0.3f);
          } },
        { "Talking Lead", "Lead", "A saw through a formant (vowel) filter that opens with the envelope: a voice-like 'wow'.",
          [] (B& b) {
              Neon n; n.v2 = 0.5f; n.cutoff = 3000.0f; n.envAmt = 0.0f; n.mono = true; n.legato = true; n.glide = 0.04f; n.level = 2.6f;
              neon (b, n);
              b.set ("f2_on", 1.0f).set ("f2_type", 3.0f).set ("f2_cutoff", 1000.0f).set ("f2_reso", 0.5f).set ("f2_a", 0.1f).set ("f2_b", 1.0f)
                  .set ("f_routing", 0.0f).set ("f2_vol", 2.5f);
              env (b, "env3", 0.15f, 0.5f, 0.4f, 0.3f);
              b.route ("Env 3", "f2_a", 0.5f);
              b.route ("Mod Wheel", "f2_a", 0.5f);
              delay (b, 0, kDiv8D, 0.3f, 0.2f, true);
              hall (b, 0.3f);
              nMacros (b, "Vowel", "f2_a", 0.4f, "Release", "env1_r", 0.3f);
          } },
        // ================================================================ KEYS AND STABS
        { "FM Keys 84", "Keys", "The glassy 80s FM electric piano with chorus: every power ballad had one.",
          [] (B& b) {
              fmVoice (b, 0, 1.0f, 0.15f, 0.75f);
              env (b, "env2", 0.001f, 1.0f, 0.1f, 0.4f);
              b.route ("Env 2", "e1_wt_pm", 0.3f, "Velocity", 1.0f);
              fmVoice (b, 1, 14.0f, 0.5f, 0.0f);
              b.set ("e2_fine", 0.07f);
              env (b, "env3", 0.001f, 0.3f, 0.0f, 0.1f);
              b.route ("Env 3", "e2_vol", 0.5f, "Velocity", 0.8f);
              filter (b, 0, kLP12, 12000.0f, 0.0f, 0.3f);
              env (b, "env1", 0.0f, 4.0f, 0.0f, 0.5f);
              b.route ("Keytrack", "env1_d", -0.3f);
              b.set ("amp_velo", 0.7f);
              vChorus (b, 0, 1, 0.4f);
              delay (b, 1, kDiv8D, 0.25f, 0.15f, true);
              hall (b, 0.3f);
              nMacros (b, "Bark", "e1_wt_pm", 0.25f, "Decay", "env1_d", 0.35f);
          } },
        { "Synth Brass Stabs", "Strings/Brass", "Punchy 80s synth brass for chord stabs, with chorus.",
          [] (B& b) {
              Neon n; n.v2 = 0.7f; n.voices = 2; n.cutoff = 600.0f; n.envAmt = 0.45f; n.fa = 0.02f; n.fd = 0.35f; n.fs = 0.35f;
              n.a = 0.01f; n.d = 0.4f; n.s = 0.6f; n.r = 0.25f;
              n.level = 1.6f;
              neon (b, n);
              vChorus (b, 0, 0, 0.35f);
              room (b, 0.25f);
              nMacros (b, "Bite", "env2_d", -0.2f, "Release", "env1_r", 0.3f);
          } },
        { "Neon Chord Stab", "Keys", "One key plays a whole minor-ninth-style chord stab, with chorus and echo.",
          [] (B& b) {
              Neon n; n.v2 = 0.5f; n.w2 = kSquare; n.cutoff = 900.0f; n.reso = 0.15f; n.envAmt = 0.4f; n.fd = 0.25f; n.fs = 0.0f;
              n.d = 0.35f; n.s = 0.0f; n.r = 0.3f;
              neon (b, n);
              uni (b, 0, 1, 4, 0.0f, 0.6f);
              b.set ("e1_uni_chord", 8.0f); // Minor 7
              vChorus (b, 0, 1, 0.45f);
              delay (b, 1, kDiv8D, 0.3f, 0.2f, true);
              hall (b, 0.28f);
              nMacros (b, "Echo", "fx_a2_p8", 0.35f, "Decay", "env1_d", 0.35f);
          } },
        { "Retro EP", "Keys", "A warm tine electric piano with a wide auto-pan: the late-80s soft-rock ballad.",
          [] (B& b) {
              fmVoice (b, 0, 1.0f, 0.08f, 0.8f);
              env (b, "env2", 0.001f, 1.2f, 0.0f, 0.5f);
              b.route ("Env 2", "e1_wt_pm", 0.3f, "Velocity", 1.0f);
              fmVoice (b, 1, 14.0f, 0.35f, 0.0f);
              env (b, "env3", 0.001f, 0.2f, 0.0f, 0.1f);
              b.route ("Env 3", "e2_vol", 0.35f, "Velocity", 0.8f);
              filter (b, 0, kLP12, 4000.0f, 0.0f, 0.3f);
              env (b, "env1", 0.0f, 5.0f, 0.0f, 0.45f);
              b.route ("Keytrack", "env1_d", -0.3f);
              b.set ("amp_velo", 0.65f);
              tremolo (b, 0, 0.4f, 0.45f);
              chorus (b, 1, 0.3f);
              hall (b, 0.28f);
              nMacros (b, "Pan", "fx_a1_p1", 0.4f, "Decay", "env1_d", 0.35f);
          } },
        { "Poly Keys 80s", "Keys", "Plucky PWM poly keys that sustain softly: the bread-and-butter 80s comp.",
          [] (B& b) {
              Neon n; n.w1 = kSquare; n.w2 = kSaw; n.v2 = 0.3f; n.cutoff = 1000.0f; n.envAmt = 0.3f; n.fd = 0.6f; n.fs = 0.3f;
              n.d = 0.8f; n.s = 0.4f; n.r = 0.4f;
              neon (b, n);
              lfo (b, 0, LfoTriangle, 0.6f);
              b.route ("LFO 1", "e1_o1_width", 0.2f);
              vChorus (b, 0, 1, 0.4f);
              room (b, 0.25f);
              nMacros (b, "Chorus", "fx_a1_p5", 0.3f, "Decay", "env1_d", 0.35f);
          } },
        { "Neon Keys", "Keys", "Bright, bell-edged synth keys: a narrow pulse an octave up over a soft saw.",
          [] (B& b) {
              Neon n; n.w1 = kSaw; n.v1 = 0.5f; n.w2 = kSquare; n.v2 = 0.4f; n.c2 = 12.0f; n.width = 0.15f; n.cutoff = 2000.0f; n.envAmt = 0.25f;
              n.fd = 0.4f; n.fs = 0.2f; n.d = 1.5f; n.s = 0.2f; n.r = 0.5f;
              n.level = 1.8f;
              neon (b, n);
              vChorus (b, 0, 1, 0.4f);
              delay (b, 1, kDiv8D, 0.25f, 0.15f, true);
              hall (b, 0.28f);
              nMacros (b, "Bell", "e1_o2_vol", 0.3f, "Decay", "env1_d", 0.35f);
          } },
    };
    return list;
}

juce::StringArray pack11Tags (const juce::String& presetName)
{
    static const std::map<juce::String, const char*> tags {
        { "Outrun Bass", "Synthwave, Bass, Pulsing" }, { "Octave Runner Bass", "Synthwave, Bass, Pulsing" },
        { "Galloping Bass", "Synthwave, Bass, Pulsing" }, { "Analog Saw Bass", "Synthwave, Bass, Ladder" },
        { "Sub Pulse Bass", "Synthwave, Bass, Sub, Pulsing" }, { "Dark Synth Bass", "Synthwave, Bass, Pulsing, Dark" },
        { "Funk Bass 80s", "Synthwave, Bass, Funk" }, { "Triplet Bass", "Synthwave, Bass, Pulsing" },
        { "Night City Arp", "Synthwave, Arp" }, { "Neon Up Arp", "Synthwave, Arp" }, { "Arpeggio Bells", "Synthwave, Arp, Bells" },
        { "Pluck Sequence 16ths", "Synthwave, Sequence, Pluck" }, { "Gated Chord Seq", "Synthwave, Sequence, Pulsing" },
        { "Octave Arp 8ths", "Synthwave, Arp" }, { "Triplet Arp", "Synthwave, Arp" }, { "Dark Arp", "Synthwave, Arp, Dark" },
        { "Shimmer Arp", "Synthwave, Arp, Shimmer" }, { "Pulse Pad Seq", "Synthwave, Pad, Pulsing" },
        { "Neon Pluck", "Synthwave, Pluck" }, { "Glass Pluck 80s", "Synthwave, Pluck, FM" }, { "Retro Pluck", "Synthwave, Pluck, Lo-Fi" },
        { "Echo Pluck", "Synthwave, Pluck, Delay" }, { "Square Pluck", "Synthwave, Pluck" }, { "Chorus Pluck", "Synthwave, Pluck" },
        { "Muted Pluck", "Synthwave, Pluck, Short" }, { "Synth Marimba", "Synthwave, Pluck, FM, Mallet" },
        { "Sunset Pad", "Synthwave, Pad, Big" }, { "Midnight Highway Pad", "Synthwave, Pad, Dark" }, { "Neon Haze", "Synthwave, Pad, PWM, Lo-Fi" },
        { "Retro Strings", "Synthwave, Strings" }, { "VHS Pad", "Synthwave, Pad, Lo-Fi" }, { "Analog Dream", "Synthwave, Pad, Worship" },
        { "Starfield Pad", "Synthwave, Pad, Shimmer" }, { "Choir Haze", "Synthwave, Pad, Choir" }, { "Warm Saw Bed", "Synthwave, Pad, Worship" },
        { "Ocean Drive Pad", "Synthwave, Pad, Evolving" },
        { "Hero Lead", "Synthwave, Lead, Big" }, { "Sync Lead 84", "Synthwave, Lead, Sync" }, { "Square Lead 80s", "Synthwave, Lead" },
        { "Brass Lead 80s", "Synthwave, Lead, Brass" }, { "Whistle Lead", "Synthwave, Lead" }, { "Chorus Saw Lead", "Synthwave, Lead" },
        { "Octave Lead 80s", "Synthwave, Lead, Driven" }, { "Talking Lead", "Synthwave, Lead" },
        { "FM Keys 84", "Synthwave, Electric Piano, FM" }, { "Synth Brass Stabs", "Synthwave, Brass, Synth Brass, Stab" },
        { "Neon Chord Stab", "Synthwave, Stab, Chord" }, { "Retro EP", "Synthwave, Electric Piano" },
        { "Poly Keys 80s", "Synthwave, Synth Keys, PWM" }, { "Neon Keys", "Synthwave, Synth Keys" },
    };
    const auto it = tags.find (presetName);
    return it == tags.end() ? juce::StringArray() : PresetManager::parseTags (it->second);
}
