# Church Synth — Feature Spec (v0.2)

Name: **Church Synth**.
A lean, reliable polysynth built for Reyffer's MainStage rig: M1 MacBook Pro, 16 GB, several instances per concert, pads, keys, plucks, leads and sequences for Saturday services.

Reference: Arturia Pigments 7.0.1 (manual + screenshots in this folder). We take its **architecture and workflow** as the model. We do not copy its code, factory content, UI artwork or brand names. Where this spec says "our take on X", we build our own version from the idea, under our own name.

---

## 1. Goals and constraints

1. **Light on CPU.** Target: a typical 8-voice pad (2 engines, unison 3, 1 filter, chorus + delay + reverb) under about half of what the same patch costs in Pigments on the M1, measured with MainStage's per-core meter.
2. **Reliable live.** No allocation, file I/O or locks on the audio thread. Preset changes never click or glitch. Never crash the host. Tails ring out across MainStage patch changes.
3. **Tailored.** Only what we use. Full exclusion list in §10. Within the sections we keep, any filter or effect that behaves in a *creatively different* way stays in, so it can be explored. Only true duplicates are cut.
4. **Familiar.** Same mental model as Pigments (2 engines + utility, 2 filters, tabbed mod sources, 3 FX buses, seq/arp), so existing Pigments knowledge transfers.

## 2. Signal flow

```
            ┌───────────── per voice ─────────────────────────────┐
 Engine 1 ──┤                                                     │
 (Analog|WT)│ Filter Mix ─► Filter 1 ─┐                           │
 Engine 2 ──┤                         ├─ Routing (series↔parallel)├─► Amp (Env Amp × Amp Mod) ─► Voice Pan
 (Analog|WT)│ Filter Mix ─► Filter 2 ─┘   Sum | Split             │
 Utility  ──┤  (osc can bypass filters → FX A / FX B / Direct)    │
            └─────────────────────────────────────────────────────┘
                                   │ (summed voices)
                     ┌─────────────┴─────────────┐
                FX Bus A (3 slots)        FX Bus B (3 slots)      A>B | B>A | A//B
                     └─────────────┬─────────────┘
                     Aux bus (3 slots, send/return, pre|post A/B)
                                   │
                              Master out
```

Everything up to and including the amp runs **per voice**. FX run **once per instance**.

## 3. Engines

### 3.1 Engine 1 and Engine 2 (identical slots)
Each slot is **Analog** or **Wavetable** (any combination, including two of the same). Each has: on/off, copy/paste/reset (with or without its mod routings), Volume, Filter Mix (F1↔F2).

**Engine tune (both types):** Coarse ±60 st (Shift snaps to octaves), Fine ±1 st, **Mod Quantize** (when Coarse is modulated, snap the result to chosen notes relative to the played key), Keyboard Tracking on/off. Analog also has **Drift** (per-note random tuning/phase offset).

#### Analog type
- **3 oscillators.** Waves: sine, triangle, saw, ramp, square. Width (PW) for triangle and square. Coarse (all), Fine in st or Hz (Osc 2 and 3), key-track on/off (Osc 2 and 3), Volume (all).
- **Hard sync** Osc 2 → Osc 1.
- **FM** into Osc 1 and Osc 2 from the Modulator.
- **Noise** with colour (red ↔ white ↔ blue) and volume.
- **Modulator:** crossfade Osc 3 ↔ Noise as the FM source, *or* switch to the other engine as the source (cross-mod, §3.3).
- **Unison** (on/off), up to 8 voices:
  - *Unison:* Voices (fractional allowed), Detune (up to ±6 st), Stereo.
  - *Chord:* Voices, chord shape (12 shapes), Stereo.
  - *Super:* supersaw-style; Mix, Detune, Stereo.

