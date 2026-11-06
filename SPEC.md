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
                     FX power (whole section on/off, 20 ms crossfade)
                                   │
                    Master volume (+6 dB output stage) ─► peak limiter (−1 dBFS) ─► out
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

In the filter Type menu each model carries a one-line description of its character, so the classic types are easy to find: **Ladder** is the fat 24 dB mono-synth ladder (Mini-style), **Sallen-Key** the biting 12 dB patchable-mono-synth filter (MS-style), **OTA 4-Pole** the smooth 24 dB 80s polysynth filter (Jupiter-style). We keep our own names in the UI.

Filter **Volume** goes from −inf to +12 dB (default 0 dB) to make up the level lost to filtering. Picking **Comb** tunes it to the keys (cutoff C4, key tracking 100 %); Comb always has some feedback, so the comb peaks show even at Resonance 0.

### 4.4 Model controls (as built)
| Model | Model Mode | Control A | Control B | Control C |
|---|---|---|---|---|
| Classic | (uses Type: 17 slopes) | | | |
| Cluster | 0 Peak, 1 Bandpass, 2 Notch, 3 LP+HP | spread | number of peaks (1–5) | |
| Phaser | | poles (2–12) | | |
| Formant | | vowel morph (A-E-I-O-U) | dry/wet blend | |
| Surgeon | 0 LP, 1 HP, 2 Notch, 3 BP | width (Notch/BP) | | |
| LoFi | 0 none, 1 pre, 2 post, 3 pre+post filter | jitter | pre-filter offset | post-filter offset |
| Comb | 0 LP, 1 HP, 2 BP, 3 feed-forward | damping | all-pass (dispersion) | |
| Drive | 0 Diode, 1 Overdrive, 2 Soft clip, 3 Tape, 4 Transistor | | | |
| Allpass | 0 Subtle, 1 Medium, 2 Hard | dry level | feedback (−/+) | |
| Ladder | | | | |
| Sallen-Key | 0 LP12, 1 HP6 | | | |
| OTA 4-Pole | 0 LP24, 1 LP12 | | | |
| SVF Morph | | mode: BP → LP → Notch → HP | | |
| Multimode | 0 LP24, 1 LP12, 2 BP24, 3 BP12, 4 HP24, 5 HP12, 6 Notch | | | |
| Lowpass Gate | 0 LPG, 1 VCA, 2 Both | level (open/close) | decay time | |

**Drive on Classic and the analog models (Ladder, Sallen-Key, OTA, SVF Morph, Multimode)** is heard even with the filter closed: an asymmetric clipper before the filter (up to +18 dB, odd and even harmonics), saturation inside the resonance loop (Classic), and a clipper after the filter (grit above the cutoff). Both clippers have automatic makeup (30 ms RMS followers), so the level doesn't depend on how hot the oscillators are; full drive is about 3–4 dB louder, like pushing a real filter.

Cutoff on Formant = formant shift (1 kHz = neutral); on LoFi = sample-rate frequency; on Comb = pitch. Resonance on Phaser = feedback, on Allpass = bandwidth, on Comb = gain. Drive on Drive = amount in the feedback loop, on Allpass = shaper.

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
- **FX power** (`fx_on`, top bar beside FX): the whole section on/off; off = the dry voices, no effect processed (20 ms crossfade, no clicks).
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

### 6.3 Effect controls (as built)
Every slot has On, Effect type, Sync and 12 generic controls (`fx_<slot>_p1..p12`, slots `a1..a3`, `b1..b3`, `x1..x3`). What each control means depends on the effect (below, in order). Choosing an effect loads its defaults; Aux slots start 100 % wet. Echoes, reverbs and choruses use an equal-power Dry/Wet so inserting them keeps the loudness steady; the others use a linear one. With Sync on, the control marked † becomes a note division.

