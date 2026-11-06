// Preset Pack 14: Special FX.
//
// Sound-design effects, made to be taken apart and learned from: nature, sci-fi,
// transitions and oddities. Each one shows a technique in its notes (pitch
// envelopes, sample & hold, filtered noise, one-shot functions, ring modulation...).
//
// Macros: M1 Brightness / Tone, M2 Space, M3 a character control, M4 Length / Speed.

#include "Packs.h"

#include "plugin/PresetHelpers.h"

#include <map>

using namespace presethelpers;

namespace
{
constexpr int kBP24 = 5, kHP24 = 9;

void engineOff (B& b) { b.set ("e1_on", 0.0f); }

void fxMacros (B& b, const char* m1, const char* m1dest, float m1amt, const char* m3, const char* m3dest, float m3amt,
               const char* m4, const char* m4dest, float m4amt)
{
    macros (b, m1, "Space", m3, m4);
    b.route ("Macro 1", m1dest, m1amt).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", m3dest, m3amt).route ("Macro 4", m4dest, m4amt);
}

// Pitch envelope on Engine 1: starts `semis` away and returns over `time`.
void pitchEnv (B& b, float semis, float time)
{
    env (b, "env3", 0.0f, time, 0.0f, 0.05f);
    b.route ("Env 3", "e1_coarse", semis / 120.0f);
}

void oneShotFunc (B& b, int shape, float seconds)
{
    b.set ("func1_shape", (float) shape).set ("func1_mode", 1.0f).set ("func1_retrig", 0.0f).set ("func1_rate", 1.0f / seconds);
}
} // namespace

