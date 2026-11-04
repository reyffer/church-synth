#pragma once

// Reference patches shared by the tests and the CPU benchmark.

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
} // namespace cs::patches
