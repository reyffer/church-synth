#pragma once

// Building blocks shared by the factory presets and the downloadable preset
// packs (packs/). Values are in real units (Hz, seconds, choice index, 0..1 knob).

#include "PresetManager.h"

#include "dsp/fx/Effects.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace presethelpers
{
using namespace cs;
using namespace cs::fx;
using B = PresetBuilder;
using Params = std::vector<std::pair<juce::String, float>>;

// Synced delay time knob for a sync-division index.
inline float syncKnob (int division) { return ((float) division + 0.5f) / (float) kNumSyncDivs; }
constexpr int kDiv4 = 7, kDiv4D = 8, kDiv8 = 10, kDiv8D = 11, kDiv8T = 12, kDiv16 = 13;
inline const char* const kSlot[9] = { "a1", "a2", "a3", "b1", "b2", "b3", "x1", "x2", "x3" };

// --- building blocks -----------------------------------------------------------
inline void hall (B& b, float send, float size = 0.7f, float decay = 0.6f)
{
    b.fx (6, FxReverb, { { 0, 0.15f }, { 1, size }, { 2, decay }, { 3, 0.45f }, { 4, 0.2f }, { 5, 0.72f } });
    b.set ("fx_aux_send", send);
}

inline void shimmer (B& b, float send, float amount = 0.45f, float decay = 0.7f)
{
    b.fx (6, FxShimmer, { { 1, amount }, { 2, 0.8f }, { 3, decay }, { 4, 0.35f }, { 5, 0.65f } });
    b.set ("fx_aux_send", send);
}

inline void delay (B& b, int slot, int division, float feedback, float mix, bool pingPong = false)
{
    b.fx (slot, FxDelay, { { 0, syncKnob (division) }, { 2, feedback }, { 3, 0.25f }, { 4, 0.6f }, { 6, pingPong ? 1.0f : 0.0f }, { 7, mix } });
    b.set (juce::String ("fx_") + kSlot[slot] + "_sync", 1.0f);
}

inline void chorus (B& b, int slot, float mix = 0.4f)
{
    b.fx (slot, FxChorus, { { 1, 0.35f }, { 2, 0.25f }, { 3, 0.2f }, { 6, 1.0f }, { 7, mix } });
}

inline void vintageChorus (B& b, int slot, int mode, float mix = 0.5f)
{
    b.fx (slot, FxVintageChorus, { { 0, ((float) mode + 0.5f) / 3.0f }, { 4, mix } });
}

inline void macros (B& b, const char* m1, const char* m2, const char* m3, const char* m4)
{
    b.macroName (0, m1).macroName (1, m2).macroName (2, m3).macroName (3, m4);
}

// Brightness / Space / Motion / Swell as most pads use them.
inline void padMacros (B& b, float cutoffAmt = 0.35f)
{
    macros (b, "Brightness", "Space", "Motion", "Swell");
    b.route ("Macro 1", "f1_cutoff", cutoffAmt);
    b.route ("Macro 2", "fx_aux_send", 0.5f);
    b.route ("Macro 4", "env1_a", 0.35f);
}

inline void env (B& b, const juce::String& p, float a, float d, float s, float r)
{
    b.set (p + "_a", a).set (p + "_d", d).set (p + "_s", s).set (p + "_r", r);
}

inline void filter (B& b, int f, int mode, float cutoff, float reso, float kbd = 0.0f, float drive = 0.0f)
{
    const juce::String p = "f" + juce::String (f + 1) + "_";
    b.set (p + "on", 1.0f).set (p + "type", 0.0f).set (p + "mode", (float) mode).set (p + "cutoff", cutoff).set (p + "reso", reso).set (p + "kbd", kbd).set (p + "drive", drive);
}

constexpr int kLP12 = 1, kLP24 = 2, kBP12 = 4, kHP12 = 8;
constexpr int kSine = 0, kTri = 1, kSaw = 2, kSquare = 4;

inline void osc (B& b, int e, int o, int wave, float vol, float coarse = 0.0f, float fine = 0.0f, float width = 0.5f)
{
    const juce::String p = "e" + juce::String (e + 1) + "_o" + juce::String (o + 1) + "_";
    b.set (p + "wave", (float) wave).set (p + "vol", vol).set (p + "coarse", coarse).set (p + "width", width);
    if (o > 0)
        b.set (p + "fine", fine);
}

inline void uni (B& b, int e, int mode, int voices, float detune, float stereo = 0.8f)
{
    const juce::String p = "e" + juce::String (e + 1) + "_";
    b.set (p + "uni_on", 1.0f).set (p + "uni_mode", (float) mode).set (p + "uni_voices", (float) voices).set (p + "uni_detune", detune).set (p + "uni_stereo", stereo);
}

inline void wavetable (B& b, int e, const char* table, float pos, float vol = 0.7f)
{
    const juce::String p = "e" + juce::String (e + 1) + "_";
    b.set (p + "on", 1.0f).set (p + "type", 1.0f).choice (p + "wt_table", table).set (p + "wt_pos", pos).set (p + "vol", vol);
}

inline void noise (B& b, int n, const char* sample, float vol, float filter = 0.0f, bool loop = true)
{
    const juce::String p = "u_n" + juce::String (n + 1) + "_";
    b.set (p + "on", 1.0f).choice (p + "type", sample).set (p + "vol", vol).set (p + "filter", filter).set (p + "loop", loop ? 1.0f : 0.0f);
    if (! loop)
        b.set (p + "phase", 0.0f); // one-shots start at the beginning
}

inline void lfo (B& b, int k, int wave, float hz, bool bipolar = true)
{
    const juce::String p = "lfo" + juce::String (k + 1);
    b.set (p + "_wave", (float) wave).set (p + "_rate", hz).set (p + "_bipolar", bipolar ? 1.0f : 0.0f);
}

inline void mono (B& b, bool legato, float glide)
{
    b.set ("voicing", legato ? 1.0f : 0.0f).set ("glide", glide).set ("glide_legato", legato ? 1.0f : 0.0f);
}

// --- sequencer patterns -------------------------------------------------------------
inline SeqPattern gatePattern (std::initializer_list<int> on, float gate = 0.5f)
{
    SeqPattern p;
    int i = 0;
    for (int v : on)
    {
        p.steps[(size_t) i].on = v != 0;
        p.steps[(size_t) i].gate = gate;
        p.steps[(size_t) i].velocity = (i % 4 == 0) ? 1.0f : 0.7f;
        ++i;
    }
    return p;
}

// --- shared by the preset packs --------------------------------------------------
// Filter with a model (Ladder, OTA...) instead of the Classic type.
inline void model (B& b, int f, int type, int submode, float cutoff, float reso, float kbd = 0.0f, float drive = 0.0f)
{
    const juce::String p = "f" + juce::String (f + 1) + "_";
    b.set (p + "on", 1.0f).set (p + "type", (float) type).set (p + "submode", (float) submode).set (p + "cutoff", cutoff)
        .set (p + "reso", reso).set (p + "kbd", kbd).set (p + "drive", drive);
}

// Wavetable engine as a two-operator FM voice: a sine carrier (Basic Shapes,
// position 0) phase-modulated by a sine at `ratio`.
inline void fmVoice (B& b, int e, float ratio, float pm, float vol = 0.8f)
{
    const juce::String p = "e" + juce::String (e + 1) + "_";
    wavetable (b, e, "Basic Shapes", 0.0f, vol);
    b.set (p + "wt_mod_wave", 0.0f).set (p + "wt_mod_tune_mode", 0.0f).set (p + "wt_mod_ratio", ratio).set (p + "wt_pm", pm);
}

inline void engineOn (B& b, int e, float vol)
{
    b.set ("e" + juce::String (e + 1) + "_on", 1.0f).set ("e" + juce::String (e + 1) + "_vol", vol);
}

inline void tremolo (B& b, int slot, float amount, float rate)
{
    b.fx (slot, FxPanner, { { 0, amount }, { 1, rate } });
}

inline void room (B& b, float send) { hall (b, send, 0.45f, 0.45f); }

inline void bassSeq (B& b, int rate, std::initializer_list<int> steps, float gate)
{
    b.set ("seq_on", 1.0f).set ("seq_mode", 1.0f).set ("seq_rate", (float) rate).set ("seq_vel_blend", 0.6f);
    b.set ("seq_length", (float) steps.size());
    b.pattern (gatePattern (steps, gate));
}

// Tonewheel organ from sine "drawbars" (0..8 each, like the real thing: every step
// is 3 dB): 16', 5 1/3', 8', 4', 2 2/3', 2', then 1 1/3' or 1' on the utility osc.
// Percussion (optional) is a fast-decaying 2 2/3' or 4' from Env 3.
inline void drawbars (B& b, std::array<int, 9> db, int percussion = 0)
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
inline void rotary (B& b, float drive, float mix = 1.0f)
{
    b.fx (0, FxRotary, { { 5, drive }, { 7, mix } });
    b.route ("Mod Wheel", "fx_a1_p1", 1.0f);
}
} // namespace presethelpers