#### Wavetable type
- Tables up to **256 frames × 2048 samples**. Position, Morph on/off (smooth vs stepped), 2D/3D view (low-frame-rate).
- **Freq/Ring mod:** Linear FM, Exponential FM, Ring mod, with amount.
- **Phase mod:** amount; initial phase; retrigger source: Key, Random, Free (self), Mod Osc.
- **Phase transform** (phase distortion): about 7 shapes (our own set), amount, mod amount.
- **Wavefold:** 3 fold shapes, amount, mod amount.
- **Wavetable volume** (pre-processing) and **Output** (post).
- **Unison:** same 3 modes as Analog, plus **Phase** (0 = all voices same phase for punch, 1 = random).
- **Modulation oscillator:** feeds the FM/PM/PD/fold mod knobs and can be mixed in as audio. Tune mode Ratio / Absolute / Relative / Fixed Hz, Fine, Volume. Waves: sine, tri, saw, ramp, square, and 5 noise colours (blue, white, pink, red, rumble). Or switch to the other engine as the source.
- **User wavetables:** load WAV (2048 samples per frame; longer files are sliced into frames, up to 256). Folder import, delete, missing-file "Locate / Ignore" on preset load.

### 3.2 Utility engine (always present)
- **Noise 1 and Noise 2** (identical sample-based players): browser by category, on/off, Phase (Key / Random start), key-track on/off, Tune ±36 st, tilt filter (LP ← 0 → HP), Loop on/off, Length (one-shot), Filter Mix, Volume.
- **Oscillator:** sine, tri, saw, ramp, square, Width, Coarse ±36 st (default −12, a sub), Fine, key-track, Mod Quantize, Filter Mix, Output with routing **Filters / FX A / FX B / Direct**.
- *Excluded:* external audio input on Noise 2.

### 3.3 Engine cross-modulation
Either main engine can use the other as its Modulator source (both at once is allowed). The source engine must be on; its volume knob doesn't affect the mod depth, so you can modulate with it silently. The mirrored controls depend on the source type: Analog source = Coarse, Fine, Noise, FM amount; Wavetable source = Coarse, Fine, Position, table select.

### 3.4 Content we must create ourselves
We can't ship Arturia's wavetables or noise samples. Plan:
- **Wavetables:** generated in code at build time. Basic shapes, PWM sweeps, saw↔square morphs, additive "drawbar organ", vowel/formant sweeps, bright/dark harmonic sweeps, soft "vintage" shapes. Target 40–60 tables in a few categories. Plus user import.
- **Noise:** generated in code: white, pink, red, blue, rumble, wide (stereo) variants, plus textures (vinyl crackle, tape hiss, air/breath, rain-like). Plus user WAV import (for sounds Reyffer records or downloads).

## 4. Filters

Two identical slots. Each has: type menu (Digital / Analog-modelled groups) with prev/next arrows, on/off (bypass keeps settings), Volume, Pan, **Filter FM** amount and source menu (or key-track amount on types without FM), visualiser with drag (X = cutoff, Y = resonance), copy F1↔F2, swap.

**Decision:** every type below stays. Each one has its own behaviour, not just a different curve. Cluster gives multi-peak resonances, Surgeon is ultra-steep and clean, LoFi does sample-rate grit, Comb gives plucked/metallic tones, Drive and Allpass bring nonlinear feedback, and each analog model has its own saturation and resonance character. Build priority is lower for Cluster, Surgeon and Multimode 4-pole.

