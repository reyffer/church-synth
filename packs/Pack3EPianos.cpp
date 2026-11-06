// Preset Pack 3: Electric Pianos.
//
// Thirty electric pianos in four families, all synthesised (no samples):
//   Tine I / Tine II  tine-and-pickup pianos (Rhodes-style; I = more bark, II = cleaner, more bell)
//   FM                two-operator FM pianos (DX-style)
//   Reed              reed electric pianos (Wurlitzer-style)
//   Clav              plucked-string keyboard (Clavinet-style)
// Same rules as the other packs: built in code, exported and level-matched by
// `cs_state_test --export-pack <dir>`.
//
// Macros: M1 Bark / Brightness, M2 Space, M3 a character control (Tremolo, Chorus,
// Grit, Phase...), M4 Decay.

#include "Packs.h"

#include "plugin/PresetHelpers.h"

#include <map>

using namespace presethelpers;

namespace
{
constexpr int kDistTube = 2, kDistOverdrive = 4;

void distortion (B& b, int slot, int type, float drive, float mix)
{
    b.fx (slot, FxDistortion, { { 0, ((float) type + 0.5f) / 16.0f }, { 1, drive }, { 10, mix } });
}

void phaser (B& b, int slot, float depth, float rate, float mix)
{
    b.fx (slot, FxPhaser, { { 3, depth }, { 4, rate }, { 7, mix } });
}

// Natural piano decay: high notes die away sooner than low ones.
void pianoDecay (B& b, float decay, float release, float velo)
{
    env (b, "env1", 0.0f, decay, 0.0f, release);
    b.route ("Keytrack", "env1_d", -0.3f);
    b.set ("amp_velo", velo);
}

// Tine piano: a sine body with velocity "bark" (1:1 phase mod from Env 2), a short
// high tine at the strike (Engine 2, ratio 14), and pickup grit (filter drive) that
// grows with velocity. Tine I = more bark and growl; Tine II = cleaner, more tine (bell).
struct Tine { float bark, tine, tineDecay, cutoff, grit, decay; };
constexpr Tine kTineI { 0.35f, 0.30f, 0.20f, 3200.0f, 0.35f, 5.5f };
constexpr Tine kTineII { 0.22f, 0.50f, 0.30f, 7000.0f, 0.15f, 6.0f };

void tine (B& b, const Tine& t)
{
    fmVoice (b, 0, 1.0f, 0.05f, 0.8f);
    env (b, "env2", 0.001f, 1.2f, 0.0f, 0.5f);
    b.route ("Env 2", "e1_wt_pm", t.bark, "Velocity", 1.0f);
    fmVoice (b, 1, 14.0f, 0.4f, 0.0f);
    env (b, "env3", 0.001f, t.tineDecay, 0.0f, 0.1f);
    b.route ("Env 3", "e2_vol", t.tine, "Velocity", 0.8f);
    filter (b, 0, kLP12, t.cutoff, 0.0f, 0.35f);
    b.route ("Velocity", "f1_drive", t.grit);
    filter (b, 1, kLP12, t.cutoff * 1.6f, 0.0f, 0.35f); // tames the fizz the grit adds above the pickup tone
    b.set ("f_routing", 0.0f);
    pianoDecay (b, t.decay, 0.45f, 0.65f);
}

// FM piano (DX-style): two carrier/modulator pairs. Engine 1 is the body (with
// velocity bark from Env 2); Engine 2 is the bright "tine" pair, slightly detuned
// so the two beat gently, faded by Env 3.
struct Fm
{
    float bodyRatio, bodyPm, bark, barkDecay;
    float tineRatio, tinePm, tine, tineDecay;
    float detune, cutoff, decay;
    int modWave = 0; // 0 sine, 1 triangle
};

void fm (B& b, const Fm& f)
{
    fmVoice (b, 0, f.bodyRatio, f.bodyPm, 0.75f);
    b.set ("e1_wt_mod_wave", (float) f.modWave);
    env (b, "env2", 0.001f, f.barkDecay, 0.0f, 0.4f);
    b.route ("Env 2", "e1_wt_pm", f.bark, "Velocity", 1.0f);
    fmVoice (b, 1, f.tineRatio, f.tinePm, 0.0f);
    b.set ("e2_fine", f.detune);
    env (b, "env3", 0.001f, f.tineDecay, 0.0f, 0.2f);
    b.route ("Env 3", "e2_vol", f.tine, "Velocity", 0.7f);
    filter (b, 0, kLP12, f.cutoff, 0.0f, 0.3f);
    pianoDecay (b, f.decay, 0.5f, 0.7f);
}

// Reed piano (Wurlitzer-style): a triangle plus a narrow pulse (the nasal reed),
// a lowpass that velocity and Env 2 open, and grit (filter drive) when played hard.
// Tremolo: LFO 1 dips Engine 1's volume, so velocity still controls the level.
void reed (B& b, float pulse, float width, float cutoff, float grit, float tremolo)
{
    osc (b, 0, 0, kTri, 0.75f);
    osc (b, 0, 1, kSquare, pulse, 0.0f, 0.0f, width);
    b.set ("e1_vol", 0.8f);
    filter (b, 0, kLP12, cutoff, 0.1f, 0.5f);
    b.set ("f1_vol", 1.6f);
    env (b, "env2", 0.001f, 0.5f, 0.15f, 0.3f);
    b.route ("Env 2", "f1_cutoff", 0.3f, "Velocity", 1.0f);
    b.route ("Velocity", "f1_drive", grit);
    env (b, "env1", 0.0f, 3.5f, 0.0f, 0.2f);
    b.route ("Keytrack", "env1_d", -0.25f);
    b.set ("amp_velo", 0.6f);
    lfo (b, 0, LfoSine, 5.6f, false);
    b.set ("lfo1_retrig", 3.0f);
    if (tremolo > 0.0f)
        b.route ("LFO 1", "e1_vol", -tremolo);
}

// Clav (Clavinet-style): a narrow pulse (the pulse width works like the pickup
// position: it removes some harmonics) plus a little saw, a hammer click, a
// snappy filter envelope and a quick, damped release.
void clav (B& b, float width, float cutoff, float envAmt, float decay, float sustain)
{
    osc (b, 0, 0, kSquare, 0.75f, 0.0f, 0.0f, width);
    osc (b, 0, 1, kSaw, 0.3f);
    b.set ("e1_vol", 0.75f);
    noise (b, 0, "Pluck Noise", 0.07f, 0.3f, false);
    filter (b, 0, kLP12, cutoff, 0.15f, 0.6f, 0.15f); // a little grit
    env (b, "env2", 0.001f, 0.18f, 0.25f, 0.1f);
    b.route ("Env 2", "f1_cutoff", envAmt, "Velocity", 1.0f);
    filter (b, 1, kHP12, 150.0f, 0.0f);
    b.set ("f_routing", 0.0f);
    env (b, "env1", 0.0f, decay, sustain, 0.05f);
    b.route ("Keytrack", "env1_d", -0.2f);
    b.set ("amp_velo", 0.7f);
}
} // namespace

