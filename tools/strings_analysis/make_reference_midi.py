#!/usr/bin/env python3
"""Writes strings-reference.mid: the phrase set Reyffer renders through a sampled strings
library (and we render through the Section engine) so the attacks, vibrato, legato and
releases can be measured side by side. 120 BPM, 480 ticks per beat (1 beat = 0.5 s).

Dynamics are sent three ways so it works whatever the library listens to: note velocity,
CC1 (mod wheel, usual dynamics control) and CC11 (expression), all set just before each note.
A text marker names every section (visible in a DAW). See STRINGS-ANALYSIS.md.
Usage: make_reference_midi.py [out.mid]"""
import struct, sys

TPB = 480
SEC = TPB * 2  # ticks per second at 120 BPM

events = []  # (tick, order, bytes)

def at(sec): return int(round(sec * SEC))
def cc(t, num, val): events.append((at(t), 0, bytes([0xB0, num, val])))
def marker(t, text):
    b = text.encode('ascii')
    events.append((at(t), -1, bytes([0xFF, 0x06, len(b)]) + b))
def note(t, dur, key, vel):
    events.append((at(t), 2, bytes([0x90, key, vel])))
    events.append((at(t + dur), 1, bytes([0x80, key, 0])))
def dyn(t, v):  # v 0..127 on CC1 and CC11, a little before the note
    cc(max(0.0, t - 0.05), 1, v)
    cc(max(0.0, t - 0.05), 11, v)

P, MF, F = 40, 80, 118
t = 1.0
# A. Single notes: 5 pitches x 3 dynamics, held 4 s, 1.5 s apart (release tails).
marker(t, "A single notes p/mf/f")
for key in (55, 62, 69, 76, 81):  # G3 D4 A4 E5 A5
    for v in (P, MF, F):
        dyn(t, v); note(t, 4.0, key, v); t += 5.5
# B. Short strokes: 8 x 150 ms at mf, then f (A4), 0.4 s apart.
t += 1.0; marker(t, "B short strokes mf then f")
for v in (MF, F):
    dyn(t, v)
    for i in range(8):
        note(t, 0.15, 69, v); t += 0.4
    t += 1.5
# C. Swell: A4 held 7 s, CC1/CC11 from 20 to 120 and back.
t += 1.0; marker(t, "C swell A4")
cc(t - 0.05, 1, 20); cc(t - 0.05, 11, 20)
note(t, 7.0, 69, MF)
for i in range(1, 71):
    x = i / 70.0
    v = int(20 + 100 * (x * 2 if x < 0.5 else (1 - x) * 2))
    cc(t + x * 7.0, 1, v); cc(t + x * 7.0, 11, v)
t += 9.0
# D. Legato pairs (50 ms overlap): a step, a fifth, an octave, down a third.
marker(t, "D legato pairs mf")
for a, b in ((69, 71), (69, 76), (69, 81), (69, 66)):
    dyn(t, MF)
    note(t, 1.55, a, MF); note(t + 1.5, 1.5, b, MF); t += 4.5
# E. Detached vs legato line (same notes), mf: D major scale fragment.
line = (62, 64, 66, 67, 69, 67, 66, 64, 62)
t += 0.5; marker(t, "E detached line mf")
dyn(t, MF)
for k in line:
    note(t, 0.42, k, MF); t += 0.5
t += 2.0; marker(t, "E legato line mf")
dyn(t, MF)
for k in line:
    note(t, 0.53, k, MF); t += 0.5
t += 3.0
# F. Section chord (only with an ensemble patch): D3 A3 F#4 D5, 4 s, mf then f.
marker(t, "F chord mf then f")
for v in (MF, F):
    dyn(t, v)
    for k in (50, 57, 66, 74):
        note(t, 4.0, k, v)
    t += 6.0
marker(t, "end")

events.sort(key=lambda e: (e[0], e[1]))
def vlq(n):
    out = [n & 0x7F]; n >>= 7
    while n: out.insert(0, (n & 0x7F) | 0x80); n >>= 7
    return bytes(out)
trk = bytearray()
trk += vlq(0) + bytes([0xFF, 0x51, 0x03]) + (500000).to_bytes(3, 'big')  # 120 BPM
trk += vlq(0) + bytes([0xFF, 0x03, 17]) + b"Strings reference"
last = 0
for tick, _, data in events:
    trk += vlq(tick - last) + data; last = tick
trk += vlq(0) + bytes([0xFF, 0x2F, 0x00])
out = sys.argv[1] if len(sys.argv) > 1 else "strings-reference.mid"
with open(out, "wb") as f:
    f.write(b"MThd" + struct.pack(">IHHH", 6, 0, 1, TPB))
    f.write(b"MTrk" + struct.pack(">I", len(trk)) + trk)
print(f"wrote {out}: {last / SEC:.1f} s")
