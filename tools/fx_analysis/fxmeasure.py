"""FX measurements used to tune Church Synth's effects against reference renders.

Usage:
  python3 fxmeasure.py <signals_dir> <refs_dir> <fxcli> [group ...]
groups: chorus vintage jupiter unison reverb (default: all)

signals_dir: the test signals (01_click_x3.wav ... 05_sweep_20-20k_10s.wav, gen_signals.py)
refs_dir:    reference renders (juno_*, jupiter_*, pigment_*)
fxcli:       path to cs_fxcli (built from tools/fxcli.cpp)
"""
import glob, os, subprocess, sys
import numpy as np, soundfile as sf
from scipy.signal import correlate, welch, csd, butter, sosfilt

sr = 48000

def ld(p):
    x, r = sf.read(p, dtype='float64')
    assert r == sr, p
    return x if x.ndim == 2 else np.stack([x, x], 1)

def run(fxcli, type_, params, x):
    out = subprocess.run([fxcli, str(type_), params], input=x.astype('<f4').tobytes(), capture_output=True, check=True).stdout
    return np.frombuffer(out, '<f4').reshape(-1, 2).astype(np.float64)

def db(x): return 10 * np.log10(np.maximum(x, 1e-30))

# --- chorus-type ---------------------------------------------------------------------
def whiten(x):
    n = len(x); X = np.fft.rfft(x); f = np.fft.rfftfreq(n, 1 / sr)
    return np.fft.irfft(X * np.sqrt(np.maximum(f, 20)) * (f < 16000), n)

def taps(out, dry, t0, t1, win=0.04, hop=0.01, maxlag_ms=30):
    """Wet tap lag (ms) and amplitude per window, and dry-tap amplitude, for one channel."""
    ML = int(maxlag_ms / 1000 * sr); W = int(win * sr); H = int(hop * sr)
    lags = np.arange(ML + 1) / sr * 1000
    res = []
    for s in range(int(t0 * sr), int(t1 * sr) - W, H):
        d = dry[s - ML:s + W]; o = out[s:s + W]
        if np.std(o) < 1e-6: continue
        c = correlate(d, o, 'valid')[::-1]
        c /= (np.linalg.norm(o) * np.sqrt(np.mean(d ** 2) * W) + 1e-12)
        m = lags > 0.8
        k = np.argmax(np.abs(c[m]))
        res.append((s / sr, lags[m][k], abs(c[m][k]), np.max(np.abs(c[lags < 0.6]))))
    return np.array(res)

def lfo_period(t, lag):
    """Period of the tap-lag movement from the autocorrelation of the lag trace."""
    y = lag - lag.mean()
    if np.std(y) < 0.02: return np.nan
    dt = t[1] - t[0]
    ac = np.correlate(y, y, 'full')[len(y) - 1:]; ac /= ac[0]
    neg = np.argmax(ac < 0)
    if neg == 0: return np.nan
    k = neg + np.argmax(ac[neg:])
    return k * dt

bands = [(60, 150), (150, 400), (400, 1000), (1000, 2500), (2500, 6000), (6000, 12000), (12000, 18000)]

def spec_metrics(y, x, seg):
    y = y[seg]; x = x[seg]
    f, Pd = welch(x, sr, nperseg=8192)
    M = (y[:, 0] + y[:, 1]) / 2; S = (y[:, 0] - y[:, 1]) / 2
    _, Pm = welch(M, sr, nperseg=8192); _, Ps = welch(S, sr, nperseg=8192)
    _, Pl = welch(y[:, 0], sr, nperseg=8192); _, Pr = welch(y[:, 1], sr, nperseg=8192)
    _, Clr = csd(y[:, 0], y[:, 1], sr, nperseg=8192)
    out = []
    for lo, hi in bands:
        m = (f >= lo) & (f < hi)
        g = db((Pl[m].mean() + Pr[m].mean()) / 2 / Pd[m].mean())
        sm = db(Ps[m].mean() / Pm[m].mean())
        coh = np.sum(np.real(Clr[m])) / np.sqrt(np.sum(Pl[m]) * np.sum(Pr[m]))
        out.append((g, sm, coh))
    return np.array(out)

