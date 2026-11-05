// Preset Pack 1: Keys, Strings & Synths.
//
// A downloadable set of user presets (not built into the plugin). Built in
// code like the factory set, then written out as .csp files and level-matched
// by `cs_state_test --export-pack <dir>`. Install: copy the pack folder into
// Documents/Church Synth/Presets.
//
// Macro convention as the factory set: M1 Brightness / Tone, M2 Space, M3 a
// per-sound character control, M4 Decay / Attack / Glide.

#include "Packs.h"

#include "plugin/PresetHelpers.h"

#include <array>
#include <map>

using namespace presethelpers;

namespace
{
// Filter with a model (Ladder, OTA...) instead of the Classic type.
void model (B& b, int f, int type, int submode, float cutoff, float reso, float kbd = 0.0f, float drive = 0.0f)
{
    const juce::String p = "f" + juce::String (f + 1) + "_";
    b.set (p + "on", 1.0f).set (p + "type", (float) type).set (p + "submode", (float) submode).set (p + "cutoff", cutoff)
        .set (p + "reso", reso).set (p + "kbd", kbd).set (p + "drive", drive);
}

// Wavetable engine as a two-operator FM voice: a sine carrier (Basic Shapes,
// position 0) phase-modulated by a sine at `ratio`.
void fmVoice (B& b, int e, float ratio, float pm, float vol = 0.8f)
{
    const juce::String p = "e" + juce::String (e + 1) + "_";
    wavetable (b, e, "Basic Shapes", 0.0f, vol);
    b.set (p + "wt_mod_wave", 0.0f).set (p + "wt_mod_tune_mode", 0.0f).set (p + "wt_mod_ratio", ratio).set (p + "wt_pm", pm);
}

void engineOn (B& b, int e, float vol)
{
    b.set ("e" + juce::String (e + 1) + "_on", 1.0f).set ("e" + juce::String (e + 1) + "_vol", vol);
}

void tremolo (B& b, int slot, float amount, float rate)
{
    b.fx (slot, FxPanner, { { 0, amount }, { 1, rate } });
}

void room (B& b, float send) { hall (b, send, 0.45f, 0.45f); }

// The classic one-oscillator chorus polysynth: saw + pulse (PWM) + sub square,
// a gentle highpass, a 24 dB lowpass and the vintage chorus.
void chorusPoly (B& b, float saw, float pulse, float sub, float cutoff, float reso, int chorusMode, float chorusMix, float chorusWidth = 0.7f)
{
    osc (b, 0, 0, kSaw, saw);
    osc (b, 0, 1, kSquare, pulse, 0.0f, 0.0f, 0.5f);
    osc (b, 0, 2, kSquare, sub, -12.0f);
    b.set ("e1_vol", 0.7f);
    lfo (b, 0, LfoTriangle, 0.65f);
    b.route ("LFO 1", "e1_o2_width", 0.3f);
    filter (b, 0, kLP24, cutoff, reso, 0.5f);
    b.set ("f1_analog", 1.0f);
    filter (b, 1, kHP12, 110.0f, 0.0f);
    b.set ("f_routing", 0.0f);
    vintageChorus (b, 0, chorusMode, chorusMix);
    b.set ("fx_a1_p6", chorusWidth); // keep it mono-safe for a live PA
}

void bassSeq (B& b, int rate, std::initializer_list<int> steps, float gate)
{
    b.set ("seq_on", 1.0f).set ("seq_mode", 1.0f).set ("seq_rate", (float) rate).set ("seq_vel_blend", 0.6f);
    b.set ("seq_length", (float) steps.size());
    b.pattern (gatePattern (steps, gate));
}

// Tonewheel organ from sine "drawbars" (0..8 each, like the real thing: every step
// is 3 dB): 16', 5 1/3', 8', 4', 2 2/3', 2', then 1 1/3' or 1' on the utility osc.
// Percussion (optional) is a fast-decaying 2 2/3' or 4' from Env 3.
void drawbars (B& b, std::array<int, 9> db, int percussion = 0)
{
    auto amp = [] (int v) { return v <= 0 ? 0.0f : std::pow (10.0f, (float) (v - 8) * 3.0f / 20.0f); };
    static const float kSemis[6] = { -12.0f, 7.0f, 0.0f, 12.0f, 19.0f, 24.0f };
    for (int i = 0; i < 6; ++i)
        osc (b, i / 3, i % 3, kSine, amp (db[(size_t) i]) * 0.7f, kSemis[i]);
    engineOn (b, 0, 0.55f);
    engineOn (b, 1, 0.55f);
    const int top = std::max (db[7], db[8]);
    if (top > 0 || db[6] > 0)
        b.set ("u_osc_on", 1.0f).set ("u_osc_wave", 0.0f).set ("u_osc_coarse", db[8] >= db[7] ? 36.0f : 31.0f)
            .set ("u_osc_vol", amp (std::max (top, db[6])) * 0.35f);
    noise (b, 0, "Key Click", 0.12f, 0.4f, false);
    if (percussion > 0)
    {
        env (b, "env3", 0.001f, 0.28f, 0.0f, 0.1f);
        b.route ("Env 3", percussion == 3 ? "e2_o2_vol" : "e2_o1_vol", 0.6f);
    }
    env (b, "env1", 0.004f, 0.1f, 1.0f, 0.07f);
    b.set ("amp_velo", 0.0f);
}

// Rotary speaker in slot A1; the mod wheel and Macro 3 switch it to Fast.
void rotary (B& b, float drive, float mix = 1.0f)
{
    b.fx (0, FxRotary, { { 5, drive }, { 7, mix } });
    b.route ("Mod Wheel", "fx_a1_p1", 1.0f);
}

void organMacros (B& b)
{
    macros (b, "Tone", "Space", "Rotary Fast", "Drive");
    b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "fx_a1_p1", 1.0f).route ("Macro 4", "fx_a1_p6", 0.6f);
}

// Two-oscillator polysynth voice (saw + pulse) with its own drift.
void twoOsc (B& b, float saw, float pulse, float width, float detune, float drift = 0.3f)
{
    osc (b, 0, 0, kSaw, saw);
    osc (b, 0, 1, kSquare, pulse, 0.0f, detune, width);
    b.set ("e1_vol", 0.6f).set ("e1_drift", drift);
}
} // namespace

