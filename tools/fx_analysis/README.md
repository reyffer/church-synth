# FX analysis

Measures Church Synth's effects against reference renders (other synths' choruses
and reverbs run on the same test signals). Results and targets: the project doc
`church-synth/FX-ANALYSIS.md`.

1. `python3 gen_signals.py` writes the five test signals (or use the existing ones).
2. Render the references in the other plugins (100 % wet or their usual mix; note it).
3. Build `cs_fxcli` (`cmake --build build --target cs_fxcli`).
4. `python3 fxmeasure.py <signals> <refs> build/cs_fxcli [vintage|jupiter|chorus|unison|reverb] [key=p1,p2,...]`
   prints rate, tap delays, L/R phase, mono, level, per-band gain, side/mid and L/R
   coherence (chorus-type), or RT60 per band, tail tilt, pitch smear, early density and
   tail level (reverb). `reverb=`, `chorus=`, `unison=`, `vint=` (MODE = the mode knob)
   and `jup=` override our knob values (0..1, `-` keeps the default).
5. `cs_state_test --render-fx <signals> <out>` writes WAVs of the standard set
   (defaults, the three Vintage modes, Wide Poly, Lush reverb) for listening.

Always level-match when listening: the reference choruses add dry + wet without
compensation (+3 to +4 dB).