### 4.1 Digital types
| Type | Modes / notes | Key params |
|---|---|---|
| **Classic** | LP 6/12/24/36, BP 12/24/36, HP 6/12/24/36, Notch 12/24/36, AP 12/24/36 | Cutoff, Reso, Drive, Digital↔Analog resonance |
| **Cluster** | Peak / BP / Notch with 1–5 peaks; LP+HP | Cutoff, Reso, Spread, Peaks |
| **Phaser** | 2–12 poles | Cutoff, Feedback, Poles |
| **Formant** | vowel morph | Freq shift, Morph, Q, Blend |
| **Surgeon** | very steep LP / HP / BP / Notch | Cutoff, Spread (BP/Notch) |
| **LoFi** | downsampling filter | Freq, Jitter, pre/post filter on/off + freq |
| **Comb** | LP / HP / BP / feed-forward | Freq, Gain, Kbd, Damping, All-pass |
| **Drive filter** *(our take on Rage)* | 5 saturation characters in the resonance loop: diode, overdrive, soft-clip, tape, transistor | Cutoff, Reso, Drive |
| **Allpass filter** *(our take on Ripple)* | Subtle / Medium / Hard (more all-pass stages) | Cutoff, Reso, Dry, Mode (shaper / +fb / −fb), Drive, Feedback |

*Excluded:* Reverb filter.

### 4.2 Analog-modelled types (generic circuit models, our own names)
| Type | Inspired by | Modes |
|---|---|---|
| **Ladder 24** | classic transistor ladder | LP24, with Drive |
| **Sallen-Key** | compact patchable mono synth | LP12, HP6 |
| **OTA 4-pole** | early-80s Japanese poly | LP12, LP24 |
| **SVF Morph** | 70s state-variable expander module | LP ↔ Notch ↔ HP morph, BP at min |
| **Multimode 4-pole** | 80s flagship poly | 7 modes |
| **Lowpass Gate** | vactrol LPG | LPG / VCA / Both; Level, Time |

Analog types accept **audio-rate FM** from the engines' modulators or outputs.

### 4.3 Routing and amp
- **Filter routing** knob: F1→F2 (series) ↔ F1//F2 (parallel), with a continuous blend.
- **Sum / Split:** Sum feeds both filters to the FX section. Split sends F1→FX A and F2→FX B (or reversed), parallel only.
- **Amp Mod:** source menu (any of the 24 sources, Env Amp is always applied anyway) + amount.
- **Voice Pan**, **Send Level** (same parameter as the Aux Send on the FX page).

## 5. Modulation

### 5.1 Sources (24, colour-grouped in a strip like Pigments)
Velocity, Aftertouch, Mod Wheel, Keytrack, Expression (CC11) · Env Amp, Env 2, Env 3 · LFO 1–3 · Function 1–3 · Random 1–3 · Combinator 1–3 · Macro 1–4.

### 5.2 Routing
- Drag a source onto any knob, **or** use a source view (all its destinations) or a target view (all sources hitting one knob).
- Each route has an **amount** and an optional **sidechain** (a second source scaling that route, e.g. mod wheel scales LFO → pitch).
- Coloured mod rings on knobs; hover "quick edit" mini-knobs to adjust existing routes.
- Bipolar/unipolar handled per source.
- Per-voice evaluation; see §9 for how we keep this cheap.

### 5.3 Envelopes (3)
Env Amp is fixed to the amp and retriggers per key; Env 2 and 3 have a selectable retrigger source. Attack 1 ms–20 s, Decay 1 ms–20 s, Sustain, Release, Attack curve, Decay curve (log ↔ lin ↔ exp), Release-link to Decay, **ADSR / ADR** mode, S-shaped default attack/release (no clicks), drag breakpoints in the graph.

### 5.4 LFOs (3)
Waves: sine, triangle, square, saw, ramp, S&H. Symmetry, Rate (Hz / Sync / Straight / Triplet / Dotted), Phase, one of **KeyTrack** (±200 %) / **Fade** (to 20 s) / **Smooth**, Retrig source, uni/bipolar. Quick shape presets.

### 5.5 Functions (3)
Drawable multi-point curves (up to 64 points), curve handles per segment. Draw modes: free, steps, ramp up, ramp down; curve toggle; grid (X × Y) with snap. Modes: **Loop, One-shot, Envelope** (sustain point), **Envelope-Loop** (loop markers). Rate with sync options, Scale (bipolar, can invert), Smooth, polarity, retrig source. Shift left/right, ×2, copy to another Function, 6 basic shape buttons, user presets.