const std::vector<FactoryPreset>& pack14Presets()
{
    static const std::vector<FactoryPreset> list {
        // ================================================================ NATURE
        { "Thunder", "FX", "A thunder clap: a bright crack that rolls away into a long, uneven rumble. Press one key.",
          [] (B& b) {
              engineOff (b);
              noise (b, 0, "Brown", 0.9f, 0.0f);
              noise (b, 1, "White Wide", 0.6f, 0.0f);
              env (b, "env3", 0.0f, 0.25f, 0.0f, 0.1f);
              b.set ("u_n2_vol", 0.0f);
              b.route ("Env 3", "u_n2_vol", 0.7f);
              filter (b, 0, kLP12, 300.0f, 0.1f, 0.0f);
              env (b, "env2", 0.0f, 1.5f, 0.0f, 2.0f);
              b.route ("Env 2", "f1_cutoff", 0.45f);
              b.set ("rand1_type", 0.0f).set ("rand1_rate", 4.0f).set ("rand1_smooth", 0.7f).set ("rand1_retrig", 3.0f);
              b.set ("amp_mod_src", 14.0f).set ("amp_velo", 0.6f);
              env (b, "env1", 0.005f, 7.0f, 0.0f, 3.0f);
              b.set ("f1_vol", 2.0f);
              hall (b, 0.45f, 0.95f, 0.9f);
              fxMacros (b, "Tone", "f1_cutoff", 0.3f, "Rumble", "amp_velo", 0.3f, "Length", "env1_d", 0.3f);
          } },
        { "Rain", "Texture", "Steady rain, with a little low patter. Hold a key; Macro 3 makes it heavier.",
          [] (B& b) {
              engineOff (b);
              noise (b, 0, "Rain", 0.8f, 0.0f);
              noise (b, 1, "Pink Wide", 0.15f, 0.0f);
              filter (b, 0, kLP12, 6000.0f, 0.0f, 0.0f);
              b.set ("f1_vol", 1.6f);
              env (b, "env1", 1.5f, 1.0f, 1.0f, 2.5f);
              b.set ("amp_velo", 0.0f);
              room (b, 0.2f);
              fxMacros (b, "Tone", "f1_cutoff", 0.3f, "Downpour", "u_n2_vol", 0.4f, "Fade", "env1_a", 0.3f);
          } },
        { "Howling Wind", "Texture", "Gusting wind with a whistle you can tune: the keys move the whistle's pitch.",
          [] (B& b) {
              engineOff (b);
              noise (b, 0, "Wind", 0.8f, 0.0f);
              noise (b, 1, "White Wide", 0.3f, 0.0f);
              filter (b, 0, kBP12, 600.0f, 0.45f, 1.0f);
              b.set ("u_n1_vol", 1.0f);
              b.set ("rand1_type", 0.0f).set ("rand1_rate", 0.4f).set ("rand1_smooth", 1.0f).set ("rand1_retrig", 3.0f);
              b.route ("Random 1", "f1_cutoff", 0.15f);
              b.set ("amp_mod_src", 14.0f).set ("amp_velo", 0.5f);
              b.set ("f1_vol", 4.0f);
              env (b, "env1", 1.5f, 1.0f, 1.0f, 2.5f);
              hall (b, 0.35f, 0.85f, 0.8f);
              fxMacros (b, "Tone", "f1_cutoff", 0.3f, "Whistle", "f1_reso", 0.4f, "Gust Speed", "rand1_rate", 0.3f);
          } },
        { "Ocean Waves", "Texture", "Waves rolling in and out every few seconds. Hold a key.",
          [] (B& b) {
              engineOff (b);
              noise (b, 0, "Ocean", 0.8f, 0.0f);
              noise (b, 1, "Pink Wide", 0.4f, 0.0f);
              filter (b, 0, kLP12, 900.0f, 0.0f, 0.0f);
              lfo (b, 0, LfoSine, 0.12f, false);
              b.set ("lfo1_retrig", 3.0f);
              b.route ("LFO 1", "f1_cutoff", 0.35f);
              b.set ("amp_mod_src", 8.0f).set ("amp_velo", 0.7f);
              b.set ("f1_vol", 1.8f);
              env (b, "env1", 2.0f, 1.0f, 1.0f, 3.0f);
              hall (b, 0.3f, 0.85f, 0.8f);
              fxMacros (b, "Tone", "f1_cutoff", 0.3f, "Swell", "amp_velo", 0.2f, "Wave Speed", "lfo1_rate", 0.3f);
          } },
        { "Campfire", "Texture", "A crackling fire: soft roar underneath, pops and crackles on top.",
          [] (B& b) {
              engineOff (b);
              noise (b, 0, "Fire", 0.8f, 0.0f);
              noise (b, 1, "Vinyl Crackle", 0.4f, 0.2f);
              filter (b, 0, kLP12, 5000.0f, 0.0f, 0.0f);
              b.set ("f1_vol", 1.6f);
              env (b, "env1", 1.0f, 1.0f, 1.0f, 2.0f);
              b.set ("amp_velo", 0.0f);
              room (b, 0.2f);
              fxMacros (b, "Tone", "f1_cutoff", 0.3f, "Crackle", "u_n2_vol", 0.3f, "Fade", "env1_a", 0.3f);
          } },
        { "Distant Storm", "Texture", "Rain and a low rolling rumble far away: a whole storm from one key.",
          [] (B& b) {
              engineOff (b);
              noise (b, 0, "Rain", 0.6f, 0.2f);
              noise (b, 1, "Rumble Drone", 0.7f, 0.0f);
              filter (b, 0, kLP12, 3500.0f, 0.0f, 0.0f);
              b.set ("rand1_type", 0.0f).set ("rand1_rate", 0.15f).set ("rand1_smooth", 1.0f).set ("rand1_retrig", 3.0f);
              b.route ("Random 1", "u_n2_vol", 0.4f);
              b.set ("f1_vol", 1.8f);
              env (b, "env1", 2.0f, 1.0f, 1.0f, 3.0f);
              b.set ("amp_velo", 0.0f);
              hall (b, 0.4f, 0.9f, 0.85f);
              fxMacros (b, "Tone", "f1_cutoff", 0.3f, "Rumble", "u_n2_vol", 0.3f, "Fade", "env1_a", 0.3f);
          } },
        // ================================================================ SCI-FI
        { "Laser Zap", "FX", "A classic 'pew': a bright tone diving three octaves in a split second. Play it hard and short.",
          [] (B& b) {
              osc (b, 0, 0, kSaw, 0.6f);
              osc (b, 0, 1, kSquare, 0.6f, 0.0f, 0.1f, 0.3f);
              b.set ("e1_vol", 0.7f).set ("e1_coarse", 12.0f);
              pitchEnv (b, 36.0f, 0.18f);
              filter (b, 0, kLP12, 6000.0f, 0.2f, 0.3f);
              env (b, "env1", 0.001f, 0.25f, 0.0f, 0.1f);
              delay (b, 0, kDiv16, 0.35f, 0.25f, true);
              room (b, 0.25f);
              fxMacros (b, "Tone", "f1_cutoff", 0.3f, "Dive", "env3_d", 0.3f, "Length", "env1_d", 0.3f);
          } },
        { "UFO Hover", "Texture", "A wobbling, metallic hum hovering overhead: ring modulation plus a fast vibrato.",
          [] (B& b) {
              wavetable (b, 0, "Basic Shapes", 0.0f, 0.7f);
              b.set ("e1_wt_mod_wave", 0.0f).set ("e1_wt_mod_tune_mode", 0.0f).set ("e1_wt_mod_ratio", 1.5f).set ("e1_wt_fm_type", 2.0f)
                  .set ("e1_wt_fm", 0.6f);
              b.set ("e1_coarse", 24.0f);
              lfo (b, 0, LfoSine, 7.0f);
              b.set ("lfo1_retrig", 3.0f);
              b.route ("LFO 1", "e1_fine", 0.5f);
              lfo (b, 1, LfoSine, 0.1f);
              b.set ("lfo2_retrig", 3.0f);
              b.route ("LFO 2", "e1_wt_mod_ratio", 0.05f);
              filter (b, 0, kLP12, 4000.0f, 0.1f, 0.3f);
              b.set ("f1_vol", 1.5f);
              env (b, "env1", 1.0f, 1.0f, 1.0f, 2.0f);
              b.fx (0, FxPhaser, { { 3, 0.6f }, { 4, 0.1f }, { 7, 0.4f } });
              hall (b, 0.35f, 0.85f, 0.8f);
              fxMacros (b, "Tone", "f1_cutoff", 0.3f, "Wobble", "lfo1_rate", 0.2f, "Metal", "e1_wt_fm", 0.3f);
          } },
        { "Alarm Siren", "FX", "A wailing siren: the pitch rises and falls by a fifth, slowly. Hold a key.",
          [] (B& b) {
              osc (b, 0, 0, kSaw, 0.6f);
              osc (b, 0, 1, kSquare, 0.5f, 0.0f, 0.05f, 0.5f);
              b.set ("e1_vol", 0.6f);
              lfo (b, 0, LfoTriangle, 0.4f, false);
              b.set ("lfo1_retrig", 0.0f);
              b.route ("LFO 1", "e1_coarse", 7.0f / 120.0f);
              filter (b, 0, kLP12, 3000.0f, 0.2f, 0.4f);
              env (b, "env1", 0.05f, 1.0f, 1.0f, 0.5f);
              room (b, 0.3f);
              fxMacros (b, "Tone", "f1_cutoff", 0.3f, "Wail Speed", "lfo1_rate", 0.3f, "Release", "env1_r", 0.3f);
          } },
        { "Robot Chatter", "Texture", "A robot 'talking': random pitches and vowels stepping several times a second.",
          [] (B& b) {
              osc (b, 0, 0, kSquare, 0.7f, 0.0f, 0.0f, 0.3f);
              osc (b, 0, 1, kSaw, 0.4f);
              b.set ("e1_vol", 0.6f);
              b.set ("rand1_type", 2.0f).set ("rand1_rate", 9.0f).set ("rand1_retrig", 3.0f);
              b.route ("Random 1", "e1_coarse", 7.0f / 120.0f);
              b.set ("rand2_type", 2.0f).set ("rand2_rate", 9.0f).set ("rand2_retrig", 3.0f);
              b.route ("Random 2", "f2_a", 0.4f);
              filter (b, 0, kLP12, 3000.0f, 0.1f, 0.3f);
              b.set ("f2_on", 1.0f).set ("f2_type", 3.0f).set ("f2_cutoff", 1000.0f).set ("f2_reso", 0.4f).set ("f2_a", 0.4f).set ("f2_b", 1.0f)
                  .set ("f_routing", 0.0f).set ("f2_vol", 1.6f);
              env (b, "env1", 0.01f, 1.0f, 1.0f, 0.2f);
              room (b, 0.2f);
              fxMacros (b, "Tone", "f1_cutoff", 0.3f, "Talk Speed", "rand1_rate", 0.2f, "Vowels", "f2_reso", 0.3f);
              b.route ("Macro 3", "rand2_rate", 0.2f);
          } },
        { "Teleport", "FX", "A shimmering beam-up: a rising tone and noise climbing into a shimmer, over two seconds.",
          [] (B& b) {
              osc (b, 0, 0, kSine, 0.6f);
              osc (b, 0, 1, kTri, 0.4f, 7.0f);
              b.set ("e1_vol", 0.6f).set ("e1_noise_vol", 0.15f);
              uni (b, 0, 0, 4, 0.2f, 1.0f);
              oneShotFunc (b, 0, 2.0f);
              b.route ("Function 1", "e1_coarse", 24.0f / 120.0f).route ("Function 1", "f1_cutoff", 0.4f);
              lfo (b, 0, LfoSine, 12.0f);
              b.route ("LFO 1", "e1_fine", 0.1f);
              filter (b, 0, kLP12, 800.0f, 0.2f, 0.0f);
              b.set ("f1_vol", 1.6f);
              env (b, "env1", 0.3f, 2.5f, 0.0f, 1.0f);
              shimmer (b, 0.5f, 0.6f, 0.85f);
              fxMacros (b, "Tone", "f1_cutoff", 0.3f, "Sparkle", "e1_noise_vol", 0.2f, "Rise Time", "func1_rate", -0.15f);
          } },
        { "Computer Bleeps", "Texture", "A retro computer 'thinking': random square-wave beeps on a grid. Hold a key.",
          [] (B& b) {
              osc (b, 0, 0, kSquare, 0.7f);
              b.set ("e1_vol", 0.6f).set ("e1_coarse", 12.0f);
              b.set ("rand1_type", 2.0f).set ("rand1_sync", 1.0f).set ("rand1_div", (float) kDiv16).set ("rand1_retrig", 3.0f);
              b.route ("Random 1", "e1_coarse", 12.0f / 120.0f);
              b.set ("e1_q_mask", 0x91 /* C, E, G */);
              b.set ("func1_shape", 13.0f).set ("func1_sync", 1.0f).set ("func1_div", 3.0f).set ("func1_retrig", 3.0f);
              b.set ("amp_mod_src", 11.0f).set ("amp_velo", 1.0f);
              filter (b, 0, kLP12, 5000.0f, 0.0f, 0.0f);
              b.set ("f1_vol", 1.5f);
              env (b, "env1", 0.01f, 1.0f, 1.0f, 0.2f);
              delay (b, 0, kDiv8D, 0.3f, 0.2f, true);
              room (b, 0.2f);
              fxMacros (b, "Tone", "f1_cutoff", 0.3f, "Range", "e1_coarse", 0.05f, "Release", "env1_r", 0.3f);
          } },
        { "Power Down", "FX", "A machine winding down: the pitch sinks two octaves and the sound dies away. Press once.",
          [] (B& b) {
              osc (b, 0, 0, kSaw, 0.7f);
              osc (b, 0, 1, kSquare, 0.5f, -12.0f, 0.05f, 0.4f);
              b.set ("e1_vol", 0.6f);
              oneShotFunc (b, 0, 1.5f);
              b.route ("Function 1", "e1_coarse", -24.0f / 120.0f).route ("Function 1", "f1_cutoff", -0.4f);
              filter (b, 0, kLP12, 3000.0f, 0.15f, 0.3f);
              env (b, "env1", 0.005f, 1.8f, 0.0f, 0.2f);
              b.set ("env1_dcurve", 0.0f); // linear fade, so the whole dive is heard
              room (b, 0.25f);
              fxMacros (b, "Tone", "f1_cutoff", 0.3f, "Drop", "func1_rate", -0.15f, "Length", "env1_d", 0.3f);
          } },
        // ================================================================ TRANSITIONS AND IMPACTS
        { "Sub Drop Impact", "FX", "A cinematic boom: a sine diving into the sub with a noise hit and a long reverb.",
          [] (B& b) {
              osc (b, 0, 0, kSine, 0.9f);
              osc (b, 0, 1, kTri, 0.2f, 12.0f);
              b.set ("e1_vol", 0.8f).set ("e1_coarse", -12.0f);
              pitchEnv (b, 24.0f, 0.5f);
              b.set ("env3_dcurve", 4.0f);
              noise (b, 0, "Thump", 0.6f, 0.0f, false);
              filter (b, 0, kLP12, 3000.0f, 0.0f, 0.0f);
              b.set ("f1_vol", 1.6f);
              env (b, "env1", 0.001f, 2.5f, 0.0f, 1.5f);
              hall (b, 0.35f, 0.95f, 0.9f);
              fxMacros (b, "Tone", "f1_cutoff", 0.3f, "Punch", "u_n1_vol", 0.3f, "Length", "env1_d", 0.3f);
          } },
        { "Whoosh Pass", "FX", "Something rushing past: filtered noise that rises, peaks and falls while it pans across.",
          [] (B& b) {
              engineOff (b);
              noise (b, 0, "White Wide", 0.9f, 0.0f);
              filter (b, 0, kBP12, 300.0f, 0.3f, 0.0f);
              env (b, "env2", 0.7f, 0.8f, 0.0f, 0.5f);
              b.route ("Env 2", "f1_cutoff", 0.5f);
              b.set ("env2_acurve", -6.0f);
              oneShotFunc (b, 0, 1.5f);
              b.route ("Function 1", "voice_pan", 0.6f); // knob units: 0.6 = from -0.6 to +0.6
              b.set ("voice_pan", -0.6f);
              b.set ("f1_vol", 4.0f);
              env (b, "env1", 0.7f, 0.8f, 0.0f, 0.5f);
              b.set ("env1_acurve", -6.0f);
              hall (b, 0.3f, 0.85f, 0.8f);
              fxMacros (b, "Tone", "f1_cutoff", 0.3f, "Whistle", "f1_reso", 0.4f, "Length", "env1_a", 0.3f);
          } },
        { "Reverse Crash", "FX", "A reversed cymbal: noise that swells up faster and faster, then stops dead. Press it before the downbeat.",
          [] (B& b) {
              engineOff (b);
              noise (b, 0, "White Wide", 0.9f, 0.0f);
              noise (b, 1, "Swell", 0.5f, 0.0f, false);
              filter (b, 0, kHP12, 2000.0f, 0.1f, 0.0f);
              env (b, "env1", 1.6f, 0.05f, 0.0f, 0.05f);
              b.set ("env1_acurve", -12.0f);
              env (b, "env2", 1.6f, 0.05f, 0.0f, 0.05f);
              b.route ("Env 2", "f1_cutoff", 0.25f);
              b.set ("f1_vol", 1.6f);
              hall (b, 0.25f, 0.8f, 0.7f);
              fxMacros (b, "Tone", "f1_cutoff", 0.3f, "Body", "u_n2_vol", 0.3f, "Length", "env1_a", 0.3f);
          } },
        { "Noise Sweep Up", "FX", "A white-noise riser over two bars: a highpass sweeping up, synced to the tempo.",
          [] (B& b) {
              engineOff (b);
              noise (b, 0, "White Wide", 0.9f, 0.0f);
              filter (b, 0, kHP12, 150.0f, 0.3f, 0.0f);
              b.set ("func1_shape", 0.0f).set ("func1_sync", 1.0f).set ("func1_div", 2.0f).set ("func1_mode", 1.0f).set ("func1_retrig", 0.0f);
              b.route ("Function 1", "f1_cutoff", 0.55f);
              b.set ("amp_mod_src", 11.0f).set ("amp_velo", 0.8f);
              b.set ("f1_vol", 1.6f);
              env (b, "env1", 0.01f, 1.0f, 1.0f, 1.0f);
              delay (b, 0, kDiv8, 0.4f, 0.2f, true);
              hall (b, 0.35f, 0.85f, 0.8f);
              fxMacros (b, "Tone", "f1_cutoff", 0.2f, "Resonance", "f1_reso", 0.4f, "Release", "env1_r", 0.3f);
          } },
        // ================================================================ ODDITIES
        { "Clock Ticks", "Texture", "Tick, tock: a mechanical clock on the beat, following the tempo. Hold a key.",
          [] (B& b) {
              engineOff (b);
              noise (b, 0, "Click", 1.0f, 0.0f, false);
              b.set ("u_n1_kbd", 1.0f);
              filter (b, 0, kBP12, 3000.0f, 0.35f, 0.5f);
              b.set ("f1_vol", 4.0f);
              env (b, "env1", 0.001f, 0.08f, 0.0f, 0.05f);
              b.set ("seq_on", 1.0f).set ("seq_mode", 1.0f).set ("seq_rate", (float) kDiv4).set ("seq_length", 2.0f);
              {
                  SeqPattern p = gatePattern ({ 1, 1 }, 0.3f);
                  p.steps[1].pitch = 5;
                  p.steps[1].velocity = 0.8f;
                  b.pattern (p);
              }
              room (b, 0.25f);
              fxMacros (b, "Tone", "f1_cutoff", 0.3f, "Wood", "f1_reso", -0.3f, "Release", "env1_d", 0.2f);
          } },
        { "Radio Tuning", "Texture", "Turning an old radio dial: static that changes colour, with whistles drifting through.",
          [] (B& b) {
              osc (b, 0, 0, kSine, 0.3f);
              b.set ("e1_vol", 0.4f).set ("e1_coarse", 24.0f);
              b.set ("rand1_type", 0.0f).set ("rand1_rate", 0.7f).set ("rand1_smooth", 0.8f).set ("rand1_retrig", 3.0f);
              b.route ("Random 1", "e1_coarse", 12.0f / 120.0f).route ("Random 1", "f1_cutoff", 0.3f);
              noise (b, 0, "Static", 0.6f, 0.0f);
              noise (b, 1, "Digital Static", 0.3f, 0.0f);
              filter (b, 0, kBP12, 1500.0f, 0.3f, 0.0f);
              b.set ("f1_vol", 3.6f);
              env (b, "env1", 0.2f, 1.0f, 1.0f, 0.5f);
              b.set ("amp_velo", 0.0f);
              room (b, 0.15f);
              fxMacros (b, "Tone", "f1_cutoff", 0.3f, "Tuning Speed", "rand1_rate", 0.3f, "Whistle", "e1_vol", 0.3f);
          } },
        { "Underwater Bubbles", "Texture", "Bubbles rising under water: little sine blips that sweep up, randomly, in a muffled space. Hold a chord.",
          [] (B& b) {
              osc (b, 0, 0, kSine, 0.8f);
              b.set ("e1_vol", 0.7f).set ("e1_coarse", 12.0f);
              env (b, "env3", 0.0f, 0.06f, 0.0f, 0.05f);
              b.route ("Env 3", "e1_coarse", -12.0f / 120.0f);
              filter (b, 0, kLP12, 1500.0f, 0.1f, 0.3f);
              b.set ("f1_vol", 2.0f);
              env (b, "env1", 0.001f, 0.1f, 0.0f, 0.08f);
              b.set ("seq_on", 1.0f).set ("seq_mode", 0.0f).set ("seq_arp_order", (float) ArpRandom).set ("seq_arp_octaves", 2.0f)
                  .set ("seq_rate", (float) kDiv16);
              {
                  SeqPattern p = gatePattern ({ 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 1, 0 }, 0.4f);
                  for (auto& st : p.steps)
                      st.prob = 0.7f;
                  b.pattern (p);
              }
              chorus (b, 0, 0.5f);
              hall (b, 0.4f, 0.8f, 0.7f);
              fxMacros (b, "Murk", "f1_cutoff", 0.3f, "Pop", "env3_d", 0.2f, "Release", "env1_d", 0.2f);
          } },
    };
    return list;
}

