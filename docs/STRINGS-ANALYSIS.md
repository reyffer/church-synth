# Church Synth — Section strings analysis plan (2026-11-28)

**Why.** After v0.6 Reyffer found the Section engine good for assembled strings and organs but weak for **solo strings**. The plan is the same as for the chorus effects (FX-ANALYSIS.md): render one fixed MIDI file through a sampled reference and through Church Synth, measure both, change the engine against numbers, re-measure. **Status: waiting for the reference renders.**

## What the engine does now (v0.6)
- Per note, up to 8 players: a band-limited saw (strings), each with its own detune / drift, entry delay (Looseness), vibrato rate and depth, a 12 dB low-pass and a level that rises with **one exponential** (Attack: 180 ms soft → ≈12 ms marcato).
- Bite: a brightness and noise burst decaying in ≈50 ms. One bow-noise band-pass around the note. A stereo body bank after the players.
- Vibrato fades in after Vib Delay, identical in shape for every note; dynamics move level, brightness and vibrato depth together.

## Why solo is harder than section (hypotheses to check)
In a section, six players smear each other: the start of each note and the steady tone are averaged. A single player exposes:
1. **The onset.** A real bow has a noisy, unstable phase (tens of ms, longer when soft) before the string locks into a steady tone; pitch and brightness settle during it. Ours is a clean saw fading in: either "synth pad" (slow) or "click" (fast).
2. **A static steady tone.** Real sustained notes keep changing a little: bow speed and pressure move the brightness and level, and the period jitters. Ours is perfectly steady between vibrato cycles.
3. **Vibrato through the body.** On a real instrument vibrato moves the harmonics across fixed body resonances, so the *level and tone* of each harmonic wobble too, not just the pitch. Our body bank is after a single summed signal, which may be too smooth to show this.
4. **Releases.** The bow leaving the string: a short damp-down plus the body ringing. Ours is the amp envelope's release.
5. **Legato.** A bow change or a finger change has a short dip and a small burst of noise; a slide has a portamento shape. Ours uses `slur()` with a noise boost.

## What to render (Reyffer)
- **MIDI file:** `strings-reference.mid` (made by `tools/strings_analysis/make_reference_midi.py`, ≈2.5 min, 120 BPM). Dynamics are sent as velocity, CC1 and CC11 together, so it works whichever the library uses. Markers name each section:
  - A: single notes G3, D4, A4, E5, A5 at p / mf / f, held 4 s.
  - B: short strokes at mf, then f.
  - C: a 7 s swell on A4 (CC1 / CC11 20 → 120 → 20).
  - D: legato pairs (step, fifth, octave, third down).
  - E: the same line detached, then legato.
  - F: a four-note chord at mf, then f (ensemble patch only).
- **Patches:** solo violin first; then solo cello (the same file transposed down two octaves is fine), and an ensemble patch for comparison with the section presets.
- **Settings:** 48 kHz / 24-bit WAV, **dry** (no reverb, room mics off or "close" only), default vibrato, no humanise or round-robin randomness if it can be turned off. Write down the library, patch name and any setting changed.
- One render per patch; the whole file in one WAV.

## What we measure (both renders)
| Measurement | How | Engine control it informs |
|---|---|---|
| Attack time (10 → 90 %) and overshoot | RMS envelope, 2 ms hop, per note and dynamic | Attack curve, a two-stage onset |
| Onset noise ratio over the first 300 ms | harmonic vs residual energy (pitch-synchronous) | Bite, bow noise envelope |
| Brightness over the first 300 ms and in the sustain | spectral centroid and harmonic slope (H1…H10) | brightness envelope, dynamics → brightness |
| Pitch at the onset | f0 track, cents vs the settled pitch over 0–300 ms | an onset pitch settle (like brass Scoop, much smaller) |
| Vibrato onset, rate, depth, regularity | f0 track: delay to half depth, Hz, cents, cycle-to-cycle variation | Vib Delay / Rate / Depth, per-cycle randomness |
| Amplitude and tone wobble with vibrato | per-harmonic level modulation in phase with f0 | body placement, per-player bodies |
| Steady-state micro-variation | slow level / centroid / f0 movement after vibrato is removed | a slow "bow" random source |
| Release time and tail colour | level drop and centroid after note-off | a release stage in the engine |
| Legato transition | level dip, noise burst, pitch path between notes (D, E) | `slur()` shape |
| Short-stroke shape | peak time, decay, noise share (B) | a "short" articulation |
| Body spectrum | long-term average spectrum of the sustained notes vs pitch | body bank shapes |
| Ensemble spread (F) | f0 spread (cents), onset spread (ms), L/R coherence | Intonation, Looseness, Width |

The scripts will go in `tools/strings_analysis` next to the MIDI generator, reusing the f0 / envelope helpers from `tools/fx_analysis/fxmeasure.py`. Church Synth renders of the same MIDI will come from `cs_state_test` (a `--render-midi` path to add).

## Rules for whoever builds it
- **Level-match every listening test.**
- Parameter IDs and the `SectionFamily` / `SectionBody` lists are append-only. New controls go at the end of `Parameters.cpp`; anything whose 0 would change an old preset needs `kStateVersion` + `migrateState`.
- Packs 4 II / 5 II and the four factory Section presets must keep their character, or be re-levelled and re-auditioned if a change moves them on purpose.
- Bench gate: `section_strings_8voices` / `section_brass_8voices` within noise unless a change is agreed (a solo voice can afford more per player than a section of eight).
