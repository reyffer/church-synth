# Church Synth — Feature Spec (v0.6)

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
 (§3.1, 3.5)│ Filter Mix ─► Filter 1 ─┐                           │
 Engine 2 ──┤                         ├─ Routing (series↔parallel)├─► Amp (Env Amp × Amp Mod) ─► Voice Pan
 (§3.1, 3.5)│ Filter Mix ─► Filter 2 ─┘   Sum | Split             │
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
Each slot is **Analog**, **Wavetable**, **Organ** or **Section** (any combination, including two of the same; Organ since v0.5, Section since v0.6, §3.5). Planned: **Modelled** (§3.5). Each has: on/off, copy/paste/reset (with or without its mod routings), Volume, Filter Mix (F1↔F2).

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

### 3.5 New engine types: Organ (built, v0.5), Section (built, v0.6) and Modelled (planned)

Goal: cover the three sounds that still need other plugins in the MainStage rig (organ, strings, brass), so only the acoustic piano stays sample-based. References to beat or match: Logic's Vintage B3 (itself a component model, not samples), Crow Hill Pocket Strings and Logic Studio Horns (both multi-sampled).

**Rules for all three**
- **Appended to the Engine 1 / Engine 2 type menu:** Analog, Wavetable, **Organ**, **Section**, **Modelled** (`EngineType` 2, 3, 4). Order is permanent. The type parameter gains choices, so saved states and presets must keep loading as the same type: store and read the index, never a normalised value; a state test loads v0.4 presets and checks every engine type.
- **Zero cost when not chosen.** The engine type is dispatched once per block, never per sample. A voice holds the state of one engine type at a time (union / variant), so voices don't grow for patches that never use the new types. Content (tables, body responses) is generated only when a preset selects the type.
- **Bench gate.** Before any of these merges, every existing `cs_bench` case (analog pad, wavetable pad, EP and `pad_fx_*`) must be unchanged within noise. Each new engine gets its own bench cases and a CPU target (below).
- **Same voice path after the engine:** Volume, Filter Mix, filters, amp, FX, modulation, Coarse / Fine / Keytrack, Mod Quantize. Every new knob is a modulation destination and a host parameter (new permanent IDs, appended).
- **No samples, no copied content.** Everything is generated in code. We model the instrument families, not any product's code or presets.
- **Unison:** hidden for Organ (one tone per key, like the real thing); replaced by players for Section; off for Modelled (too heavy; use Section or Super Unison instead).
- **Help-bar text** for every new control, "How it's built" notes on every new preset, as for the existing engines.

