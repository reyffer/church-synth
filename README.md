# Church Synth

A lean, reliable hybrid synth for live worship, built to run several instances in MainStage on an M1 MacBook Pro. Its architecture is modelled on Arturia Pigments (2 engines + utility, 2 filters, deep modulation, 3 FX buses, seq/arp), cut down to what we actually use. The full feature list is in [SPEC.md](SPEC.md).

**Status: Milestone 7 (all planned milestones done).** Two engine slots (Analog or Wavetable, with cross-mod), Utility engine, 40 generated wavetables and 34 noises, user WAV import, 2 filters with 15 models each, the full modulation system (Env 2/3, 3 LFOs, 3 drawable Functions, 3 Random generators, 3 Combinators, 4 Macros, a 64-route matrix with sidechains), the FX section (buses A, B and Aux, 19 effects), a 16-step sequencer / arpeggiator, and a Pigments-style editor with a preset browser, 42 factory presets aimed at worship, undo/redo, MIDI learn and drag-and-drop modulation.

## Using it

- **Top bar:** menu, preset name (click for the browser), favourite, save, previous / next preset, the SYNTH / FX / SEQ pages, FX and Seq power buttons, and master volume with an output meter. The output has a peak limiter at −1 dBFS.
- **Modulation:** click a source in the coloured strip (LFO 1, Env 2, M 1...) and drag any knob to set how much it moves that knob; double-click a knob to remove that source from it. You can also drag a source tile onto a knob. Coloured rings show what is modulating each knob. Click **Done** (footer), press Esc or click any empty area to go back to normal knob editing. **Mod routes** in the footer lists everything (and sidechains).
- **Envelopes:** drag the points on the curves (attack, decay / sustain, release); Shift for fine moves, double-click to reset.
- **FX:** choices like Distortion Type are menus; Param EQ has a curve with draggable points (mouse wheel = Q).
- **Macros:** 4 big knobs bottom right. Double-click a macro's name to rename it. Factory presets use M1 Brightness, M2 Space, M3 Motion, M4 Swell.
- **MIDI learn:** right-click any knob or button, choose MIDI learn, move a controller. Mappings are saved with the MainStage patch (not in presets).
- **Undo / redo:** footer buttons, History menu, or Cmd+Z / Cmd+Shift+Z.
- **Presets:** saved as `.csp` files in `Documents/Church Synth/Presets` (copy that folder between Mac and PC). Sequencer patterns and FX presets live next to them.
- **Window size:** drag the corner, or menu > Window size.

## Getting a build

Every successful build is published to the **Latest build** release:
**https://github.com/reyffer/church-synth/releases/tag/latest**. Download the zip you need from there.

(Each build run's page under **Actions** also shows the CPU benchmark measured on an Apple Silicon machine.)

### Mac (MainStage / Logic)

1. Download and unzip `ChurchSynth-AU-mac.zip` and copy **Church Synth.component** to `~/Library/Audio/Plug-Ins/Components/`.
2. The build is not notarised by Apple, so clear the download flag once in Terminal:
   ```
   xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/"Church Synth.component"
   ```
3. Restart MainStage. It appears under **AU Instruments → Reyffer → Church Synth**.

Updating: quit MainStage, replace the component, run step 2 again, reopen.

### Windows (VST3, for sound design)

Download and unzip `ChurchSynth-VST3-windows.zip`, then copy **Church Synth.vst3** to `C:\Program Files\Common Files\VST3\`.

`ChurchSynth-Standalone-windows.zip` has a standalone `.exe` (no DAW needed). It opens all MIDI inputs automatically. On Windows, a MIDI device can only be used by one program at a time, so close any DAW that is holding the controller first.

## Your own wavetables and noises

In the editor, open an engine's wavetable menu (or a noise menu on the Utility tab) and choose **Load your own...**. The file is copied to `Documents/Church Synth/Wavetables` or `.../Noises`, and the patch remembers it by file name. Copy that folder to another computer and the same patches load there too.

- Wavetables: WAV/AIFF/FLAC, one single cycle per 2048 samples, up to 256 frames (the usual wavetable format; longer files are sliced into frames).
- Noises: any sample up to 20 s, mono or stereo.

## Building locally

Requires CMake ≥ 3.22 and a C++20 compiler. JUCE is fetched automatically.

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build          # DSP tests
./build/cs_bench renders/       # CPU benchmark + WAV renders
```

`-DCS_BUILD_PLUGIN=OFF` builds only the DSP core, tests and benchmark (no JUCE needed).

## Layout

```
source/dsp/      DSP core: plain C++, no JUCE (oscillators, filters, envelopes, voices)
source/plugin/   JUCE plugin: parameters, processor, presets (FactoryPresets.cpp), editor (ui/)
resources/       Embedded font (Inter, SIL Open Font License)
tests/           DSP tests, CPU benchmark, reference patches
SPEC.md          Feature spec and decisions log
```

## Licensing

Original code. Built with [JUCE](https://juce.com) 8 (AGPLv3 / commercial dual licence). Private, non-commercial use for our church is fine under either. Revisit the licence before ever distributing builds outside the team.
