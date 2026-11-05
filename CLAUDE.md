# Church Synth: notes for Claude

Read SPEC.md first: it is the source of truth for features and holds the decisions log.

## Who / what
- Owner: Reyffer (church FOH engineer, London). Live use: MainStage on an M1 MacBook Pro 16 GB, several instances per concert. Sound design on a Windows PC (VST3).
- Goal: Pigments-like architecture, lighter on CPU, never glitches live.

## Hard rules
- **Audio thread:** no allocation, locks, file I/O, logging or exceptions. All buffers are fixed-size (see `kBlock`, `kMaxVoices`, `kMaxUnison`).
- **Parameter IDs are permanent.** Never rename or remove an ID in `source/plugin/Parameters.cpp`; saved MainStage concerts depend on them. Add new IDs instead. Macros 1–4 must stay the first parameters.
- **Original code only.** Surge XT (GPL) may be read as a DSP reference, never copied. Never copy Arturia content (wavetables, noises, presets, artwork) or use their product/effect names in the UI. Our names: Drive filter (Rage-like), Allpass filter (Ripple-like), Erode (Corroder-like), Vintage Chorus (Juno-style), Vintage Flanger.
- **Factory content order is permanent.** `kWavetableList` / `kNoiseList` in `ContentFactory.cpp` are indexed by host parameters: append only, never reorder or remove. Content can be retuned, but don't change a table's character so much that saved presets sound wrong.
- **Never put `SynthCore`, `Voice` or the processor on the stack** (they're ~1 MB; Windows' default stack is 1 MB). Use `std::make_unique`.
- DSP core (`source/dsp`) stays JUCE-free so it can be tested and benchmarked anywhere.
- Every DSP change: `ctest` passes and `cs_bench` doesn't regress without a reason.

## Build / verify
```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release   # add -DFETCHCONTENT_SOURCE_DIR_JUCE=<path> to reuse a local JUCE
cmake --build build
ctest --test-dir build --output-on-failure   # DSP tests + plugin state test (needs a display on Linux: xvfb-run)
./build/cs_bench [renders_dir]
```
CI (`.github/workflows/build.yml`): macOS universal AU/VST3/Standalone with auval and pluginval (strictness 7); Windows VST3/Standalone with pluginval. The Apple Silicon benchmark is posted to the run summary.