| Effect | Controls |
|---|---|
| Delay | Time†, Fine (±30 ms), Feedback, Low Cut, High Cut, Width, Ping-Pong, Dry/Wet |
| Tape Echo | Time†, Fine, Intensity, Input Drive, Wow & Flutter, Width, Ping-Pong, Dry/Wet |
| Pitch Shift Delay | Time†, Pitch (±12 st), Feedback, Harmonize (normal / + oct up / + oct down), Spray, Stereo Detune, Tone, Dry/Wet |
| Reverb | Pre-Delay, Size, Decay, Damping, Low Cut, High Cut, Width, Dry/Wet (8-line feedback delay network) |
| Shimmer | Pitch, Shimmer (pitched feedback), Size, Decay, Modulation, High Cut, Ducking, Width, Dry/Wet |
| Compressor | Threshold, Ratio, Attack, Release, Makeup, Auto Makeup, Output, Dry/Wet |
| Multiband | Low X-Over, High X-Over, Amount (down + up compression, auto make-up), Attack, Release, Low / Mid / High Out, Output |
| Multi Filter | Type (LP/HP/BP 12/24/36, Notch, Comb FF, Comb FB), Cutoff, Resonance, Dry/Wet |
| Param EQ | Low Shelf Freq/Gain, Peak 1/2/3 Freq/Gain, High Shelf Freq/Gain, Peak Q (shared), Scale |
| Distortion | Type (Soft Clip, Hard Clip, Tube, Tape, Overdrive, Fuzz, Crunch, Diode, Sine Fold, Triangle Fold, Half Rectify, Full Rectify, Asymmetric, Bit Shape, Saturate, Destroy), Drive, Auto Gain, Output, Filter (off/pre/post), Filter Type (LP/HP/BP), Cutoff, Resonance, Slope (12/24/36), Dark, Dry/Wet. 2× oversampled |
| Bitcrusher | Bit Depth, Downsample, Jitter, Low Cut, High Cut, Dry/Wet |
| Erode | Mod Type (sine/tri/saw/square/noise), Frequency, Amount, Noise Blend, Band Freq, Bandwidth, Spread, Drive, Dry/Wet |
| Super Unison | Voices (2–8), Detune (up to ±100 cents, curved), Drift, Width (voices alternate sides), Low Cut, High Cut, Dry/Wet |
| Chorus | Delay, Depth, Rate†, Feedback, Voices (1–4), Shape (sine/square), Stereo, Dry/Wet |
| Vintage Chorus | Mode (I: 0.5 Hz, gentle; II: 0.83 Hz and about 4× the pitch movement of I; I+II: 9.75 Hz, a fast shallow shimmer), Rate and Depth scale the mode's values, Phase, Dry/Wet, Width (0 = a chorus line per side, 1 = the classic sum/difference outputs: very wide in stereo, folds back to dry in mono; fades out from 50 % to 100 % wet) |
| Flanger | Delay, Depth, Rate†, Feedback (negative below centre), Low Cut, High Cut, Stereo, Shape (sine/tri), Dry/Wet |
| Vintage Flanger | Rate†, Delay, Feedback, Depth, Wide, Mono Input, Dry/Wet |
| Phaser | Frequency, Feedback, LFO Wave (6), Depth, Rate†, Poles (2–12), Stereo, Dry/Wet |
| Panner | Amount, Rate†, Invert, Mono Bass, Bass Cutoff, Natural Law, Shape (sine/tri/square-ish) |
| Rotary | Speed (Slow / Fast switch: MIDI-learn it, or route the mod wheel to it), Slow Rate (horn 0.3–1.6 Hz), Fast Rate (4–8.5 Hz), Ramp (how long the rotors take to change speed; the drum is about 4× slower than the horn), Horn/Drum balance, Drive (tube-style preamp), Spread (angle between the two mics), Dry/Wet, Brake (stops the rotors). Mono in, 800 Hz crossover, Doppler + amplitude modulation per rotor. Appended as FX type 20 |

