# Church Synth

A lean, reliable hybrid synth for live worship, built to run several instances in MainStage on an M1 MacBook Pro. Its architecture is modelled on Arturia Pigments (2 engines + utility, 2 filters, deep modulation, 3 FX buses, seq/arp), cut down to what we actually use. The full feature list is in [SPEC.md](SPEC.md).

**Status: Milestone 6.** Two engine slots (Analog or Wavetable, with cross-mod), Utility engine, 40 generated wavetables and 34 noises, user WAV import, 2 filters with 15 models each (Classic, Cluster, Phaser, Formant, Surgeon, LoFi, Comb, Drive, Allpass, Ladder, Sallen-Key, OTA 4-Pole, SVF Morph, Multimode, Lowpass Gate) with audio-rate filter FM and Sum/Split output, amp envelope, and the full modulation system: Env 2/3, 3 LFOs, 3 Functions, 3 Random generators, 3 Combinators, 4 Macros, a 64-route matrix with sidechains, aftertouch and tempo sync, plus the FX section: buses A and B (3 slots each, serial or parallel), an Aux send bus (3 slots) and 19 effects (Delay, Tape Echo, Pitch Shift Delay, Reverb, Shimmer, Compressor, Multiband, Multi Filter, Param EQ, Distortion, Bitcrusher, Erode, Super Unison, Chorus, Vintage Chorus, Flanger, Vintage Flanger, Phaser, Panner). A 16-step sequencer / arpeggiator (6 tracks with polymetry, probability, slide, scales, random with Regen/Auto-Regen, host sync and Hold latch) completes the sound engine. The editor is temporary (tabs: Sound, Modulation, FX, Seq/Arp, Files); the Pigments-style UI comes in Milestone 7.

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

In the editor, use **Load file...** next to an engine's wavetable or a noise slot. The file is copied to `Documents/Church Synth/Wavetables` or `.../Noises`, and the patch remembers it by file name. Copy that folder to another computer and the same patches load there too.

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
source/plugin/   JUCE plugin wrapper: parameters, processor, editor
tests/           DSP tests, CPU benchmark, reference patches
SPEC.md          Feature spec and decisions log
```

## Licensing

Original code. Built with [JUCE](https://juce.com) 8 (AGPLv3 / commercial dual licence). Private, non-commercial use for our church is fine under either. Revisit the licence before ever distributing builds outside the team.