### 5.6 Random (3 slots, each one of)
- **Random:** Rate, Smooth, Distance, Jitter, polarity, retrig.
- **Voice Modulator:** 1–8 steps, new value per voice; modes Cycle / Reassign / Random; randomise, reset, polarity. (Great for analog-style per-voice variation on pads.)
- **Sample & Hold:** sample-from source, Rise, Fall, Link, Rate, retrig.
- **Turing:** Rate, Flip, Length (1–64), retrig.
- **Binary:** Rate, Probability, Correlation, retrig.

### 5.7 Combinators (3)
Source, Mod, Type, Amount. Types: **Sum, Diff, Multiply, Divide, Crossfade, Lag, Threshold, Offset, Remap** (drawable curve), **Envelope Follower** (Input gain, Threshold, Attack, Release; taps an internal signal). Output clamped to ±1.

### 5.8 Macros (4)
Named, unipolar, assignable to many targets (with sidechains). Exposed as the first 4 host parameters so MainStage knobs map to them instantly.

### 5.9 Keyboard tab
Hold (acts like sustain, also latches seq/arp), Transpose (octave, fine; fine is a mod target), Glide time (to 10 s) + Legato-only, Voicing **Mono / Mono Legato / Poly Reassign / Poly Rotate** + max voices, pitch-bend Up and Down ranges set separately + "bend affects release" toggle, editable **Velocity, Aftertouch and Keytrack curves** (2 movable mid-points).

## 6. Effects

### 6.1 Structure
- **FX A** and **FX B** insert buses, **3 slots each**. Routing **A>B, B>A, A//B**, bus volumes A and B.
- **Aux bus**, 3 slots, Send / Return, **pre or post** A/B.
- Every slot: effect type menu, on/off, per-effect presets (factory + user save/delete), drag to swap, copy to another slot. Bus on/off.
- All time-based parameters sync to host tempo (straight / triplet / dotted).

### 6.2 Effect list
| Group | Effect | Key params |
|---|---|---|
| Spatial | **Delay** | Time/sync, Fine ±30 ms, Feedback, HP, LP, Width, Ping-pong, Dry/Wet |
| | **Tape Echo** | Input drive, Time/sync, Fine, Intensity, Width, Ping-pong, Dry/Wet |
| | **Pitch-shift Delay** | Time/sync, Pitch, Harmonize (normal/oct up/oct down), Feedback, Stereo offset/detune, Spray, HP, LP, Dry/Wet |
| | **Reverb** | Pre-delay, Size, Decay, Damping, Input HP/LP, Width, Dry/Wet. Presets: Room, Hall, Cathedral, Slap, Echoes |
| | **Shimmer** | Pitch, Feedback, Size, Modulation, HP, LP, Ducking, Width, Harmonize, Dry/Wet |
| Dynamics | **Compressor** | Threshold, Ratio, Attack, Release, Makeup (auto), Output, GR meter, Dry/Wet |
| | **Multiband** | 3 bands, comp + expansion bars, 2 crossovers, band on/off, Input, Amount, Attack, Release, band outputs, Output |
| Filter/EQ | **Multi Filter** | LP/HP/BP 12/24/36, Notch, Comb FB/FF; Freq, Q, Dry/Wet |
| | **Param EQ** | 5 fully parametric bands (Q on shelves too), Scale, curve with drag |
| Distortion | **Distortion** | 16 types (our own set, e.g. soft clip, hard clip, tube, tape, overdrive, fuzz, crunch, diode, fold, sine fold, half-rectify, full-rectify, asymmetric, bit-shape, saturate, destroy), Drive, Auto gain, Out, filter pre/post (type, cutoff, reso, slope 12/24/36), Dark (fixed 6 kHz LP) |
| | **Bitcrusher** | Bit depth 1.5–16, Downsample 1–80×, Scale, Jitter, Smooth, HP, LP, Dry/Wet |
| | **Erode** *(our take on Corroder)* | Short delay FM'd by a selectable source (sine/tri/saw/square/noise, an engine, a filter output), Noise blend, band-pass Freq + Bandwidth on the mod signal, Spread, Drive, Amount |
| Modulation | **Super Unison** | Voices, Detune, Rate, Width, Retrig, HP, LP, Dry/Wet. *Processes the summed output: a cheap alternative to per-voice unison.* |
| | **Chorus** | Delay, Depth, Rate, Feedback, Voices, sine/square LFO, Stereo, Dry/Wet |
| | **Vintage Chorus** *(BBD, Juno-style)* | Modes I / II / I+II, Rate (Hz/sync), Depth, Phase, Dry/Wet |
| | **Flanger** | Delay, Depth, Rate/sync, Feedback, HP, LP, Negative, Stereo, sine/tri |
| | **Vintage Flanger** *(our take on the 70s-flanger model)* | Rate (Hz/sync), Delay, Feedback, Depth, Wide, Mono input |
| | **Phaser** | Freq, Feedback, LFO wave (6), Amount, Rate/sync, Poles, Stereo, Dry/Wet |
| | **Panner** | Amount, Rate/sync, Invert, Mono-bass + cutoff, Natural/Linear |