In the editor, controls that are really choices or switches (types, modes, waves, poles, voices, Ping-Pong, Auto Gain, Dark...) are menus or on/off pills instead of knobs. Param EQ has a response curve with draggable band points (drag = frequency + gain, mouse wheel = Peak Q), with smaller knobs below.

Bus controls: `fx_on`, `fx_routing` (A>B, B>A, A//B), `fx_a_on`, `fx_b_on`, `fx_aux_on`, `fx_a_vol`, `fx_b_vol` (0–150 %), `fx_aux_send`, `fx_aux_return`, `fx_aux_pre`. Every slot control, bus volume and the aux send/return are modulation destinations (evaluated once per block, from global sources only: per-voice sources like Env 2 use the newest voice).

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

### 7.1 As built (Milestone 6)
- **Host parameters** (automatable, saved with the patch): `seq_on`, `seq_mode` (Arp / Seq), `seq_rate` (the shared sync-division list, default 1/16), `seq_swing` (50–75 %), `seq_direction` (Forward, Backward, Ping-Pong, Random), `seq_length` (1–16), `seq_polymetry`, `seq_reset` (realign after N steps, 0 = off), `seq_arp_order` (As Played, Up, Down, Up & Down Incl., Up & Down Excl., Random), `seq_arp_octaves` (1–4), `seq_vel_blend` (played ↔ step velocity), `seq_transpose` (±24 st), `seq_scale`, `seq_snap`, `seq_autoregen` (Off, 1/16 bar … 8 bars), `seq_bpm` (tempo used only when the host gives none, e.g. standalone; also drives synced LFOs/FX there), `seq_lock` (used by the preset browser in M7). Mod destinations: Seq Transpose, Seq Swing, Seq Velocity Blend.
- **Pattern** (saved in the plugin state as a `SEQ` child, not as host parameters): 16 columns × 6 tracks — Steps (on/off + probability), Pitch (0–12 st above the played key), Velocity, Octave (±2), Gate (5–400 % of a step), Slide (off, or a glide of 20–500 ms). Per track: on/off, length (with Polymetry), rate divider (/1, /2, /3, /4, /6, /8, /16), Random amount, rotate ◀ ▶. Per column: Lock (keeps its random values on Regen). A Custom scale has 12 per-note weights.
- **Random** is derived from a seed, so a pattern always replays the same way. **Generate** writes a new pattern from a seed: the first step plays the root, beats are likelier than 8ths and 8ths than 16ths, pitches come from the chosen scale (root and fifth a little likelier, small steps likelier than leaps), with accents, gate lengths and optional octave jumps and slides. Its arrow menu sets Sparse / Medium / Busy, range (a fifth or an octave), octave jumps and slides; locked steps are kept. Regen = a new seed (if every Random amount is 0, Regen first sets Steps 25 %, Velocity 30 %, Octave 25 %, Gate 20 %, and Pitch 40 % in Seq mode, so it always makes a new variation); Auto Regen moves the seed at each period; Apply random bakes what you hear into the steps; Clear random zeroes the amounts. Steps random flips some steps on/off; Pitch random picks notes by the scale weights (generative scales favour chord tones); Velocity/Octave/Gate random offset the value; Slide random toggles slides.
- **Scales:** Chromatic, Major, Natural/Harmonic/Melodic Minor, Dorian, Phrygian, Lydian, Mixolydian, Locrian, Major/Minor Pentatonic, Blues, Fifth, Major (Chord Tones), Minor (Chord Tones), Root & Fifth Heavy, Custom. Order is permanent (append only).
- **Seq mode** plays every held key at each step (a held chord becomes a rhythmic chord pattern), transposed by Pitch/Octave/Transpose. **Arp mode** ignores Pitch; Octave adds to the arp note. Chord arp: when an engine is in Unison Chord mode the arp spreads that chord and those voices play single notes.
- **Timing:** sample-accurate. Host playing → steps follow the song position (bar-aligned, loop jumps followed); a key pressed up to a quarter step late still plays that step at once. Host stopped → the pattern starts the moment the first key goes down. Rate changes keep the beat position.
- **Hold** (the synth's Hold button) latches the keys: a new chord after letting go replaces the latched one. The sustain pedal also latches while down. Seq notes ignore the pedal at the voice level.
- **Slide:** the note before a slide step is held into it and the voice glides (no retrigger), TB-style.


## 8. Presets, browser, host

- **Preset browser:** banks (Factory / User), categories (Pad, Keys, Pluck, Lead, Bass, Strings/Brass, Texture, Sequence, FX), search (also matches tags), favourites, Init preset, prev/next arrows, save / save as / delete, **Rescan** (re-reads the Presets folder, subfolders included).
- **How it's built:** each preset can carry notes (the recipe, what each part does, what to try), shown in a scrollable panel in the browser. Factory presets have them built in; preset files carry them in a `NOTES` element. Written as the teaching tool for the aux keys player.
- **Tags:** every preset can have any number of tags. Tag chips above the list filter it (several = presets with all of them). Edit Tags (or right-click a preset) ticks tags on and off or makes a new one; right-click a chip to rename or delete a tag everywhere. Defaults come from the preset (factory: built in; files: a `tags` attribute); the user's edits are kept in `Documents/Church Synth/Preset Tags.xml`, so they work on factory presets too. Renames and deletes are also remembered as rules and applied to presets added later (a new pack still saying "Chorus Poly" lands under the user's "Juno"); using a deleted tag again lifts its delete rule. Saving a preset writes its tags into the file.
- **Undo / redo / history.**
- **Host parameters:** Macros 1–4 first, then a curated list (cutoffs, engine volumes, FX dry/wet, aux send). MIDI learn for anything else.
- **Play Mode** (polyphony) in the footer, plus a **CPU meter**.
- **Formats:** **AU** (Mac, MainStage/Logic) and **VST3** (Mac + Windows, for sound design on the Windows PC), built from the same code from Milestone 1. AU is the one we test against for live use. **Presets are our own file format** (not host state), so a preset saved on Windows loads identically in MainStage.
- **UI:** **the same layout as Pigments** (top bar, engine tabs, filter column, routing/amp column, colour-coded mod-source strip, tabbed mod section, macros panel, FX page, Seq page), **dark only**. It should feel the same to use, but with our own artwork: knobs, fonts, colours, icons and logo. Resizable. Visualisers update at a low frame rate and stop completely when the window is closed.

### 8.1 As built (Milestone 7)
- **Editor:** top bar (menu, lotus logo + CHURCH SYNTH evenly spaced with the version (v0.2) in small grey; clicking it returns to the Synth page, preset bar with browser / favourite / save / previous-next, SYNTH · ⏻FX · ⏻SEQ with each power switch right against its label, MONO check (folds the output to mono while lit; never saved), master Width, master volume + output meter). The footer's **help bar** explains whatever control is under the mouse, in plain words., Synth page (Engine 1 / Engine 2 / Utility tabs, two filter panels with live response curves computed from the real filter code and per-model control names, routing / amp column), FX page (insert routing and bus levels, bus boxes with drag-to-swap and right-click copy, three slot editors with per-effect presets), Seq page (settings column with pattern browser, track grid with per-track random / divider / length / rotate, Shift-drag to scale a lane), the 24-tile mod strip with live traces, mod tabs (Keyboard curves + Mod Quantize keyboards, Envelopes, LFO, Functions with a drawing editor, Random, Combinate) and Macros with editable names, footer (Mod routes list, Play Mode panel, undo / redo / history, CPU and voices). Drawn at 1280×800, resizable with a fixed aspect ratio (60–200 %), size remembered. Font: Inter (OFL). Visuals refresh at 15–30 Hz and stop when the window closes.
- **Modulation editing:** select a source tile, then drag knobs to set amounts (double-click removes); or drag a tile onto a knob. Rings show every route in the source colours. A banner and a **Done** button show while a source is selected; Done, Esc, clicking the tile again or clicking anywhere that isn't a knob or the strip leaves that mode. Dropping a tile onto a knob makes the route without entering it.
- **Envelope curves:** drag the points (attack peak = attack time; decay corner = decay time and sustain level; end = release). Shift = fine, double-click = default.
- **Logo:** the church lotus (light grey) beside the CHURCH SYNTH wordmark. Analog Width knobs are only active on Square and Triangle.
- **Levels:** a +6 dB output stage after Master Volume and a peak limiter at −1 dBFS (instant attack, 150 ms release, soft above −0.4 dBFS), so the synth is about as loud as other soft synths without clipping. Unison keeps the level of a single voice; looped noises are normalised to the same RMS. Factory presets are matched to a −4 dBFS peak for a 4-note chord and −7 dBFS for mono patches.
- **Presets:** `.csp` (XML) in `Documents/Church Synth/Presets`; banks All / Factory / User / Favourites, 9 categories, search, Init, Save / Save As / Delete, favourites in `Favourites.txt`. 42 factory presets built in code, level-matched. Patterns (`.cspat`) and FX presets (`.csfx`) in their own folders; 10 factory patterns, 24 factory FX presets. Seq Lock keeps the pattern and Seq settings when a preset loads.
- **Host / MIDI:** every parameter stays a host parameter (IDs are permanent, so no curated subset); Macros 1–4 are first. MIDI learn on any control (right-click), saved with the host state, not in presets.
- **Undo:** history of up to 100 snapshots (parameter gestures, route / pattern / shape edits, preset loads), jump to any step from the History menu.

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
| 2026-11-06 | Preset packs round 7: Pack 8 Ladder (20, Moog-style): 8 mono basses (incl. a bass-pedal sound for holding the root and a 16th sequence), 8 mono leads (mod-wheel vibrato, trill, portamento), 4 ladder polys. Three oscillators + noise, filter Drive as the overdriven mixer, ladder 24 dB, filter and loudness contours with release = decay (decay switch). |
| 2026-11-06 | Preset packs round 6: Pack 7 Grand Poly (20, Jupiter-style). Two detuned, drifting oscillators per voice and no chorus; cross-mod via a silent Osc 3 FM-ing Osc 1; hard sync with Env 2 sweeping the synced osc; non-resonant highpass + OTA 4-pole / 2-pole; separate filter and amp envelopes; Unison (mono, stacked unison) and Dual (Engine 2 through Filter 2 in parallel). |
| 2026-11-06 | Preset packs round 5: Pack 6 Chorus Poly (20, Juno-style). Built strictly on the original's architecture so the pack teaches it: one oscillator (saw / pulse with PWM) + sub + noise, highpass positions, OTA 4-pole lowpass, one envelope shared by filter and amp (or amp gate), delayed LFO, Vintage Chorus I / II / I + II; light velocity. Pack names stay neutral; the README maps Chorus Poly = Juno-style. |
| 2026-11-06 | Preset packs round 4: Pack 5 Brass (20: R&B / soul synth brass, sharp stabs, fall, sforzando-crescendo, blast; trumpet, trombone, horn section, French horns, solo / muted trumpet, flugelhorn). Shared recipe: filter opening half fixed, half velocity (louder = brighter), Env 3 pitch scoop, breath noise, unison players; the fall tunes the engine down and holds it up with Env 3 so the release falls. Brass chorus width 50 % to stay focused and mono-safe. |
| 2026-11-06 | Preset packs round 3: Pack 4 Orchestral Strings (20: full section layered across registers, violins / violas / cellos / basses, marcato, staccato, pizzicato, tremolo, flautando, ostinato, solo violin and cello, worship and cinematic beds). Synth strings with an orchestral approach (unison = section size, Formant body, bow noise, delayed vibrato); described honestly as not a sampled orchestra. Full Strings Dynamic uses the mod wheel for dynamics (soft/dark to loud/bright), the orchestral-library convention. |
| 2026-11-06 | Preset packs round 2: Pack 3 Electric Pianos (30: Tine I / Tine II = Rhodes-style early / later, FM = DX-style, Reed = Wurlitzer-style, Clav = Clavinet-style; synth-built, velocity-responsive, keytracked decay). Owner's feedback on Pack 2: keep textures tonal, so Air Drone, Wind Swell and Prayer Atmosphere were reworked (comb-tuned breath, more pad, less noise). Standing guideline for future packs: atmospheres and FX lean tonal rather than noise. `--export-pack` / `--render-pack` take an optional pack-name filter. |
| 2026-11-06 | v0.2.1 and preset packs round 1. Owner's plan: 14 downloadable packs, built in code, one per round so each can be heard and corrected; order = church use first (Worship Elements; then E-Pianos, Orchestral, Brass; then the classic-synth packs; then Synthwave, EDM, Famous Sounds, Special FX; acoustic pianos as 20 synth-built pianos). Famous-sound presets get nod names ("Countdown Brass") and recreate timbres only: no copied factory patches, no riffs. Pack 1 gets "How it's built" notes on all 75 presets; Pack 2 Worship Elements (50: pads, drones, transitions, bells, rhythmic beds, subs). Packs live in `packs/`, each with presets, tags and notes; the state test fails on a pack preset without notes or tags. Texture presets are level-matched for one held note (−7 dBFS); Texture and FX presets are measured over 10 s so swells and risers reach full level. v0.2.1 fix: the Amp Mod gain (velocity, tremolo, gates) is smoothed per sample (~2 ms); a legato note with a new velocity used to click. The header shows the fix number (v0.2.1). |
| 2026-11-06 | v0.2. Attack can go to 0 ms (truly instant from silence; retriggers still start from the current level); plucky factory presets use it. Idle instances skip all processing once notes and FX tails are below −100 dBFS for 0.25 s (≈0 % CPU). New `master_width` parameter and an editor-only Mono check. Tag renames/deletes remembered for future imports. "How it's built" notes on all 42 factory presets; help bar. Logo is a home button; version shown. Reliability: a threaded stress test (now in ctest) and ThreadSanitizer found (1) a noise-player read past the end of the sample when the noise was swapped for a shorter one mid-note (or with extreme Tune), giving NaN or a crash, the likely cause of the one-off Intel AU crash in pluginval; (2) a non-atomic flag read from parameter callbacks. Both fixed. |
| 2026-11-05 | Feedback round 2. Filter Drive reworked (pre + in-loop + post saturation with automatic makeup; it used to make a saw slightly darker). Vintage Chorus modes made clearly different. New Rotary effect (FX type 20). Browser: user tags (rename / delete / filter) and Rescan. Top bar: power switches next to their labels, logo spacing evened. Seq: Generate (scale-aware pattern writer). Preset Pack 1 grows to 75 presets (organs, more strings, Grand / Vintage / Chorus Poly sets), all tagged. Deferred at the owner's choice: a separate "Church Synth FX" audio-effect plugin (instrument plugins get no audio input in MainStage). |
| 2026-11-05 | Preset Pack 1 (50 presets: electric pianos, felt pianos, strings, bells, evolving and polysynth pads, 80s chorus-polysynth sounds, ladder leads, pulsing bass sequences) delivered as downloadable `.csp` files to copy into `Documents/Church Synth/Presets`, not built into the plugin. Source in `packs/`. |
| 2026-11-05 | Feedback round 1 (Reaper, Mac AU + Windows VST3). Fixed: knobs stuck in modulation mode after clicking a source (critical); Comb filter flat (now always resonant, tuned to the keys when picked); Regen did nothing with no Random amounts; quiet output (+6 dB stage + limiter; the oscillators themselves measured correct, the difference was level); unison and sparse noises losing level; Vintage Chorus too narrow (new Width control); Super Unison too subtle. Added: FX power button, Param EQ graph, menus for discrete FX controls, draggable envelope points, filter Volume up to +12 dB, filter type descriptions (Ladder / Sallen-Key / OTA 4-Pole are the Mini / MS / Jupiter-style filters), Width knob only on Square/Triangle, church lotus logo. Factory presets re-levelled; user patches saved before this play about 6 dB louder (Master Volume brings them back). Legato and Glide stay in Play Mode; they can be MIDI-learned like any control. |
| 2026-11-04 | Milestone 7 done: Pigments-style editor, presets and browser, 42 factory presets, undo/redo/history, MIDI learn, drag-and-drop modulation, drawable functions, keyboard curve editors, Mod Quantize keyboards, FX presets / swap / copy, pattern browser and Seq Lock, Shift-drag lane scaling (§8.1). Choices made: all parameters remain host parameters (permanent IDs) instead of a curated subset; factory content is built in code so nothing needs installing; logo was a gothic arch with a cross (replaced by the church lotus on 2026-11-05); font Inter (OFL). Still simplified: Param EQ peaks share one Q, the Erode modulator is an internal oscillator/noise. |
| 2026-11-04 | Milestone 6 done: Seq/Arp as in §7.1. Choices made: Seq mode plays the whole held chord each step; the pattern lives in the plugin state (not as 100+ host parameters); random values come from a seed (Regen = new seed) so patterns replay exactly; added Arp Octaves (1–4) and an internal tempo for hosts without one. Interim until the UI milestone: lanes in a plain grid (no Shift-drag scaling of a track yet); pattern browser (Factory / From preset / User) and Lock behaviour come with presets in M7. |
| 2026-11-04 | Milestone 5 done: FX A / FX B (3 slots each, A>B, B>A, A//B, bus volumes), Aux bus (3 slots, send/return, pre/post), 19 effects (§6.3), tempo sync, FX controls as modulation destinations. Effects are created off the audio thread when a slot's type changes and swapped in atomically. Effect type order is permanent (append only). Interim until the UI milestone: an FX tab with generic knobs; per-effect presets, drag-to-swap and copy-to-slot move to M7; Param EQ peaks share one Q; the Erode modulator is an internal oscillator/noise (no engine or filter sources yet). |
| 2026-11-04 | Milestone 4 done: 15 filter models (Classic, Cluster, Phaser, Formant, Surgeon, LoFi, Comb, Drive, Allpass, Ladder, Sallen-Key, OTA 4-Pole, SVF Morph, Multimode, Lowpass Gate), audio-rate filter FM (Engine 1, Engine 2, Utility, Noise), Sum / Split / Split Reversed output. Each filter exposes Model, Model Mode, Cutoff, Resonance, Drive and Controls A/B/C; what A/B/C do per model is listed in §4.4 (the UI milestone labels them per model). Voices now write to Main / FX A / FX B / Direct buses, summed until the FX milestone. |
| 2026-11-04 | Milestone 3 done: Env 2/3, 3 LFOs, 3 Functions (16 preset shapes), 3 Random (5 types), 3 Combinators (10 types), 4 Macros, 64-route matrix with sidechains, amp mod source, Mod Quantize, aftertouch (channel + poly), CC11, tempo sync. Interim until the UI milestone: functions use preset shapes (drawing editor in M7), keyboard curves are a single bend control (2-point editor in M7), quantize notes are a 12-bit mask (mini-keyboard in M7), routes are edited in a list (drag-and-drop in M7). Filter FM source menu moves to M4 with the filters. |
| 2026-11-04 | Milestone 2 done: Wavetable + Utility engines, cross-mod, 40 generated wavetables, 34 generated noises, WAV import. Phase-transform shapes are our own set: Pulse Width, Skew, Mirror, Sync, Steps, Bend, Squeeze. Wavetable/noise list order is permanent (append only). |