def chorus_report(name, y, dry, bursts):
    """y: output stereo, dry: mono dry (pink-noise file)."""
    w = whiten(dry); wy = np.stack([whiten(y[:, 0]), whiten(y[:, 1])], 1)
    tl = np.concatenate([taps(wy[:, 0], w, a, b) for a, b in bursts])
    tr = np.concatenate([taps(wy[:, 1], w, a, b) for a, b in bursts])
    a, b = bursts[0]
    t0 = taps(wy[:, 0], w, a, b)
    per = lfo_period(t0[:, 0], t0[:, 1])
    seg = np.zeros(len(dry), bool)
    for a, b in bursts: seg[int(a * sr):int(b * sr)] = True
    sm = spec_metrics(y, dry, seg)
    lvl = db(np.mean(y[seg] ** 2) / np.mean(dry[seg] ** 2))
    lr_same = np.max(np.abs(y[:, 0] - y[:, 1])) < 1e-6
    # opposite phase: correlation of L and R lag traces
    n = min(len(tl), len(tr))
    rho = np.corrcoef(tl[:n, 1], tr[:n, 1])[0, 1] if np.std(tl[:n, 1]) > 0.02 and np.std(tr[:n, 1]) > 0.02 else np.nan
    print(f"{name:26s} rate {1/per if per==per else float('nan'):5.2f} Hz | lag L {np.percentile(tl[:,1],3):5.1f}-{np.percentile(tl[:,1],97):5.1f} ms"
          f" R {np.percentile(tr[:,1],3):5.1f}-{np.percentile(tr[:,1],97):5.1f} | wet/dry tap {np.median(tl[:,2])/max(np.median(tl[:,3]),1e-9):4.2f}"
          f" | L/R lag corr {rho:5.2f} | mono {'yes' if lr_same else 'no '} | level {lvl:+5.1f} dB")
    print(f"{'':26s} gain/band  " + " ".join(f"{g:+5.1f}" for g in sm[:, 0]))
    print(f"{'':26s} side/mid   " + " ".join(f"{v:+5.1f}" for v in sm[:, 1]))
    print(f"{'':26s} coherence  " + " ".join(f"{v:5.2f}" for v in sm[:, 2]))

# --- reverb ---------------------------------------------------------------------------
def edc_rt(sig, lo=None, hi=None):
    x = sig
    if lo: x = sosfilt(butter(4, [lo, hi], btype='band', fs=sr, output='sos'), x)
    e = np.cumsum((x ** 2)[::-1])[::-1]; e = db(e / e[0])
    i5 = np.argmax(e < -5); i25 = np.argmax(e < -25)
    return np.nan if i25 <= i5 else 3 * (i25 - i5) / sr

def kurt(z): z = z - z.mean(); return np.mean(z ** 4) / np.mean(z ** 2) ** 2

def reverb_report(name, y, onsets, sine=None, sine_end=None, tail_start=0.012, length=7.5):
    rt_b, ons, pk, k50, tail, tilt = [], [], [], [], [], []
    for o in onsets[:2]:
        seg = y[o:o + int(length * sr)]; mono = seg.mean(1)
        env = np.sqrt(np.convolve(mono ** 2, np.ones(480) / 480, 'same'))
        ts = int(tail_start * sr); tl = env[ts:]
        ons.append((ts + np.argmax(tl > tl.max() * 10 ** (-30 / 20))) / sr); pk.append((ts + np.argmax(tl)) / sr)
        rt_b.append([edc_rt(mono[ts:], lo, hi) for lo, hi in [(88, 177), (177, 355), (355, 710), (710, 1420), (1420, 2840), (2840, 5680), (5680, 11360)]])
        k50.append(kurt(seg[int(0.05 * sr):int(0.07 * sr), 0]))
        dryE = np.sum(mono[:ts] ** 2); tail.append(db(np.sum(mono[ts:] ** 2) / dryE))
        f, P = welch(mono[int(0.2 * sr):int(1.2 * sr)], sr, nperseg=4096)
        ref = P[(f > 700) & (f < 1400)].mean()
        tilt.append([db(P[(f > lo) & (f < hi)].mean() / ref) for lo, hi in [(2500, 3500), (5000, 7000), (8500, 11500)]])
        a = seg[int(0.1 * sr):int(1.5 * sr)]
        corr = np.corrcoef(a[:, 0], a[:, 1])[0, 1]
    rt_b = np.nanmean(rt_b, 0); tilt = np.mean(tilt, 0)
    smear = np.nan
    if sine is not None:
        s = sine[int((sine_end + 0.3) * sr):int((sine_end + 1.8) * sr)].mean(1)
        N = 1 << 20; S = np.abs(np.fft.rfft(s * np.hanning(len(s)), N)); fq = np.fft.rfftfreq(N, 1 / sr)
        m = (fq > 380) & (fq < 500); Sm = 20 * np.log10(S[m] / S[m].max() + 1e-12); fm = fq[m]
        # power-weighted spread around the tone (cents, RMS), over three tail windows
        spreads = []
        for w0 in (0.2, 0.7, 1.2):
            s2 = sine[int((sine_end + w0) * sr):int((sine_end + w0 + 0.5) * sr)].mean(1)
            S2 = np.abs(np.fft.rfft(s2 * np.hanning(len(s2)), N)) ** 2
            m2 = (fq > 420) & (fq < 460); P2 = S2[m2]; c2 = 1200 * np.log2(fq[m2] / 440.0)
            mu = np.sum(P2 * c2) / np.sum(P2); spreads.append(np.sqrt(np.sum(P2 * (c2 - mu) ** 2) / np.sum(P2)))
        smear = np.mean(spreads)
    print(f"{name:26s} onset/peak {np.mean(ons)*1000:4.0f}/{np.mean(pk)*1000:4.0f} ms | RT60 " + " ".join(f"{v:4.1f}" for v in rt_b)
          + f" | tilt 3k/6k/10k " + "/".join(f"{v:+4.0f}" for v in tilt)
          + f" | smear {smear:4.0f} c | kurt50 {np.mean(k50):5.0f} | tail {np.mean(tail):+5.1f} dB | L/R corr {corr:+.2f}")