const std::vector<FactoryPreset>& pack3Presets()
{
    static const std::vector<FactoryPreset> list {
        // ================================================================ TINE I (more bark)
        { "Tine I Stage", "Keys", "Classic tine stage piano, the earlier, barkier kind. Soft = round, hard = growl.",
          [] (B& b) {
              tine (b, kTineI);
              room (b, 0.15f);
              macros (b, "Bark", "Space", "Grit", "Decay");
              b.route ("Macro 1", "e1_wt_pm", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "f1_drive", 0.4f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "Tine I Suitcase", "Keys", "Tine piano through a stereo auto-pan amp, with a little tube warmth. The ballad classic.",
          [] (B& b) {
              tine (b, kTineI);
              tremolo (b, 0, 0.5f, 0.45f);
              distortion (b, 1, kDistTube, 0.12f, 0.35f);
              room (b, 0.2f);
              macros (b, "Bark", "Space", "Pan", "Decay");
              b.route ("Macro 1", "e1_wt_pm", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "fx_a1_p1", 0.45f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "Tine I Ballad", "Keys", "Dark, soft tine piano with a light chorus and a hall. For slow worship songs.",
          [] (B& b) {
              Tine t = kTineI;
              t.bark = 0.2f;
              t.cutoff = 2400.0f;
              t.grit = 0.2f;
              tine (b, t);
              chorus (b, 0, 0.3f);
              hall (b, 0.3f, 0.75f, 0.65f);
              macros (b, "Bark", "Space", "Chorus", "Decay");
              b.route ("Macro 1", "e1_wt_pm", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "fx_a1_p8", 0.4f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "Tine I Growl", "Keys", "Hard-played tine piano into an overdriven amp. Gospel and soul energy.",
          [] (B& b) {
              Tine t = kTineI;
              t.bark = 0.5f;
              t.grit = 0.6f;
              t.cutoff = 4000.0f;
              tine (b, t);
              distortion (b, 0, kDistOverdrive, 0.25f, 0.5f);
              room (b, 0.15f);
              macros (b, "Bark", "Space", "Drive", "Decay");
              b.route ("Macro 1", "e1_wt_pm", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "fx_a1_p2", 0.35f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "Tine Lo-Fi", "Keys", "Tine piano on an old tape: dark, wobbly, a little hiss. Intimate and nostalgic.",
          [] (B& b) {
              Tine t = kTineI;
              t.cutoff = 2200.0f;
              tine (b, t);
              noise (b, 0, "Tape Hiss", 0.025f, 0.0f);
              b.fx (0, FxTapeEcho, { { 0, syncKnob (kDiv4) }, { 2, 0.2f }, { 4, 0.6f }, { 7, 0.2f } });
              b.set ("fx_a1_sync", 1.0f);
              room (b, 0.2f);
              macros (b, "Bark", "Space", "Hiss", "Decay");
              b.route ("Macro 1", "e1_wt_pm", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "u_n1_vol", 0.1f).route ("Macro 4", "env1_d", 0.35f);
          } },
        // ================================================================ TINE II (cleaner, more bell)
        { "Tine II Stage", "Keys", "The later tine stage piano: cleaner and more bell-like, less bark, very even.",
          [] (B& b) {
              tine (b, kTineII);
              room (b, 0.15f);
              macros (b, "Bark", "Space", "Grit", "Decay");
              b.route ("Macro 1", "e1_wt_pm", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "f1_drive", 0.4f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "Tine II Phaser", "Keys", "Clean tine piano through a slow phaser. The 70s soul and jazz-funk sound.",
          [] (B& b) {
              tine (b, kTineII);
              phaser (b, 0, 0.5f, 0.1f, 0.45f);
              room (b, 0.15f);
              macros (b, "Bark", "Space", "Phase", "Decay");
              b.route ("Macro 1", "e1_wt_pm", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "fx_a1_p8", 0.35f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "Tine II Chorus", "Keys", "Clean tine piano with chorus and a soft dotted-eighth echo. Modern pop and worship.",
          [] (B& b) {
              tine (b, kTineII);
              chorus (b, 0, 0.45f);
              delay (b, 1, kDiv8D, 0.2f, 0.12f, true);
              hall (b, 0.25f);
              macros (b, "Bark", "Space", "Chorus", "Decay");
              b.route ("Macro 1", "e1_wt_pm", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "fx_a1_p8", 0.4f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "Tine II Bell", "Keys", "Tine piano with the bell on top turned up: sparkly, glassy, great for high voicings.",
          [] (B& b) {
              Tine t = kTineII;
              t.tine = 0.75f;
              t.tineDecay = 0.6f;
              tine (b, t);
              hall (b, 0.3f, 0.75f, 0.65f);
              macros (b, "Bark", "Space", "Bell", "Decay");
              b.route ("Macro 1", "e1_wt_pm", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "e2_wt_pm", 0.3f).route ("Macro 4", "env1_d", 0.35f);
          } },
        // ================================================================ FM (DX-style)
        { "FM Ballad EP", "Keys", "The classic 80s FM electric piano: glassy tine, soft body, gentle chorus.",
          [] (B& b) {
              fm (b, { 1.0f, 0.10f, 0.30f, 1.0f, 14.0f, 0.45f, 0.45f, 0.25f, 0.07f, 9000.0f, 4.5f });
              chorus (b, 0, 0.3f);
              room (b, 0.2f);
              macros (b, "Bark", "Space", "Chorus", "Decay");
              b.route ("Macro 1", "e1_wt_pm", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "fx_a1_p8", 0.4f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "FM Glass EP", "Keys", "FM piano with a long, ringing glass tine and a ping-pong echo. Shimmering ballad keys.",
          [] (B& b) {
              fm (b, { 1.0f, 0.08f, 0.20f, 1.0f, 14.0f, 0.50f, 0.65f, 0.80f, 0.08f, 12000.0f, 5.0f });
              delay (b, 0, kDiv4D, 0.25f, 0.15f, true);
              hall (b, 0.35f, 0.8f, 0.7f);
              macros (b, "Bark", "Space", "Glass", "Decay");
              b.route ("Macro 1", "e1_wt_pm", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "env3_d", 0.3f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "FM Soft EP", "Keys", "Round, warm FM piano with barely any tine. Sits quietly under singers.",
          [] (B& b) {
              fm (b, { 1.0f, 0.04f, 0.15f, 0.8f, 14.0f, 0.35f, 0.20f, 0.20f, 0.05f, 3500.0f, 4.0f });
              chorus (b, 0, 0.25f);
              hall (b, 0.3f, 0.75f, 0.65f);
              macros (b, "Bark", "Space", "Chorus", "Decay");
              b.route ("Macro 1", "e1_wt_pm", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "fx_a1_p8", 0.4f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "FM Bright EP", "Keys", "Bright, cutting FM piano that pokes through a full band.",
          [] (B& b) {
              fm (b, { 1.0f, 0.18f, 0.35f, 0.8f, 14.0f, 0.55f, 0.55f, 0.30f, 0.07f, 14000.0f, 4.0f });
              chorus (b, 0, 0.35f);
              room (b, 0.2f);
              macros (b, "Bark", "Space", "Chorus", "Decay");
              b.route ("Macro 1", "e1_wt_pm", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "fx_a1_p8", 0.4f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "FM Bell EP", "Keys", "FM piano with a bell layer (a 3.5 ratio) that rings on after the strike.",
          [] (B& b) {
              fm (b, { 1.0f, 0.08f, 0.25f, 1.0f, 3.5f, 0.30f, 0.45f, 1.40f, 0.03f, 10000.0f, 5.0f });
              hall (b, 0.35f, 0.8f, 0.7f);
              macros (b, "Bark", "Space", "Bell", "Decay");
              b.route ("Macro 1", "e1_wt_pm", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "e2_wt_pm", 0.3f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "FM Wide EP", "Keys", "FM piano spread wide by a vintage chorus and a ping-pong echo.",
          [] (B& b) {
              fm (b, { 1.0f, 0.10f, 0.30f, 1.0f, 14.0f, 0.45f, 0.45f, 0.25f, 0.10f, 9000.0f, 4.5f });
              vintageChorus (b, 0, 2, 0.5f);
              b.set ("fx_a1_p6", 0.8f);
              delay (b, 1, kDiv8D, 0.25f, 0.15f, true);
              hall (b, 0.25f);
              macros (b, "Bark", "Space", "Chorus", "Decay");
              b.route ("Macro 1", "e1_wt_pm", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "fx_a1_p5", 0.4f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "FM Keys and Pad", "Keys", "Soft FM piano with a warm pad that fades in underneath while you hold the chord.",
          [] (B& b) {
              fmVoice (b, 0, 1.0f, 0.08f, 0.0f);
              env (b, "env2", 0.001f, 4.5f, 0.0f, 0.5f);
              b.route ("Env 2", "e1_vol", 0.75f).route ("Env 2", "e1_wt_pm", 0.2f, "Velocity", 1.0f);
              b.route ("Keytrack", "env2_d", -0.3f);
              wavetable (b, 1, "Warm Pad", 0.4f, 0.0f);
              uni (b, 1, 0, 3, 0.12f, 0.7f);
              env (b, "env3", 1.8f, 1.0f, 1.0f, 2.5f);
              b.set ("env3_link", 0.0f);
              b.route ("Env 3", "e2_vol", 0.25f);
              filter (b, 0, kLP12, 6000.0f, 0.0f, 0.3f);
              env (b, "env1", 0.0f, 1.0f, 1.0f, 2.5f);
              b.set ("amp_velo", 0.6f);
              chorus (b, 0, 0.25f);
              hall (b, 0.35f, 0.8f, 0.7f);
              macros (b, "Bark", "Space", "Pad", "Release");
              b.route ("Macro 1", "e1_wt_pm", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "e2_vol", 0.25f).route ("Macro 4", "env1_r", 0.35f);
          } },
        { "FM Punch EP", "Keys", "Very dynamic FM piano: soft is gentle, hard really punches. Made for expressive playing.",
          [] (B& b) {
              fm (b, { 1.0f, 0.15f, 0.50f, 0.6f, 14.0f, 0.50f, 0.50f, 0.20f, 0.06f, 12000.0f, 3.5f });
              b.set ("amp_velo", 0.85f);
              room (b, 0.15f);
              macros (b, "Bark", "Space", "Dynamics", "Decay");
              b.route ("Macro 1", "e1_wt_pm", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "amp_velo", 0.15f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "FM Velvet EP", "Keys", "Soft-edged FM piano (triangle modulator) with a slow stereo pan. Smooth and warm.",
          [] (B& b) {
              Fm f { 1.0f, 0.12f, 0.20f, 1.2f, 14.0f, 0.30f, 0.30f, 0.25f, 0.05f, 4000.0f, 5.0f };
              f.modWave = 1;
              fm (b, f);
              tremolo (b, 0, 0.25f, 0.4f);
              hall (b, 0.3f, 0.75f, 0.65f);
              macros (b, "Bark", "Space", "Pan", "Decay");
              b.route ("Macro 1", "e1_wt_pm", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "fx_a1_p1", 0.4f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "FM Soul EP", "Keys", "90s soul and R&B ballad FM piano: chorus, a gentle pan and a touch of tube.",
          [] (B& b) {
              fm (b, { 1.0f, 0.14f, 0.30f, 1.0f, 14.0f, 0.45f, 0.35f, 0.25f, 0.05f, 7000.0f, 5.0f });
              chorus (b, 0, 0.35f);
              tremolo (b, 1, 0.2f, 0.45f);
              distortion (b, 2, kDistTube, 0.12f, 0.35f);
              room (b, 0.2f);
              macros (b, "Bark", "Space", "Pan", "Decay");
              b.route ("Macro 1", "e1_wt_pm", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "fx_a2_p1", 0.4f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "FM Hollow EP", "Keys", "FM piano whose body uses a 3:1 modulator: hollow and reedy, with a slow phaser.",
          [] (B& b) {
              fm (b, { 3.0f, 0.12f, 0.25f, 1.0f, 14.0f, 0.40f, 0.35f, 0.20f, 0.06f, 8000.0f, 4.5f });
              phaser (b, 0, 0.4f, 0.1f, 0.3f);
              room (b, 0.2f);
              macros (b, "Bark", "Space", "Phase", "Decay");
              b.route ("Macro 1", "e1_wt_pm", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "fx_a1_p8", 0.35f).route ("Macro 4", "env1_d", 0.35f);
          } },
        // ================================================================ REED (Wurlitzer-style)
        { "Reed Classic", "Keys", "Reed electric piano: nasal, warm, gets a raspy bite when you dig in.",
          [] (B& b) {
              reed (b, 0.28f, 0.28f, 1800.0f, 0.3f, 0.0f);
              room (b, 0.18f);
              macros (b, "Brightness", "Space", "Grit", "Decay");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "f1_drive", 0.4f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "Reed Tremolo", "Keys", "Reed piano with its built-in amp tremolo. Macro 3 sets the depth.",
          [] (B& b) {
              reed (b, 0.28f, 0.28f, 1800.0f, 0.3f, 0.25f);
              b.route ("LFO 1", "e1_vol", -0.25f, "Macro 3", 1.0f);
              room (b, 0.18f);
              macros (b, "Brightness", "Space", "Tremolo", "Decay");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "Reed Driven", "Keys", "Reed piano pushed into an overdriven amp. Rock and blues rasp.",
          [] (B& b) {
              reed (b, 0.32f, 0.25f, 2200.0f, 0.6f, 0.0f);
              distortion (b, 0, kDistOverdrive, 0.3f, 0.5f);
              room (b, 0.15f);
              macros (b, "Brightness", "Space", "Drive", "Decay");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "fx_a1_p2", 0.35f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "Reed Soft", "Keys", "Gentle, dark reed piano with chorus. Warm and close, for quiet songs.",
          [] (B& b) {
              reed (b, 0.15f, 0.3f, 1200.0f, 0.1f, 0.0f);
              chorus (b, 0, 0.25f);
              hall (b, 0.3f, 0.75f, 0.65f);
              macros (b, "Brightness", "Space", "Chorus", "Decay");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "fx_a1_p8", 0.4f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "Reed Phaser", "Keys", "Reed piano with a little tremolo and a slow phaser. 70s pop.",
          [] (B& b) {
              reed (b, 0.28f, 0.28f, 1900.0f, 0.3f, 0.08f);
              phaser (b, 0, 0.5f, 0.12f, 0.4f);
              room (b, 0.18f);
              macros (b, "Brightness", "Space", "Phase", "Decay");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "fx_a1_p8", 0.35f).route ("Macro 4", "env1_d", 0.35f);
          } },
        // ================================================================ CLAV (Clavinet-style)
        { "Clav Funk", "Keys", "Bright, snappy clav for funk rhythm parts. Play short, percussive notes.",
          [] (B& b) {
              clav (b, 0.15f, 2500.0f, 0.35f, 1.6f, 0.35f);
              room (b, 0.1f);
              macros (b, "Brightness", "Space", "Bite", "Decay");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "f1_reso", 0.3f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "Clav Muted", "Keys", "Muted, plucky clav: each note stops almost at once. Tight and rhythmic.",
          [] (B& b) {
              clav (b, 0.25f, 1800.0f, 0.3f, 0.25f, 0.0f);
              room (b, 0.1f);
              macros (b, "Brightness", "Space", "Bite", "Length");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "f1_reso", 0.3f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "Clav Wah", "Keys", "Clav through an envelope wah: every note goes 'wow'. Harder = more wah.",
          [] (B& b) {
              clav (b, 0.18f, 500.0f, 0.45f, 1.4f, 0.3f);
              filter (b, 0, kBP12, 500.0f, 0.55f, 0.5f);
              b.set ("env2_d", 0.3f).set ("env2_s", 0.1f);
              b.set ("f1_vol", 2.6f);
              room (b, 0.1f);
              macros (b, "Brightness", "Space", "Wah", "Decay");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "f1_reso", 0.3f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "Clav Phaser", "Keys", "Clav through a deep phaser: swirling funk.",
          [] (B& b) {
              clav (b, 0.15f, 2500.0f, 0.35f, 1.6f, 0.35f);
              phaser (b, 0, 0.6f, 0.15f, 0.5f);
              room (b, 0.1f);
              macros (b, "Brightness", "Space", "Phase", "Decay");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "fx_a1_p8", 0.35f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "Clav Mellow", "Keys", "Rounder, darker clav with a light chorus: rhythm without the sharp edge.",
          [] (B& b) {
              clav (b, 0.4f, 1200.0f, 0.2f, 1.8f, 0.4f);
              chorus (b, 0, 0.2f);
              room (b, 0.15f);
              macros (b, "Brightness", "Space", "Chorus", "Decay");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "fx_a1_p8", 0.4f).route ("Macro 4", "env1_d", 0.35f);
          } },
    };
    return list;
}

// Tags: Tine = Rhodes-style, FM = DX-style, Reed = Wurlitzer-style, Clav = Clavinet-style.
juce::StringArray pack3Tags (const juce::String& presetName)
{
    static const std::map<juce::String, const char*> tags {
        { "Tine I Stage", "Electric Piano, Tine" }, { "Tine I Suitcase", "Electric Piano, Tine, Tremolo" },
        { "Tine I Ballad", "Electric Piano, Tine, Worship" }, { "Tine I Growl", "Electric Piano, Tine, Driven" },
        { "Tine Lo-Fi", "Electric Piano, Tine, Lo-Fi" },
        { "Tine II Stage", "Electric Piano, Tine" }, { "Tine II Phaser", "Electric Piano, Tine, Phaser" },
        { "Tine II Chorus", "Electric Piano, Tine, Chorus, Worship" }, { "Tine II Bell", "Electric Piano, Tine, Worship" },
        { "FM Ballad EP", "Electric Piano, FM, Worship" }, { "FM Glass EP", "Electric Piano, FM, Worship" },
        { "FM Soft EP", "Electric Piano, FM, Worship" }, { "FM Bright EP", "Electric Piano, FM" },
        { "FM Bell EP", "Electric Piano, FM, Bells" }, { "FM Wide EP", "Electric Piano, FM, Chorus" },
        { "FM Keys and Pad", "Electric Piano, FM, Pad, Worship" }, { "FM Punch EP", "Electric Piano, FM" },
        { "FM Velvet EP", "Electric Piano, FM, Tremolo" }, { "FM Soul EP", "Electric Piano, FM, Tremolo" },
        { "FM Hollow EP", "Electric Piano, FM, Phaser" },
        { "Reed Classic", "Electric Piano, Reed" }, { "Reed Tremolo", "Electric Piano, Reed, Tremolo" },
        { "Reed Driven", "Electric Piano, Reed, Driven" }, { "Reed Soft", "Electric Piano, Reed, Worship" },
        { "Reed Phaser", "Electric Piano, Reed, Phaser" },
        { "Clav Funk", "Clav, Funk" }, { "Clav Muted", "Clav, Funk" }, { "Clav Wah", "Clav, Funk" },
        { "Clav Phaser", "Clav, Funk, Phaser" }, { "Clav Mellow", "Clav" },
    };
    const auto it = tags.find (presetName);
    return it == tags.end() ? juce::StringArray() : PresetManager::parseTags (it->second);
}