const std::vector<FactoryPreset>& pack1Presets()
{
    static const std::vector<FactoryPreset> list {
        // ================================================================ ELECTRIC PIANOS
        { "Tine Piano", "Keys", "Bell-like FM electric piano: a soft body with a tine ping on top. Velocity brings out the bark.",
          [] (B& b) {
              fmVoice (b, 0, 1.0f, 0.12f, 0.8f);
              env (b, "env2", 0.001f, 1.4f, 0.0f, 0.5f);
              b.route ("Env 2", "e1_wt_pm", 0.35f, "Velocity", 1.0f);
              fmVoice (b, 1, 14.0f, 0.45f, 0.0f);
              b.set ("e2_on", 1.0f);
              env (b, "env3", 0.001f, 0.22f, 0.0f, 0.1f);
              b.route ("Env 3", "e2_vol", 0.45f, "Velocity", 0.8f);
              filter (b, 0, kLP12, 9000.0f, 0.0f, 0.3f);
              env (b, "env1", 0.002f, 4.5f, 0.0f, 0.45f);
              b.set ("amp_velo", 0.7f);
              tremolo (b, 0, 0.22f, 0.45f);
              chorus (b, 1, 0.2f);
              room (b, 0.2f);
              macros (b, "Bark", "Space", "Tremolo", "Decay");
              b.route ("Macro 1", "e1_wt_pm", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "fx_a1_p1", 0.6f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "Suitcase Keys", "Keys", "Warm tine piano with a wide stereo auto-pan and a little amp grit. The classic ballad EP.",
          [] (B& b) {
              osc (b, 0, 0, kSine, 0.9f);
              osc (b, 0, 1, kSine, 0.12f, 12.0f, 0.02f);
              osc (b, 0, 2, kSine, 0.0f, 0.0f);
              b.set ("e1_mod_src", 0.0f).set ("e1_o1_fm", 0.1f).set ("e1_vol", 0.8f);
              env (b, "env2", 0.001f, 0.8f, 0.05f, 0.4f);
              b.route ("Env 2", "e1_o1_fm", 0.28f, "Velocity", 1.0f);
              filter (b, 0, kLP12, 4200.0f, 0.05f, 0.4f, 0.25f);
              b.set ("f1_analog", 1.0f);
              env (b, "env1", 0.002f, 4.0f, 0.0f, 0.4f);
              b.set ("amp_velo", 0.7f);
              tremolo (b, 0, 0.55f, 0.5f);
              b.fx (1, FxDistortion, { { 0, (2 + 0.5f) / 16.0f }, { 1, 0.18f }, { 10, 0.45f } });
              room (b, 0.18f);
              macros (b, "Bark", "Space", "Pan", "Grit");
              b.route ("Macro 1", "e1_o1_fm", 0.25f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "fx_a1_p1", 0.45f).route ("Macro 4", "fx_a2_p2", 0.5f);
          } },
        { "Reed Piano", "Keys", "Reedy, slightly nasal electric piano with a fast amp tremolo. Push the velocity for bite.",
          [] (B& b) {
              osc (b, 0, 0, kTri, 0.75f);
              osc (b, 0, 1, kSquare, 0.28f, 0.0f, 0.0f, 0.28f);
              b.set ("e1_vol", 0.8f);
              filter (b, 0, kLP12, 1800.0f, 0.15f, 0.5f);
              env (b, "env2", 0.001f, 0.5f, 0.1f, 0.3f);
              b.route ("Env 2", "f1_cutoff", 0.3f, "Velocity", 1.0f);
              env (b, "env1", 0.002f, 2.6f, 0.12f, 0.25f);
              lfo (b, 0, LfoSine, 5.6f, false);
              b.set ("lfo1_retrig", 3.0f);
              b.set ("amp_mod_src", 8.0f).set ("amp_velo", 0.25f);
              b.fx (0, FxDistortion, { { 0, (4 + 0.5f) / 16.0f }, { 1, 0.15f }, { 10, 0.4f } });
              room (b, 0.18f);
              macros (b, "Brightness", "Space", "Tremolo", "Decay");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "amp_velo", 0.4f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "Eighties Tines", "Keys", "Bright, glassy FM piano with chorus: the classic pop ballad keys.",
          [] (B& b) {
              fmVoice (b, 0, 1.0f, 0.22f, 0.75f);
              env (b, "env2", 0.001f, 1.0f, 0.1f, 0.4f);
              b.route ("Env 2", "e1_wt_pm", 0.3f, "Velocity", 1.0f);
              fmVoice (b, 1, 14.0f, 0.6f, 0.0f);
              b.set ("e2_on", 1.0f);
              env (b, "env3", 0.001f, 0.35f, 0.0f, 0.2f);
              b.route ("Env 3", "e2_vol", 0.55f, "Velocity", 0.6f);
              filter (b, 0, kLP12, 12000.0f, 0.0f, 0.2f);
              env (b, "env1", 0.002f, 3.5f, 0.05f, 0.5f);
              b.set ("amp_velo", 0.6f);
              chorus (b, 0, 0.4f);
              delay (b, 1, kDiv8D, 0.25f, 0.15f, true);
              hall (b, 0.22f, 0.6f, 0.5f);
              macros (b, "Bark", "Space", "Chorus", "Decay");
              b.route ("Macro 1", "e1_wt_pm", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "fx_a1_p8", 0.4f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "Dreamy EP", "Keys", "Soft tine piano washed in chorus, a dotted echo and shimmer. For quiet worship moments.",
          [] (B& b) {
              fmVoice (b, 0, 1.0f, 0.08f, 0.8f);
              env (b, "env2", 0.001f, 1.6f, 0.0f, 0.8f);
              b.route ("Env 2", "e1_wt_pm", 0.22f, "Velocity", 1.0f);
              filter (b, 0, kLP12, 5000.0f, 0.0f, 0.3f);
              env (b, "env1", 0.003f, 5.0f, 0.0f, 1.6f);
              b.set ("amp_velo", 0.55f);
              vintageChorus (b, 0, 1, 0.5f);
              delay (b, 1, kDiv4D, 0.35f, 0.22f, true);
              shimmer (b, 0.3f, 0.35f, 0.75f);
              macros (b, "Bark", "Space", "Echo", "Decay");
              b.route ("Macro 1", "e1_wt_pm", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "fx_a2_p8", 0.4f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "Electric Grand", "Keys", "Electric grand piano: bright hammer, short shimmer of detune and a light chorus.",
          [] (B& b) {
              osc (b, 0, 0, kSaw, 0.55f);
              osc (b, 0, 1, kTri, 0.45f, 12.0f, 0.03f);
              b.set ("e1_vol", 0.75f);
              noise (b, 0, "Key Click", 0.2f, 0.3f, false);
              filter (b, 0, kLP24, 1500.0f, 0.1f, 0.75f);
              env (b, "env2", 0.001f, 0.9f, 0.25f, 0.5f);
              b.route ("Env 2", "f1_cutoff", 0.3f).route ("Velocity", "f1_cutoff", 0.2f);
              env (b, "env1", 0.002f, 3.2f, 0.08f, 0.5f);
              b.set ("amp_velo", 0.65f);
              chorus (b, 0, 0.3f);
              room (b, 0.2f);
              macros (b, "Brightness", "Space", "Hammer", "Decay");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "u_n1_vol", 0.4f).route ("Macro 4", "env1_d", 0.35f);
          } },
        // ================================================================ FELT PIANOS / KEYS
        { "Felt Piano", "Keys", "Intimate felted piano: soft hammer thump, warm tone, a small room. Lovely for prayer.",
          [] (B& b) {
              osc (b, 0, 0, kTri, 0.85f);
              osc (b, 0, 1, kSine, 0.25f, 12.0f, 0.03f);
              uni (b, 0, 0, 2, 0.03f, 0.5f);
              b.set ("e1_vol", 0.8f);
              noise (b, 0, "Thump", 0.35f, -0.3f, false);
              filter (b, 0, kLP12, 900.0f, 0.0f, 0.65f);
              env (b, "env2", 0.001f, 0.6f, 0.3f, 0.4f);
              b.route ("Env 2", "f1_cutoff", 0.18f).route ("Velocity", "f1_cutoff", 0.25f);
              env (b, "env1", 0.003f, 3.8f, 0.0f, 0.35f);
              b.set ("amp_velo", 0.7f);
              room (b, 0.28f);
              macros (b, "Tone", "Space", "Hammer", "Decay");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "u_n1_vol", 0.4f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "Felt Lullaby", "Keys", "Darker felt piano with a little tape hiss and wobble, and a longer tail.",
          [] (B& b) {
              osc (b, 0, 0, kSine, 0.85f);
              osc (b, 0, 1, kTri, 0.22f, 12.0f, 0.02f);
              osc (b, 0, 2, kSine, 0.06f, 19.0f);
              b.set ("e1_vol", 0.8f);
              noise (b, 0, "Soft Click", 0.25f, -0.4f, false);
              noise (b, 1, "Tape Hiss", 0.04f, 0.2f);
              filter (b, 0, kLP12, 700.0f, 0.0f, 0.6f);
              b.route ("Velocity", "f1_cutoff", 0.22f);
              env (b, "env1", 0.004f, 4.5f, 0.0f, 0.9f);
              b.set ("amp_velo", 0.6f);
              b.fx (0, FxTapeEcho, { { 2, 0.25f }, { 3, 0.2f }, { 4, 0.45f }, { 7, 0.18f } });
              hall (b, 0.32f, 0.6f, 0.55f);
              macros (b, "Tone", "Space", "Hiss", "Decay");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "u_n2_vol", 0.15f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "Felt Upright", "Keys", "Brighter felt upright with more hammer: sits in a band mix.",
          [] (B& b) {
              osc (b, 0, 0, kTri, 0.7f);
              osc (b, 0, 1, kSaw, 0.3f, 0.0f, 0.04f);
              b.set ("e1_vol", 0.8f);
              noise (b, 0, "Thump", 0.25f, -0.2f, false);
              noise (b, 1, "Key Click", 0.1f, 0.2f, false);
              filter (b, 0, kLP24, 1200.0f, 0.05f, 0.7f);
              env (b, "env2", 0.001f, 0.7f, 0.25f, 0.4f);
              b.route ("Env 2", "f1_cutoff", 0.22f).route ("Velocity", "f1_cutoff", 0.3f);
              env (b, "env1", 0.002f, 3.2f, 0.0f, 0.35f);
              b.set ("amp_velo", 0.7f);
              room (b, 0.22f);
              macros (b, "Tone", "Space", "Hammer", "Decay");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "u_n1_vol", 0.4f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "Keys and Pad", "Keys", "Tine piano with a soft pad that fades in underneath as you hold the chord.",
          [] (B& b) {
              fmVoice (b, 0, 1.0f, 0.12f, 0.7f);
              env (b, "env2", 0.001f, 1.2f, 0.0f, 0.5f);
              b.route ("Env 2", "e1_wt_pm", 0.3f, "Velocity", 1.0f);
              engineOn (b, 1, 0.0f);
              osc (b, 1, 0, kSaw, 0.8f);
              uni (b, 1, 0, 4, 0.25f, 0.9f);
              env (b, "env3", 1.8f, 1.0f, 1.0f, 2.2f);
              b.set ("env3_link", 0.0f);
              b.route ("Env 3", "e2_vol", 0.32f);
              b.set ("e2_fmix", 1.0f);
              filter (b, 0, kLP12, 9000.0f, 0.0f, 0.2f);
              filter (b, 1, kLP24, 1300.0f, 0.1f, 0.3f);
              b.set ("f_routing", 1.0f);
              env (b, "env1", 0.002f, 6.0f, 0.5f, 2.0f);
              b.set ("amp_velo", 0.55f);
              vintageChorus (b, 0, 0, 0.35f);
              hall (b, 0.3f, 0.7f, 0.6f);
              macros (b, "Bark", "Space", "Pad", "Release");
              b.route ("Macro 1", "e1_wt_pm", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "e2_vol", 0.3f).route ("Macro 4", "env1_r", 0.35f);
          } },
        // ================================================================ STRINGS
        { "Chamber Strings", "Strings/Brass", "Warm string section with a natural bow. Mod wheel adds vibrato, velocity brightens.",
          [] (B& b) {
              wavetable (b, 0, "String Ensemble", 0.5f, 0.6f);
              uni (b, 0, 0, 6, 0.16f, 0.9f);
              engineOn (b, 1, 0.3f);
              osc (b, 1, 0, kSaw, 0.8f);
              uni (b, 1, 0, 3, 0.3f, 0.8f);
              filter (b, 0, kLP12, 2300.0f, 0.05f, 0.4f);
              filter (b, 1, kHP12, 160.0f, 0.0f);
              b.set ("f_routing", 0.0f);
              env (b, "env1", 0.35f, 1.0f, 1.0f, 0.9f);
              b.route ("Velocity", "f1_cutoff", 0.2f);
              lfo (b, 0, LfoSine, 5.2f);
              b.route ("LFO 1", "e1_fine", 0.05f, "Mod Wheel", 1.0f).route ("LFO 1", "e2_fine", 0.05f, "Mod Wheel", 1.0f);
              chorus (b, 0, 0.25f);
              hall (b, 0.35f, 0.75f, 0.65f);
              macros (b, "Brightness", "Space", "Vibrato", "Attack");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "lfo1_rate", 0.15f).route ("Macro 4", "env1_a", 0.35f);
          } },
        { "Ensemble Strings", "Strings/Brass", "String-machine ensemble: buzzy saws through a deep triple chorus. Pure late-70s.",
          [] (B& b) {
              for (int e = 0; e < 2; ++e)
              {
                  engineOn (b, e, 0.5f);
                  osc (b, e, 0, kSaw, 0.85f, e == 0 ? 0.0f : 12.0f);
                  uni (b, e, 0, 3, 0.12f, 0.7f);
              }
              filter (b, 0, kLP12, 4200.0f, 0.0f, 0.3f);
              filter (b, 1, kHP12, 220.0f, 0.0f);
              b.set ("f_routing", 0.0f);
              env (b, "env1", 0.25f, 1.0f, 1.0f, 1.2f);
              vintageChorus (b, 0, 2, 0.65f);
              b.set ("fx_a1_p6", 0.6f);
              b.fx (1, FxPhaser, { { 3, 0.4f }, { 4, 0.12f }, { 7, 0.25f } });
              hall (b, 0.3f);
              macros (b, "Brightness", "Space", "Phase", "Attack");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "fx_a2_p8", 0.5f).route ("Macro 4", "env1_a", 0.35f);
          } },
        { "Slow Strings Swell", "Strings/Brass", "Cinematic strings that bloom slowly from dark to bright. Hold and let it rise.",
          [] (B& b) {
              wavetable (b, 0, "String Ensemble", 0.35f, 0.6f);
              uni (b, 0, 0, 6, 0.2f, 1.0f);
              engineOn (b, 1, 0.35f);
              osc (b, 1, 0, kSaw, 0.8f, -12.0f);
              uni (b, 1, 0, 3, 0.25f);
              filter (b, 0, kLP24, 600.0f, 0.1f, 0.4f);
              env (b, "env2", 2.2f, 3.0f, 0.7f, 2.0f);
              b.route ("Env 2", "f1_cutoff", 0.35f).route ("Env 2", "e1_wt_pos", 0.3f);
              env (b, "env1", 1.4f, 1.0f, 1.0f, 2.2f);
              hall (b, 0.4f, 0.85f, 0.75f);
              macros (b, "Brightness", "Space", "Bloom", "Attack");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "env2_a", -0.3f).route ("Macro 4", "env1_a", 0.35f);
          } },
        { "Pizzicato Strings", "Strings/Brass", "Plucked string section with a short room. For light rhythmic parts.",
          [] (B& b) {
              wavetable (b, 0, "String Ensemble", 0.55f, 0.7f);
              uni (b, 0, 0, 4, 0.1f, 0.8f);
              noise (b, 0, "Pluck Noise", 0.12f, 0.3f, false);
              filter (b, 0, kLP12, 1800.0f, 0.05f, 0.5f);
              env (b, "env2", 0.001f, 0.18f, 0.0f, 0.15f);
              b.route ("Env 2", "f1_cutoff", 0.3f).route ("Velocity", "f1_cutoff", 0.2f);
              env (b, "env1", 0.002f, 0.38f, 0.0f, 0.3f);
              b.set ("amp_velo", 0.7f);
              hall (b, 0.3f, 0.55f, 0.5f);
              macros (b, "Brightness", "Space", "Snap", "Decay");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "u_n1_vol", 0.4f).route ("Macro 4", "env1_d", 0.4f);
          } },
        { "Tremolo Strings", "Strings/Brass", "Bowed-tremolo strings: a fast, nervous shimmer for builds and tension.",
          [] (B& b) {
              wavetable (b, 0, "String Ensemble", 0.5f, 0.6f);
              uni (b, 0, 0, 6, 0.16f, 0.9f);
              filter (b, 0, kLP12, 3400.0f, 0.05f, 0.4f);
              env (b, "env1", 0.15f, 1.0f, 1.0f, 0.8f);
              lfo (b, 0, LfoTriangle, 9.0f, false);
              b.set ("lfo1_retrig", 3.0f);
              b.set ("amp_mod_src", 8.0f).set ("amp_velo", 0.55f);
              hall (b, 0.35f, 0.75f, 0.65f);
              macros (b, "Brightness", "Space", "Speed", "Attack");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "lfo1_rate", 0.2f).route ("Macro 4", "env1_a", 0.35f);
          } },
        { "Silk Strings", "Strings/Brass", "High, airy strings with an octave shimmer. Floats above the band.",
          [] (B& b) {
              wavetable (b, 0, "String Ensemble", 0.65f, 0.55f);
              uni (b, 0, 0, 6, 0.14f, 1.0f);
              b.set ("e1_coarse", 12.0f);
              engineOn (b, 1, 0.25f);
              wavetable (b, 1, "Airy Pad", 0.6f, 0.25f);
              filter (b, 0, kLP12, 5500.0f, 0.0f, 0.3f);
              filter (b, 1, kHP12, 300.0f, 0.0f);
              b.set ("f_routing", 0.0f);
              env (b, "env1", 0.7f, 1.0f, 1.0f, 2.0f);
              lfo (b, 0, LfoSine, 5.0f);
              b.set ("lfo1_fade", 1.2f);
              b.route ("LFO 1", "e1_fine", 0.03f);
              shimmer (b, 0.35f, 0.4f, 0.75f);
              macros (b, "Brightness", "Space", "Air", "Attack");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "e2_vol", 0.4f).route ("Macro 4", "env1_a", 0.35f);
          } },
        // ================================================================ BELLS
        { "Tubular Bells", "Pluck", "Big orchestral chime with a long, slightly inharmonic ring.",
          [] (B& b) {
              fmVoice (b, 0, 3.5f, 0.3f, 0.8f);
              env (b, "env2", 0.001f, 3.0f, 0.0f, 2.0f);
              b.route ("Env 2", "e1_wt_pm", 0.35f, "Velocity", 0.8f);
              engineOn (b, 1, 0.25f);
              osc (b, 1, 0, kSine, 0.8f, -12.0f);
              filter (b, 0, kLP12, 9000.0f, 0.0f, 0.2f);
              env (b, "env1", 0.001f, 6.0f, 0.0f, 3.0f);
              b.set ("amp_velo", 0.6f);
              hall (b, 0.35f, 0.8f, 0.7f);
              macros (b, "Brightness", "Space", "Hum", "Decay");
              b.route ("Macro 1", "e1_wt_pm", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "e2_vol", 0.4f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "Music Box", "Pluck", "Tiny metal comb music box, an octave up, with a soft echo.",
          [] (B& b) {
              osc (b, 0, 0, kSine, 0.85f);
              osc (b, 0, 1, kSine, 0.22f, 24.0f, 0.06f);
              osc (b, 0, 2, kSine, 0.08f, 31.0f);
              b.set ("e1_coarse", 12.0f).set ("e1_vol", 0.75f);
              noise (b, 0, "Pluck Noise", 0.1f, 0.5f, false);
              filter (b, 0, kLP12, 9000.0f, 0.0f, 0.2f);
              env (b, "env1", 0.001f, 1.6f, 0.0f, 1.2f);
              b.set ("amp_velo", 0.5f);
              delay (b, 0, kDiv8D, 0.3f, 0.18f, true);
              hall (b, 0.3f, 0.6f, 0.55f);
              macros (b, "Brightness", "Space", "Echo", "Decay");
              b.route ("Macro 1", "e1_o2_vol", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "fx_a1_p8", 0.4f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "Glockenspiel", "Pluck", "Bright glockenspiel: a pure tone with quickly fading metallic overtones.",
          [] (B& b) {
              osc (b, 0, 0, kSine, 0.8f);
              osc (b, 0, 1, kSine, 0.05f, 18.0f, -0.42f); // 2.76 x (17.58 st)
              osc (b, 0, 2, kSine, 0.0f, 29.0f, 0.2f);
              b.set ("e1_coarse", 24.0f).set ("e1_vol", 0.75f);
              b.set ("e1_o3_kbd", 1.0f);
              env (b, "env3", 0.001f, 0.35f, 0.0f, 0.2f);
              b.route ("Env 3", "e1_o2_vol", 0.45f).route ("Env 3", "e1_o3_vol", 0.25f);
              noise (b, 0, "Click", 0.08f, 0.6f, false);
              filter (b, 0, kLP12, 14000.0f, 0.0f, 0.0f);
              env (b, "env1", 0.001f, 2.2f, 0.0f, 1.4f);
              b.set ("amp_velo", 0.6f);
              hall (b, 0.3f, 0.65f, 0.55f);
              macros (b, "Brightness", "Space", "Metal", "Decay");
              b.route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "e1_o2_vol", 0.3f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "Crystal Bells", "Pluck", "Glassy, sparkling bells into a shimmer tail. Ethereal and wide.",
          [] (B& b) {
              wavetable (b, 0, "Fibonacci Partials", 0.4f, 0.7f);
              uni (b, 0, 0, 3, 0.06f, 1.0f);
              filter (b, 0, kLP12, 8000.0f, 0.0f, 0.3f);
              env (b, "env2", 0.001f, 1.5f, 0.0f, 1.0f);
              b.route ("Env 2", "e1_wt_pos", 0.35f).route ("Velocity", "e1_wt_pos", 0.15f);
              env (b, "env1", 0.001f, 3.5f, 0.0f, 2.5f);
              b.set ("amp_velo", 0.55f);
              delay (b, 0, kDiv4D, 0.35f, 0.2f, true);
              shimmer (b, 0.4f, 0.5f, 0.8f);
              macros (b, "Brightness", "Space", "Echo", "Decay");
              b.route ("Macro 1", "e1_wt_pos", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "fx_a1_p8", 0.4f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "Celesta", "Pluck", "Soft celesta: a sweet bell-piano for gentle melodies.",
          [] (B& b) {
              fmVoice (b, 0, 4.0f, 0.1f, 0.8f);
              env (b, "env2", 0.001f, 0.5f, 0.0f, 0.3f);
              b.route ("Env 2", "e1_wt_pm", 0.25f, "Velocity", 1.0f);
              b.set ("e1_coarse", 12.0f);
              noise (b, 0, "Soft Click", 0.12f, 0.2f, false);
              filter (b, 0, kLP12, 8000.0f, 0.0f, 0.2f);
              env (b, "env1", 0.001f, 1.8f, 0.0f, 0.9f);
              b.set ("amp_velo", 0.6f);
              hall (b, 0.28f, 0.6f, 0.55f);
              macros (b, "Brightness", "Space", "Click", "Decay");
              b.route ("Macro 1", "e1_wt_pm", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "u_n1_vol", 0.3f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "Bells into Pad", "Pad", "A bell strike that blossoms into a warm pad while you hold the keys.",
          [] (B& b) {
              wavetable (b, 0, "FM Ratio 7", 0.45f, 0.65f);
              env (b, "env2", 0.001f, 2.0f, 0.15f, 1.5f);
              b.route ("Env 2", "e1_wt_pos", 0.3f);
              engineOn (b, 1, 0.0f);
              wavetable (b, 1, "Warm Pad", 0.35f, 0.0f);
              uni (b, 1, 0, 4, 0.22f, 0.9f);
              env (b, "env3", 2.0f, 1.0f, 1.0f, 3.0f);
              b.route ("Env 3", "e2_vol", 0.6f);
              filter (b, 0, kLP12, 5000.0f, 0.05f, 0.3f);
              env (b, "env1", 0.001f, 4.0f, 0.55f, 3.0f);
              hall (b, 0.38f, 0.8f, 0.7f);
              macros (b, "Brightness", "Space", "Pad", "Release");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "e2_vol", 0.3f).route ("Macro 4", "env1_r", 0.35f);
          } },
        // ================================================================ EVOLVING PADS
        { "Harmonic Drift", "Pad", "Two wavetables slowly scanning through their harmonics. Moves without ever getting busy.",
          [] (B& b) {
              wavetable (b, 0, "Harmonic Scan", 0.3f, 0.55f);
              uni (b, 0, 0, 3, 0.18f, 0.9f);
              wavetable (b, 1, "Vowels", 0.5f, 0.35f);
              b.set ("e2_fine", 0.07f);
              filter (b, 0, kLP12, 2400.0f, 0.1f, 0.25f);
              env (b, "env1", 1.6f, 1.0f, 1.0f, 3.5f);
              b.set ("func1_shape", 3.0f).set ("func1_rate", 0.05f).set ("func1_retrig", 3.0f);
              b.set ("rand1_type", 0.0f).set ("rand1_rate", 0.25f).set ("rand1_smooth", 1.0f).set ("rand1_retrig", 3.0f);
              b.route ("Function 1", "e1_wt_pos", 0.4f).route ("Random 1", "e2_wt_pos", 0.3f).route ("Random 1", "f1_cutoff", 0.06f);
              delay (b, 0, kDiv4D, 0.3f, 0.15f, true);
              hall (b, 0.4f, 0.85f, 0.75f);
              padMacros (b, 0.3f);
              b.route ("Macro 3", "func1_rate", 0.4f);
          } },
        { "Breathing Vowels", "Pad", "A soft vocal pad that opens and closes like slow breathing.",
          [] (B& b) {
              wavetable (b, 0, "Vowels", 0.2f, 0.6f);
              uni (b, 0, 0, 4, 0.16f, 0.9f);
              wavetable (b, 1, "Breathy Vox", 0.4f, 0.3f);
              noise (b, 0, "Air", 0.08f, 0.2f);
              filter (b, 0, kLP12, 3000.0f, 0.1f, 0.3f);
              env (b, "env1", 1.2f, 1.0f, 1.0f, 3.0f);
              lfo (b, 0, LfoTriangle, 0.07f);
              lfo (b, 1, LfoSine, 0.11f);
              b.set ("lfo1_retrig", 3.0f).set ("lfo2_retrig", 3.0f);
              b.route ("LFO 1", "e1_wt_pos", 0.3f).route ("LFO 2", "f1_cutoff", 0.07f).route ("LFO 2", "e2_wt_pos", 0.25f);
              shimmer (b, 0.35f, 0.35f, 0.75f);
              padMacros (b, 0.3f);
              b.route ("Macro 3", "lfo1_rate", 0.35f);
          } },
        { "Glass Tides", "Pad", "Glass and shimmer tables drifting against each other, with a slow phaser.",
          [] (B& b) {
              wavetable (b, 0, "Glass Pad", 0.3f, 0.5f);
              uni (b, 0, 0, 3, 0.15f, 1.0f);
              wavetable (b, 1, "Shimmer Evolve", 0.5f, 0.4f);
              b.set ("e2_coarse", 12.0f).set ("e2_fine", 0.05f);
              filter (b, 0, kLP12, 6000.0f, 0.05f, 0.2f);
              env (b, "env1", 2.0f, 1.0f, 1.0f, 4.0f);
              lfo (b, 0, LfoSine, 0.045f);
              lfo (b, 1, LfoTriangle, 0.07f);
              b.set ("lfo1_retrig", 3.0f).set ("lfo2_retrig", 3.0f);
              b.route ("LFO 1", "e1_wt_pos", 0.3f).route ("LFO 2", "e2_wt_pos", -0.3f);
              b.fx (0, FxPhaser, { { 3, 0.5f }, { 4, 0.08f }, { 7, 0.3f } });
              hall (b, 0.42f, 0.9f, 0.8f);
              padMacros (b, 0.25f);
              b.route ("Macro 3", "lfo1_rate", 0.4f);
          } },
        { "Slow Bloom", "Pad", "Starts as a dark hum and slowly opens into a bright, shimmering wash.",
          [] (B& b) {
              wavetable (b, 0, "Warm Saw Sweep", 0.0f, 0.55f);
              uni (b, 0, 0, 5, 0.22f, 1.0f);
              engineOn (b, 1, 0.3f);
              osc (b, 1, 0, kSine, 0.8f, -12.0f);
              filter (b, 0, kLP24, 500.0f, 0.15f, 0.3f);
              env (b, "env1", 3.0f, 1.0f, 1.0f, 4.0f);
              env (b, "env2", 6.0f, 2.0f, 0.85f, 4.0f);
              b.route ("Env 2", "f1_cutoff", 0.4f).route ("Env 2", "e1_wt_pos", 0.7f);
              shimmer (b, 0.45f, 0.5f, 0.85f);
              padMacros (b, 0.3f);
              b.route ("Macro 3", "env2_a", -0.3f);
          } },
        { "Spectral Motion", "Pad", "Harmonics stepping on a slow random sequence: alive and rhythmic, never the same.",
          [] (B& b) {
              wavetable (b, 0, "Odd Harmonics", 0.4f, 0.55f);
              uni (b, 0, 0, 3, 0.2f, 0.9f);
              wavetable (b, 1, "Digital Harmonics", 0.3f, 0.3f);
              b.set ("e2_coarse", -12.0f);
              filter (b, 0, kLP12, 4000.0f, 0.15f, 0.25f);
              env (b, "env1", 1.0f, 1.0f, 1.0f, 3.0f);
              b.set ("rand1_type", 3.0f).set ("rand1_rate", 0.6f).set ("rand1_smooth", 0.6f).set ("rand1_retrig", 3.0f);
              b.route ("Random 1", "e1_wt_pos", 0.35f).route ("Random 1", "e2_wt_pos", -0.25f);
              b.fx (0, FxPhaser, { { 3, 0.4f }, { 4, 0.1f }, { 7, 0.25f } });
              delay (b, 1, kDiv4D, 0.35f, 0.18f, true);
              hall (b, 0.4f, 0.85f, 0.75f);
              padMacros (b, 0.3f);
              b.route ("Macro 3", "rand1_rate", 0.4f);
          } },
        { "Aurora", "Pad", "Huge, wide supersaw wash with a slow rise-and-fall filter and a big shimmer.",
          [] (B& b) {
              osc (b, 0, 0, kSaw, 0.9f);
              uni (b, 0, 2, 7, 0.28f, 1.0f);
              b.set ("e1_uni_mix", 0.7f).set ("e1_vol", 0.55f);
              engineOn (b, 1, 0.3f);
              osc (b, 1, 0, kTri, 0.8f, 12.0f);
              filter (b, 0, kLP24, 1100.0f, 0.15f, 0.2f);
              env (b, "env1", 2.0f, 1.0f, 1.0f, 4.5f);
              b.set ("func1_shape", 8.0f).set ("func1_rate", 0.06f).set ("func1_retrig", 3.0f);
              b.route ("Function 1", "f1_cutoff", 0.25f);
              shimmer (b, 0.5f, 0.55f, 0.85f);
              padMacros (b, 0.3f);
              b.route ("Macro 3", "func1_rate", 0.4f);
          } },
        // ================================================================ POLYSYNTH PADS
        { "Poly Brass Pad", "Pad", "Big brassy analog polysynth pad with an opening filter swell. Bold under a chorus.",
          [] (B& b) {
              osc (b, 0, 0, kSaw, 0.8f);
              osc (b, 0, 1, kSaw, 0.7f, 0.0f, 0.1f);
              uni (b, 0, 0, 2, 0.08f, 0.8f);
              b.set ("e1_vol", 0.6f);
              model (b, 0, FilterSvfMorph, 0, 900.0f, 0.2f, 0.4f, 0.1f);
              b.set ("f1_a", 0.33f);
              env (b, "env2", 0.25f, 1.2f, 0.55f, 0.8f);
              b.route ("Env 2", "f1_cutoff", 0.35f).route ("Velocity", "f1_cutoff", 0.15f);
              env (b, "env1", 0.15f, 1.0f, 0.9f, 0.9f);
              chorus (b, 0, 0.25f);
              hall (b, 0.28f);
              macros (b, "Brightness", "Space", "Swell", "Attack");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "env2_a", 0.35f).route ("Macro 4", "env1_a", 0.35f);
          } },
        { "Vintage Poly Pad", "Pad", "Classic late-70s polysynth pad: saw and pulse, a smooth 4-pole filter, slow PWM.",
          [] (B& b) {
              osc (b, 0, 0, kSaw, 0.75f);
              osc (b, 0, 1, kSquare, 0.6f, 0.0f, 0.07f, 0.35f);
              b.set ("e1_vol", 0.6f).set ("e1_drift", 0.3f);
              lfo (b, 0, LfoTriangle, 0.3f);
              b.route ("LFO 1", "e1_o2_width", 0.12f);
              model (b, 0, FilterOta, 0, 1400.0f, 0.2f, 0.4f);
              env (b, "env2", 0.8f, 2.0f, 0.5f, 1.5f);
              b.route ("Env 2", "f1_cutoff", 0.25f);
              env (b, "env1", 0.5f, 1.0f, 1.0f, 1.8f);
              delay (b, 0, kDiv4D, 0.3f, 0.12f, true);
              hall (b, 0.3f);
              padMacros (b, 0.3f);
              b.route ("Macro 3", "lfo1_rate", 0.4f);
          } },
        { "Wide Saw Pad", "Pad", "Wide supersaw pad with a sub octave. Fills the room behind the band.",
          [] (B& b) {
              osc (b, 0, 0, kSaw, 0.9f);
              uni (b, 0, 2, 7, 0.22f, 1.0f);
              b.set ("e1_uni_mix", 0.65f).set ("e1_vol", 0.55f);
              engineOn (b, 1, 0.3f);
              osc (b, 1, 0, kSquare, 0.7f, -12.0f);
              model (b, 0, FilterMultimode, 0, 2200.0f, 0.1f, 0.3f);
              env (b, "env1", 0.6f, 1.0f, 1.0f, 2.0f);
              chorus (b, 0, 0.25f);
              hall (b, 0.32f);
              padMacros (b, 0.3f);
              b.route ("Macro 3", "e1_uni_detune", 0.3f);
          } },
        { "Analog Sweep Pad", "Pad", "Resonant ladder-filter pad with a slow sweep. The filter breathes across the chord.",
          [] (B& b) {
              osc (b, 0, 0, kSaw, 0.8f);
              osc (b, 0, 1, kSaw, 0.7f, 0.0f, -0.08f);
              uni (b, 0, 0, 2, 0.1f, 0.9f);
              b.set ("e1_vol", 0.6f);
              model (b, 0, FilterLadder, 0, 600.0f, 0.45f, 0.3f, 0.2f);
              env (b, "env1", 0.8f, 1.0f, 1.0f, 2.5f);
              lfo (b, 0, LfoSine, 0.08f);
              b.set ("lfo1_retrig", 3.0f);
              b.route ("LFO 1", "f1_cutoff", 0.22f);
              delay (b, 0, kDiv4D, 0.35f, 0.15f, true);
              hall (b, 0.32f);
              macros (b, "Brightness", "Space", "Sweep", "Resonance");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "lfo1_rate", 0.4f).route ("Macro 4", "f1_reso", 0.3f);
          } },
        { "Expressive Poly", "Pad", "Warm, vintage polysynth pad you play with your hands: velocity and aftertouch open it up.",
          [] (B& b) {
              osc (b, 0, 0, kSaw, 0.8f);
              osc (b, 0, 1, kSaw, 0.6f, 0.0f, 0.06f);
              b.set ("e1_vol", 0.6f).set ("e1_drift", 0.4f);
              filter (b, 0, kLP12, 1000.0f, 0.2f, 0.4f);
              filter (b, 1, kHP12, 120.0f, 0.0f);
              b.set ("f1_analog", 1.0f).set ("f_routing", 0.0f);
              b.route ("Velocity", "f1_cutoff", 0.25f).route ("Aftertouch", "f1_cutoff", 0.25f);
              lfo (b, 0, LfoSine, 5.5f);
              b.route ("LFO 1", "e1_fine", 0.05f, "Aftertouch", 1.0f);
              env (b, "env1", 0.3f, 1.0f, 1.0f, 1.5f);
              b.set ("amp_velo", 0.5f);
              b.fx (0, FxChorus, { { 1, 0.4f }, { 2, 0.15f }, { 4, 0.66f }, { 6, 1.0f }, { 7, 0.35f } });
              hall (b, 0.3f);
              padMacros (b, 0.3f);
          } },
        // ================================================================ CHORUS POLYSYNTH (classic 80s one-oscillator poly)
        { "Chorus Poly Pad", "Pad", "The classic 80s chorus polysynth pad: saw, PWM and sub through a deep stereo chorus.",
          [] (B& b) {
              chorusPoly (b, 0.7f, 0.5f, 0.35f, 1800.0f, 0.15f, 1, 0.5f);
              env (b, "env2", 0.4f, 1.5f, 0.6f, 1.5f);
              b.route ("Env 2", "f1_cutoff", 0.15f);
              env (b, "env1", 0.4f, 1.0f, 1.0f, 1.6f);
              hall (b, 0.28f);
              padMacros (b, 0.3f);
              b.route ("Macro 3", "lfo1_rate", 0.35f);
          } },
        { "Chorus Poly Strings", "Strings/Brass", "Chorus-polysynth strings: bright saws, a highpass and chorus I + II.",
          [] (B& b) {
              chorusPoly (b, 0.85f, 0.0f, 0.0f, 3600.0f, 0.05f, 2, 0.55f, 0.6f);
              b.set ("f2_cutoff", 240.0f);
              env (b, "env1", 0.3f, 1.0f, 1.0f, 1.3f);
              hall (b, 0.3f);
              padMacros (b, 0.3f);
          } },
        { "Chorus Poly Brass", "Strings/Brass", "Punchy chorus-polysynth brass stabs with a snappy filter envelope.",
          [] (B& b) {
              chorusPoly (b, 0.85f, 0.0f, 0.35f, 700.0f, 0.15f, 0, 0.4f);
              env (b, "env2", 0.04f, 0.5f, 0.45f, 0.3f);
              b.route ("Env 2", "f1_cutoff", 0.45f).route ("Velocity", "f1_cutoff", 0.15f);
              env (b, "env1", 0.02f, 1.0f, 0.9f, 0.35f);
              room (b, 0.22f);
              macros (b, "Brightness", "Space", "Bite", "Attack");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "env2_d", -0.3f).route ("Macro 4", "env2_a", 0.4f);
          } },
        { "Chorus Poly Keys", "Keys", "Plucky PWM keys with chorus II: the bright 80s ballad comp.",
          [] (B& b) {
              chorusPoly (b, 0.3f, 0.75f, 0.3f, 900.0f, 0.15f, 1, 0.5f);
              env (b, "env2", 0.001f, 0.6f, 0.2f, 0.4f);
              b.route ("Env 2", "f1_cutoff", 0.35f).route ("Velocity", "f1_cutoff", 0.15f);
              env (b, "env1", 0.002f, 1.6f, 0.3f, 0.5f);
              b.set ("amp_velo", 0.55f);
              delay (b, 1, kDiv8D, 0.3f, 0.15f, true);
              room (b, 0.22f);
              macros (b, "Brightness", "Space", "Echo", "Decay");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "fx_a2_p8", 0.4f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "Chorus Poly Bass", "Bass", "Round square-and-sub bass with a short filter pluck and chorus I. Classic 80s bass.",
          [] (B& b) {
              chorusPoly (b, 0.0f, 0.8f, 0.6f, 450.0f, 0.2f, 0, 0.3f, 0.3f);
              b.set ("f2_cutoff", 40.0f);
              b.set ("lfo1_rate", 0.2f);
              mono (b, true, 0.02f);
              env (b, "env2", 0.001f, 0.35f, 0.15f, 0.2f);
              b.route ("Env 2", "f1_cutoff", 0.3f).route ("Velocity", "f1_cutoff", 0.15f);
              env (b, "env1", 0.003f, 0.8f, 0.75f, 0.15f);
              macros (b, "Brightness", "Space", "Punch", "Release");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 3", "env2_d", 0.3f).route ("Macro 4", "env1_r", 0.4f);
          } },
        { "Chorus Poly Arp", "Sequence", "Bright PWM arpeggio over two octaves in 16ths, with chorus and a dotted echo. Hold a chord.",
          [] (B& b) {
              chorusPoly (b, 0.4f, 0.7f, 0.25f, 1200.0f, 0.2f, 1, 0.45f);
              env (b, "env2", 0.001f, 0.22f, 0.0f, 0.2f);
              b.route ("Env 2", "f1_cutoff", 0.35f);
              env (b, "env1", 0.002f, 0.35f, 0.0f, 0.25f);
              b.set ("seq_on", 1.0f).set ("seq_mode", 0.0f).set ("seq_arp_order", (float) ArpUp).set ("seq_arp_octaves", 2.0f).set ("seq_rate", (float) kDiv16);
              b.pattern (gatePattern ({ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, 0.5f));
              delay (b, 1, kDiv8D, 0.35f, 0.22f, true);
              hall (b, 0.25f);
              macros (b, "Brightness", "Space", "Echo", "Gate");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "fx_a2_p8", 0.4f).route ("Macro 4", "env1_d", 0.35f);
          } },
        // ================================================================ LADDER LEADS
        { "Ladder Lead", "Lead", "Classic three-oscillator mono lead through a ladder filter, with glide. Mod wheel = vibrato.",
          [] (B& b) {
              b.set ("f1_vol", 2.0f); // +6 dB after the filter (ladder and short gates lose level)
              osc (b, 0, 0, kSaw, 0.8f);
              osc (b, 0, 1, kSaw, 0.7f, 0.0f, 0.08f);
              osc (b, 0, 2, kSquare, 0.45f, -12.0f);
              b.set ("e1_vol", 0.9f);
              mono (b, true, 0.06f);
              model (b, 0, FilterLadder, 0, 1200.0f, 0.3f, 0.5f, 0.35f);
              env (b, "env2", 0.005f, 0.6f, 0.4f, 0.3f);
              b.route ("Env 2", "f1_cutoff", 0.3f).route ("Velocity", "f1_cutoff", 0.12f);
              env (b, "env1", 0.005f, 0.5f, 0.9f, 0.25f);
              lfo (b, 0, LfoSine, 5.5f);
              b.route ("LFO 1", "e1_fine", 0.06f, "Mod Wheel", 1.0f);
              delay (b, 0, kDiv8D, 0.3f, 0.15f);
              room (b, 0.18f);
              macros (b, "Brightness", "Space", "Resonance", "Glide");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "f1_reso", 0.35f);
          } },
        { "Fat Mono Lead", "Lead", "Thick, driven ladder lead with a sub and a touch of tube distortion. Cuts through anything.",
          [] (B& b) {
              b.set ("f1_vol", 2.0f); // +6 dB after the filter (ladder and short gates lose level)
              osc (b, 0, 0, kSaw, 0.8f);
              osc (b, 0, 1, kSquare, 0.65f, 0.0f, -0.06f);
              osc (b, 0, 2, kSaw, 0.5f, -12.0f);
              b.set ("e1_vol", 0.9f).set ("e1_drift", 0.2f);
              mono (b, true, 0.08f);
              model (b, 0, FilterLadder, 0, 1500.0f, 0.25f, 0.5f, 0.6f);
              env (b, "env2", 0.003f, 0.4f, 0.5f, 0.3f);
              b.route ("Env 2", "f1_cutoff", 0.25f);
              env (b, "env1", 0.004f, 0.5f, 0.9f, 0.2f);
              lfo (b, 0, LfoSine, 5.2f);
              b.route ("LFO 1", "e1_fine", 0.06f, "Mod Wheel", 1.0f);
              b.fx (0, FxDistortion, { { 0, (2 + 0.5f) / 16.0f }, { 1, 0.3f }, { 10, 0.5f } });
              delay (b, 1, kDiv4, 0.3f, 0.12f);
              room (b, 0.15f);
              macros (b, "Brightness", "Space", "Drive", "Glide");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "fx_a1_p2", 0.4f);
          } },
        { "Soft Ladder Lead", "Lead", "Mellow square-and-triangle lead, almost flute-like, with a delayed vibrato.",
          [] (B& b) {
              b.set ("f1_vol", 2.0f); // +6 dB after the filter (ladder and short gates lose level)
              osc (b, 0, 0, kSquare, 0.7f);
              osc (b, 0, 1, kTri, 0.5f, 12.0f);
              b.set ("e1_vol", 0.9f);
              mono (b, true, 0.08f);
              model (b, 0, FilterLadder, 0, 900.0f, 0.15f, 0.5f, 0.1f);
              env (b, "env2", 0.04f, 0.6f, 0.6f, 0.3f);
              b.route ("Env 2", "f1_cutoff", 0.15f);
              env (b, "env1", 0.03f, 0.5f, 1.0f, 0.3f);
              lfo (b, 0, LfoSine, 5.0f);
              b.set ("lfo1_fade", 0.7f);
              b.route ("LFO 1", "e1_fine", 0.05f);
              delay (b, 0, kDiv4, 0.3f, 0.15f);
              hall (b, 0.25f);
              macros (b, "Brightness", "Space", "Vibrato", "Glide");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "lfo1_rate", 0.2f);
          } },
        { "Resonant Sweep Lead", "Lead", "Squelchy, vocal ladder lead: a high-resonance filter sweeps every note.",
          [] (B& b) {
              b.set ("f1_vol", 2.0f); // +6 dB after the filter (ladder and short gates lose level)
              osc (b, 0, 0, kSaw, 0.85f);
              osc (b, 0, 1, kSaw, 0.6f, -12.0f, 0.05f);
              b.set ("e1_vol", 0.85f);
              mono (b, false, 0.05f);
              model (b, 0, FilterLadder, 0, 500.0f, 0.62f, 0.4f, 0.3f);
              env (b, "env2", 0.02f, 0.7f, 0.25f, 0.4f);
              b.route ("Env 2", "f1_cutoff", 0.45f).route ("Velocity", "f1_cutoff", 0.15f);
              env (b, "env1", 0.005f, 0.6f, 0.85f, 0.3f);
              delay (b, 0, kDiv8D, 0.35f, 0.18f, true);
              room (b, 0.18f);
              macros (b, "Brightness", "Space", "Sweep", "Resonance");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "env2_d", 0.35f).route ("Macro 4", "f1_reso", 0.3f);
          } },
        { "Fifth Ladder Lead", "Lead", "Saw lead with a fifth on top and a long portamento: big and anthemic.",
          [] (B& b) {
              b.set ("f1_vol", 2.0f); // +6 dB after the filter (ladder and short gates lose level)
              osc (b, 0, 0, kSaw, 0.8f);
              osc (b, 0, 1, kSaw, 0.55f, 7.0f, 0.03f);
              osc (b, 0, 2, kSquare, 0.4f, -12.0f);
              b.set ("e1_vol", 0.85f);
              mono (b, true, 0.15f);
              model (b, 0, FilterLadder, 0, 1800.0f, 0.25f, 0.5f, 0.3f);
              env (b, "env2", 0.01f, 0.8f, 0.5f, 0.3f);
              b.route ("Env 2", "f1_cutoff", 0.2f);
              env (b, "env1", 0.01f, 0.5f, 0.9f, 0.35f);
              lfo (b, 0, LfoSine, 5.5f);
              b.route ("LFO 1", "e1_fine", 0.06f, "Mod Wheel", 1.0f);
              delay (b, 0, kDiv4D, 0.35f, 0.2f, true);
              hall (b, 0.25f);
              macros (b, "Brightness", "Space", "Fifth", "Glide");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "e1_o2_vol", -0.4f);
          } },
        // ================================================================ PULSING BASS SEQUENCES
        { "Pulse Bass 8ths", "Bass", "Driving eighth-note ladder bass. Hold one note; it follows the tempo.",
          [] (B& b) {
              b.set ("f1_vol", 2.0f); // +6 dB after the filter (ladder and short gates lose level)
              osc (b, 0, 0, kSaw, 0.8f);
              osc (b, 0, 1, kSquare, 0.6f, -12.0f);
              b.set ("e1_vol", 0.9f);
              mono (b, false, 0.0f);
              model (b, 0, FilterLadder, 0, 380.0f, 0.3f, 0.3f, 0.3f);
              env (b, "env2", 0.001f, 0.22f, 0.1f, 0.15f);
              b.route ("Env 2", "f1_cutoff", 0.3f).route ("Velocity", "f1_cutoff", 0.15f);
              env (b, "env1", 0.002f, 0.4f, 0.6f, 0.08f);
              bassSeq (b, kDiv8, { 1, 1, 1, 1, 1, 1, 1, 1 }, 0.6f);
              macros (b, "Brightness", "Space", "Punch", "Gate");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 3", "env2_d", 0.3f).route ("Macro 4", "env1_s", 0.3f);
          } },
        { "Octave Pulse Bass", "Bass", "Sixteenth-note bass that bounces between the note and its octave. Instant energy.",
          [] (B& b) {
              b.set ("f1_vol", 2.0f); // +6 dB after the filter (ladder and short gates lose level)
              osc (b, 0, 0, kSaw, 0.8f);
              osc (b, 0, 1, kSquare, 0.5f, 0.0f, 0.04f);
              b.set ("e1_vol", 0.85f);
              mono (b, false, 0.0f);
              model (b, 0, FilterLadder, 0, 450.0f, 0.25f, 0.35f, 0.25f);
              env (b, "env2", 0.001f, 0.15f, 0.05f, 0.1f);
              b.route ("Env 2", "f1_cutoff", 0.3f);
              env (b, "env1", 0.002f, 0.25f, 0.4f, 0.06f);
              b.set ("seq_on", 1.0f).set ("seq_mode", 1.0f).set ("seq_rate", (float) kDiv16).set ("seq_vel_blend", 0.6f);
              auto p = gatePattern ({ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, 0.5f);
              for (int i = 1; i < kSeqSteps; i += 2)
                  p.steps[(size_t) i].octave = 1;
              b.pattern (p);
              room (b, 0.1f);
              macros (b, "Brightness", "Space", "Punch", "Gate");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "env2_d", 0.3f).route ("Macro 4", "env1_s", 0.3f);
          } },
        { "Rolling Bass 16ths", "Bass", "Syncopated 3-3-2 sixteenth bass with accents. Great under a four-on-the-floor build.",
          [] (B& b) {
              b.set ("f1_vol", 2.0f); // +6 dB after the filter (ladder and short gates lose level)
              osc (b, 0, 0, kSaw, 0.8f);
              osc (b, 0, 1, kSine, 0.6f, -12.0f);
              b.set ("e1_vol", 0.9f);
              mono (b, false, 0.0f);
              model (b, 0, FilterLadder, 0, 420.0f, 0.35f, 0.3f, 0.25f);
              env (b, "env2", 0.001f, 0.2f, 0.05f, 0.1f);
              b.route ("Env 2", "f1_cutoff", 0.35f).route ("Velocity", "f1_cutoff", 0.2f);
              env (b, "env1", 0.002f, 0.3f, 0.5f, 0.06f);
              b.set ("seq_on", 1.0f).set ("seq_mode", 1.0f).set ("seq_rate", (float) kDiv16).set ("seq_vel_blend", 0.8f);
              auto p = gatePattern ({ 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 1 }, 0.55f);
              for (int i : { 0, 3, 6, 8, 11 })
                  p.steps[(size_t) i].velocity = 1.0f;
              for (int i : { 2, 5, 10, 13, 15 })
                  p.steps[(size_t) i].velocity = 0.55f;
              p.steps[15].octave = 1;
              b.pattern (p);
              macros (b, "Brightness", "Space", "Punch", "Gate");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 3", "env2_d", 0.3f).route ("Macro 4", "env1_s", 0.3f);
          } },
        { "Gated Sub Pulse", "Bass", "Deep sub in pumping eighths: felt in the chest, keeps the room moving under pads.",
          [] (B& b) {
              osc (b, 0, 0, kSine, 0.9f);
              osc (b, 0, 1, kTri, 0.25f, 12.0f);
              b.set ("e1_vol", 1.0f);
              mono (b, false, 0.0f);
              filter (b, 0, kLP12, 700.0f, 0.0f, 0.0f);
              env (b, "env1", 0.01f, 0.35f, 0.5f, 0.1f);
              bassSeq (b, kDiv8, { 1, 1, 1, 1, 1, 1, 1, 1 }, 0.8f);
              macros (b, "Brightness", "Space", "Overtone", "Gate");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 3", "e1_o2_vol", 0.4f).route ("Macro 4", "env1_s", 0.3f);
          } },
        { "Dark Pulse Bass", "Bass", "Sixteenth-note bass with a slow filter wave that rises and falls over several bars. For builds.",
          [] (B& b) {
              b.set ("f1_vol", 2.0f); // +6 dB after the filter (ladder and short gates lose level)
              osc (b, 0, 0, kSaw, 0.8f);
              osc (b, 0, 1, kSaw, 0.6f, -12.0f, 0.05f);
              b.set ("e1_vol", 0.85f);
              mono (b, false, 0.0f);
              model (b, 0, FilterOta, 0, 300.0f, 0.35f, 0.3f, 0.3f);
              env (b, "env2", 0.001f, 0.16f, 0.0f, 0.1f);
              b.route ("Env 2", "f1_cutoff", 0.25f);
              env (b, "env1", 0.002f, 0.22f, 0.35f, 0.06f);
              lfo (b, 0, LfoSine, 0.08f, false);
              b.set ("lfo1_retrig", 3.0f);
              b.route ("LFO 1", "f1_cutoff", 0.25f);
              bassSeq (b, kDiv16, { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, 0.5f);
              delay (b, 0, kDiv8D, 0.25f, 0.12f, true);
              room (b, 0.12f);
              macros (b, "Brightness", "Space", "Sweep", "Gate");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "lfo1_rate", 0.3f).route ("Macro 4", "env1_s", 0.3f);
          } },
        { "Synthwave Bass", "Bass", "Bright, punchy sixteenth bass with chorus, the retro-80s drive.",
          [] (B& b) {
              b.set ("f1_vol", 2.0f); // +6 dB after the filter (ladder and short gates lose level)
              osc (b, 0, 0, kSaw, 0.8f);
              osc (b, 0, 1, kSaw, 0.6f, 0.0f, 0.1f);
              osc (b, 0, 2, kSquare, 0.5f, -12.0f);
              b.set ("e1_vol", 0.85f);
              mono (b, false, 0.0f);
              model (b, 0, FilterOta, 0, 600.0f, 0.2f, 0.35f, 0.2f);
              env (b, "env2", 0.001f, 0.18f, 0.1f, 0.1f);
              b.route ("Env 2", "f1_cutoff", 0.35f);
              env (b, "env1", 0.002f, 0.25f, 0.5f, 0.06f);
              bassSeq (b, kDiv16, { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, 0.45f);
              vintageChorus (b, 0, 0, 0.3f);
              macros (b, "Brightness", "Space", "Punch", "Gate");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 3", "env2_d", 0.3f).route ("Macro 4", "env1_s", 0.3f);
          } },

        // ================================================================ ORGANS (rotary: mod wheel or Macro 3 = fast)
        { "Gospel Drawbars", "Keys", "Tonewheel organ, first three drawbars out, with third percussion and a rotary. Mod wheel = fast.",
          [] (B& b) {
              drawbars (b, { 8, 8, 8, 0, 0, 0, 0, 0, 0 }, 3);
              filter (b, 0, kLP12, 9000.0f, 0.0f, 0.0f, 0.12f);
              rotary (b, 0.25f);
              room (b, 0.15f);
              organMacros (b);
          } },
        { "Full Drawbars", "Keys", "All the drawbars out and an overdriven rotary: the big rock and gospel organ.",
          [] (B& b) {
              drawbars (b, { 8, 8, 8, 8, 8, 8, 0, 8, 8 });
              filter (b, 0, kLP12, 8000.0f, 0.0f, 0.0f, 0.3f);
              rotary (b, 0.55f);
              room (b, 0.15f);
              organMacros (b);
          } },
        { "Jazz Organ", "Keys", "Soft 8' and 4' with second percussion and a slow rotary. Warm and round.",
          [] (B& b) {
              drawbars (b, { 0, 0, 8, 8, 0, 0, 0, 0, 0 }, 2);
              filter (b, 0, kLP12, 7000.0f, 0.0f, 0.0f);
              rotary (b, 0.15f);
              room (b, 0.15f);
              organMacros (b);
          } },
        { "Chapel Organ", "Keys", "Gentle flute-like drawbars (8' with a touch of 4' and 2') for quiet worship.",
          [] (B& b) {
              drawbars (b, { 0, 0, 8, 4, 0, 3, 0, 0, 0 });
              filter (b, 0, kLP12, 6000.0f, 0.0f, 0.0f);
              b.set ("env1_a", 0.03f).set ("env1_r", 0.25f);
              rotary (b, 0.05f, 0.7f);
              hall (b, 0.25f, 0.7f, 0.6f);
              organMacros (b);
          } },
        { "Organ Swell Pad", "Pad", "Drawbar organ that swells in slowly under a chord, with a slow rotary and a hall.",
          [] (B& b) {
              drawbars (b, { 6, 0, 8, 6, 0, 4, 0, 0, 0 });
              b.set ("u_n1_vol", 0.0f);
              filter (b, 0, kLP12, 5000.0f, 0.0f, 0.0f);
              env (b, "env1", 0.9f, 0.5f, 1.0f, 1.2f);
              rotary (b, 0.1f, 0.8f);
              hall (b, 0.35f, 0.8f, 0.7f);
              organMacros (b);
          } },
        { "Full Pipe Organ", "Keys", "Church pipe organ with principals, an octave and a mixture rank, in a stone hall.",
          [] (B& b) {
              wavetable (b, 0, "Pipe Organ", 0.55f, 0.55f);
              wavetable (b, 1, "Pipe Organ", 0.35f, 0.3f);
              b.set ("e2_coarse", 12.0f);
              b.set ("u_osc_on", 1.0f).set ("u_osc_wave", 0.0f).set ("u_osc_coarse", 19.0f).set ("u_osc_vol", 0.08f);
              engineOn (b, 0, 0.55f);
              filter (b, 0, kLP12, 7000.0f, 0.0f, 0.3f);
              env (b, "env1", 0.04f, 0.5f, 1.0f, 0.7f);
              b.set ("amp_velo", 0.0f);
              b.fx (6, FxReverb, { { 0, 0.25f }, { 1, 0.95f }, { 2, 0.88f }, { 3, 0.4f }, { 5, 0.7f } });
              b.set ("fx_aux_send", 0.42f);
              macros (b, "Brightness", "Room", "Mixture", "Release");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "u_osc_vol", 0.2f).route ("Macro 4", "env1_r", 0.5f);
          } },
        { "Soft Flute Stop", "Keys", "A single soft flute rank, breathy and gentle, in a big room.",
          [] (B& b) {
              wavetable (b, 0, "Flute Pipe", 0.35f, 0.6f);
              noise (b, 0, "Air", 0.04f, 0.3f);
              filter (b, 0, kLP12, 4500.0f, 0.0f, 0.3f);
              env (b, "env1", 0.06f, 0.5f, 1.0f, 0.6f);
              b.set ("amp_velo", 0.0f);
              lfo (b, 0, LfoSine, 5.0f);
              b.route ("LFO 1", "e1_fine", 0.01f);
              b.fx (6, FxReverb, { { 0, 0.2f }, { 1, 0.9f }, { 2, 0.8f }, { 3, 0.45f }, { 5, 0.65f } });
              b.set ("fx_aux_send", 0.4f);
              macros (b, "Brightness", "Room", "Breath", "Release");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "u_n1_vol", 0.2f).route ("Macro 4", "env1_r", 0.5f);
          } },
        // ================================================================ MORE STRINGS
        { "Expressive Strings", "Strings/Brass", "Section strings you play with the mod wheel: it swells the volume and brightness, like a bow.",
          [] (B& b) {
              wavetable (b, 0, "String Ensemble", 0.45f, 0.45f);
              uni (b, 0, 0, 6, 0.16f, 0.9f);
              engineOn (b, 1, 0.25f);
              osc (b, 1, 0, kSaw, 0.8f);
              uni (b, 1, 0, 3, 0.25f, 0.8f);
              filter (b, 0, kLP12, 1200.0f, 0.05f, 0.4f);
              b.route ("Mod Wheel", "f1_cutoff", 0.3f).route ("Mod Wheel", "e1_vol", 0.35f).route ("Mod Wheel", "e2_vol", 0.15f);
              env (b, "env1", 0.25f, 1.0f, 1.0f, 0.9f);
              lfo (b, 0, LfoSine, 5.2f);
              b.set ("lfo1_fade", 0.8f);
              b.route ("LFO 1", "e1_fine", 0.03f);
              hall (b, 0.35f, 0.75f, 0.65f);
              macros (b, "Brightness", "Space", "Vibrato", "Attack");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "lfo1_rate", 0.15f).route ("Macro 4", "env1_a", 0.35f);
          } },
        { "Octave Strings", "Strings/Brass", "Strings doubled an octave up: big film-score lines from one finger.",
          [] (B& b) {
              wavetable (b, 0, "String Ensemble", 0.5f, 0.5f);
              uni (b, 0, 0, 6, 0.15f, 0.9f);
              engineOn (b, 1, 0.35f);
              wavetable (b, 1, "String Ensemble", 0.6f, 0.35f);
              b.set ("e2_coarse", 12.0f);
              uni (b, 1, 0, 4, 0.14f, 1.0f);
              filter (b, 0, kLP12, 2600.0f, 0.05f, 0.4f);
              filter (b, 1, kHP12, 150.0f, 0.0f);
              b.set ("f_routing", 0.0f);
              env (b, "env1", 0.2f, 1.0f, 1.0f, 1.0f);
              b.route ("Velocity", "f1_cutoff", 0.2f);
              lfo (b, 0, LfoSine, 5.2f);
              b.route ("LFO 1", "e1_fine", 0.04f, "Mod Wheel", 1.0f).route ("LFO 1", "e2_fine", 0.04f, "Mod Wheel", 1.0f);
              hall (b, 0.38f, 0.8f, 0.7f);
              macros (b, "Brightness", "Space", "Octave", "Attack");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "e2_vol", -0.35f).route ("Macro 4", "env1_a", 0.35f);
          } },
        { "Low Strings", "Strings/Brass", "Dark cellos and basses: a warm, slow-bowed low end for builds and endings.",
          [] (B& b) {
              wavetable (b, 0, "String Ensemble", 0.3f, 0.55f);
              uni (b, 0, 0, 5, 0.14f, 0.7f);
              engineOn (b, 1, 0.35f);
              osc (b, 1, 0, kSaw, 0.8f, -12.0f);
              uni (b, 1, 0, 3, 0.2f, 0.5f);
              filter (b, 0, kLP24, 900.0f, 0.1f, 0.4f);
              env (b, "env2", 0.6f, 1.5f, 0.6f, 1.0f);
              b.route ("Env 2", "f1_cutoff", 0.2f).route ("Velocity", "f1_cutoff", 0.15f);
              env (b, "env1", 0.45f, 1.0f, 1.0f, 1.3f);
              lfo (b, 0, LfoSine, 4.8f);
              b.route ("LFO 1", "e1_fine", 0.04f, "Mod Wheel", 1.0f);
              hall (b, 0.32f, 0.8f, 0.7f);
              macros (b, "Brightness", "Space", "Vibrato", "Attack");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "lfo1_rate", 0.15f).route ("Macro 4", "env1_a", 0.35f);
          } },
        { "Spiccato Strings", "Strings/Brass", "Short, bouncing bow strokes for rhythmic string parts.",
          [] (B& b) {
              wavetable (b, 0, "String Ensemble", 0.5f, 0.65f);
              uni (b, 0, 0, 5, 0.12f, 0.9f);
              noise (b, 0, "Pluck Noise", 0.06f, 0.2f, false);
              filter (b, 0, kLP12, 2200.0f, 0.05f, 0.5f);
              env (b, "env2", 0.005f, 0.15f, 0.2f, 0.1f);
              b.route ("Env 2", "f1_cutoff", 0.2f).route ("Velocity", "f1_cutoff", 0.25f);
              env (b, "env1", 0.008f, 0.22f, 0.15f, 0.2f);
              b.set ("amp_velo", 0.7f);
              hall (b, 0.3f, 0.6f, 0.55f);
              macros (b, "Brightness", "Space", "Bite", "Length");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "u_n1_vol", 0.3f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "Strings and Choir", "Strings/Brass", "Strings with a soft choir 'ah' blended in. Instant big moment.",
          [] (B& b) {
              wavetable (b, 0, "String Ensemble", 0.5f, 0.45f);
              uni (b, 0, 0, 6, 0.16f, 0.9f);
              engineOn (b, 1, 0.35f);
              wavetable (b, 1, "Choir Ah to Oo", 0.2f, 0.35f);
              uni (b, 1, 0, 3, 0.12f, 1.0f);
              filter (b, 0, kLP12, 3000.0f, 0.05f, 0.35f);
              env (b, "env1", 0.5f, 1.0f, 1.0f, 1.6f);
              b.route ("Mod Wheel", "e2_wt_pos", 0.5f);
              hall (b, 0.4f, 0.85f, 0.75f);
              macros (b, "Brightness", "Space", "Choir", "Attack");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "e2_vol", 0.35f).route ("Macro 4", "env1_a", 0.35f);
          } },
        { "Poly Strings", "Strings/Brass", "Analog polysynth strings: soft saws, slow filter and a wide chorus.",
          [] (B& b) {
              twoOsc (b, 0.8f, 0.0f, 0.5f, 0.0f, 0.35f);
              uni (b, 0, 0, 3, 0.14f, 0.9f);
              model (b, 0, FilterOta, 1, 3000.0f, 0.1f, 0.4f);
              filter (b, 1, kHP12, 200.0f, 0.0f);
              b.set ("f_routing", 0.0f);
              env (b, "env1", 0.4f, 1.0f, 1.0f, 1.3f);
              chorus (b, 0, 0.35f);
              hall (b, 0.32f);
              padMacros (b, 0.3f);
          } },
        // ================================================================ GRAND POLY (8-voice flagship 80s polysynth)
        { "Grand Poly Pad", "Pad", "Huge flagship-polysynth pad: two detuned oscillators per voice, a creamy 4-pole filter.",
          [] (B& b) {
              osc (b, 0, 0, kSaw, 0.75f);
              osc (b, 0, 1, kSaw, 0.65f, 0.0f, 0.09f);
              osc (b, 0, 2, kSquare, 0.3f, -12.0f);
              uni (b, 0, 0, 2, 0.08f, 0.9f);
              b.set ("e1_vol", 0.55f).set ("e1_drift", 0.35f);
              model (b, 0, FilterOta, 0, 1500.0f, 0.15f, 0.4f, 0.1f);
              filter (b, 1, kHP12, 90.0f, 0.0f);
              b.set ("f_routing", 0.0f);
              env (b, "env2", 0.7f, 2.0f, 0.6f, 1.5f);
              b.route ("Env 2", "f1_cutoff", 0.2f);
              env (b, "env1", 0.5f, 1.0f, 1.0f, 1.8f);
              lfo (b, 0, LfoTriangle, 0.2f);
              b.route ("LFO 1", "f1_cutoff", 0.04f);
              chorus (b, 0, 0.2f);
              hall (b, 0.32f);
              padMacros (b, 0.3f);
          } },
        { "Grand Poly Brass", "Strings/Brass", "Bold, bright polysynth brass with a punchy filter swell. For big stabs.",
          [] (B& b) {
              osc (b, 0, 0, kSaw, 0.8f);
              osc (b, 0, 1, kSaw, 0.7f, 0.0f, -0.07f);
              b.set ("e1_vol", 0.6f).set ("e1_drift", 0.25f);
              model (b, 0, FilterOta, 0, 600.0f, 0.2f, 0.45f, 0.15f);
              env (b, "env2", 0.06f, 0.6f, 0.5f, 0.35f);
              b.route ("Env 2", "f1_cutoff", 0.45f).route ("Velocity", "f1_cutoff", 0.15f);
              env (b, "env1", 0.03f, 1.0f, 0.9f, 0.35f);
              room (b, 0.2f);
              macros (b, "Brightness", "Space", "Bite", "Attack");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "env2_d", -0.3f).route ("Macro 4", "env2_a", 0.4f);
          } },
        { "Grand Poly Sweep", "Pad", "Polysynth pad with a resonant filter that slowly sweeps open and closed.",
          [] (B& b) {
              osc (b, 0, 0, kSaw, 0.75f);
              osc (b, 0, 1, kSquare, 0.6f, 0.0f, 0.08f, 0.4f);
              b.set ("e1_vol", 0.55f).set ("e1_drift", 0.3f);
              model (b, 0, FilterOta, 0, 700.0f, 0.5f, 0.4f, 0.15f);
              lfo (b, 0, LfoTriangle, 0.07f);
              b.set ("lfo1_retrig", 3.0f);
              b.route ("LFO 1", "f1_cutoff", 0.2f);
              env (b, "env1", 0.6f, 1.0f, 1.0f, 2.0f);
              delay (b, 0, kDiv4D, 0.35f, 0.15f, true);
              hall (b, 0.32f);
              padMacros (b, 0.3f);
              b.route ("Macro 3", "lfo1_rate", 0.4f);
          } },
        { "Grand Poly Arp", "Sequence", "Up-and-down polysynth arpeggio in 16ths with an echo. Hold a chord.",
          [] (B& b) {
              osc (b, 0, 0, kSaw, 0.7f);
              osc (b, 0, 1, kSquare, 0.5f, 12.0f, 0.04f, 0.4f);
              b.set ("e1_vol", 0.6f).set ("f1_vol", 1.7f);
              model (b, 0, FilterOta, 0, 1100.0f, 0.3f, 0.4f, 0.15f);
              env (b, "env2", 0.001f, 0.25f, 0.1f, 0.2f);
              b.route ("Env 2", "f1_cutoff", 0.35f);
              env (b, "env1", 0.002f, 0.4f, 0.0f, 0.3f);
              b.set ("seq_on", 1.0f).set ("seq_mode", 0.0f).set ("seq_arp_order", (float) ArpUpDownExcl).set ("seq_arp_octaves", 2.0f).set ("seq_rate", (float) kDiv16);
              b.pattern (gatePattern ({ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, 0.5f));
              delay (b, 1, kDiv8D, 0.35f, 0.2f, true);
              hall (b, 0.22f);
              macros (b, "Brightness", "Space", "Echo", "Gate");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "fx_a2_p8", 0.4f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "Grand Poly Bells", "Pluck", "Metallic cross-modulated polysynth bells with a long tail.",
          [] (B& b) {
              osc (b, 0, 0, kSine, 0.8f);
              osc (b, 0, 1, kSine, 0.25f, 12.0f, 0.03f);
              osc (b, 0, 2, kSine, 0.0f, 19.0f, 0.15f);
              b.set ("e1_mod_src", 0.0f).set ("e1_o1_fm", 0.15f).set ("e1_vol", 0.7f);
              env (b, "env2", 0.001f, 1.6f, 0.0f, 1.0f);
              b.route ("Env 2", "e1_o1_fm", 0.3f, "Velocity", 1.0f);
              model (b, 0, FilterOta, 1, 8000.0f, 0.0f, 0.3f);
              env (b, "env1", 0.001f, 3.0f, 0.0f, 2.0f);
              b.set ("amp_velo", 0.55f);
              chorus (b, 0, 0.25f);
              hall (b, 0.35f, 0.75f, 0.65f);
              macros (b, "Metal", "Space", "Chorus", "Decay");
              b.route ("Macro 1", "e1_o1_fm", 0.25f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "fx_a1_p8", 0.4f).route ("Macro 4", "env1_d", 0.35f);
          } },
        // ================================================================ MORE CHORUS POLY
        { "Chorus Poly Organ", "Keys", "The chorus polysynth as an organ: square, sub and a straight envelope through chorus II.",
          [] (B& b) {
              chorusPoly (b, 0.0f, 0.75f, 0.5f, 2600.0f, 0.0f, 1, 0.5f);
              b.set ("lfo1_rate", 0.0f);
              env (b, "env1", 0.004f, 0.2f, 1.0f, 0.1f);
              b.set ("amp_velo", 0.0f);
              room (b, 0.18f);
              macros (b, "Brightness", "Space", "Sub", "Release");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "e1_o3_vol", 0.3f).route ("Macro 4", "env1_r", 0.4f);
          } },
        { "Chorus Poly Sweep", "Pad", "Chorus-polysynth pad with a slow resonant filter sweep and chorus I + II.",
          [] (B& b) {
              chorusPoly (b, 0.75f, 0.45f, 0.3f, 900.0f, 0.4f, 2, 0.5f);
              lfo (b, 1, LfoTriangle, 0.06f);
              b.set ("lfo2_retrig", 3.0f);
              b.route ("LFO 2", "f1_cutoff", 0.22f);
              env (b, "env1", 0.7f, 1.0f, 1.0f, 2.0f);
              hall (b, 0.3f);
              padMacros (b, 0.3f);
              b.route ("Macro 3", "lfo2_rate", 0.4f);
          } },
        { "Chorus Poly Soft Pad", "Pad", "Gentle, dark chorus-polysynth pad on chorus I: sits quietly under worship.",
          [] (B& b) {
              chorusPoly (b, 0.55f, 0.5f, 0.3f, 900.0f, 0.1f, 0, 0.55f);
              env (b, "env1", 0.8f, 1.0f, 1.0f, 2.2f);
              hall (b, 0.32f, 0.75f, 0.65f);
              padMacros (b, 0.3f);
              b.route ("Macro 3", "lfo1_rate", 0.35f);
          } },
        // ================================================================ VINTAGE POLY (late-70s five-voice classic)
        { "Vintage Poly Brass", "Strings/Brass", "Warm, slightly unstable late-70s polysynth brass with a slow filter swell.",
          [] (B& b) {
              twoOsc (b, 0.8f, 0.6f, 0.4f, 0.08f, 0.45f);
              model (b, 0, FilterOta, 0, 700.0f, 0.15f, 0.4f, 0.1f);
              env (b, "env2", 0.15f, 0.8f, 0.5f, 0.4f);
              b.route ("Env 2", "f1_cutoff", 0.38f).route ("Velocity", "f1_cutoff", 0.12f);
              env (b, "env1", 0.08f, 1.0f, 0.9f, 0.45f);
              room (b, 0.22f);
              macros (b, "Brightness", "Space", "Swell", "Attack");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "env2_a", 0.35f).route ("Macro 4", "env1_a", 0.35f);
          } },
        { "Vintage Poly Keys", "Keys", "Plucky polysynth keys, saw and narrow pulse: the classic singer-songwriter comp.",
          [] (B& b) {
              twoOsc (b, 0.7f, 0.5f, 0.3f, 0.06f, 0.35f);
              model (b, 0, FilterOta, 0, 800.0f, 0.15f, 0.5f);
              env (b, "env2", 0.001f, 0.7f, 0.2f, 0.4f);
              b.route ("Env 2", "f1_cutoff", 0.35f).route ("Velocity", "f1_cutoff", 0.15f);
              env (b, "env1", 0.002f, 1.8f, 0.25f, 0.5f);
              b.set ("amp_velo", 0.55f);
              chorus (b, 0, 0.25f);
              room (b, 0.2f);
              macros (b, "Brightness", "Space", "Chorus", "Decay");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "fx_a1_p8", 0.4f).route ("Macro 4", "env1_d", 0.35f);
          } },
        { "Vintage Poly Sync", "Lead", "Hard-sync polysynth sweep: the second oscillator's pitch follows the envelope. Mod wheel adds more.",
          [] (B& b) {
              osc (b, 0, 0, kSaw, 0.0f);
              osc (b, 0, 1, kSaw, 0.8f, 7.0f);
              b.set ("e1_sync", 1.0f).set ("e1_vol", 0.9f).set ("f1_vol", 2.5f);
              env (b, "env2", 0.001f, 0.9f, 0.3f, 0.4f);
              b.route ("Env 2", "e1_o2_coarse", 0.12f).route ("Mod Wheel", "e1_o2_coarse", 0.15f);
              model (b, 0, FilterOta, 0, 3500.0f, 0.1f, 0.4f);
              env (b, "env1", 0.003f, 0.8f, 0.8f, 0.3f);
              mono (b, true, 0.04f);
              delay (b, 0, kDiv8D, 0.3f, 0.15f);
              room (b, 0.18f);
              macros (b, "Brightness", "Space", "Sync", "Glide");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "e1_o2_coarse", 0.15f);
          } },
        { "Vintage Poly Strings", "Strings/Brass", "Soft polysynth strings with slow PWM and a hint of chorus.",
          [] (B& b) {
              twoOsc (b, 0.7f, 0.6f, 0.45f, 0.05f, 0.4f);
              lfo (b, 0, LfoTriangle, 0.4f);
              b.route ("LFO 1", "e1_o2_width", 0.15f);
              model (b, 0, FilterOta, 0, 2400.0f, 0.1f, 0.4f);
              env (b, "env1", 0.45f, 1.0f, 1.0f, 1.4f);
              chorus (b, 0, 0.25f);
              hall (b, 0.32f);
              padMacros (b, 0.3f);
              b.route ("Macro 3", "lfo1_rate", 0.4f);
          } },
    };
    return list;
}