## Architecture
- `SynthCore` → 16 `Voice`s → per voice: 2 engine slots (each `AnalogEngine` or `WavetableEngine`) + `UtilityEngine` → filter-mix split → 2 `MultiFilter`s (15 models incl. `ClassicFilter`; series↔parallel or Split) → amp `Envelope` → pan → buses Main / FX A / FX B / Direct → `FxRack` (in `SynthCore::renderChunk`; `fx_on` off = dry with a 20 ms crossfade) → master volume × 2 (+6 dB output stage) → peak limiter at −1 dBFS. Utility osc can bypass the filters (post buffer → its routed bus).
- Filter models: `MultiFilter.h` (controls per model documented at the top and in SPEC §4.4). Drive uses `AutoGainSaturator` (MathUtil.h) before and after the filter so it changes character, not level. `FilterType` order is append-only. Feedback paths inside filters must stay bounded under audio-rate FM (soft-clip them).
- Cross-mod: an engine's modulator can be the other engine's pre-volume output; the source engine renders first (mutual cross-mod uses the previous 16-sample block).
- Content (`Content.h/.cpp`, `ContentFactory.cpp`): 40 wavetables + 34 noises synthesised in code, mip-mapped (11 levels), generated lazily and cached process-wide. The plugin's `Loader` thread resolves selections and user files into atomic pointers; the audio thread only reads pointers.
- User files are copied to `Documents/Church Synth/Wavetables|Noises` and referenced by file name in the plugin state.
- Modulation (`ModSources.h/.cpp`, `ModMatrix.h/.cpp`): per control block each voice computes only the sources something uses (`GlobalMod::used`), copies `SynthParams` into `local`, adds route offsets in each destination's normalised (knob) space, then renders with `local`. LFO/Function/Random in Mono/Legato/Free-running retrig use a single global instance in `SynthCore`; Poly Kbd uses per-voice instances. Source *settings* (e.g. LFO rate) read last block's modulated values.
- Destinations use host parameter IDs; routes are saved by source name + destination ID in the plugin state (`ROUTES` child) and handed to the audio thread through a seqlock. Function/remap preset lists and sync divisions are append-only.
- FX (`source/dsp/fx`): `FxRack` runs buses A / B / Aux (3 slots each) on the summed voice buses; `Effects.cpp` holds the 20 effects (Rotary appended last) and the `kFxInfo` table (names, controls, defaults). `FxType` order is append-only. Effects are allocated on the message thread by `FxHost` (plugin timer / prepareToPlay / state load), published through atomic pointers and freed two render epochs later; the rack skips a slot whose effect doesn't match its type yet. FX modulation destinations are `global` (evaluated once per chunk in `SynthCore`). Echo/reverb/chorus effects use equal-power Dry/Wet; a test keeps default settings within ±1.5 dB.
- Seq/Arp (`SeqArp.h/.cpp`): owned by `SynthCore`. With `seq.on`, keys go to the sequencer and it plays voices through `SeqSink` (sample-accurate: `SynthCore::render` renders up to each event). Timing is kept in steps; host playing → follows ppq. The pattern (`SeqPattern`) is plugin state (`SEQ` child) handed over through a seqlock; random offsets are hashed from a seed (`seqResolve`), so the editor can preview them. Track, scale, auto-regen and divider lists are append-only.
- Plugin state (`createStateTree` / `restoreStateTree` in the processor) is one ValueTree: APVTS params + `ROUTES` + `SEQ` + `FUNCS` (drawn function shapes) + `MIDIMAP` + macro names + user file names + preset name. Presets (`PresetManager`, `.csp` XML) are the same tree without `MIDIMAP`; factory presets/patterns/FX presets are built in code (`FactoryPresets.cpp`, `PresetBuilder` sets real-unit values by param ID). Undo = snapshots of that tree taken on parameter gesture ends from the editor and on explicit `checkpoint()` calls.
- Editor (`source/plugin/ui/`): drawn at 1280×800 and scaled (fixed aspect). `UiContext` holds the selected mod source and route helpers; `Knob`/`Pill`/`Toggle`/`Stepper` bind to params. Pages: `SynthPage`, `FxPage`, `SeqPage`; `ModStrip` + `ModTabs`; `Header` (top bar, footer); `PresetBrowser`. Live values come from `SynthCore::uiSource` atomics, published only while an editor is open. The state test can screenshot every page (`cs_state_test --snapshot <dir> [preset]`) and render the factory set (`--render-presets <dir>`, `--preset-report`).
- Factory presets are level-matched (peak ≈ −4 dBFS for a 4-note chord, ≈ −7 dBFS for mono). After changing DSP gain staging run `cs_state_test --level-report`: it plays 12 dB down (below the limiter) and prints each preset's true peak and the `master_vol` that hits the target; `--preset-report` shows the post-limiter levels.
- Preset packs (`packs/`): downloadable user presets, built in code with the same helpers as the factory set (`source/plugin/PresetHelpers.h`), compiled only into `cs_state_test`. `cs_state_test --export-pack <dir>` writes the `.csp` files, level-matched (three quiet runs, loudest kept); `--render-pack <dir> <wavdir>` renders an audition WAV per preset. `PresetBuilder::errors` lists unknown IDs / menu items / routes; the state test fails on any. Each pack has a tag table (`pack1Tags`); factory tags are in `factoryPresetTags` (FactoryPresets.cpp). User tag edits live in `Documents/Church Synth/Preset Tags.xml` keyed by `PresetInfo::key()`.
- Editor details worth knowing: clicking anywhere but a knob or the mod strip leaves mod-edit mode (root `mouseDown` via `addMouseListener`); discrete FX controls are listed in `fxChoices` (FxPage.cpp) and shown as menus; `EqGraph` computes the Param EQ curve from the real effect's impulse response; `EnvDisplay` points are draggable. The logo is `resources/images/lotus.png` (white alpha mask, tinted when drawn).
- Control rate = 16 samples (pitch, filter coefficients, envelope points); audio rate for oscillators, FM and filtering.
- Oscillators use polyBLEP/polyBLAMP with one sample of latency, so hard sync is band-limited exactly.
- Voice stealing does a 3 ms fade before the new note starts. Retriggering a sounding voice never resets oscillator phase.

## Milestones (SPEC §9)
1. ✅ Core voice: Analog engine, Classic filter, amp env, voicing, CI, benchmark.
2. ✅ Wavetable + Utility engines, cross-mod, generated wavetables/noises, WAV import.
3. ✅ Modulation system (envs, LFOs, functions, random, combinators, macros, routing, sidechains).
4. ✅ All filter types + Sum/Split routing.
5. ✅ FX buses and all effects.
6. ✅ Seq/Arp.
7. ✅ Presets/browser + Pigments-style UI (dark only).

## Performance notes
- Benchmark baseline (x86 container, M1 numbers are in CI): analog pad 8 voices ≈ 5.5 %, wavetable pad ≈ 3.5 %, worst case ≈ 29 % of one core. Content generation for everything ≈ 2.4 s total, but only what's selected is generated.
- Seq/Arp cost is negligible (arp 16ths + delay + reverb on a supersaw ≈ 3 % of a core).
- FX cost (same container): chorus + delay + reverb ≈ 1.5 % of a core; all 9 slots busy ≈ 8 %. The bench has `pad_fx_*` cases.
- Planned optimisation: process 4 unison sub-voices with SIMD (NEON/SSE), skip silent engines entirely (already done), cheaper sine.
