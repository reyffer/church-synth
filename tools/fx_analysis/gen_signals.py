"""Writes the five FX test signals (48 kHz / 24-bit stereo) into the current folder."""
import numpy as np, soundfile as sf
sr = 48000
def st(x): return np.stack([x, x], 1)
def fade(x, ms=10):
    n = int(sr * ms / 1000); w = np.linspace(0, 1, n); x[:n] *= w; x[-n:] *= w[::-1]; return x
def sil(s): return np.zeros(int(sr * s))
def db(d): return 10 ** (d / 20)
out = {}
# 01 clicks: 3 clicks, 8 s apart (reverb tails, delay taps)
x = sil(24.0)
for t in (0.5, 8.5, 16.5): x[int(t * sr)] = db(-6)
out['01_click_x3.wav'] = x
# 02 sine 440 Hz, 10 s + 6 s silence (chorus rate/depth, reverb pitch smear)
t = np.arange(int(10 * sr)) / sr
out['02_sine_440_10s.wav'] = np.concatenate([sil(0.5), fade(db(-12) * np.sin(2 * np.pi * 440 * t)), sil(6)])
# 03 band-limited saw 110 Hz, 10 s (brightness/richness)
saw = sum(((-1) ** (k + 1)) / k * np.sin(2 * np.pi * 110 * k * t) for k in range(1, int(20000 / 110) + 1)) * (2 / np.pi)
out['03_saw_110_10s.wav'] = np.concatenate([sil(0.5), fade(db(-12) * saw / np.max(np.abs(saw))), sil(6)])
# 04 pink noise bursts 2 s x2, 6 s gaps (EQ tilt, width, chorus taps)
rng = np.random.default_rng(1)
def pink(n):
    f = np.fft.rfftfreq(n, 1 / sr); s = np.fft.rfft(rng.standard_normal(n)); s[1:] /= np.sqrt(f[1:]); s[0] = 0
    p = np.fft.irfft(s, n); return p / np.sqrt(np.mean(p ** 2))
b = lambda: fade(db(-18) * pink(2 * sr))
out['04_pink_burst_x2.wav'] = np.concatenate([sil(0.5), b(), sil(6), b(), sil(6)])
# 05 exponential sine sweep 20 Hz-20 kHz, 10 s + 8 s silence (frequency response, reverb IR)
T = 10; f1, f2 = 20, 20000; L = T / np.log(f2 / f1); tt = np.arange(int(T * sr)) / sr
sw = np.sin(2 * np.pi * f1 * L * (np.exp(tt / L) - 1))
out['05_sweep_20-20k_10s.wav'] = np.concatenate([sil(0.5), fade(db(-12) * sw, 50), sil(8)])
for k, v in out.items():
    sf.write(k, st(v.astype(np.float32)), sr, subtype='PCM_24')
    print(k, round(len(v) / sr, 1), 's')