juce::StringArray pack14Tags (const juce::String& presetName)
{
    static const std::map<juce::String, const char*> tags {
        { "Thunder", "Special FX, Nature" }, { "Rain", "Special FX, Nature" }, { "Howling Wind", "Special FX, Nature" },
        { "Ocean Waves", "Special FX, Nature" }, { "Campfire", "Special FX, Nature" }, { "Distant Storm", "Special FX, Nature" },
        { "Laser Zap", "Special FX, Sci-Fi" }, { "UFO Hover", "Special FX, Sci-Fi, Ring Mod" }, { "Alarm Siren", "Special FX, Sci-Fi" },
        { "Robot Chatter", "Special FX, Sci-Fi" }, { "Teleport", "Special FX, Sci-Fi, Shimmer" }, { "Computer Bleeps", "Special FX, Sci-Fi" },
        { "Power Down", "Special FX, Sci-Fi" },
        { "Sub Drop Impact", "Special FX, Transition, Impact, Cinematic" }, { "Whoosh Pass", "Special FX, Transition" },
        { "Reverse Crash", "Special FX, Transition" }, { "Noise Sweep Up", "Special FX, Transition, Riser" },
        { "Clock Ticks", "Special FX, Oddity" }, { "Radio Tuning", "Special FX, Oddity" }, { "Underwater Bubbles", "Special FX, Oddity" },
    };
    const auto it = tags.find (presetName);
    return it == tags.end() ? juce::StringArray() : PresetManager::parseTags (it->second);
}
