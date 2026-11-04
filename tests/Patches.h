#pragma once

// Reference patches shared by the tests and the CPU benchmark.

#include "dsp/Content.h"
#include "dsp/ModMatrix.h"
#include "dsp/Params.h"

namespace cs::patches
{
// One saw, LP12 open: the cheapest useful sound.
inline SynthParams init()
{
    return SynthParams {};
}

// Typical worship pad: 2 engines, detuned saws + square, unison 4, LP24 analog.
inline SynthParams pad()
{
    SynthParams p;
    for (int e = 0; e < 2; ++e)
    {
        auto& eng = p.engine[e];
        eng.on = true;
        eng.osc[0] = { WaveSaw, 0.5f, 0.8f, 0.0f, 0.0f, true, 0.0f };
        eng.osc[1] = { e == 0 ? WaveSquare : WaveSaw, 0.35f, 0.5f, e == 0 ? -12.0f : 0.0f, 0.07f, true, 0.0f };
        eng.drift = 0.3f;
        eng.unisonOn = true;
        eng.unisonMode = UnisonClassic;
        eng.unisonVoices = 4;
        eng.detune = 0.3f;
        eng.stereo = 0.8f;
        eng.volume = 0.6f;
    }
    p.filter[0] = { true, LP24, 1800.0f, 0.25f, 0.1f, true, 0.3f, 1.0f, 0.0f };
    p.ampEnv.attack = 0.8f;
    p.ampEnv.release = 2.0f;
    p.maxVoices = 8;
    return p;
}

// Supersaw lead-ish layer.
inline SynthParams supersaw()
{
    SynthParams p;
    auto& eng = p.engine[0];
    eng.osc[0].wave = WaveSaw;
    eng.unisonOn = true;
    eng.unisonMode = UnisonSuper;
    eng.detune = 0.35f;
    eng.superMix = 0.7f;
    eng.stereo = 1.0f;
    p.filter[0] = { true, LP12, 6000.0f, 0.1f, 0.0f, true, 0.0f, 1.0f, 0.0f };
    return p;
}

// Deliberately heavy: everything on, 16 voices, unison 8, sync + FM, two 36 dB filters.
inline SynthParams worstCase()
{
    SynthParams p;
    for (int e = 0; e < 2; ++e)
    {
        auto& eng = p.engine[e];
        eng.on = true;
        for (int o = 0; o < 3; ++o)
            eng.osc[o] = { o == 2 ? WaveSine : WaveSaw, 0.5f, 0.6f, (float) (o * 7), 0.03f, true, o < 2 ? 0.2f : 0.0f };
        eng.sync = true;
        eng.noiseVolume = 0.1f;
        eng.modSource = 0.3f;
        eng.unisonOn = true;
        eng.unisonVoices = 8;
        eng.detune = 0.3f;
        eng.drift = 0.5f;
        eng.filterMix = 0.5f;
    }
    p.filter[0] = { true, LP36, 2500.0f, 0.5f, 0.5f, true, 0.5f, 1.0f, -0.3f };
    p.filter[1] = { true, HP36, 200.0f, 0.3f, 0.2f, true, 0.0f, 1.0f, 0.3f };
    p.filterRouting = 0.5f;
    p.maxVoices = 16;
    return p;
}
// Wavetable pad: 2 wavetable engines with unison, morph position, LP24.
inline SynthParams wavetablePad()
{
    SynthParams p;
    auto& lib = ContentLibrary::get();
    for (int e = 0; e < 2; ++e)
    {
        auto& eng = p.engine[(size_t) e];
        eng.on = true;
        eng.type = EngineWavetable;
        eng.wt.table = lib.factoryWavetable (e == 0 ? 30 : 15); // Warm Pad, Vowels
        eng.wt.position = e == 0 ? 0.4f : 0.2f;
        eng.unisonOn = true;
        eng.unisonVoices = 4;
        eng.detune = 0.3f;
        eng.stereo = 0.8f;
        eng.volume = 0.6f;
    }
    p.filter[0] = { true, LP24, 2500.0f, 0.2f, 0.0f, true, 0.3f, 1.0f, 0.0f };
    p.ampEnv.attack = 0.8f;
    p.ampEnv.release = 2.0f;
    return p;
}

// Analog saw + breath noise + sub oscillator (Utility engine).
inline SynthParams withUtility()
{
    SynthParams p;
    auto& lib = ContentLibrary::get();
    p.utility.noise[0].sample = lib.factoryNoise (19); // Breath
    p.utility.noise[0].volume = 0.4f;
    p.utility.noise[1].sample = lib.factoryNoise (29); // Chiff (one-shot)
    p.utility.noise[1].loop = false;
    p.utility.noise[1].volume = 0.5f;
    p.utility.oscVolume = 0.5f;
    p.filter[0] = { true, LP24, 4000.0f, 0.1f, 0.0f, true, 0.0f, 1.0f, 0.0f };
    return p;
}

// Wavetable engine FM'd by an analog engine (cross-mod), plus phase transform and fold.
inline SynthParams crossMod()
{
    SynthParams p;
    auto& lib = ContentLibrary::get();
    auto& wt = p.engine[0];
    wt.type = EngineWavetable;
    wt.wt.table = lib.factoryWavetable (0);
    wt.wt.position = 0.3f;
    wt.modFromEngine = true;
    wt.wt.fmAmount = 0.3f;
    wt.wt.pdAmount = 0.4f;
    wt.wt.pdShape = PdSkew;
    wt.wt.foldAmount = 0.3f;
    auto& an = p.engine[1];
    an.on = true;
    an.volume = 0.0f; // heard only through the cross-mod
    an.coarse = 7.0f;
    an.osc[0].wave = WaveSine;
    return p;
}

// Full wavetable feature load: mod oscillator driving PM, PD and fold, unison 8, two engines.
inline SynthParams wavetableHeavy()
{
    SynthParams p = wavetablePad();
    for (auto& eng : p.engine)
    {
        eng.unisonVoices = 8;
        eng.wt.pmAmount = 0.3f;
        eng.wt.pdAmount = 0.3f;
        eng.wt.pdMod = 0.2f;
        eng.wt.foldAmount = 0.2f;
        eng.wt.foldMod = 0.2f;
        eng.wt.fmAmount = 0.2f;
        eng.wt.modRatio = 2.0f;
    }
    p.maxVoices = 16;
    return p;
}
// Analog pad with a typical amount of modulation (6 routes, mixed poly / global sources).
inline SynthParams modulatedPad (ModRouting& r)
{
    SynthParams p = pad();
    p.lfo[0].rate.hz = 0.3f;
    p.lfo[1].retrig = RetrigFreeRun;
    p.lfo[1].rate.hz = 5.0f;
    p.random[0].type = RandRandom;
    p.random[0].smooth = 0.8f;
    p.comb[0] = { CombSum, SrcLfo1, SrcLfo2, 0.3f, 0, FollowVoice, 1.0f, 0.0f, 0.01f, 0.2f };
    p.env2.decay = 1.5f;
    auto add = [&r] (int s, const char* dest, float amt, int sc = -1) {
        r.routes[r.count++] = { s, findModDest (dest), amt, sc, 1.0f };
    };
    r.count = 0;
    add (SrcLfo1, "f1_cutoff", 0.15f);
    add (SrcEnv2, "f1_cutoff", 0.25f);
    add (SrcLfo2, "e1_fine", 0.02f, SrcModWheel);
    add (SrcRand1, "e2_uni_detune", 0.1f);
    add (SrcComb1, "f1_reso", 0.2f);
    add (SrcVelocity, "f1_drive", 0.2f);
    r.prepare();
    return p;
}
} // namespace cs::patches
