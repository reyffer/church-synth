// Preset Pack 15: Acoustic Pianos.
//
// Twenty pianos built entirely from synthesis: grands, soul / gospel pianos,
// uprights and character pianos. They won't pass for a sampled concert grand in
// a quiet room, but they show how far synthesis gets with the things that make a
// piano a piano:
//   - a hammer strike (a noise transient) that gets brighter with velocity,
//   - three slightly detuned strings per note (unison) that beat gently,
//   - a "double decay": a bright attack that fades fast (Engine 2) over a body that
//     rings much longer (Engine 1),
//   - slightly stretched upper partials (real piano overtones are a little sharp),
//   - high notes die away faster than low notes (key tracking on the decay),
//   - a wide velocity range, a damper-like release and a room.
//
// Macros: M1 Tone (brightness), M2 Space, M3 a character control, M4 Decay.

#include "Packs.h"

#include "plugin/PresetHelpers.h"

#include <map>

using namespace presethelpers;

namespace
{
struct Piano
{
    float body = 0.58f;                              // body tone: position on the Sine-to-Saw table
    int strings = 3; float stringDetune = 0.11f;    // unison "strings" (0.11 = about +/-0.8 cents)
    float cutoff = 630.0f, kbd = 0.6f;               // resting tone (soft touch, after the strike), key tracking
    float strike = 0.35f, strikeDecay = 0.5f;        // hammer brightening (Env 2, scaled by velocity)
    float velBright = 0.25f;                         // static brightness from velocity
    float attackLayer = 0.35f, attackDecay = 0.35f;  // Engine 2: bright FM attack that fades fast
    float hammer = 0.18f; const char* hammerNoise = "Key Click";
    float decay = 10.0f, release = 0.35f, velo = 0.8f;
    float level = 0.5f;
};

void piano (B& b, const Piano& p)
{
    // Engine 1: the long-ringing body. A wavetable between a sine and a saw gives a
    // saw-like row of harmonics that falls away faster than a raw saw.
    // A harder strike also makes the string itself richer: velocity moves the table position.
    wavetable (b, 0, "Sine to Saw", p.body - 0.15f, 0.6f);
    b.route ("Velocity", "e1_wt_pos", 0.3f);
    // The strings of one note: slightly detuned unison voices that all start in phase
    // (the hammer hits them together) and drift apart, so the tone blooms and beats.
    if (p.strings > 1)
    {
        uni (b, 0, 0, p.strings, p.stringDetune, 0.5f);
        b.set ("e1_wt_uni_phase", 0.0f);
    }

    // Engine 2: the bright attack (FM partials) that fades quickly: the "double decay".
    fmVoice (b, 1, 2.01f, 0.5f, 0.0f);
    env (b, "env3", 0.0f, p.attackDecay, 0.0f, 0.2f);
    b.set ("env3_link", 0.0f);
    b.route ("Env 3", "e2_vol", p.attackLayer, "Velocity", 1.0f);

    // Hammer.
    noise (b, 0, p.hammerNoise, p.hammer, 0.3f, false);
    b.route ("Velocity", "u_n1_vol", p.hammer * 0.5f);

    // Tone: brighter with key, velocity and the strike.
    filter (b, 0, kLP12, p.cutoff, 0.0f, p.kbd);
    b.set ("f1_vol", p.level);
    env (b, "env2", 0.0f, p.strikeDecay, 0.0f, 0.3f);
    b.route ("Env 2", "f1_cutoff", p.strike, "Velocity", 1.0f);
    b.route ("Velocity", "f1_cutoff", p.velBright);

    // Amp: no sustain (a piano note always dies away). The steepest decay curve fades
    // evenly in decibels, like a real string; high notes die away much faster.
    env (b, "env1", 0.0f, p.decay, 0.0f, p.release);
    b.set ("env1_dcurve", 20.0f);
    b.route ("Keytrack", "env1_d", -0.28f);
    b.set ("amp_velo", p.velo);
    b.set ("max_voices", 16.0f);
}

void pMacros (B& b, const char* m3, const char* m3dest, float m3amt)
{
    macros (b, "Tone", "Space", m3, "Decay");
    b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", m3dest, m3amt).route ("Macro 4", "env1_d", 0.12f);
}

void stage (B& b, float send, float size, float decay) { hall (b, send, size, decay); }
} // namespace

