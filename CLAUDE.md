# Church Synth: notes for Claude

Read SPEC.md first: it is the source of truth for features and holds the decisions log.

## Who / what
- Owner: Reyffer (church FOH engineer, London). Live use: MainStage on an M1 MacBook Pro 16 GB, several instances per concert. Sound design on a Windows PC (VST3).
- Goal: Pigments-like architecture, lighter on CPU, never glitches live.

## Hard rules
- **Audio thread:** no allocation, locks, file I/O, logging or exceptions. All buffers are fixed-size (see `kBlock`, `kMaxVoices`, `kMaxUnison`).
- **Parameter IDs are permanent.** Never rename or remove an ID in `source/plugin/Parameters.cpp`; saved MainStage concerts depend on them. Add new IDs instead. Macros 1–4 must stay the first parameters.
- **Original code only.** Surge XT (GPL) may be read as a DSP reference, never copied. Never copy Arturia content (wavetables, noises, presets, artwork) or use their product/effect names in the UI. Our names: Drive filter (Rage-like), Allpass filter (Ripple-like), Erode (Corroder-like), Vintage Chorus (Juno-style), Vintage Flanger.
- DSP core (`source/dsp`) stays JUCE-free so it can be tested and benchmarked anywhere.
- Every DSP change: `ctest` passes and `cs_bench` doesn't regress without a reason.

## Build / verify
```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release   # add -DFETCHCONTENT_SOURCE_DIR_JUCE=<path> to reuse a local JUCE
cmake --build build
ctest --test-dir build --output-on-failure
./build/cs_bench [renders_dir]
```
CI (`.github/workflows/build.yml`): macOS universal AU/VST3/Standalone with auval and pluginval (strictness 7); Windows VST3/Standalone with pluginval. The Apple Silicon benchmark is posted to the run summary.

## Architecture
- `SynthCore` → 16 `Voice`s → per voice: 2 `AnalogEngine`s → filter-mix split → 2 `ClassicFilter`s (series↔parallel) → amp `Envelope` → pan.
- Control rate = 16 samples (pitch, filter coefficients, envelope points); audio rate for oscillators, FM and filtering.
- Oscillators use polyBLEP/polyBLAMP with one sample of latency, so hard sync is band-limited exactly.
- Voice stealing does a 3 ms fade before the new note starts. Retriggering a sounding voice never resets oscillator phase.

## Milestones (SPEC §9)
1. ✅ Core voice: Analog engine, Classic filter, amp env, voicing, CI, benchmark.
2. Wavetable + Utility engines, cross-mod, generated wavetables/noises, WAV import.
3. Modulation system (envs, LFOs, functions, random, combinators, macros, routing, sidechains).
4. All filter types + Sum/Split routing.
5. FX buses and all effects.
6. Seq/Arp.
7. Presets/browser + Pigments-style UI (dark only).

## Performance notes
- Benchmark baseline (x86 container, M1 numbers are in CI): pad 8 voices ≈ 5 % of one core; worst case (16 voices × 2 engines × 3 osc × unison 8) ≈ 29 %.
- Planned optimisation: process 4 unison sub-voices with SIMD (NEON/SSE), skip silent engines entirely (already done), cheaper sine.