**Decision:** all effects stay except the Vocoder. It was dropped together with external audio input, and without a mic or track input it has little to work with.

## 7. Sequencer / Arpeggiator

- On/off, **Seq / Arp** mode, **Lock** (keep the pattern when changing presets), clear.
- **Pattern browser:** Factory / From preset / User; save patterns separately from presets.
- **16 steps**, adjustable length 1–16; **Polymetry** (per-track lengths) + Reset (realign after N steps).
- **Tracks:** On/Off + Probability, Pitch (seq only, 1 octave, track transpose is a mod target), Velocity (step vs incoming blend %), Octave ±2, Gate 5–400 %, Slide (ms).
- Per track: random amount (dice), rate divider (½ to 1/16), reset, rotate ◀ ▶, per-step random lock, Shift-drag to scale a whole track.
- **Playback:** forward, backward, ping-pong, random; Rate (BPM / Sync / Straight / Triplet / Dotted); Swing 50–75 %.
- **Generation:** Regen (dice), Auto-regen (1/16 bar to 8 bars), clear random, apply random.
- **Scales:** Chromatic, Major, Natural/Harmonic/Melodic Minor, Dorian, Phrygian, Lydian, Mixolydian, Locrian, Major/Minor Pentatonic, Blues, Fifth, plus generative scales and **Custom** with per-note probability weights; Snap-to-scale.
- **Arp:** As Played, Up, Down, Up&Down incl., Up&Down excl., Random; chord-arp when an engine uses Unison Chord mode. Hold latches.
- Follows MainStage tempo, so a patch change that sets BPM also sets the seq/arp.

## 8. Presets, browser, host

- **Preset browser:** banks (Factory / User), categories (Pad, Keys, Pluck, Lead, Bass, Strings/Brass, Texture, Sequence, FX), search, favourites, Init preset, prev/next arrows, save / save as / delete.
- **Undo / redo / history.**
- **Host parameters:** Macros 1–4 first, then a curated list (cutoffs, engine volumes, FX dry/wet, aux send). MIDI learn for anything else.
- **Play Mode** (polyphony) in the footer, plus a **CPU meter**.
- **Formats:** **AU** (Mac, MainStage/Logic) and **VST3** (Mac + Windows, for sound design on the Windows PC), built from the same code from Milestone 1. AU is the one we test against for live use. **Presets are our own file format** (not host state), so a preset saved on Windows loads identically in MainStage.
- **UI:** **the same layout as Pigments** (top bar, engine tabs, filter column, routing/amp column, colour-coded mod-source strip, tabbed mod section, macros panel, FX page, Seq page), **dark only**. It should feel the same to use, but with our own artwork: knobs, fonts, colours, icons and logo. Resizable. Visualisers update at a low frame rate and stop completely when the window is closed.

