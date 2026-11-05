// Factory content: every wavetable and noise is synthesised here from maths,
// so the plugin ships no recorded or third-party material.
//
// ORDER IS PERMANENT (host parameters store the index). Append new items only.

#include "Content.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <random>

namespace cs::content
{
using Spec = Wavetable::Spectrum;
using cd = std::complex<double>;
constexpr double kPiD = 3.14159265358979323846;
constexpr int kH = Wavetable::kMaxHarmonic;

// ===========================================================================
// Wavetable list
// ===========================================================================
const ContentLibrary::Entry kWavetableList[] = {
    { "Basic", "Basic Shapes" },          // 0
    { "Basic", "Saw to Square" },         // 1
    { "Basic", "Pulse Width" },           // 2
    { "Basic", "Sine to Saw" },           // 3
    { "Basic", "Sine to Square" },        // 4
    { "Analog", "Warm Saw Sweep" },       // 5
    { "Analog", "Resonant LP Sweep" },    // 6
    { "Analog", "Resonant HP Sweep" },    // 7
    { "Analog", "Square LP Sweep" },      // 8
    { "Analog", "Sync Sweep" },           // 9
    { "Analog", "Sync Square" },          // 10
    { "Analog", "Vintage Saw" },          // 11
    { "Organ", "Drawbars" },              // 12
    { "Organ", "Pipe Organ" },            // 13
    { "Organ", "Flute Pipe" },            // 14
    { "Vocal", "Vowels" },                // 15
    { "Vocal", "Choir Ah to Oo" },        // 16
    { "Vocal", "Vowels High" },           // 17
    { "Vocal", "Breathy Vox" },           // 18
    { "FM", "FM Ratio 1" },               // 19
    { "FM", "FM Ratio 2" },               // 20
    { "FM", "FM Ratio 3" },               // 21
    { "FM", "FM Ratio 7" },               // 22
    { "Digital", "Digital Harmonics" },   // 23
    { "Digital", "Odd Harmonics" },       // 24
    { "Digital", "Even Harmonics" },      // 25
    { "Digital", "Harmonic Scan" },       // 26
    { "Digital", "Glass" },               // 27
    { "Digital", "Fibonacci Partials" },  // 28
    { "Pad", "String Ensemble" },         // 29
    { "Pad", "Warm Pad" },                // 30
    { "Pad", "Glass Pad" },               // 31
    { "Pad", "Shimmer Evolve" },          // 32
    { "Pad", "Airy Pad" },                // 33
    { "Shaped", "Sine Fold" },            // 34
    { "Shaped", "Triangle Fold" },        // 35
    { "Shaped", "Drive Sweep" },          // 36
    { "Shaped", "Bit Steps" },            // 37
    { "Shaped", "Rectify" },              // 38
    { "Shaped", "Comb Saw" },             // 39
};
const int kNumWavetables = (int) (sizeof (kWavetableList) / sizeof (kWavetableList[0]));

namespace
{
Spec emptySpec() { return Spec ((size_t) kH + 1, std::complex<float> {}); }

// Harmonic amplitudes in sine phase.
Spec additive (const std::function<double (int)>& amp, int maxH = kH)
{
    Spec s = emptySpec();
    for (int h = 1; h <= std::min (maxH, kH); ++h)
        s[(size_t) h] = std::complex<float> (0.0f, (float) (-0.5 * amp (h)));
    return s;
}

// Single-cycle function of phase (0..1), sampled 8x oversampled so harmonics above
// 1024 are discarded cleanly instead of folding back.
Spec fromTime (const std::function<double (double)>& f, int n = 16384)
{
    std::vector<cd> buf ((size_t) n);
    for (int i = 0; i < n; ++i)
        buf[(size_t) i] = f ((double) i / (double) n);
    fft (buf, false);
    Spec s = emptySpec();
    for (int h = 1; h <= kH && h < n / 2; ++h)
        s[(size_t) h] = std::complex<float> ((float) (buf[(size_t) h].real() / n), (float) (buf[(size_t) h].imag() / n));
    return s;
}

Spec mix (const Spec& a, const Spec& b, double t)
{
    Spec s = emptySpec();
    for (size_t h = 0; h < s.size(); ++h)
        s[h] = a[h] * (float) (1.0 - t) + b[h] * (float) t;
    return s;
}

std::vector<Spec> morph (const std::vector<Spec>& keys, int frames)
{
    std::vector<Spec> out;
    for (int f = 0; f < frames; ++f)
    {
        const double x = (double) f / (double) (frames - 1) * (double) (keys.size() - 1);
        const size_t k = std::min ((size_t) x, keys.size() - 2);
        out.push_back (mix (keys[k], keys[k + 1], x - (double) k));
    }
    return out;
}

std::vector<Spec> sweep (int frames, const std::function<Spec (double)>& gen)
{
    std::vector<Spec> out;
    for (int f = 0; f < frames; ++f)
        out.push_back (gen ((double) f / (double) (frames - 1)));
    return out;
}

double sawAmp (int h) { return 2.0 / (kPiD * h); }
double squareAmp (int h) { return (h % 2) ? 4.0 / (kPiD * h) : 0.0; }
double triAmp (int h) { return (h % 2) ? 8.0 / (kPiD * kPiD * h * h) * (((h - 1) / 2) % 2 ? -1.0 : 1.0) : 0.0; }

double pulse (double p, double duty) { return (p < duty ? 1.0 : -1.0) - (2.0 * duty - 1.0); }

// Magnitude of a resonant 2-pole low-pass at ratio x = f / fc.
double lp2 (double x, double q)
{
    const double x2 = x * x;
    return 1.0 / std::sqrt ((1.0 - x2) * (1.0 - x2) + x2 / (q * q));
}
double hp2 (double x, double q) { return x * x * lp2 (x, q); }

// Sum of formant resonances at frequency f (Hz).
double formants (double f, const double (&fr)[3], const double (&gain)[3])
{
    double a = 0.0;
    for (int i = 0; i < 3; ++i)
    {
        const double bw = 60.0 + fr[i] * 0.06;
        const double d = (f - fr[i]) / bw;
        a += gain[i] / (1.0 + d * d);
    }
    return a;
}

Spec vowel (double f0, const double (&fr)[3], const double (&gain)[3])
{
    return additive ([&] (int h) {
        const double f = f0 * h;
        if (f > 12000.0) return 0.0;
        return formants (f, fr, gain) / std::sqrt ((double) h);
    });
}

// Approximate vowel formants (Hz) for a generic adult voice.
constexpr double kA[3] = { 750, 1200, 2700 }, kE[3] = { 450, 1800, 2600 }, kI[3] = { 300, 2200, 3000 },
                 kO[3] = { 500, 850, 2600 }, kU[3] = { 330, 750, 2400 };
constexpr double kVowelGain[3] = { 1.0, 0.6, 0.25 };

std::vector<Spec> fmTable (double ratio)
{
    return sweep (64, [ratio] (double t) {
        const double index = t * 7.0;
        return fromTime ([=] (double p) { return std::sin (2.0 * kPiD * p + index * std::sin (2.0 * kPiD * ratio * p)); });
    });
}

std::vector<Spec> randomKeys (unsigned seed, int keys, int frames, double tilt, int maxH)
{
    std::mt19937 rng (seed);
    std::uniform_real_distribution<double> u (0.0, 1.0);
    std::vector<Spec> k;
    for (int i = 0; i < keys; ++i)
    {
        std::vector<double> a ((size_t) maxH + 1);
        for (int h = 1; h <= maxH; ++h)
            a[(size_t) h] = (u (rng) < 0.6 ? u (rng) : 0.0) / std::pow ((double) h, tilt);
        a[1] = std::max (a[1], 0.5);
        k.push_back (additive ([a, maxH] (int h) { return h <= maxH ? a[(size_t) h] : 0.0; }));
    }
    return morph (k, frames);
}

double wrap (double x) { return x - std::floor (x); }
} // namespace

std::vector<Spec> makeFactoryWavetable (int index)
{
    switch (index)
    {
        case 0: // Basic Shapes: sine -> triangle -> saw -> square -> 25 % pulse
            return morph ({ additive ([] (int h) { return h == 1 ? 1.0 : 0.0; }),
                            additive (triAmp), additive (sawAmp), additive (squareAmp),
                            fromTime ([] (double p) { return pulse (p, 0.25); }) },
                          64);
        case 1:
            return morph ({ additive (sawAmp), additive (squareAmp) }, 32);
        case 2:
            return sweep (64, [] (double t) { return fromTime ([t] (double p) { return pulse (p, 0.5 - 0.47 * t); }); });
        case 3:
            return sweep (64, [] (double t) { return additive ([t] (int h) { return sawAmp (h) * std::pow (t, (h - 1) * 0.12); }); });
        case 4:
            return sweep (64, [] (double t) { return additive ([t] (int h) { return squareAmp (h) * std::pow (t, (h - 1) * 0.12); }); });
        case 5: // Warm Saw Sweep: saw through a gentle 2-pole low-pass, cutoff rising
            return sweep (64, [] (double t) {
                const double fc = std::pow (300.0, t) * 1.5;
                return additive ([fc] (int h) { return sawAmp (h) * lp2 (h / fc, 0.6); });
            });
        case 6:
            return sweep (64, [] (double t) {
                const double fc = 1.0 + std::pow (200.0, t);
                return additive ([fc] (int h) { return sawAmp (h) * lp2 (h / fc, 5.0) * 0.4; });
            });
        case 7:
            return sweep (64, [] (double t) {
                const double fc = 1.0 + std::pow (30.0, t); // stays audible at the top of the sweep
                return additive ([fc] (int h) { return sawAmp (h) * hp2 (h / fc, 4.0) * 0.4; });
            });
        case 8:
            return sweep (64, [] (double t) {
                const double fc = std::pow (300.0, t) * 1.5;
                return additive ([fc] (int h) { return squareAmp (h) * lp2 (h / fc, 1.2); });
            });
        case 9: // Sync Sweep: slave saw reset by the master cycle, ratio 1 -> 7
            return sweep (64, [] (double t) {
                const double r = 1.0 + 6.0 * t;
                return fromTime ([r] (double p) { return 1.0 - 2.0 * wrap (p * r); });
            });
        case 10:
            return sweep (64, [] (double t) {
                const double r = 1.0 + 6.0 * t;
                return fromTime ([r] (double p) { return pulse (wrap (p * r), 0.5); });
            });
        case 11: // Vintage Saw: capacitor-style curved ramp, increasingly curved
            return sweep (32, [] (double t) {
                const double k = 0.5 + 6.0 * t;
                return fromTime ([k] (double p) { return 1.0 - 2.0 * (1.0 - std::exp (-k * p)) / (1.0 - std::exp (-k)); });
            });
        case 12: // Drawbars: footages 8', 4', 2 2/3', 2', 1 3/5', 1 1/3', 1' -> harmonics 1,2,3,4,5,6,8
        {
            const int hs[7] = { 1, 2, 3, 4, 5, 6, 8 };
            const double regs[6][7] = { { 1, 0, 0, 0, 0, 0, 0 },     { 1, 0.8, 0, 0, 0, 0, 0 },
                                        { 1, 0.8, 0.8, 0, 0, 0, 0 }, { 1, 1, 1, 0.6, 0, 0, 0 },
                                        { 1, 1, 1, 1, 0.5, 0.5, 0.5 }, { 1, 1, 1, 1, 1, 1, 1 } };
            std::vector<Spec> keys;
            for (auto& r : regs)
                keys.push_back (additive ([&] (int h) {
                    for (int i = 0; i < 7; ++i)
                        if (hs[i] == h) return r[i];
                    return 0.0;
                }));
            return morph (keys, 64);
        }
        case 13: // Pipe Organ: soft principal -> bright reed
            return morph ({ additive ([] (int h) { return h > 24 ? 0.0 : 1.0 / std::pow (h, 1.6) * (h == 2 || h == 4 ? 1.6 : 1.0); }),
                            additive ([] (int h) { return h > 40 ? 0.0 : 1.0 / std::pow (h, 0.9) * (h % 2 ? 1.0 : 0.35); }) },
                          32);
        case 14: // Flute Pipe: fundamental with a little 2nd/3rd, brightening
            return sweep (32, [] (double t) {
                return additive ([t] (int h) { return h == 1 ? 1.0 : h <= 5 ? (0.05 + 0.25 * t) / (h * h) * 4.0 : 0.0; });
            });
        case 15:
            return morph ({ vowel (110, kA, kVowelGain), vowel (110, kE, kVowelGain), vowel (110, kI, kVowelGain),
                            vowel (110, kO, kVowelGain), vowel (110, kU, kVowelGain) },
                          64);
        case 16:
            return morph ({ vowel (130, kA, kVowelGain), vowel (130, kO, kVowelGain), vowel (130, kU, kVowelGain) }, 48);
        case 17:
            return morph ({ vowel (220, kA, kVowelGain), vowel (220, kE, kVowelGain), vowel (220, kI, kVowelGain),
                            vowel (220, kO, kVowelGain), vowel (220, kU, kVowelGain) },
                          64);
        case 18: // Breathy Vox: vowel body plus random-phase upper harmonics
        {
            std::mt19937 rng (18);
            std::uniform_real_distribution<double> ph (0.0, 2.0 * kPiD);
            std::vector<double> phases ((size_t) kH + 1);
            for (auto& p : phases) p = ph (rng);
            return sweep (32, [&] (double t) {
                Spec s = vowel (130, kA, kVowelGain);
                for (int h = 20; h <= 160; ++h)
                {
                    const double a = 0.04 * t / std::sqrt ((double) h);
                    s[(size_t) h] += std::polar ((float) (0.5 * a), (float) phases[(size_t) h]);
                }
                return s;
            });
        }
        case 19: return fmTable (1.0);
        case 20: return fmTable (2.0);
        case 21: return fmTable (3.0);
        case 22: return fmTable (7.0);
        case 23: return randomKeys (23, 8, 64, 0.8, 48);
        case 24:
            return sweep (64, [] (double t) {
                const double n = 1.0 + t * 63.0;
                return additive ([n] (int h) { return (h % 2 && h <= n) ? 1.0 / h : 0.0; });
            });
        case 25:
            return sweep (64, [] (double t) {
                const double n = 1.0 + t * 63.0;
                return additive ([n] (int h) { return h == 1 ? 1.0 : (h % 2 == 0 && h <= n) ? 1.0 / h : 0.0; });
            });
        case 26: // Harmonic Scan: a band of harmonics walking up the series
            return sweep (64, [] (double t) {
                const double c = 1.0 + t * 31.0;
                return additive ([c] (int h) { const double d = (h - c) / 1.5; return (h == 1 ? 0.3 : 0.0) + std::exp (-d * d); });
            });
        case 27: // Glass: square-number partials, upper ones growing
            return sweep (32, [] (double t) {
                return additive ([t] (int h) {
                    const int r = (int) std::lround (std::sqrt ((double) h));
                    if (r * r != h) return 0.0;
                    return h == 1 ? 1.0 : (0.2 + 0.8 * t) / std::sqrt ((double) r);
                });
            });
        case 28: // Fibonacci partials
        {
            const int fib[] = { 1, 2, 3, 5, 8, 13, 21, 34, 55 };
            return sweep (32, [&] (double t) {
                return additive ([&] (int h) {
                    for (int i = 0; i < 9; ++i)
                        if (fib[i] == h) return std::pow (0.35 + 0.5 * t, i);
                    return 0.0;
                });
            });
        }
        case 29: // String Ensemble: saw with a body resonance hump, brightening
            return sweep (64, [] (double t) {
                const double fc = 4.0 + 60.0 * t;
                return additive ([fc] (int h) {
                    const double body = 1.0 + 0.8 * std::exp (-std::pow ((h - 6.0) / 3.0, 2.0));
                    return sawAmp (h) * body * lp2 (h / fc, 0.7);
                });
            });
        case 30: // Warm Pad: triangle-ish to soft saw
            return sweep (64, [] (double t) {
                return additive ([t] (int h) { return (h % 2 ? 1.0 : t) / std::pow ((double) h, 1.6 - 0.6 * t) * lp2 (h / (3.0 + 30.0 * t), 0.7); });
            });
        case 31: // Glass Pad: sine + octave + 12th with growing sparkle
            return sweep (64, [] (double t) {
                return additive ([t] (int h) {
                    if (h == 1) return 1.0;
                    if (h == 2) return 0.5;
                    if (h == 3) return 0.3;
                    if (h == 4 || h == 6 || h == 8) return 0.15 * t;
                    if (h >= 12 && h <= 24 && h % 4 == 0) return 0.08 * t * t;
                    return 0.0;
                });
            });
        case 32: // Shimmer Evolve: smooth random walk over harmonics 1..40
        {
            std::mt19937 rng (32);
            std::normal_distribution<double> n (0.0, 0.12);
            std::vector<double> a (41, 0.0);
            for (int h = 1; h <= 40; ++h) a[(size_t) h] = 0.6 / std::sqrt ((double) h);
            std::vector<Spec> frames;
            for (int f = 0; f < 64; ++f)
            {
                for (int h = 2; h <= 40; ++h)
                    a[(size_t) h] = std::clamp (a[(size_t) h] + n (rng) / std::sqrt ((double) h), 0.0, 1.0);
                frames.push_back (additive ([&a] (int h) { return h <= 40 ? a[(size_t) h] : 0.0; }));
            }
            return frames;
        }
        case 33: // Airy Pad: sine body with a noisy, random-phase top that rises
        {
            std::mt19937 rng (33);
            std::uniform_real_distribution<double> ph (0.0, 2.0 * kPiD);
            std::vector<double> phases ((size_t) kH + 1);
            for (auto& p : phases) p = ph (rng);
            return sweep (64, [&] (double t) {
                Spec s = additive ([] (int h) { return h == 1 ? 1.0 : h == 2 ? 0.3 : 0.0; });
                for (int h = 8; h <= 200; ++h)
                    s[(size_t) h] = std::polar ((float) (0.5 * 0.08 * t / std::sqrt ((double) h)), (float) phases[(size_t) h]);
                return s;
            });
        }
        case 34:
            return sweep (64, [] (double t) {
                const double g = 1.0 + 7.0 * t;
                return fromTime ([g] (double p) { return std::sin (0.5 * kPiD * g * std::sin (2.0 * kPiD * p)); });
            });
        case 35:
            return sweep (64, [] (double t) {
                const double g = 1.0 + 7.0 * t;
                return fromTime ([g] (double p) {
                    const double x = g * std::sin (2.0 * kPiD * p);
                    const double w = wrap ((x + 1.0) * 0.25);
                    return 1.0 - 4.0 * std::abs (w - 0.5);
                });
            });
        case 36:
            return sweep (64, [] (double t) {
                const double g = 1.0 + 30.0 * t * t;
                return fromTime ([g] (double p) { return std::tanh (g * std::sin (2.0 * kPiD * p)); });
            });
        case 37:
            return sweep (32, [] (double t) {
                const double levels = std::round (std::pow (2.0, 6.0 * (1.0 - t))) + 1.0;
                return fromTime ([levels] (double p) { return std::round (std::sin (2.0 * kPiD * p) * levels) / levels; });
            });
        case 38: // Rectify: sine -> full-wave rectified sine
            return sweep (32, [] (double t) {
                return fromTime ([t] (double p) {
                    const double s = std::sin (2.0 * kPiD * p);
                    return (1.0 - t) * s + t * (2.0 * std::abs (s) - 4.0 / kPiD);
                });
            });
        case 39: // Comb Saw: saw with a moving comb of notches
        default:
            return sweep (64, [] (double t) {
                const double d = 0.02 + 0.3 * t;
                return additive ([d] (int h) { return sawAmp (h) * std::abs (std::cos (kPiD * h * d)); });
            });
    }
}

// ===========================================================================
// Noise list
// ===========================================================================
const ContentLibrary::Entry kNoiseList[] = {
    { "Classic", "White" },          // 0
    { "Classic", "Pink" },           // 1
    { "Classic", "Brown" },          // 2
    { "Classic", "Blue" },           // 3
    { "Classic", "Violet" },         // 4
    { "Classic", "Rumble" },         // 5
    { "Classic", "White Wide" },     // 6
    { "Classic", "Pink Wide" },      // 7
    { "Classic", "Velvet" },         // 8
    { "Analog", "Analog Hiss" },     // 9
    { "Analog", "Tape Hiss" },       // 10
    { "Analog", "Static" },          // 11
    { "Texture", "Vinyl Crackle" },  // 12
    { "Texture", "Vinyl Dust" },     // 13
    { "Texture", "Rain" },           // 14
    { "Texture", "Wind" },           // 15
    { "Texture", "Ocean" },          // 16
    { "Texture", "Fire" },           // 17
    { "Texture", "Air" },            // 18
    { "Texture", "Breath" },         // 19
    { "Texture", "Sparkle Air" },    // 20
    { "Texture", "Rumble Drone" },   // 21
    { "Digital", "Bit Noise" },      // 22
    { "Digital", "Sample & Hold" },  // 23
    { "Digital", "Digital Static" }, // 24
    { "Digital", "Ring Noise" },     // 25
    { "Transient", "Click" },        // 26
    { "Transient", "Soft Click" },   // 27
    { "Transient", "Thump" },        // 28
    { "Transient", "Chiff" },        // 29
    { "Transient", "Breath Attack" },// 30
    { "Transient", "Key Click" },    // 31
    { "Transient", "Pluck Noise" },  // 32
    { "Transient", "Swell" },        // 33
};
const int kNumNoises = (int) (sizeof (kNoiseList) / sizeof (kNoiseList[0]));

namespace
{
constexpr double kSr = 48000.0;
using Buf = std::vector<double>;

struct Gen
{
    std::mt19937 rng;
    explicit Gen (unsigned seed) : rng (seed) {}
    double uni() { return std::uniform_real_distribution<double> (0.0, 1.0) (rng); }
    double bi() { return uni() * 2.0 - 1.0; }
    double gauss() { return std::normal_distribution<double> (0.0, 1.0) (rng); }
};

// Stationary noise shaped in the frequency domain. Inherently a seamless loop.
Buf spectral (Gen& g, size_t n, const std::function<double (double hz)>& shape)
{
    std::vector<cd> b (n);
    for (size_t k = 1; k < n / 2; ++k)
    {
        const double hz = (double) k * kSr / (double) n;
        const double a = hz < 20.0 ? 0.0 : shape (hz);
        const cd c (g.gauss() * a, g.gauss() * a);
        b[k] = c;
        b[n - k] = std::conj (c);
    }
    fft (b, true);
    Buf out (n);
    for (size_t i = 0; i < n; ++i)
        out[i] = b[i].real();
    return out;
}

// Smooth periodic control signal (integer cycles per loop) -> seamless modulation.
double periodic (size_t i, size_t n, double cycles, double phase = 0.0)
{
    return std::sin (2.0 * kPiD * (cycles * (double) i / (double) n + phase));
}

// Adds a decaying event at position (wrapping around the loop).
void addEvent (Buf& b, size_t pos, const Buf& ev)
{
    for (size_t i = 0; i < ev.size(); ++i)
        b[(pos + i) % b.size()] += ev[i];
}

Buf click (Gen& g, double ms, double brightness)
{
    const size_t len = (size_t) (ms * 0.001 * kSr * 4.0) + 8;
    Buf e (len);
    double lp = 0.0;
    const double coef = 0.05 + 0.9 * brightness;
    for (size_t i = 0; i < len; ++i)
    {
        const double env = std::exp (-(double) i / (ms * 0.001 * kSr));
        lp += coef * (g.bi() - lp);
        e[i] = (brightness > 0.5 ? g.bi() : lp) * env;
    }
    return e;
}

Buf tone (double hz, double decayMs, double startPhase = 0.0)
{
    const size_t len = (size_t) (decayMs * 0.001 * kSr * 5.0);
    Buf e (len);
    for (size_t i = 0; i < len; ++i)
        e[i] = std::sin (2.0 * kPiD * hz * (double) i / kSr + startPhase) * std::exp (-(double) i / (decayMs * 0.001 * kSr));
    return e;
}

// Simple resonant band-pass (state-variable), applied in place. Run twice over a loop
// so the filter state at the loop point matches (seamless).
void bandpass (Buf& b, const std::function<double (size_t)>& hzAt, double q, bool loop)
{
    double ic1 = 0.0, ic2 = 0.0;
    Buf out (b.size());
    for (int pass = 0; pass < (loop ? 2 : 1); ++pass)
        for (size_t i = 0; i < b.size(); ++i)
        {
            const double gg = std::tan (kPiD * std::min (hzAt (i), 20000.0) / kSr);
            const double k = 1.0 / q;
            const double a1 = 1.0 / (1.0 + gg * (gg + k)), a2 = gg * a1, a3 = gg * a2;
            const double v3 = b[i] - ic2;
            const double v1 = a1 * ic1 + a2 * v3;
            const double v2 = ic2 + a2 * ic1 + a3 * v3;
            ic1 = 2.0 * v1 - ic1;
            ic2 = 2.0 * v2 - ic2;
            out[i] = v1 * k;
        }
    b = out;
}

void onePole (Buf& b, double hz, bool highpass, bool loop)
{
    const double c = 1.0 - std::exp (-2.0 * kPiD * hz / kSr);
    double s = 0.0;
    Buf out (b.size());
    for (int pass = 0; pass < (loop ? 2 : 1); ++pass)
        for (size_t i = 0; i < b.size(); ++i)
        {
            s += c * (b[i] - s);
            out[i] = highpass ? b[i] - s : s;
        }
    b = out;
}

std::unique_ptr<NoiseSample> finish (const char* name, Buf l, Buf r, bool oneShot)
{
    if (r.empty())
        r = l;
    double peak = 0.0, sum2 = 0.0;
    for (size_t i = 0; i < l.size(); ++i)
    {
        peak = std::max ({ peak, std::abs (l[i]), std::abs (r[i]) });
        sum2 += l[i] * l[i];
    }
    const double rms = std::sqrt (sum2 / (double) l.size());
    // Loops: same loudness for every noise (RMS 0.25). Sparse ones (crackle,
    // dust) would need huge peaks for that, so their peaks may go up to 4x and
    // are then softly rounded above 1 (max ~2.5). One-shots: peak 0.9.
    double gain = oneShot ? 0.9 / std::max (peak, 1.0e-9) : 0.25 / std::max (rms, 1.0e-9);
    gain = std::min (gain, (oneShot ? 0.98 : 4.0) / std::max (peak, 1.0e-9));
    auto shape = [] (double x) {
        const double a = std::abs (x);
        return a <= 1.0 ? x : std::copysign (1.0 + 1.5 * (1.0 - std::exp (-(a - 1.0) / 1.5)), x);
    };

    auto ns = std::make_unique<NoiseSample>();
    ns->name = name;
    ns->sampleRate = (float) kSr;
    ns->oneShot = oneShot;
    ns->left.resize (l.size());
    ns->right.resize (l.size());
    for (size_t i = 0; i < l.size(); ++i)
    {
        ns->left[i] = (float) shape (l[i] * gain);
        ns->right[i] = (float) shape (r[i] * gain);
    }
    return ns;
}

Buf envelope (size_t n, double attackMs, double decayMs)
{
    Buf e (n);
    for (size_t i = 0; i < n; ++i)
    {
        const double t = (double) i / kSr * 1000.0;
        e[i] = (t < attackMs ? t / attackMs : 1.0) * std::exp (-std::max (0.0, t - attackMs) / decayMs);
    }
    return e;
}
} // namespace

std::unique_ptr<NoiseSample> makeFactoryNoise (int index)
{
    const char* name = kNoiseList[std::clamp (index, 0, kNumNoises - 1)].name;
    Gen g ((unsigned) (1000 + index));
    constexpr size_t kLoop = 1u << 17; // ~2.7 s at 48 kHz
    constexpr size_t kLong = 1u << 18; // ~5.5 s for slow textures

    auto flat = [] (double) { return 1.0; };
    auto pink = [] (double hz) { return 1.0 / std::sqrt (hz); };
    auto brown = [] (double hz) { return 1.0 / hz; };

    switch (index)
    {
        case 0: return finish (name, spectral (g, kLoop, flat), {}, false);
        case 1: return finish (name, spectral (g, kLoop, pink), {}, false);
        case 2: return finish (name, spectral (g, kLoop, brown), {}, false);
        case 3: return finish (name, spectral (g, kLoop, [] (double hz) { return std::sqrt (hz); }), {}, false);
        case 4: return finish (name, spectral (g, kLoop, [] (double hz) { return hz; }), {}, false);
        case 5: return finish (name, spectral (g, kLoop, [] (double hz) { return hz < 150.0 ? 1.0 / hz : 0.0; }), {}, false);
        case 6: { auto l = spectral (g, kLoop, flat); auto r = spectral (g, kLoop, flat); return finish (name, l, r, false); }
        case 7: { auto l = spectral (g, kLoop, pink); auto r = spectral (g, kLoop, pink); return finish (name, l, r, false); }
        case 8: // Velvet: sparse random +/-1 impulses
        {
            Buf b (kLoop, 0.0);
            const size_t gap = (size_t) (kSr / 2000.0);
            for (size_t i = 0; i + gap < kLoop; i += gap)
                b[i + (size_t) (g.uni() * (double) gap)] = g.uni() < 0.5 ? -1.0 : 1.0;
            return finish (name, b, {}, false);
        }
        case 9: // Analog Hiss: pink body with an airy top
            return finish (name, spectral (g, kLoop, [] (double hz) { return std::pow (hz, -0.3) / (1.0 + std::pow (hz / 14000.0, 4.0)); }), {}, false);
        case 10: // Tape Hiss: mid-high hiss with slow flutter in level
        {
            auto b = spectral (g, kLoop, [] (double hz) { return std::exp (-std::pow (std::log (hz / 5000.0), 2.0) * 0.6); });
            for (size_t i = 0; i < kLoop; ++i)
                b[i] *= 1.0 + 0.12 * periodic (i, kLoop, 3.0) + 0.06 * periodic (i, kLoop, 7.0, 0.3);
            return finish (name, b, {}, false);
        }
        case 11: // Static: bursts of crackly noise
        {
            Buf b (kLoop, 0.0);
            for (int k = 0; k < 90; ++k)
            {
                const size_t pos = (size_t) (g.uni() * kLoop);
                addEvent (b, pos, click (g, 2.0 + g.uni() * 25.0, 1.0));
            }
            auto hiss = spectral (g, kLoop, flat);
            for (size_t i = 0; i < kLoop; ++i)
                b[i] = b[i] * 3.0 + hiss[i] * 0.003;
            return finish (name, b, {}, false);
        }
        case 12: // Vinyl Crackle: rumble + faint hiss + random pops
        case 13: // Vinyl Dust: softer, fewer pops
        {
            const bool dust = index == 13;
            auto b = spectral (g, kLoop, [] (double hz) { return hz < 80.0 ? 0.004 / hz : 0.00002; });
            const int pops = dust ? 90 : 220;
            for (int k = 0; k < pops; ++k)
            {
                const double amp = std::pow (g.uni(), 3.0) * (dust ? 0.3 : 1.0);
                auto ev = click (g, dust ? 1.5 : 0.6, dust ? 0.2 : 0.9);
                for (auto& v : ev) v *= amp * (g.uni() < 0.5 ? -1.0 : 1.0);
                addEvent (b, (size_t) (g.uni() * kLoop), ev);
            }
            if (dust) onePole (b, 6000.0, false, true);
            return finish (name, b, {}, false);
        }
        case 14: // Rain: many small droplets over a soft wash
        {
            Buf l = spectral (g, kLoop, pink), r = spectral (g, kLoop, pink);
            for (auto* ch : { &l, &r })
            {
                for (auto& v : *ch) v *= 0.002;
                for (int k = 0; k < 1500; ++k)
                {
                    auto ev = tone (1500.0 + g.uni() * 4500.0, 1.5 + g.uni() * 4.0, g.uni() * 6.0);
                    const double a = 0.02 + 0.2 * std::pow (g.uni(), 2.0);
                    for (auto& v : ev) v *= a;
                    addEvent (*ch, (size_t) (g.uni() * kLoop), ev);
                }
            }
            return finish (name, l, r, false);
        }
        case 15: // Wind: band-pass noise with a slowly sweeping centre and gusts
        {
            Buf l = spectral (g, kLong, flat), r = spectral (g, kLong, flat);
            bandpass (l, [] (size_t i) { return 500.0 + 350.0 * periodic (i, kLong, 2.0) + 150.0 * periodic (i, kLong, 5.0, 0.2); }, 4.0, true);
            bandpass (r, [] (size_t i) { return 520.0 + 330.0 * periodic (i, kLong, 2.0, 0.15) + 160.0 * periodic (i, kLong, 5.0, 0.6); }, 4.0, true);
            for (size_t i = 0; i < kLong; ++i)
            {
                const double gust = 0.6 + 0.4 * periodic (i, kLong, 3.0, 0.1);
                l[i] *= gust;
                r[i] *= gust;
            }
            return finish (name, l, r, false);
        }
        case 16: // Ocean: low wash swelling in and out
        {
            Buf l = spectral (g, kLong, [] (double hz) { return 1.0 / (std::sqrt (hz) * (1.0 + std::pow (hz / 1500.0, 2.0))); });
            Buf r = spectral (g, kLong, [] (double hz) { return 1.0 / (std::sqrt (hz) * (1.0 + std::pow (hz / 1500.0, 2.0))); });
            for (size_t i = 0; i < kLong; ++i)
            {
                const double swell = std::pow (0.5 + 0.5 * periodic (i, kLong, 1.0, 0.75), 2.0) * 0.85 + 0.15;
                l[i] *= swell;
                r[i] *= swell;
            }
            return finish (name, l, r, false);
        }
        case 17: // Fire: low roar plus sharp crackles
        {
            Buf b = spectral (g, kLoop, [] (double hz) { return hz < 400.0 ? 1.0 / hz : 0.02 / hz; });
            for (auto& v : b) v *= 0.3;
            for (int k = 0; k < 160; ++k)
            {
                auto ev = click (g, 0.4 + g.uni() * 1.2, 1.0);
                const double a = 0.05 + 0.6 * std::pow (g.uni(), 2.0);
                for (auto& v : ev) v *= a;
                addEvent (b, (size_t) (g.uni() * kLoop), ev);
            }
            return finish (name, b, {}, false);
        }
        case 18: // Air: soft high noise
            return finish (name, spectral (g, kLoop, [] (double hz) { return hz < 2500.0 ? std::pow (hz / 2500.0, 2.0) : std::pow (2500.0 / hz, 0.3); }),
                           spectral (g, kLoop, [] (double hz) { return hz < 2500.0 ? std::pow (hz / 2500.0, 2.0) : std::pow (2500.0 / hz, 0.3); }), false);
        case 19: // Breath: formant-shaped noise breathing in and out
        {
            auto shape = [] (double hz) {
                auto bump = [hz] (double c, double w) { const double d = (hz - c) / w; return 1.0 / (1.0 + d * d); };
                return bump (1100.0, 500.0) + 0.6 * bump (2800.0, 900.0) + 0.15;
            };
            Buf b = spectral (g, kLong, shape);
            for (size_t i = 0; i < kLong; ++i)
                b[i] *= std::pow (0.5 + 0.5 * periodic (i, kLong, 2.0, 0.75), 1.5) * 0.9 + 0.1;
            return finish (name, b, {}, false);
        }
        case 20: // Sparkle Air: very high shimmer with twinkles
        {
            Buf l = spectral (g, kLoop, [] (double hz) { return hz < 7000.0 ? 0.0 : 1.0; });
            Buf r = spectral (g, kLoop, [] (double hz) { return hz < 7000.0 ? 0.0 : 1.0; });
            for (auto* ch : { &l, &r })
            {
                for (auto& v : *ch) v *= 0.3;
                for (int k = 0; k < 400; ++k)
                {
                    auto ev = tone (8000.0 + g.uni() * 7000.0, 3.0 + g.uni() * 10.0, g.uni() * 6.0);
                    for (auto& v : ev) v *= 0.002 + 0.01 * g.uni();
                    addEvent (*ch, (size_t) (g.uni() * kLoop), ev);
                }
            }
            return finish (name, l, r, false);
        }
        case 21: // Rumble Drone: low beating partials (integer cycles per loop) + brown noise
        {
            Buf b = spectral (g, kLong, brown);
            for (auto& v : b) v *= 0.4;
            for (double hz : { 41.0, 55.0, 61.8, 82.4 })
            {
                const double cycles = std::round (hz * (double) kLong / kSr);
                for (size_t i = 0; i < kLong; ++i)
                    b[i] += 0.002 * periodic (i, kLong, cycles) * (0.7 + 0.3 * periodic (i, kLong, 2.0, hz * 0.01));
            }
            return finish (name, b, {}, false);
        }
        case 22: // Bit Noise: 1-bit random at 12 kHz
        {
            Buf b (kLoop);
            double v = 1.0;
            for (size_t i = 0; i < kLoop; ++i)
            {
                if (i % 4 == 0) v = g.uni() < 0.5 ? -1.0 : 1.0;
                b[i] = v;
            }
            return finish (name, b, {}, false);
        }
        case 23: // Sample & Hold: random steps of random length
        {
            Buf b (kLoop);
            size_t i = 0;
            while (i < kLoop)
            {
                const double v = g.bi();
                const size_t len = (size_t) ((0.005 + 0.035 * g.uni()) * kSr);
                for (size_t k = 0; k < len && i < kLoop; ++k, ++i)
                    b[i] = v;
            }
            b[kLoop - 1] = b[0];
            onePole (b, 8000.0, false, true);
            return finish (name, b, {}, false);
        }
        case 24: // Digital Static: crushed bursts
        {
            Buf b (kLoop, 0.0);
            for (int k = 0; k < 60; ++k)
            {
                const size_t pos = (size_t) (g.uni() * kLoop);
                const size_t len = (size_t) ((0.01 + 0.08 * g.uni()) * kSr);
                const double step = 0.25 + 0.5 * g.uni();
                double held = 0.0;
                for (size_t n = 0; n < len; ++n)
                {
                    if (n % 6 == 0) held = std::round (g.bi() / step) * step;
                    b[(pos + n) % kLoop] += held;
                }
            }
            return finish (name, b, {}, false);
        }
        case 25: // Ring Noise: white noise ring-modulated by a 1 kHz sine
        {
            Buf b = spectral (g, kLoop, flat);
            const double cycles = std::round (1000.0 * (double) kLoop / kSr);
            for (size_t i = 0; i < kLoop; ++i)
                b[i] *= periodic (i, kLoop, cycles);
            return finish (name, b, {}, false);
        }
        // ---- one-shots ------------------------------------------------------
        case 26: return finish (name, click (g, 0.8, 1.0), {}, true);
        case 27: { auto c = click (g, 3.0, 0.15); return finish (name, c, {}, true); }
        case 28: // Thump: falling sine
        {
            const size_t n = (size_t) (0.5 * kSr);
            Buf b (n);
            double ph = 0.0;
            for (size_t i = 0; i < n; ++i)
            {
                const double t = (double) i / kSr;
                const double hz = 45.0 + 85.0 * std::exp (-t / 0.03);
                ph += 2.0 * kPiD * hz / kSr;
                b[i] = std::sin (ph) * std::exp (-t / 0.12);
            }
            return finish (name, b, {}, true);
        }
        case 29: // Chiff: short band-passed puff
        {
            const size_t n = (size_t) (0.4 * kSr);
            Buf b (n);
            for (auto& v : b) v = g.bi();
            bandpass (b, [] (size_t) { return 2500.0; }, 2.0, false);
            const auto e = envelope (n, 3.0, 60.0);
            for (size_t i = 0; i < n; ++i) b[i] *= e[i];
            return finish (name, b, {}, true);
        }
        case 30: // Breath Attack
        {
            const size_t n = (size_t) (1.2 * kSr);
            Buf b (n);
            for (auto& v : b) v = g.bi();
            bandpass (b, [] (size_t) { return 1200.0; }, 1.2, false);
            const auto e = envelope (n, 40.0, 250.0);
            for (size_t i = 0; i < n; ++i) b[i] *= e[i];
            return finish (name, b, {}, true);
        }
        case 31: // Key Click: two tiny mechanical clicks and a soft knock
        {
            Buf b ((size_t) (0.15 * kSr), 0.0);
            addEvent (b, 0, click (g, 0.5, 1.0));
            auto second = click (g, 0.7, 0.7);
            for (auto& v : second) v *= 0.6;
            addEvent (b, (size_t) (0.002 * kSr), second);
            auto knock = tone (180.0, 15.0);
            for (auto& v : knock) v *= 0.4;
            addEvent (b, 0, knock);
            return finish (name, b, {}, true);
        }
        case 32: // Pluck Noise: noise burst through a short feedback comb
        {
            const size_t n = (size_t) (0.6 * kSr);
            Buf b (n, 0.0);
            const size_t d = (size_t) (0.005 * kSr);
            for (size_t i = 0; i < n; ++i)
            {
                const double x = i < (size_t) (0.004 * kSr) ? g.bi() : 0.0;
                const double fb = i >= d ? 0.5 * (b[i - d] + (i > d ? b[i - d - 1] : 0.0)) * 0.97 : 0.0;
                b[i] = x + fb;
            }
            return finish (name, b, {}, true);
        }
        case 33: // Swell: rising, brightening noise that stops abruptly
        default:
        {
            const size_t n = (size_t) (1.6 * kSr);
            Buf b (n);
            double lp = 0.0;
            for (size_t i = 0; i < n; ++i)
            {
                const double t = (double) i / (double) n;
                const double c = 0.01 + 0.6 * t * t;
                lp += c * (g.bi() - lp);
                const double tail = i > n - 480 ? (double) (n - i) / 480.0 : 1.0;
                b[i] = lp * std::pow (t, 2.5) * tail;
            }
            return finish (name, b, {}, true);
        }
    }
}
} // namespace cs::content