const std::vector<FactoryPreset>& pack15Presets()
{
    static const std::vector<FactoryPreset> list {
        // ================================================================ GRANDS
        { "Concert Grand", "Keys", "A big, open concert grand in a hall: full low end, clear top, wide dynamics.",
          [] (B& b) {
              Piano p; p.cutoff = 810.0f; p.strike = 0.4f; p.attackLayer = 0.4f; p.decay = 12.0f;
              piano (b, p);
              stage (b, 0.32f, 0.85f, 0.75f);
              pMacros (b, "Hammer", "u_n1_vol", 0.2f);
          } },
        { "Studio Grand", "Keys", "A close-miked, balanced studio grand with a small room. Sits well in a band.",
          [] (B& b) {
              Piano p; p.cutoff = 720.0f; p.strike = 0.35f; p.decay = 10.0f;
              piano (b, p);
              room (b, 0.18f);
              pMacros (b, "Hammer", "u_n1_vol", 0.2f);
          } },
        { "Warm Grand", "Keys", "A darker, rounder grand: soft top, warm middle. Lovely under a vocal.",
          [] (B& b) {
              Piano p; p.body = 0.46f; p.cutoff = 400.0f; p.strike = 0.3f; p.attackLayer = 0.25f; p.hammer = 0.12f;
              p.hammerNoise = "Thump";
              piano (b, p);
              stage (b, 0.28f, 0.75f, 0.65f);
              pMacros (b, "Hammer", "u_n1_vol", 0.2f);
          } },
        { "Bright Pop Grand", "Keys", "A bright, punchy, slightly compressed grand that cuts through a pop mix.",
          [] (B& b) {
              Piano p; p.level = 0.28f; p.body = 0.70f; p.cutoff = 1260.0f; p.strike = 0.45f; p.attackLayer = 0.5f; p.velBright = 0.25f; p.decay = 9.0f;
              piano (b, p);
              b.fx (0, FxCompressor, { { 0, 0.6f }, { 1, 0.35f }, { 2, 0.3f }, { 3, 0.4f } });
              room (b, 0.2f);
              pMacros (b, "Sparkle", "e2_wt_pm", 0.2f);
          } },
        { "Soft Grand", "Keys", "A gentle, intimate grand played softly: muted hammers, a long, quiet tail.",
          [] (B& b) {
              Piano p; p.body = 0.42f; p.cutoff = 360.0f; p.strike = 0.2f; p.attackLayer = 0.15f; p.hammer = 0.1f;
              p.hammerNoise = "Soft Click"; p.velo = 0.55f; p.decay = 11.0f;
              piano (b, p);
              stage (b, 0.3f, 0.8f, 0.7f);
              pMacros (b, "Hammer", "u_n1_vol", 0.15f);
          } },
        { "Ballad Grand", "Keys", "A singing ballad grand with a long hall and a soft echo. Slow songs and big chords.",
          [] (B& b) {
              Piano p; p.cutoff = 680.0f; p.strike = 0.35f; p.decay = 12.0f; p.release = 0.5f;
              piano (b, p);
              delay (b, 0, kDiv4D, 0.2f, 0.08f, true);
              stage (b, 0.38f, 0.9f, 0.8f);
              pMacros (b, "Hammer", "u_n1_vol", 0.2f);
          } },
        { "Rock Grand", "Keys", "A hard, bright grand with a touch of drive: rock and roll piano that bites.",
          [] (B& b) {
              Piano p; p.body = 0.74f; p.cutoff = 1120.0f; p.strike = 0.45f; p.attackLayer = 0.45f; p.hammer = 0.25f; p.decay = 8.0f;
              piano (b, p);
              b.set ("f1_drive", 0.25f);
              room (b, 0.18f);
              pMacros (b, "Drive", "f1_drive", 0.3f);
          } },
        { "Worship Grand", "Keys", "A warm, spacious grand for worship: soft attack, gentle brightness, a big room.",
          [] (B& b) {
              Piano p; p.body = 0.54f; p.cutoff = 540.0f; p.strike = 0.3f; p.attackLayer = 0.3f; p.hammer = 0.14f; p.decay = 12.0f;
              p.release = 0.5f;
              piano (b, p);
              stage (b, 0.38f, 0.9f, 0.8f);
              pMacros (b, "Hammer", "u_n1_vol", 0.2f);
          } },
        // ================================================================ SOUL, R&B, GOSPEL
        { "Soul Ballad Piano", "Keys", "Warm, round and present, with a soft, singing tone: the neo-soul / R&B ballad piano.",
          [] (B& b) {
              Piano p; p.level = 0.31f; p.body = 0.54f; p.cutoff = 500.0f; p.strike = 0.35f; p.attackLayer = 0.3f;
              p.hammer = 0.12f; p.hammerNoise = "Thump"; p.decay = 11.0f; p.velo = 0.7f;
              piano (b, p);
              b.fx (0, FxCompressor, { { 0, 0.65f }, { 1, 0.25f }, { 2, 0.4f }, { 3, 0.5f } });
              room (b, 0.25f);
              pMacros (b, "Warmth", "e1_wt_pos", -0.15f);
          } },
        { "Neo-Soul Piano", "Keys", "A darker, intimate soul piano with a hint of tape warmth and wobble. Late-night chords.",
          [] (B& b) {
              Piano p; p.body = 0.50f; p.cutoff = 380.0f; p.strike = 0.3f; p.attackLayer = 0.25f; p.hammer = 0.1f;
              p.hammerNoise = "Thump"; p.decay = 9.0f;
              piano (b, p);
              // A slow, free-running pitch wobble (tape wow); Macro 3 scales it.
              lfo (b, 0, LfoSine, 0.4f);
              b.set ("lfo1_retrig", 3.0f);
              b.route ("LFO 1", "e1_fine", 0.04f, "Macro 3", 0.75f).route ("LFO 1", "e2_fine", 0.04f, "Macro 3", 0.75f);
              b.fx (0, FxTapeEcho, { { 0, syncKnob (kDiv4) }, { 2, 0.1f }, { 3, 0.3f }, { 4, 0.3f }, { 7, 0.1f } });
              b.set ("fx_a1_sync", 1.0f);
              room (b, 0.22f);
              macros (b, "Tone", "Space", "Wobble", "Decay");
              b.route ("Macro 1", "f1_cutoff", 0.3f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 4", "env1_d", 0.12f);
          } },
        { "Gospel Grand", "Keys", "A bright, punchy, powerful grand for gospel runs and big shout chords.",
          [] (B& b) {
              Piano p; p.level = 0.30f; p.body = 0.66f; p.cutoff = 990.0f; p.strike = 0.45f; p.attackLayer = 0.45f; p.velBright = 0.25f; p.hammer = 0.2f;
              p.decay = 9.0f; p.velo = 0.8f;
              piano (b, p);
              b.fx (0, FxCompressor, { { 0, 0.6f }, { 1, 0.3f }, { 2, 0.25f }, { 3, 0.4f } });
              room (b, 0.25f);
              pMacros (b, "Hammer", "u_n1_vol", 0.2f);
          } },
        // ================================================================ UPRIGHTS
        { "Upright Piano", "Keys", "A homely upright: boxier, more mid-range, shorter sustain than a grand.",
          [] (B& b) {
              Piano p; p.level = 0.37f; p.cutoff = 580.0f; p.strike = 0.35f; p.decay = 6.5f; p.hammer = 0.22f;
              piano (b, p);
              b.set ("f2_on", 1.0f).set ("f2_type", 0.0f).set ("f2_mode", 8.0f).set ("f2_cutoff", 150.0f).set ("f_routing", 0.0f);
              room (b, 0.2f);
              pMacros (b, "Hammer", "u_n1_vol", 0.2f);
          } },
        { "Honky-Tonk", "Keys", "An out-of-tune saloon upright: the strings of each note deliberately detuned against each other.",
          [] (B& b) {
              Piano p; p.level = 0.38f; p.strings = 3; p.stringDetune = 0.27f; p.cutoff = 810.0f; p.strike = 0.4f; p.decay = 6.0f; p.hammer = 0.25f;
              piano (b, p);
              b.set ("f2_on", 1.0f).set ("f2_type", 0.0f).set ("f2_mode", 8.0f).set ("f2_cutoff", 180.0f).set ("f_routing", 0.0f);
              room (b, 0.18f);
              pMacros (b, "Detune", "e1_uni_detune", 0.08f);
          } },
        { "Old Upright", "Keys", "A tired old upright on a dusty recording: dark, slightly out of tune, a little hiss.",
          [] (B& b) {
              Piano p; p.stringDetune = 0.19f; p.cutoff = 360.0f; p.strike = 0.3f; p.decay = 5.5f; p.hammer = 0.2f;
              piano (b, p);
              noise (b, 1, "Tape Hiss", 0.02f, 0.0f);
              b.fx (0, FxTapeEcho, { { 0, syncKnob (kDiv4) }, { 2, 0.1f }, { 4, 0.4f }, { 7, 0.1f } });
              b.set ("fx_a1_sync", 1.0f);
              room (b, 0.2f);
              pMacros (b, "Hiss", "u_n2_vol", 0.08f);
          } },
        // ================================================================ CHARACTER
        { "Muted Piano", "Keys", "A piano with the felt rail down: muffled, soft, very short. Intimate and percussive.",
          [] (B& b) {
              Piano p; p.body = 0.42f; p.cutoff = 270.0f; p.strike = 0.2f; p.attackLayer = 0.1f; p.hammer = 0.12f;
              p.hammerNoise = "Thump"; p.decay = 1.6f; p.release = 0.2f;
              piano (b, p);
              room (b, 0.22f);
              pMacros (b, "Thump", "u_n1_vol", 0.2f);
          } },
        { "Lo-Fi Piano", "Keys", "A piano on a worn cassette: wobbly pitch, dull top, vinyl dust. Chill and nostalgic.",
          [] (B& b) {
              Piano p; p.cutoff = 400.0f; p.strike = 0.25f; p.decay = 7.0f;
              piano (b, p);
              noise (b, 1, "Vinyl Dust", 0.04f, 0.0f);
              lfo (b, 0, LfoSine, 0.5f);
              b.set ("lfo1_retrig", 3.0f);
              b.route ("LFO 1", "e1_fine", 0.05f).route ("LFO 1", "e2_fine", 0.05f);
              b.fx (0, FxTapeEcho, { { 0, syncKnob (kDiv4) }, { 2, 0.15f }, { 4, 0.6f }, { 7, 0.15f } });
              b.set ("fx_a1_sync", 1.0f);
              room (b, 0.22f);
              pMacros (b, "Dust", "u_n2_vol", 0.1f);
          } },
        { "Dark Cinematic Piano", "Keys", "A very dark, distant piano in an enormous space. Single notes and slow chords for film moments.",
          [] (B& b) {
              Piano p; p.body = 0.46f; p.cutoff = 270.0f; p.strike = 0.25f; p.attackLayer = 0.2f; p.hammer = 0.1f;
              p.hammerNoise = "Thump"; p.decay = 14.0f; p.release = 1.0f;
              piano (b, p);
              delay (b, 0, kDiv4D, 0.3f, 0.12f, true);
              hall (b, 0.5f, 0.95f, 0.9f);
              pMacros (b, "Echo", "fx_a1_p8", 0.3f);
          } },
        { "Toy Piano", "Keys", "A tiny toy piano: metal rods instead of strings, bright, bell-like and a little clunky.",
          [] (B& b) {
              fmVoice (b, 0, 3.5f, 0.2f, 0.7f);
              env (b, "env2", 0.001f, 0.3f, 0.0f, 0.2f);
              b.route ("Env 2", "e1_wt_pm", 0.3f, "Velocity", 1.0f);
              b.set ("transpose", 1.0f);
              noise (b, 0, "Click", 0.2f, 0.3f, false);
              filter (b, 0, kLP12, 7000.0f, 0.0f, 0.3f);
              b.set ("f1_vol", 0.8f);
              env (b, "env1", 0.001f, 1.2f, 0.0f, 0.3f);
              b.route ("Keytrack", "env1_d", -0.2f);
              b.set ("amp_velo", 0.6f);
              room (b, 0.22f);
              macros (b, "Tone", "Space", "Clunk", "Decay");
              b.route ("Macro 1", "e1_wt_pm", 0.25f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "u_n1_vol", 0.3f).route ("Macro 4", "env1_d", 0.12f);
          } },
        // ================================================================ LAYERS
        { "Piano and Pad", "Keys", "A warm grand with a soft pad that rises underneath while you hold the chord.",
          [] (B& b) {
              Piano p; p.body = 0.54f; p.cutoff = 540.0f; p.strike = 0.3f; p.decay = 11.0f;
              piano (b, p);
              b.set ("u_osc_on", 1.0f).set ("u_osc_wave", 2.0f).set ("u_osc_vol", 0.0f).set ("u_osc_coarse", 0.0f);
              b.set ("u_osc_fmix", 1.0f);
              filter (b, 1, kLP12, 900.0f, 0.0f, 0.3f);
              b.set ("f_routing", 1.0f);
              // The pad: the Utility saw through Filter 2, faded in by a slow Swell shape on Function 1.
              b.set ("func1_shape", 7.0f).set ("func1_mode", 2.0f).set ("func1_rate", 0.4f).set ("func1_retrig", 0.0f);
              b.route ("Function 1", "u_osc_vol", 0.38f);
              stage (b, 0.35f, 0.85f, 0.75f);
              pMacros (b, "Pad", "u_osc_vol", 0.2f);
          } },
        { "Piano and Strings", "Keys", "A grand with a slow string section blooming behind it: the big ballad combination.",
          [] (B& b) {
              Piano p; p.cutoff = 630.0f; p.strike = 0.35f; p.decay = 11.0f;
              piano (b, p);
              b.set ("u_osc_on", 1.0f).set ("u_osc_wave", 2.0f).set ("u_osc_vol", 0.0f).set ("u_osc_coarse", 0.0f);
              b.set ("u_osc_fmix", 1.0f);
              filter (b, 1, kLP12, 2200.0f, 0.0f, 0.3f);
              b.set ("f_routing", 1.0f);
              b.set ("func1_shape", 7.0f).set ("func1_mode", 2.0f).set ("func1_rate", 0.6f).set ("func1_retrig", 0.0f);
              b.route ("Function 1", "u_osc_vol", 0.32f);
              lfo (b, 0, LfoSine, 5.2f);
              b.set ("lfo1_fade", 1.0f);
              b.route ("LFO 1", "u_osc_fine", 0.05f);
              chorus (b, 0, 0.2f);
              stage (b, 0.38f, 0.9f, 0.8f);
              pMacros (b, "Strings", "u_osc_vol", 0.2f);
          } },
    };
    return list;
}

juce::StringArray pack15Tags (const juce::String& presetName)
{
    static const std::map<juce::String, const char*> tags {
        { "Concert Grand", "Piano, Grand" }, { "Studio Grand", "Piano, Grand" }, { "Warm Grand", "Piano, Grand, Worship" },
        { "Bright Pop Grand", "Piano, Grand, Bright" }, { "Soft Grand", "Piano, Grand, Worship" }, { "Ballad Grand", "Piano, Grand, Worship" },
        { "Rock Grand", "Piano, Grand, Bright" }, { "Worship Grand", "Piano, Grand, Worship" },
        { "Soul Ballad Piano", "Piano, Grand, Soul" }, { "Neo-Soul Piano", "Piano, Soul, Lo-Fi" }, { "Gospel Grand", "Piano, Grand, Gospel" },
        { "Upright Piano", "Piano, Upright" }, { "Honky-Tonk", "Piano, Upright" }, { "Old Upright", "Piano, Upright, Lo-Fi" },
        { "Muted Piano", "Piano, Felt" }, { "Lo-Fi Piano", "Piano, Lo-Fi" }, { "Dark Cinematic Piano", "Piano, Cinematic, Dark" },
        { "Toy Piano", "Piano, Bells" }, { "Piano and Pad", "Piano, Pad, Worship" }, { "Piano and Strings", "Piano, Strings" },
    };
    const auto it = tags.find (presetName);
    return it == tags.end() ? juce::StringArray() : PresetManager::parseTags (it->second);
}