// Tags for this pack. Tag names (rename them in the browser): Chorus Poly = the 80s
// one-oscillator chorus polysynth; Vintage Poly = the late-70s five-voice classic;
// Grand Poly = the 80s 8-voice flagship; Ladder = mono synths with a ladder filter;
// Tonewheel = drawbar organ with a rotary speaker; String Machine = 70s string ensemble.
juce::StringArray pack1Tags (const juce::String& presetName)
{
    static const std::map<juce::String, const char*> tags {
        { "Tine Piano", "Electric Piano" }, { "Suitcase Keys", "Electric Piano" }, { "Reed Piano", "Electric Piano" },
        { "Eighties Tines", "Electric Piano, FM" }, { "Dreamy EP", "Electric Piano, Ambient, Worship" }, { "Electric Grand", "Piano, Electric Piano" },
        { "Felt Piano", "Piano, Felt, Worship" }, { "Felt Lullaby", "Piano, Felt, Lo-Fi" }, { "Felt Upright", "Piano, Felt" },
        { "Keys and Pad", "Electric Piano, Pad, Worship" },
        { "Chamber Strings", "Strings" }, { "Ensemble Strings", "Strings, String Machine" }, { "Slow Strings Swell", "Strings, Swell, Cinematic" },
        { "Pizzicato Strings", "Strings, Short" }, { "Tremolo Strings", "Strings, Cinematic" }, { "Silk Strings", "Strings, Ambient" },
        { "Tubular Bells", "Bells" }, { "Music Box", "Bells" }, { "Glockenspiel", "Bells, Mallet" }, { "Crystal Bells", "Bells, Ambient" },
        { "Celesta", "Bells" }, { "Bells into Pad", "Bells, Pad, Evolving" },
        { "Harmonic Drift", "Pad, Evolving" }, { "Breathing Vowels", "Pad, Evolving, Choir" }, { "Glass Tides", "Pad, Evolving, Ambient" },
        { "Slow Bloom", "Pad, Evolving, Swell" }, { "Spectral Motion", "Pad, Evolving" }, { "Aurora", "Pad, Ambient, Swell" },
        { "Poly Brass Pad", "Pad, Polysynth, Brass" }, { "Vintage Poly Pad", "Pad, Polysynth, Vintage Poly" }, { "Wide Saw Pad", "Pad, Polysynth" },
        { "Analog Sweep Pad", "Pad, Polysynth, Ladder" }, { "Expressive Poly", "Pad, Polysynth" },
        { "Chorus Poly Pad", "Pad, Polysynth, Chorus Poly" }, { "Chorus Poly Strings", "Strings, Polysynth, Chorus Poly" },
        { "Chorus Poly Brass", "Brass, Polysynth, Chorus Poly" }, { "Chorus Poly Keys", "Synth Keys, Polysynth, Chorus Poly" },
        { "Chorus Poly Bass", "Bass, Chorus Poly" }, { "Chorus Poly Arp", "Arp, Polysynth, Chorus Poly" },
        { "Ladder Lead", "Lead, Mono, Ladder" }, { "Fat Mono Lead", "Lead, Mono, Ladder" }, { "Soft Ladder Lead", "Lead, Mono, Ladder" },
        { "Resonant Sweep Lead", "Lead, Mono, Ladder" }, { "Fifth Ladder Lead", "Lead, Mono, Ladder" },
        { "Pulse Bass 8ths", "Bass, Pulsing, Ladder" }, { "Octave Pulse Bass", "Bass, Pulsing, Ladder" }, { "Rolling Bass 16ths", "Bass, Pulsing, Ladder" },
        { "Gated Sub Pulse", "Bass, Pulsing, Sub" }, { "Dark Pulse Bass", "Bass, Pulsing" }, { "Synthwave Bass", "Bass, Pulsing" },
        { "Gospel Drawbars", "Organ, Tonewheel" }, { "Full Drawbars", "Organ, Tonewheel" }, { "Jazz Organ", "Organ, Tonewheel" },
        { "Chapel Organ", "Organ, Tonewheel, Worship" }, { "Organ Swell Pad", "Organ, Tonewheel, Pad" },
        { "Full Pipe Organ", "Organ, Pipe Organ" }, { "Soft Flute Stop", "Organ, Pipe Organ, Worship" },
        { "Expressive Strings", "Strings" }, { "Octave Strings", "Strings, Cinematic" }, { "Low Strings", "Strings, Cinematic" },
        { "Spiccato Strings", "Strings, Short" }, { "Strings and Choir", "Strings, Choir, Cinematic" }, { "Poly Strings", "Strings, Polysynth" },
        { "Grand Poly Pad", "Pad, Polysynth, Grand Poly" }, { "Grand Poly Brass", "Brass, Polysynth, Grand Poly" },
        { "Grand Poly Sweep", "Pad, Polysynth, Grand Poly" }, { "Grand Poly Arp", "Arp, Polysynth, Grand Poly" },
        { "Grand Poly Bells", "Bells, Polysynth, Grand Poly" },
        { "Chorus Poly Organ", "Organ, Polysynth, Chorus Poly" }, { "Chorus Poly Sweep", "Pad, Polysynth, Chorus Poly" },
        { "Chorus Poly Soft Pad", "Pad, Polysynth, Chorus Poly, Worship" },
        { "Vintage Poly Brass", "Brass, Polysynth, Vintage Poly" }, { "Vintage Poly Keys", "Synth Keys, Polysynth, Vintage Poly" },
        { "Vintage Poly Sync", "Lead, Polysynth, Vintage Poly" }, { "Vintage Poly Strings", "Strings, Polysynth, Vintage Poly" },
    };
    const auto it = tags.find (presetName);
    return it == tags.end() ? juce::StringArray() : PresetManager::parseTags (it->second);
}