## 9. Engineering plan

- **Language/framework:** C++ with **JUCE** (handles AU/VST3/standalone, parameters, presets, UI).
- **From scratch**, our own code. **Surge XT** (GPL-3) is used only as a *reference* for tricky DSP (anti-aliased oscillators, filter models, BBD chorus), not copied, so our licensing stays our choice.
- **Build:** private GitHub repo. GitHub Actions builds the Mac AU + VST3 on a macOS runner and the Windows VST3 on a Windows runner, so Reyffer only downloads and installs. DSP is unit-tested offline (render a MIDI file to WAV and check level, aliasing, CPU timing) on every commit.
- **CPU techniques:**
  - Voices processed 4 at a time with SIMD (NEON on M1).
  - Modulation computed at control rate (every 16–32 samples) and smoothed. Audio-rate only where it matters (FM, filter FM, envelopes on amp).
  - Engines, filters, FX slots and mod sources that are off or unassigned cost **zero**.
  - Band-limited oscillators (polyBLEP / mip-mapped wavetables), no global oversampling. Oversampling only inside drive filters and distortion, with a quality setting.
  - Voice stealing with fast fade, idle-voice detection, denormal protection.
  - Hard caps: 16 voices × 8 unison max, with a sensible default (Poly 8).
  - Preset loading prepared off the audio thread, swapped atomically.
  - No GUI work when the editor is closed.

### Milestones (each one installable and testable in MainStage)
1. **Core voice:** Analog engine (3 osc, noise, unison), Classic filter, Env Amp, poly/mono/glide, AU build pipeline, CPU benchmark harness.
2. **Wavetable + Utility engines**, cross-mod, wavetable/noise content generator, user WAV import.
3. **Modulation system:** envelopes, LFOs, functions, random, combinators, macros, drag-and-drop routing, sidechains.
4. **Filters complete:** all types in §4, routing, Sum/Split.
5. **FX:** buses, routing, all effects in §6, effect presets.
6. **Seq/Arp.**
7. **Presets + browser + UI polish**, factory preset set aimed at worship (pads, keys, plucks, swells, ambient beds, arps).

## 10. Excluded

- Engines: Granular, Sample, Harmonic, Modal.
- Play view, preset playlists, store, tutorials and sound-design-tips panels.
- External audio input (Utility Noise 2, Modal exciters), Vocoder.
- Reverb filter type.
- Sequencer MIDI output, MPE, microtuning (MTS-ESP).
- Arturia factory wavetables, noise samples and presets. We generate our own (§3.4).

## 11. Decisions log

| Date | Decision |
|---|---|
| 2026-11-04 | Name: **Church Synth**. |
| 2026-11-04 | Build from scratch. Surge XT used only as a DSP reference, never copied. |
| 2026-11-04 | Keep Rage-style and Ripple-style filters and the Corroder-style effect. Drop the Reverb filter. |
| 2026-11-04 | Keep every filter and effect that is creatively distinct. Only true duplicates are cut. |
| 2026-11-04 | Polyphony cap 16 voices × 8 unison, default Poly 8. |
| 2026-11-04 | Drop MIDI out, MPE, microtuning. |
| 2026-11-04 | Generate all wavetables and noise sources in code (no external content). |
| 2026-11-04 | Pigments layout, our own artwork, dark only. |
| 2026-11-04 | AU + VST3 (Mac and Windows) from Milestone 1; AU is the live reference. |
| 2026-11-04 | Milestone 1 done. Standalone opens all MIDI inputs automatically (Windows standalone didn't receive MIDI). |
| 2026-11-04 | Milestone 2 done: Wavetable + Utility engines, cross-mod, 40 generated wavetables, 34 generated noises, WAV import. Phase-transform shapes are our own set: Pulse Width, Skew, Mirror, Sync, Steps, Bend, Squeeze. Wavetable/noise list order is permanent (append only). |