# --- main -----------------------------------------------------------------------------
def main():
    sig, refs, fxcli = sys.argv[1:4]
    groups = sys.argv[4:] or ['chorus', 'vintage', 'jupiter', 'unison', 'reverb']
    S = lambda n: ld(os.path.join(sig, n))
    R = lambda n: ld(os.path.join(refs, n))
    pink = S('04_pink_burst_x2.wav'); dry = pink[:, 0]
    bursts = [(0.6, 2.4), (8.6, 10.4)]
    extra = {}
    for a in sys.argv[4:]:
        if '=' in a: k, v = a.split('=', 1); extra[k] = v
    groups = [g for g in groups if '=' not in g] or ['chorus', 'vintage', 'jupiter', 'unison', 'reverb']
    if 'vintage' in groups:
        print('== Vintage Chorus vs Juno (pink bursts) ==')
        for m in ('I', 'II', 'I+II'):
            chorus_report(f'Juno {m}', R(f'juno_mode{m}_04_pink_burst_x2.wav'), dry, bursts)
        for m, k in (('I', '0.0'), ('II', '0.5'), ('I+II', '0.9')):
            chorus_report(f'CS Vintage {m}', run(fxcli, 15, extra.get('vint', '') .replace('MODE', k) or k, pink), dry, bursts)
    if 'jupiter' in groups:
        print('== Jupiter chorus (synth noise, dry = jupiter_dry) ==')
        jd = R('jupiter_dry_04_noise.wav'); jy = R('jupiter_chorus_04_noise.wav')
        n = min(len(jd), len(jy)); env = np.abs(jd[:n, 0]); idx = np.where(env > env.max() * 0.05)[0]
        b = [(idx[0] / sr + 0.2, min(idx[0] / sr + 2.0, n / sr - 0.1))]
        chorus_report('Jupiter chorus', jy[:n], jd[:n, 0], b)
        if 'jup' in extra:
            chorus_report('CS Vintage (wide)', run(fxcli, 15, extra['jup'], pink), dry, bursts)
    if 'chorus' in groups:
        print('== Chorus vs Pigments ==')
        chorus_report('Pigments Chorus', R('pigment_chorus_04_pink_burst_x2.wav'), dry, bursts)
        chorus_report('CS Chorus', run(fxcli, 14, extra.get('chorus', '-'), pink), dry, bursts)
    if 'unison' in groups:
        print('== Super Unison vs Pigments ==')
        chorus_report('Pigments Super Unison', R('pigment_superUnison_04_pink_burst_x2.wav'), dry, bursts)
        chorus_report('CS Super Unison', run(fxcli, 13, extra.get('unison', '-'), pink), dry, bursts)
    if 'reverb' in groups:
        print('== Reverb vs Jupiter modulated reverb ==')
        jc = R('jupiter_revModulated_01_click.wav'); jdc = R('jupiter_dry_01_click.wav')
        on = [int(i) for i in np.where(np.abs(jdc[:, 0]) > np.abs(jdc[:, 0]).max() * 0.5)[0]]
        onsets = [on[0]] + [o for o in on if o > on[0] + sr][:1]
        js = R('jupiter_revModulated_02_sine.wav'); jds = R('jupiter_dry_02_sine.wav')
        e = np.where(np.abs(jds[:, 0]) > 0.01)[0]
        reverb_report('Jupiter mod reverb', jc, onsets, js, e[-1] / sr)
        click = S('01_click_x3.wav'); sine = S('02_sine_440_10s.wav')
        co = [int(0.5 * sr), int(8.5 * sr)]
        p = extra.get('reverb', '-')
        reverb_report('CS Reverb', run(fxcli, 4, p, click), co, run(fxcli, 4, p, sine), 10.5)

if __name__ == '__main__':
    main()