#### Organ (tonewheel)
- **9 drawbars:** 16', 5⅓', 8', 4', 2⅔', 2', 1⅗', 1⅓', 1'. Levels 0–8, continuous with snap to whole steps; drag in the UI like real drawbars. Presets for classic registrations (full gospel, soft ballad, jazz, "all out", flutes) under our own names.
- **Tonewheels:** 91 wheels, each a near-sine with a small amount of 2nd/3rd harmonic. **Wheel Character** (clean → vintage) sets those harmonics and a small level variation per wheel. Tuning follows the tonewheel gear ratios (slightly off equal temperament), with an Equal Temperament switch.
- **Free-running phases:** each wheel's phase comes from an instance-wide clock, not from the key press. Like real spinning wheels, a key never starts at the same phase, and two drawbars that land on the same wheel add exactly in phase. This needs no shared buffer: each voice computes its 9 partials from the clock.
- **Foldback:** high footages fold back an octave past the top wheel; the 16' folds up in the lowest octave (switch "Bass All the Way Down" to keep it down, as in some organs).
- **Crosstalk / Leakage:** neighbouring wheels faintly audible (amount, default low), the "dirt" between clean and raucous.
- **Key click:** each drawbar's contact closes at a slightly random moment (0–3 ms spread) with a short click burst at key down and a softer one at key up. Click Level and Click Tone.
- **Percussion:** on/off, 2nd or 3rd harmonic, Fast / Slow decay, Soft / Normal level, single-trigger (only retriggers after all keys are released; legato playing doesn't retrigger). Option to mute the 1' drawbar while percussion is on, as on the original.
- **Vibrato / Chorus scanner:** a new FX type, **Organ Scanner** (appended as FX type 21), because the real one processes the summed organ, once per instance. Modes V1, V2, V3, C1, C2, C3 (vibrato = wet only, chorus = dry + wet), rate ≈ 6.9 Hz, depth per mode, a multi-tap scanned delay of about 1 ms. Then the existing **Rotary** (FX 20) with its Drive for the tube overdrive.
- **Playing:** Env Amp defaults to an organ gate (attack 0, release ≈ 5 ms, the click covers the edges); velocity off by default; Expression (CC11) → volume swell routed in the Init organ preset. One manual: splits and pedal bass are done in MainStage with two instances.
- **CPU target:** 8 notes × 9 partials, no unison: at most the cost of the analog pad bench case.

##### Organ as built (v0.5.0)
- **Parameters** (appended after Master, group "Engine n Organ"; all permanent): `e1_org_db1..db9` (0–8, the footages in order 16' → 1', default 888 000 000), `e1_org_char` (Wheel Character, 0.35), `e1_org_tuning` (Tonewheel / Equal), `e1_org_bass` (Bass All the Way Down, **on** by default, i.e. the 16' plays its own wheels down to the bottom; off folds it up in the lowest octave), `e1_org_leak` (0.2), `e1_org_click` (0.5), `e1_org_click_tone` (0.5), `e1_org_perc` (off), `e1_org_perc_harm` (Second / **Third**), `e1_org_perc_decay` (**Fast** / Slow), `e1_org_perc_vol` (**Normal** / Soft), `e1_org_perc_cut` (mutes the 1', on); same for `e2_`. Mod destinations: the 9 drawbars, Character, Leakage, Key Click, Click Tone.
- **Tonewheels:** wheel 1 = C1 (MIDI 24) … wheel 91; frequency = 20 rev/s × the 12 gear ratios × 2…256 teeth (so A = 55 / 110 / 220 / 440 Hz exactly, the other notes up to about 0.7 cent off equal temperament). Each voice computes its own partials (one per drawbar, slot-outer, sample-inner) from a 2048-point sine table; a wheel's starting phase = frac(wheel Hz × instance clock), so foldback partials on the same wheel add exactly in phase and pushed-in drawbars keep their phase. Keys above the 61-key range simply use the wheels that exist (foldback at the top, octave fold below wheel 1).
- **Wheel Character:** per wheel cos 2x (2nd) and 3x (3rd) harmonic terms, up to 3 % / 1.5 % (12 % / 10 % on the lowest 12 wheels) and a fixed ±1.5 dB level spread; 0 = pure sines.
- **Leakage:** each sounding drawbar brings in the wheel that shares its generator compartment (4 octaves away) at up to −30 dB (Leakage²), so it is the faint high whistle of old organs on low notes.
- **Key click:** each drawbar contact closes at its own random moment (0–3 ms); the closing ramp is 3 ms at Click 0 (no click) down to 0.05 ms at Click 1, and each contact adds a noise burst (1.2 ms decay, band-limited 400 Hz … 1.5–12 kHz by Click Tone) scaled by its drawbar level. Key up: a softer burst; the amp envelope (organ gate: attack 0, release 5 ms in the presets) ends the note, so a longer release still works.
- **Percussion:** one shared decay per engine slot in `SynthCore` (τ 0.13 s Fast, 0.42 s Slow), restarted only when a key goes down with no other key held (sustain pedal doesn't count, the sequencer's notes do), so legato notes get only the decayed level, as on the original. Normal: full level and the drawbars −3 dB; Soft: −6 dB, no drawbar drop.
- **Voice state:** an organ voice adds about 0.4 KB beside the Analog and Wavetable states (those were already side by side, so a union would have saved nothing measurable); the type is dispatched once per 16-sample block.
- **Expression (CC11)** now defaults to full (127) until the pedal sends a value, the MIDI convention; the organ presets use it as the swell through Amp Mod (amount 0.75), so with no pedal they play at full level.
- **Measured (x86 container):** 8 notes 888 + percussion + click + leakage ≈ 46 µs per 128-sample block, a fifth of the analog pad case (≈ 240 µs); all nine drawbars, full leakage, 16 notes ≈ 190 µs. Every existing bench case unchanged within noise.
- **Editor:** an Organ panel replaces the oscillators when the type is Organ: Tune and Output on the left; nine drawbars (brown = sub-harmonics, white = octaves, black = the others) pulled down by dragging, snapping to whole steps (Shift = fine), with a Registrations… menu (Jazz Trio 888 000 000, Full Gospel 888 800 008, Soft Ballad 008 500 000, Warm Worship 838 000 000, Flutes 006 040 000, Bright Lead 800 000 888, Sweet Choir 006 876 540, Rock Grind 888 600 000, All Out 888 888 888); Tonewheels, Key Click and Percussion sections on the right; LEDs for the wheels, the click and the percussion. Drawbars take modulation like knobs (coloured lines beside the slot).
- **Presets:** 11 factory presets (Keys / Bass, tags Organ + Tonewheel): Tonewheel Init, Gospel, Jazz, Ballad, Worship Tonewheel, Flutes, Lead, Choir, All Out, Rock, Bass. All: Organ Scanner → Rotary in FX A (Bass: rotary only), mod wheel past half way = fast rotary, Expression = swell, Macros Drawbars / Space / Drive / Click.

#### Section (ensemble strings and brass)
The missing piece in our current string and brass presets is not the oscillator: a real section is many *independent* players, and their tone passes through a resonant body or bell. This engine plays one note as several players.

- **Family:** Strings or Brass (menu, append-only; Choir or Woodwinds could be added later).
- **Players:** 1–8 per note (Solo 1, Chamber 3, Section 6, Full 8 as quick picks). Each player has its own band-limited source (strings: bowed saw-like wave; brass: pulse / saw blend) and its own seeded random values, so a chord always sounds the same, never "phasey" like copied voices:
  - **Intonation:** small tuning offset per player (0–15 cents spread) plus slow drift.
  - **Vibrato:** own rate (spread around 5–6 Hz) and depth per player, with a fade-in delay; Vibrato Depth and Rate are section-wide knobs that scale them.
  - **Looseness:** attack timing spread across players (0–40 ms), so entries aren't machine-tight.
  - **Seating:** players placed across the stereo field (Width) around a Position (e.g. violins left of centre, cellos right), with mono-safe panning.
- **Dynamics:** one knob that drives level, brightness and vibrato depth together (mod wheel routed to it in the Init presets, the orchestral-library convention; velocity also routable). **Brightness follows loudness non-linearly:** for brass the upper harmonics rise much faster than the fundamental as Dynamics goes up (a soft-saturating spectral shaper driven by Dynamics), for strings the tilt is gentler and adds bow pressure.
- **Noise:** bow noise (strings) or breath (brass), filtered around each player's pitch, amount follows Dynamics.
- **Attack (per family):** strings: Bite (bow pressure transient) and Attack (soft ↔ marcato). Brass: Scoop (pitch starts below and rises in 30–80 ms), Blat (short bright burst on loud attacks), Attack (soft ↔ sforzando).
- **Body / Bell resonator** (the key to realism): after the players of a note are summed, a fixed bank of resonant peaks shapes them. Vibrato moving the harmonics through these fixed peaks creates the shimmer of real strings and the "bloom" of brass. Body types (generated in code, our own response shapes): Violin, Viola, Cello, Bass, Ensemble (blend), and Trumpet, Trombone, Horn, Muted. Body Mix (dry players ↔ body). Implementation: 16 band-pass resonators (strings) or 8 (brass) per voice, processed 4 at a time with SIMD.
- **Legato:** with Voicing = Mono Legato (Keyboard tab), a slur: glide plus a short dip in brightness and noise (bow change / tongued slur). Used for solo lines.
- **Mod destinations:** Dynamics, Vibrato Depth, Vibrato Rate, Intonation, Looseness, Noise, Brightness, Body Mix, Width.
- **CPU target:** 8 notes × 6 players + body ≈ the cost of an 8-voice pad with unison 6 today. A hard cap of 64 player oscillators per instance (same as the unison cap).
- **Presets:** rebuild Pack 4 (Orchestral Strings) and Pack 5 (Brass) on this engine, keeping the old versions as they are.

##### Section as built (v0.6.0)
- **Parameters** (appended after the organ's, group "Engine n Section"; permanent): `e1_sec_family` (Strings / Brass), `e1_sec_players` (1–8, default 6), `e1_sec_into` (Intonation 0.35), `e1_sec_loose` (Looseness 0.35), `e1_sec_width` (0.7), `e1_sec_pos` (−1…1), `e1_sec_dyn` (Dynamics 0.7), `e1_sec_bright` (0.5 = neutral), `e1_sec_noise` (0.3), `e1_sec_vib_depth` (0.5), `e1_sec_vib_rate` (4–7 Hz), `e1_sec_vib_delay` (0–2 s, 0.35), `e1_sec_attack` (0.4), `e1_sec_bite` (Bite for strings, Blat for brass), `e1_sec_scoop` (brass), `e1_sec_body` (Violin, Viola, Cello, Bass, Ensemble, Trumpet, Trombone, Horn, Muted; append-only, default Ensemble), `e1_sec_body_mix` (0.7); same for `e2_`. Mod destinations: Dynamics, Vibrato Depth / Rate, Intonation, Looseness, Noise, Brightness, Body Mix, Width, Position, Attack, Bite / Blat, Scoop (Looseness only acts when a note starts).
- **Players:** each a band-limited `BlepOscillator` (strings: saw; brass: a pulse whose width narrows from 50 % to 8 % as Dynamics rises, power-normalised: the lips buzzing harder give a flatter, brighter spectrum). Seeded from the key, so a chord always sounds the same: tuning offsets spread evenly over ±7.5 cents × Intonation and jittered, a slow drift (±3 cents × Intonation, 0.05–0.2 Hz), vibrato rate ±8 % and depth 0.7–1.3× around the section values with its own phase, an entry delay up to 40 ms × Looseness (the earliest player starts at once), and a seat spread evenly over Width (shuffled, so tuning and seat are unrelated). Equal-power panning around Position, computed per block. Level = 1/√players, so the player count changes the texture, not the loudness.
- **Dynamics** (smoothed 20 ms): level over 18 dB (strings) / 22 dB (brass); each player's 12 dB low-pass at the note × 2^(1.5 + 4.5·dyn^0.8) (strings) or 2^(0.3 + 6.7·dyn^1.5) (brass), ± 1.5 octaves of Brightness; brass also a gentle soft-clip (drive 1 + 2·dyn²) and a DC blocker; vibrato depth × (0.55 + 0.45·dyn) (up to 28 cents strings, 14 brass). Measured on C4 (power-weighted centroid): strings 740 → 1417 Hz, brass 341 → 1685 Hz from Dynamics 0.2 to 1: brass is darker than strings when soft and brighter when loud.
- **Attack:** each player rises with a time constant from 180 ms (strings) / 120 ms (brass) at Attack 0 to about 12 ms at 1; brass above 0.7 overshoots (sforzando, up to +60 % × Dynamics, 0.25 s decay). Bite / Blat open the low-pass by up to 2 (strings) or 3·dyn² (brass) octaves for about 50 ms and boost the noise; Blat also drives the saturator harder. Scoop: up to −300 cents, settling with τ = (30–80 ms)/3 (shorter when loud). Measured: −2.6 semitones at 6 ms, within 3 cents by 80 ms.
- **Noise:** one band-pass per note (TPT SVF at 2× the note for bow, 3× for breath) plus a little air, × Noise × (0.2 + 0.8·dyn), boosted by Bite and the slur, seated at Position.
- **Body / bell:** a parallel bank of RBJ band-pass resonators, 16 for the string bodies and 8 for the bells, at fixed frequencies (our own shapes: the violin's air mode near 275 Hz, wood modes, a bridge hill at 2–3 kHz and a dark top; viola × 0.8, cello × 0.45, bass × 0.3 with a darker tilt; Ensemble interleaves violin and cello modes, broader; trumpet formant near 1.3 kHz, trombone 550–900 Hz, horn 420 Hz and dark, muted a narrow peak at 1.7 kHz with the lows cut) plus a direct part, normalised so the average power over 100 Hz–8 kHz is unity (body in or out stays within 4 dB). Stereo (a bank per side) so the seating survives. Shapes are built once off the audio thread; coefficients are recomputed only when the body changes.
- **Legato:** with Mono Legato, a legato note change slurs: brightness −1.5 octaves, level −22 %, noise up (strings) or down (brass), recovering with τ 45 ms, while the voice glides. A retrigger restarts the attack envelopes from the current level.
- **Player budget:** players per note = min(Players, 64 / Max Voices), so 8 voices allow 8 players and 16 voices 4 (a test checks it bit for bit).
- **Measured (x86 container):** 8 notes × 6 players + body ≈ 120 µs (strings) / 135 µs (brass) per 128-sample block, about half the analog pad case (≈ 240 µs). Bench gate: an interleaved A/B against the v0.4 code puts every existing case within ±3 %, the run-to-run spread; to get there the Organ and Section states moved to the end of `Voice` and their render functions are kept out of line (`CS_NOINLINE`), since inlining them changed the layout of the wavetable path (+5 % before).
- **Editor:** a Section panel: Tune / Output; Players (family, Players with a Size… menu for Solo 1 / Chamber 3 / Section 6 / Full 8, Intonation, Looseness, Width, Position and a seating diagram showing where the players sit); Dynamics (Dynamics, Brightness, Noise); Vibrato (Depth, Rate, Delay); Attack (Attack, Bite or Blat by family, Scoop, dimmed for strings); Body (menu, Body Mix). LEDs for the players, the noise and the body.
- **Presets:** 4 factory presets (Section Strings Init, Section Brass Init, Worship Section Strings, Worship Section Horns), and two downloadable packs: **Pack 4 II – Orchestral Strings (Section engine)** and **Pack 5 II – Brass (Section engine)**, the 20 + 20 sounds of Packs 4 and 5 rebuilt, names ending in "II" (preset keys are category + name, so the same names would have shared tags and favourites with the originals). Mod wheel = Dynamics throughout; brass also velocity → Dynamics.

#### Modelled (physical modelling) — experimental, last
Simulates the instrument itself instead of building the sound from oscillators: the most expressive option for solo lines, the heaviest and the riskiest. Only after Organ and Section are done and proven.

- **Models (menu, append-only):** **Bowed String** (a digital waveguide string with a bow-friction junction: Bow Pressure, Bow Speed, Bow Position, bridge filter) and **Lip Brass** (a waveguide bore with a lip-valve model: Breath Pressure, Lip Tension, Bore Length fine, bell filter and radiation). Both feed the Section engine's body / bell resonator.
- **Needs continuous control:** a held note only lives while pressure is applied. Default routing: mod wheel → Bow Pressure / Breath; aftertouch or Expression (CC11) as alternatives. Without it notes sound static, so Init presets set this up and the help bar says so.
- **Polyphony:** up to 4 notes (solo instrument or small divisi), no unison. 2× oversampling inside the nonlinear junction only.
- **Reliability first:** these models can squeak, whistle or blow up. Every model has level clamps, DC blockers and a stability check in the state test (sweep every control at extreme settings, fail on NaN or > 0 dBFS). Fractional-delay tuning must stay within ±3 cents over the keyboard.
- **CPU target:** one note ≤ 3× an analog voice.
- **Expected result:** expressive solo cello, violin, trumpet, horn. Full sections stay with the Section engine.

#### How we judge them
Same method as the effects (FX-ANALYSIS.md): Reyffer renders reference notes and phrases from Logic's Vintage B3, Pocket Strings and Studio Horns; we render the same MIDI through Church Synth offline and compare spectra per dynamic level, vibrato rate / depth spread, attack timing spread, stereo coherence and level. Then a level-matched blind A/B by ear in MainStage, which has the final say.

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
| Reverb | Pre-Delay, Size, Decay, Damping, Low Cut, High Cut, Width, Dry/Wet, Mod Depth (up to 6 ms on every line, own rate per line), Mod Rate (0.1–5 Hz), Brightness (high shelf on the tail, 0 = neutral, up to +8 dB at 2.5 kHz). 8-line feedback delay network, 6 input all-passes and a little of the diffused input heard directly (dense first 100 ms), left/right = two orthogonal mixes of all 8 lines. Defaults since v0.4: Decay 2.5 s, Damping 0.3, High Cut 15 kHz, Mod 0.5 / 0.55, Brightness 0.3. "Lush Modulated" FX preset ≈ a big vintage poly synth's modulated reverb |
| Shimmer | Pitch, Shimmer (pitched feedback), Size, Decay, Modulation, High Cut, Ducking, Width, Dry/Wet |
| Compressor | Threshold, Ratio, Attack, Release, Makeup, Auto Makeup, Output, Dry/Wet |
| Multiband | Low X-Over, High X-Over, Amount (down + up compression, auto make-up), Attack, Release, Low / Mid / High Out, Output |
| Multi Filter | Type (LP/HP/BP 12/24/36, Notch, Comb FF, Comb FB), Cutoff, Resonance, Dry/Wet |
| Param EQ | Low Shelf Freq/Gain, Peak 1/2/3 Freq/Gain, High Shelf Freq/Gain, Peak Q (shared), Scale |
| Distortion | Type (Soft Clip, Hard Clip, Tube, Tape, Overdrive, Fuzz, Crunch, Diode, Sine Fold, Triangle Fold, Half Rectify, Full Rectify, Asymmetric, Bit Shape, Saturate, Destroy), Drive, Auto Gain, Output, Filter (off/pre/post), Filter Type (LP/HP/BP), Cutoff, Resonance, Slope (12/24/36), Dark, Dry/Wet. 2× oversampled |
| Bitcrusher | Bit Depth, Downsample, Jitter, Low Cut, High Cut, Dry/Wet |
| Erode | Mod Type (sine/tri/saw/square/noise), Frequency, Amount, Noise Blend, Band Freq, Bandwidth, Spread, Drive, Dry/Wet |
| Super Unison | Voices (2–8 per side), Detune (up to ±100 cents, curved; default ±20), Drift, Width (0 = mono; 1 = a separate voice set per side, fully decorrelated), Low Cut, High Cut, Dry/Wet. The side signal is high-passed at 250 Hz (2-pole), so the bass stays centred |
| Chorus | Delay (1–30 ms), Depth, Rate†, Feedback, Voices (1–4), Shape (sine/square), Stereo (the right side's voices run on their own LFO, 9 % faster and offset), Dry/Wet. Defaults since v0.4: ≈15.5 ms ±4.5 ms, 0.35 Hz, 2 voices, Dry/Wet 0.45 (wet ≈ 0.85 × dry) |
| Vintage Chorus | Mode (I: 0.5 Hz; II: 0.85 Hz, same depth: both 1.7–5.5 ms; I+II: 9 Hz, 3.4–3.7 ms, mono), Rate and Depth scale the mode's values, Phase (1 = left and right taps in opposite phase), Dry/Wet, Width (stereo width of the wet, 0 = mono), Brightness (0 = dark bucket-brigade wet, rolling off above ~6 kHz; 1 = full range with a +4 dB lift at the top), Wide (0 = classic timing ≈3.6 ms; 1 = the bigger poly synth's ≈14 ms ±4 ms, 30 % slower). Taps are added to the dry with the same polarity, so mono stays full (v0.3's sum/difference Width is gone). "Wide Poly" FX preset = Wide 1 + Brightness 1 |
| Flanger | Delay, Depth, Rate†, Feedback (negative below centre), Low Cut, High Cut, Stereo, Shape (sine/tri), Dry/Wet |
| Vintage Flanger | Rate†, Delay, Feedback, Depth, Wide, Mono Input, Dry/Wet |
| Phaser | Frequency, Feedback, LFO Wave (6), Depth, Rate†, Poles (2–12), Stereo, Dry/Wet |
| Panner | Amount, Rate†, Invert, Mono Bass, Bass Cutoff, Natural Law, Shape (sine/tri/square-ish) |
| Organ Scanner | Mode (V1, V2, V3 = vibrato, wet only; C1, C2, C3 = chorus, half dry), Rate (5.5–8 Hz, default 6.9), Depth (0.5 = the mode's own depth; up to 1.5 × the line), Tone (wet low-pass 3–16 kHz), Stereo (0 = mono like the original; up to a quarter turn between the sides), Dry/Wet. A 1 ms delay line with taps every 0.125 ms; a rounded-triangle pickup sweeps a third (V1/C1), two thirds (V2/C2) or all (V3/C3) of it, crossfading two neighbouring taps at a time. Measured on a 1 kHz tone: V1 ≈ ±14 cents, V3 ≈ ±37 cents. Appended as FX type 21; FX presets Chorus C3, Chorus C1 (light), Vibrato V3, Vibrato V1 (light), Wide Chorus |
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
  - **User folders:** selecting User expands it into the Presets folder's subfolders (nested, with preset counts, e.g. one per downloaded pack); a folder shows the presets in it and below it.
  - **Favourite folders:** work like playlists (a preset can be in several). The Favourite... button (and a preset's right-click menu) ticks Favourite and each folder, or makes a new folder; putting a preset in a folder also makes it a favourite, un-favouriting takes it out of every folder. Selecting Favourites expands into its folders; right-click a folder to rename or delete it (its presets stay favourites). Kept in `Documents/Church Synth/Favourite Folders.xml` by preset key.
  - **Tag chips collapsed:** only the 12 tags used on the most presets (plus any active ones), with Show all tags (n) / Show fewer.
- **How it's built:** each preset can carry notes (the recipe, what each part does, what to try), shown in a scrollable panel in the browser. Factory presets have them built in; preset files carry them in a `NOTES` element. Written as the teaching tool for the aux keys player.
- **Tags:** every preset can have any number of tags. Tag chips above the list filter it (several = presets with all of them). Edit Tags (or right-click a preset) ticks tags on and off or makes a new one; right-click a chip to rename or delete a tag everywhere. Defaults come from the preset (factory: built in; files: a `tags` attribute); the user's edits are kept in `Documents/Church Synth/Preset Tags.xml`, so they work on factory presets too. Renames and deletes are also remembered as rules and applied to presets added later (a new pack still saying "Chorus Poly" lands under the user's "Juno"); using a deleted tag again lifts its delete rule. Saving a preset writes its tags into the file.
- **Undo / redo / history.**
- **Host parameters:** Macros 1–4 first, then a curated list (cutoffs, engine volumes, FX dry/wet, aux send). MIDI learn for anything else.
- **Play Mode** (polyphony) in the footer, plus a **CPU meter**.
- **Formats:** **AU** (Mac, MainStage/Logic) and **VST3** (Mac + Windows, for sound design on the Windows PC), built from the same code from Milestone 1. AU is the one we test against for live use. **Presets are our own file format** (not host state), so a preset saved on Windows loads identically in MainStage.
- **UI:** **the same layout as Pigments** (top bar, engine tabs, filter column, routing/amp column, colour-coded mod-source strip, tabbed mod section, macros panel, FX page, Seq page), **dark only**. It should feel the same to use, but with our own artwork: knobs, fonts, colours, icons and logo. Resizable. Visualisers update at a low frame rate and stop completely when the window is closed.

### 8.1 As built (Milestone 7)
- **Editor:** top bar (menu, lotus logo + CHURCH SYNTH evenly spaced with the version (v0.6) in small grey; clicking it returns to the Synth page, preset bar with browser / favourite / save / previous-next, SYNTH · ⏻FX · ⏻SEQ with each power switch right against its label, MONO check (folds the output to mono while lit; never saved), master Width, master volume + output meter). The footer's **help bar** explains whatever control is under the mouse, in plain words., Synth page (Engine 1 / Engine 2 / Utility tabs with **sound LEDs** on each tab, oscillator, noise, wavetable / modulator osc and Utility part: dark when silent, faint to bright green with level, measured as what the part sends into the filters after its volume, the engine volume and the amp envelope, only while the editor is open, two filter panels with live response curves computed from the real filter code and per-model control names, routing / amp column), FX page (insert routing and bus levels, bus boxes with drag-to-swap and right-click copy, three slot editors with per-effect presets), Seq page (settings column with pattern browser, track grid with per-track random / divider / length / rotate, Shift-drag to scale a lane), the 24-tile mod strip with live traces (a small dot on each tile that is routed somewhere; clicking a knob lights up the tiles of the sources modulating it; selecting a source draws faded lines to every knob it moves on screen and puts "+n" badges on the page / engine / mod tabs that hide more of its targets), mod tabs (Keyboard curves + Mod Quantize keyboards, Envelopes, LFO, Functions with a drawing editor, Random, Combinate; envelopes, LFOs and functions show a **travelling white dot** per sounding note (newest bright, older faint; one dot for a shared Mono / Legato / Free-running generator); a generator nothing uses doesn't run, so it shows no dot) and Macros with editable names, footer (Mod routes list, Play Mode panel, undo / redo / history, CPU and voices). Drawn at 1280×800, resizable with a fixed aspect ratio (60–200 %), size remembered. Font: Inter (OFL). Visuals refresh at 15–30 Hz and stop when the window closes.
- **Modulation editing (v0.6.1):** drag a source tile onto a knob, drawbar or FX menu/switch to connect it. While a tile is dragged, every control it can modulate is outlined in the source's colour. A drop makes the route at 0 % and opens the **mod wheels** on it.
- **Mod wheels:** rest the mouse ≈0.5 s on a modulated control. A panel beside it shows one wheel per incoming connection, in the source colour, outlined in the (first) connection's colour, with lines from the source tiles to the control itself (never to the panel; a source whose links are already showing isn't drawn twice). Hover or click a wheel: the lines go and the panel shows the source, the amount and the range it sweeps in real units (e.g. "Env 2 +30 % / 629 Hz → 5.00 kHz"; bipolar sources show low ↔ high). Drag a wheel up/down to set the amount (Shift = fine) or use the mouse wheel. Double-click removes the connection; right-click gives Remove / 0 % / Invert. Esc or a click elsewhere closes the panel; it also closes ≈0.27 s after the mouse leaves both the control and the panel.
- **Source tile click** = view only: lines to the controls it moves, a banner and a **Hide** button. Hide, Esc, the tile again or a click on empty space ends it. Knobs behave normally meanwhile (no edit mode).
- **FX menus / switches** are modulation targets too and show the *live* value: e.g. Rotary Speed flips to Fast with the mod wheel and is outlined in the wheel's colour. The Organ panel has a Rotary lamp (the first Rotary in the rack, live; click = Slow/Fast).
- **Envelope curves:** drag the points (attack peak = attack time; decay corner = decay time and sustain level; end = release). Shift = fine, double-click = default.
- **Logo:** the church lotus (light grey) beside the CHURCH SYNTH wordmark. Analog Width knobs are only active on Square and Triangle.
- **Levels:** a +6 dB output stage after Master Volume and a peak limiter at −1 dBFS (instant attack, 150 ms release, soft above −0.4 dBFS), so the synth is about as loud as other soft synths without clipping. Unison keeps the level of a single voice; looped noises are normalised to the same RMS. Factory presets are matched to a −4 dBFS peak for a 4-note chord and −7 dBFS for mono patches.
- **Presets:** `.csp` (XML) in `Documents/Church Synth/Presets`; banks All / Factory / User / Favourites, 9 categories, search, Init, Save / Save As / Delete, favourites in `Favourites.txt`. 57 factory presets built in code (11 organs since v0.5, 4 Section presets since v0.6), level-matched. Patterns (`.cspat`) and FX presets (`.csfx`) in their own folders; 10 factory patterns, 36 factory FX presets. Seq Lock keeps the pattern and Seq settings when a preset loads.
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
8. **Organ engine** (§3.5) + Organ Scanner FX + organ presets. *Done in v0.5.0.*
9. **Section engine** (§3.5) with body / bell resonators; Pack 4 and Pack 5 rebuilt on it. *Done in v0.6.0.*
10. **Modelled engine** (§3.5, experimental), only after 8 and 9 are proven.

## 10. Excluded

- Engines: Granular, Sample, Harmonic, Modal. (The Section engine's body / bell resonator is a fixed resonator bank inside that engine, not a playable Modal engine; Sample stays excluded.)
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
| 2026-11-06 | Preset packs round 13: Pack 14 Special FX (20: nature, sci-fi, transitions, oddities), notes written as sound-design lessons. Owner clarified the 'keep it tonal' guideline applies to worship sounds only; Special FX may be noise-based. |
| 2026-11-06 | Preset packs round 14 (last of the agreed order): Pack 15 Acoustic Pianos (20: grands, soul/gospel incl. an R&B ballad piano, uprights, character, piano+pad/strings layers), all synthesised. Recipe: 'Sine to Saw' wavetable string with 3 unison voices started in phase (Unison Phase 0) so they bloom and beat, a fast-fading FM strike layer (double decay), hammer noise, velocity to level/filter/table position, decay curve 20 with key-tracked decay. Notes are honest that these don't replace a sampled grand. `--render-pack` now plays a piano phrase for Keys presets tagged Piano, and takes `sweep` as a fifth argument (single notes C1–C7, then middle C at three velocities) for checking decay and velocity response. |
| 2026-11-11 | v0.3.0, learning aids for the aux keys players: preset browser tag chips collapsed to the 12 most-used (Show all tags), User bank expands into its subfolders, Favourite folders (playlists, a preset can be in several; Favourite Folders.xml); sound LEDs on engine tabs and every oscillator / noise / wavetable / modulator / Utility part (peak after its volume, engine volume and amp envelope, i.e. what goes into the filters; measured only while the editor is open, audio unchanged — a test checks it bit for bit); travelling dots on envelopes, LFOs and functions (one per sounding voice, newest bright; a shared generator shows one; unused generators don't run so show none); mod strip: dot on routed tiles, faded lines from the selected source to its knobs on screen plus +n badges on the tabs / pages hiding more targets, and clicking a knob lights up the sources modulating it. Bench unchanged within noise. |
| 2026-11-25 | v0.4.0 effects, tuned against reference renders (project doc FX-ANALYSIS.md; tools in `tools/fx_analysis`, `cs_fxcli`, `cs_state_test --render-fx`): Reverb gains Mod Depth / Mod Rate / Brightness, 6 diffusers + direct early diffusion (early kurtosis 343 → 8, target 13), brighter defaults; at Decay 2.9 s, Mod 0.6/0.65, Brightness 0.7 it matches the reference modulated reverb's RT60 per band, tail tilt and pitch smear (42 vs 45 cents RMS). Chorus: Pigments-like defaults, independent right-side LFO. Vintage Chorus: rebuilt on the classic structure (taps measured within 0.2 ms of the reference, opposite-phase L/R, I+II mono, same band coherence / side-mid), plus Brightness and Wide (Wide 1 + Brightness 1 matches the bigger poly chorus' side/mid and coherence within 0.5 dB / 0.05). Super Unison: per-side voice sets (mid-band L/R coherence 0.75 → 0.0 at 100 % wet), mono bass (side HP 250 Hz), default ±20 cents. Owner's decision: existing presets may sound different; states saved before (stateVersion < 5) get the reverb's new modulation defaults and neutral brightness (`migrateState`), as do old user FX presets (.csfx without the `v2` marker). |
| 2026-11-27 | Planned three new engine types (§3.5): **Organ** (tonewheel model + Organ Scanner FX), **Section** (independent players + body / bell resonator, for strings and brass) and **Modelled** (waveguide bowed string and lip brass, experimental, last). Aim: replace the other plugins in the MainStage rig except the acoustic piano. Owner's condition: existing sounds must not get heavier, so new engines cost nothing when not selected and every existing `cs_bench` case must stay within noise before merging. Build order Organ → Section → Modelled. |
| 2026-11-27 | v0.5.0, Milestone 8: Organ engine (`EngineType` 2; tonewheel model, §3.5 "Organ as built") and the Organ Scanner effect (FX type 21), 11 organ factory presets, 8 FX presets (scanner, rotary). Choices made: Bass All the Way Down on by default (the 16' keeps its own wheels; off = the folding of smaller organs); leakage = each drawbar's compartment partner wheel; percussion shared per engine slot in `SynthCore` and gated by the key count (sustain pedal ignored); organ voice state sits beside the other engines' (≈0.4 KB, no union needed); Expression (CC11) defaults to full until the pedal moves (MIDI convention; before, an unsent CC11 read as 0). Bench gate passed: every existing case within noise; the organ costs a fifth of the analog pad. A state test round-trips every engine type and every factory preset, and old presets get the organ defaults. |
| 2026-11-28 | v0.6.0, Milestone 9: Section engine (`EngineType` 3, §3.5 "Section as built"): independent seeded players, Dynamics driving level / brightness / vibrato, strings and brass attacks (Bite, Blat, Scoop, sforzando), bow / breath noise, 9 body / bell shapes, legato slur, a 64-player budget. 4 factory presets; Packs 4 and 5 rebuilt as **Pack 4 II** and **Pack 5 II** (originals unchanged). Choices made: brass players are a narrowing pulse (not a fixed pulse / saw blend) because only that made loud brass brighter than loud strings; noise is one band-pass per note (not per player); body banks are stereo; rebuilt presets are named "… II"; Expression is not used (Dynamics on the mod wheel, as in orchestral libraries). Bench gate: interleaved A/B against v0.4 within ±3 % on every case after moving the new engines' state to the end of the voice and keeping their render out of line. Also: long preset names no longer run into the category label in the top bar. |
| 2026-11-28 | v0.6.1: mod connection UX after Reyffer's request. Selecting a tile no longer turns every touched knob into a mod-amount editor; targets light up only while a tile is dragged; a drop starts at 0 % and opens the new **mod wheels** (hover ≈0.5 s on a modulated control: one wheel per connection, lines to the sources, the range in real units on hover/click, drag to edit). Choices: tile click = show links only; dropped amount = 0 %; amounts shown in % (the rest of the UI uses %); the wheel panel sits beside the control so the knob stays draggable. FX menus/switches became modulation targets with a live value; Organ panel Rotary lamp; Rotary Speed help explains the mod-wheel threshold (past halfway = Fast) and MIDI learn. Strings analysis plan written (docs/STRINGS-ANALYSIS.md, reference MIDI generator in `tools/strings_analysis`). |
| 2026-11-28 | v0.6.2: mod wheel lines now run from the source tiles to the control itself, not to the panel (it looked as if something behind the panel was modulated), and aren't drawn twice when the source's links are showing; the panel is outlined in the connection's colour from the moment it opens. |
| 2026-11-28 | v0.6.3 fix: the mod wheels' right-click menu (Remove / Set to 0 % / Invert) did nothing: the menu is its own window, so the panel's hover logic saw the mouse leave and closed the panel (dropping its target) before the item was picked. The panel now stays open while its menu is up and the action uses the route captured when the menu opened. The UI snapshot run is now in ctest (`plugin_ui_snapshots`) and checks these menu actions. |
| 2026-11-06 | Preset packs round 12: Pack 13 Famous Sounds (50 timbres with nod names: brass and stabs, leads, keys and bells, pads, basses and sequences, arps). Own recipes only: no copied factory patches, no riffs or melodies (generic arp / sequence patterns). The name-to-inspiration list was given to the owner in conversation, not shipped in the pack. |
| 2026-11-06 | Preset packs round 11: Pack 12 EDM (20: 7 plucks, 7 stabs, 6 leads). Supersaw = Super unison 7 voices; pump = Function Duck synced 1/4 on the amp; pitch rise = Env 3 on engine coarse; chord stabs = Chord unison. CPU note for the owner: zero-volume oscillators, noises and the Utility oscillator are skipped per voice (only computed when their volume, after modulation, is above 0, or when Osc 3 / noise feed FM); an engine that is on but at 0 volume is still rendered so modulation can bring it in. |
| 2026-11-06 | Preset packs round 10: Pack 11 Synthwave (50: 8 sequenced / played basses, 9 arps and sequences, 8 plucks, 11 pads, 8 leads, 6 keys and stabs). Pack exporter: if the check run lands more than 1.5 dB off target (random unison phases, free-running modulators), the level is corrected halfway and measured again. Known: four pack preset names repeat factory names (Sanctuary Pad, Dark Drone, Sync Lead, Glass Keys); they live in different banks, left as is. |
| 2026-11-06 | Preset packs round 9: Pack 10 Twin Layer (20, CS-80-style). Two complete layers per voice (Engine n -> Filter n in parallel, Classic LP12 Analog, Env 2 / Env 3 per layer); touch = velocity to both cutoffs, aftertouch to both cutoffs plus LFO vibrato, mirrored on Macro 3 for keyboards without aftertouch; ring-mod layers use the wavetable engine's Ring Mod on Basic Shapes; Ribbon Lead bends 7 semitones. This completes the five classic-synth packs. |
| 2026-11-06 | Preset packs round 8: Pack 9 Vintage Poly (20, Prophet-style). Osc 1 = the original's Osc B, Osc 2 = Osc A; poly-mod via a silent Osc 3 (Osc B's tuning) FM-ing Osc 2 plus Env 2 on that FM / on Osc 2's pitch with sync (sync sweep); wheel-mod = LFO routes scaled by the mod wheel; Classic LP24 with Analog on as the warm resonant filter. Names that would repeat another pack get a suffix ("Unison Bass 5"). |
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
