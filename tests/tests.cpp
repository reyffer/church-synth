// Sanity tests for the DSP core. Plain asserts, no framework.
// Run: ./cs_tests  (exit code 0 = pass)

#include "Patches.h"
#include "dsp/SynthCore.h"
#include "dsp/WavetableEngine.h"
#include "dsp/ModMatrix.h"
#include "dsp/ModSources.h"
#include "dsp/fx/FxHost.h"
#include "dsp/SeqArp.h"

#include <cmath>
#include <cstdio>
#include <memory>
#include <vector>

using namespace cs;

static int failures = 0;
#define CHECK(cond, msg)                                         \
    do {                                                         \
        if (! (cond)) {                                          \
            std::printf ("FAIL: %s  (%s:%d)\n", msg, __FILE__, __LINE__); \
            ++failures;                                          \
        }                                                        \
    } while (0)

struct Stats
{
    float peak = 0, rms = 0, dc = 0;
    bool finite = true;
};

static Stats renderStats (SynthCore& s, int frames, int block = 128)
{
    std::vector<float> l ((size_t) block), r ((size_t) block);
    Stats st;
    double sum2 = 0, sum = 0;
    for (int pos = 0; pos < frames; pos += block)
    {
        const int n = std::min (block, frames - pos);
        s.render (l.data(), r.data(), n);
        for (int i = 0; i < n; ++i)
        {
            if (! std::isfinite (l[(size_t) i]) || ! std::isfinite (r[(size_t) i]))
                st.finite = false;
            st.peak = std::max ({ st.peak, std::abs (l[(size_t) i]), std::abs (r[(size_t) i]) });
            sum2 += l[(size_t) i] * l[(size_t) i];
            sum += l[(size_t) i];
        }
    }
    st.rms = (float) std::sqrt (sum2 / frames);
    st.dc = (float) (sum / frames);
    return st;
}

static void testOscillatorsBoundedAndCentred()
{
    for (int w = 0; w < WaveCount; ++w)
        for (float width : { 0.1f, 0.5f, 0.9f })
            for (int note : { 24, 60, 96, 120 })
            {
                auto sPtr = std::make_unique<SynthCore>();
                SynthCore& s = *sPtr;
                s.setSampleRate (48000.0f);
                s.params.engine[0].osc[0].wave = w;
                s.params.engine[0].osc[0].width = width;
                s.params.filter[0].on = false;
                s.noteOn (note, 1.0f);
                renderStats (s, 2400); // skip attack
                const auto st = renderStats (s, 48000);
                char msg[128];
                std::snprintf (msg, sizeof msg, "osc wave %d width %.1f note %d", w, width, note);
                CHECK (st.finite, msg);
                CHECK (st.peak < 0.6f, msg);   // 1 voice * 0.25 gain * 0.8 master * ~1.2 overshoot
                CHECK (st.rms > 0.02f, msg);   // actually makes sound
                CHECK (std::abs (st.dc) < 0.02f, msg);
            }
}

static void testFiltersStable()
{
    for (int mode = 0; mode < ClassicModeCount; ++mode)
        for (float reso : { 0.0f, 0.7f, 1.0f })
            for (bool analog : { false, true })
            {
                auto sPtr = std::make_unique<SynthCore>();
                SynthCore& s = *sPtr;
                s.setSampleRate (48000.0f);
                auto& f = s.params.filter[0];
                f.mode = mode;
                f.reso = reso;
                f.analog = analog;
                f.drive = 0.5f;
                s.noteOn (48, 1.0f);
                // Sweep cutoff across the whole range while rendering.
                Stats worst;
                for (int k = 0; k < 40; ++k)
                {
                    f.cutoff = 20.0f * std::pow (1000.0f, (float) k / 39.0f);
                    const auto st = renderStats (s, 1200);
                    worst.finite = worst.finite && st.finite;
                    worst.peak = std::max (worst.peak, st.peak);
                }
                char msg[128];
                std::snprintf (msg, sizeof msg, "filter mode %d reso %.1f analog %d", mode, reso, (int) analog);
                CHECK (worst.finite, msg);
                CHECK (worst.peak < 4.0f, msg);
            }
}

static void testPolyphonyLimitAndRelease()
{
    auto sPtr = std::make_unique<SynthCore>();
    SynthCore& s = *sPtr;
    s.setSampleRate (48000.0f);
    s.params.maxVoices = 8;
    s.params.ampEnv.release = 0.05f;
    for (int n = 0; n < 20; ++n)
    {
        s.noteOn (40 + n, 0.8f);
        renderStats (s, 256);
    }
    CHECK (s.activeVoiceCount() <= 8, "never more than maxVoices active");
    for (int n = 0; n < 20; ++n)
        s.noteOff (40 + n);
    renderStats (s, 48000);
    CHECK (s.activeVoiceCount() == 0, "all voices free after release");
}

static void testStealIsRespectedOnQuickRelease()
{
    auto sPtr = std::make_unique<SynthCore>();
    SynthCore& s = *sPtr;
    s.setSampleRate (48000.0f);
    s.params.maxVoices = 1;
    s.params.voicing = VoicingPolyRotate;
    s.params.ampEnv.release = 0.02f;
    s.noteOn (60, 1.0f);
    renderStats (s, 4800);
    s.noteOn (64, 1.0f); // steals voice 0
    s.noteOff (64);      // released before the steal fade finished
    renderStats (s, 48000);
    CHECK (s.activeVoiceCount() == 0, "stolen note released during fade does not hang");
}

static void testSustainPedal()
{
    auto sPtr = std::make_unique<SynthCore>();
    SynthCore& s = *sPtr;
    s.setSampleRate (48000.0f);
    s.params.ampEnv.release = 0.05f;
    s.setSustain (true);
    s.noteOn (60, 1.0f);
    renderStats (s, 480);
    s.noteOff (60);
    renderStats (s, 24000);
    CHECK (s.activeVoiceCount() == 1, "sustain holds the note");
    s.setSustain (false);
    renderStats (s, 24000);
    CHECK (s.activeVoiceCount() == 0, "pedal up releases");
}

static void testMonoLegato()
{
    auto sPtr = std::make_unique<SynthCore>();
    SynthCore& s = *sPtr;
    s.setSampleRate (48000.0f);
    s.params.voicing = VoicingMonoLegato;
    s.params.ampEnv.release = 0.05f;
    s.noteOn (60, 1.0f);
    s.noteOn (67, 1.0f);
    renderStats (s, 4800);
    CHECK (s.activeVoiceCount() == 1, "mono uses one voice");
    s.noteOff (67);
    renderStats (s, 4800);
    CHECK (s.activeVoiceCount() == 1, "falls back to held note");
    s.noteOff (60);
    renderStats (s, 48000);
    CHECK (s.activeVoiceCount() == 0, "mono releases");
}

static void testHeavyPatchSane()
{
    for (auto p : { patches::pad(), patches::supersaw(), patches::worstCase() })
    {
        auto sPtr = std::make_unique<SynthCore>();
        SynthCore& s = *sPtr;
        s.setSampleRate (44100.0f);
        s.params = p;
        for (int n : { 48, 55, 60, 64, 67, 71, 74, 79 })
            s.noteOn (n, 0.9f);
        const auto st = renderStats (s, 44100 * 2);
        CHECK (st.finite, "heavy patch finite");
        CHECK (st.peak < 2.0f, "heavy patch level sane");
        CHECK (st.rms > 0.01f, "heavy patch audible");
    }
}

// Regression: drive used to jump +3 dB at the first touch, then get quieter.
// Since feedback round 2 it gets a few dB louder on purpose, smoothly.
static void testDriveLevelIsSmooth()
{
    auto levelDb = [] (float drive) {
        auto sPtr = std::make_unique<SynthCore>();
        SynthCore& s = *sPtr;
        s.setSampleRate (48000.0f);
        s.params.filter[0].mode = LP24;
        s.params.filter[0].cutoff = 3000.0f;
        s.params.filter[0].drive = drive;
        s.noteOn (48, 1.0f);
        renderStats (s, 24000);
        return 20.0f * std::log10 (renderStats (s, 24000).rms);
    };
    const float base = levelDb (0.0f);
    float prev = base;
    for (int i = 1; i <= 50; ++i)
    {
        const float d = (float) i / 50.0f;
        const float lv = levelDb (d);
        CHECK (std::abs (lv - prev) < 0.75f, "drive: no level jumps between neighbouring settings");
        CHECK (lv - base > -1.5f && lv - base < 5.0f, "drive: level rises at most ~5 dB (pushing the filter)");
        prev = lv;
    }
}

// Drive is audible even with the filter closed: harmonics above the cutoff go up.
static void testDriveAddsHarmonics()
{
    auto hfRatioDb = [] (float drive) {
        auto sPtr = std::make_unique<SynthCore>();
        SynthCore& s = *sPtr;
        s.setSampleRate (48000.0f);
        s.params.filter[0].mode = LP12;
        s.params.filter[0].cutoff = 700.0f;
        s.params.filter[0].drive = drive;
        s.params.masterVolume = 0.2f;
        s.noteOn (48, 1.0f);
        std::vector<float> l (48000), r (48000);
        s.render (l.data(), r.data(), 48000);
        const double f0 = 440.0 * std::pow (2.0, (48 - 69) / 12.0);
        double lo = 0.0, hi = 0.0;
        for (int h = 1; h < 40; ++h)
        {
            double re = 0.0, im = 0.0;
            for (int i = 24000; i < 48000; ++i)
            {
                const double ph = 2.0 * 3.14159265358979 * f0 * h * i / 48000.0;
                re += l[(size_t) i] * std::cos (ph);
                im -= l[(size_t) i] * std::sin (ph);
            }
            (h <= 7 ? lo : hi) += re * re + im * im;
        }
        return 10.0 * std::log10 (hi / lo);
    };
    CHECK (hfRatioDb (1.0f) - hfRatioDb (0.0f) > 4.0, "drive: brighter above the cutoff at full drive");
}


// Rotary: on Fast the horn spins at about 6.7 Hz (level wobble on a high tone at that
// rate); Slow is much slower; it ramps between the two instead of jumping.
static void testRotarySpeeds()
{
    auto wobbleHz = [] (float speedKnob, int seconds) {
        auto e = fx::makeEffect (fx::FxRotary, 48000.0f);
        float p[fx::kFxParams] {};
        const auto& d = fx::fxInfo (fx::FxRotary).defaults;
        for (int k = 0; k < fx::kFxParams; ++k) p[k] = d[(size_t) k];
        p[0] = speedKnob;
        const int n = 48000 * seconds;
        std::vector<float> l ((size_t) n), r ((size_t) n);
        for (int i = 0; i < n; ++i) l[(size_t) i] = r[(size_t) i] = 0.3f * std::sin (2.0f * 3.14159265f * 3000.0f * (float) i / 48000.0f);
        fx::FxContext fc;
        for (int i = 0; i < n; i += 256) e->process (l.data() + i, r.data() + i, 256, p, false, fc);
        // envelope of the left channel, last 2 s, in 5 ms frames; find the strongest wobble rate
        std::vector<float> env;
        for (int i = n - 96000; i + 240 <= n; i += 240)
        {
            double s2 = 0.0;
            for (int k = 0; k < 240; ++k) s2 += l[(size_t) (i + k)] * l[(size_t) (i + k)];
            env.push_back ((float) std::sqrt (s2 / 240.0));
        }
        float mean = 0.0f;
        for (float v : env) mean += v;
        mean /= (float) env.size();
        float bestHz = 0.0f, best = 0.0f;
        for (float hz = 0.25f; hz < 12.0f; hz += 0.05f)
        {
            double re = 0.0, im = 0.0;
            for (size_t k = 0; k < env.size(); ++k)
            {
                const double ph = 2.0 * 3.14159265 * hz * (double) k * 0.005;
                re += (env[k] - mean) * std::cos (ph);
                im += (env[k] - mean) * std::sin (ph);
            }
            const float m = (float) (re * re + im * im);
            if (m > best) { best = m; bestHz = hz; }
        }
        return bestHz;
    };
    const float fast = wobbleHz (1.0f, 4), slow = wobbleHz (0.0f, 6);
    CHECK (fast > 5.5f && fast < 8.0f, "rotary: fast horn about 6-7 Hz");
    CHECK (slow > 0.4f && slow < 1.4f, "rotary: slow horn below 1.5 Hz");
}


// Sequencer Generate: pitches stay in the scale, the first step plays the root,
// density changes how busy it is, and a new seed gives a new pattern.
static void testSeqGenerate()
{
    SeqParams sp;
    sp.scale = 10; // Major Pentatonic
    SeqGenerateSettings g;
    int onSparse = 0, onBusy = 0, differs = 0;
    for (uint32_t seed = 1; seed < 40; ++seed)
    {
        SeqPattern pat;
        g.density = 0.3f;
        seqGenerate (pat, sp, seed, g);
        CHECK (pat.steps[0].on && pat.steps[0].pitch == 0, "generate: first step is the root");
        for (const auto& st : pat.steps)
        {
            CHECK (kSeqScales[10].weights[(size_t) (st.pitch % 12)] > 0.0f, "generate: pitches in the scale");
            CHECK (st.pitch >= 0 && st.pitch <= 12 && std::abs (st.octave) <= 1, "generate: ranges");
            onSparse += st.on ? 1 : 0;
        }
        SeqPattern busy, other;
        g.density = 0.9f;
        seqGenerate (busy, sp, seed, g);
        seqGenerate (other, sp, seed + 100, g);
        for (int c = 0; c < kSeqSteps; ++c)
        {
            onBusy += busy.steps[(size_t) c].on ? 1 : 0;
            differs += busy.steps[(size_t) c].pitch != other.steps[(size_t) c].pitch ? 1 : 0;
        }
    }
    CHECK (onBusy > onSparse + 100, "generate: density matters");
    CHECK (differs > 200, "generate: new seed, new pattern");
}

// --- Milestone 2 ----------------------------------------------------------

static void testFactoryWavetablesPlay()
{
    auto& lib = ContentLibrary::get();
    for (int t = 0; t < lib.numFactoryWavetables(); ++t)
        for (float pos : { 0.0f, 0.5f, 1.0f })
        {
            auto sPtr = std::make_unique<SynthCore>();
            SynthCore& s = *sPtr;
            s.setSampleRate (48000.0f);
            auto& e = s.params.engine[0];
            e.type = EngineWavetable;
            e.wt.table = lib.factoryWavetable (t);
            e.wt.position = pos;
            s.params.filter[0].on = false;
            s.noteOn (57, 1.0f);
            renderStats (s, 1200);
            const auto st = renderStats (s, 9600);
            char msg[160];
            std::snprintf (msg, sizeof msg, "wavetable %d (%s) pos %.1f", t, lib.wavetableEntry (t).name, pos);
            CHECK (st.finite, msg);
            CHECK (st.rms > 0.005f, msg);
            CHECK (st.peak < 0.6f, msg);
        }
}

// Spectrum check: a high saw from the wavetable must not alias (mip-mapping works).
static void testWavetableNoAliasing()
{
    auto sPtr = std::make_unique<SynthCore>();
    SynthCore& s = *sPtr;
    s.setSampleRate (48000.0f);
    auto& e = s.params.engine[0];
    e.type = EngineWavetable;
    e.wt.table = ContentLibrary::get().factoryWavetable (1); // Saw to Square, frame 0 = saw
    e.wt.position = 0.0f;
    e.wt.morph = false;
    s.params.filter[0].on = false;
    s.params.ampVelocity = 0.0f;
    s.noteOn (100, 1.0f); // ~2637 Hz: harmonics above 9 would alias without mip-mapping
    renderStats (s, 4800);
    constexpr int n = 16384;
    std::vector<float> l (n), r (n);
    s.render (l.data(), r.data(), n);
    std::vector<std::complex<double>> b (n);
    for (int i = 0; i < n; ++i)
    {
        const double w = 0.42 - 0.5 * std::cos (2 * 3.141592653589793 * i / n) + 0.08 * std::cos (4 * 3.141592653589793 * i / n);
        b[(size_t) i] = l[(size_t) i] * w;
    }
    content::fft (b, false);
    const double f0 = 440.0 * std::pow (2.0, (100 - 69) / 12.0);
    double peak = 0.0, worstAlias = 0.0;
    for (int k = 1; k < n / 2; ++k)
    {
        const double hz = k * 48000.0 / n, mag = std::abs (b[(size_t) k]);
        peak = std::max (peak, mag);
        const double h = std::round (hz / f0);
        if (std::abs (hz - h * f0) > 80.0 && hz > 100.0 && hz < 16000.0)
            worstAlias = std::max (worstAlias, mag);
    }
    const double db = 20.0 * std::log10 (worstAlias / peak);
    CHECK (db < -45.0, "wavetable aliasing below -45 dB at 2.6 kHz");
}

static void testPhaseTransformAndFoldRanges()
{
    for (int shape = 0; shape < PdCount; ++shape)
        for (int i = 0; i <= 100; ++i)
            for (float a : { 0.0f, 0.3f, 1.0f })
            {
                const float p = WavetableEngine::transformPhase ((float) i / 101.0f, shape, a);
                CHECK (p >= 0.0f && p < 1.0f, "phase transform stays within one cycle");
            }
    for (int shape = 0; shape < FoldCount; ++shape)
        for (int i = -100; i <= 100; ++i)
        {
            const float y = WavetableEngine::fold ((float) i / 100.0f, shape, 1.0f);
            CHECK (std::isfinite (y) && std::abs (y) <= 1.01f, "fold output bounded");
        }
}

static void testAllWavetableFeatures()
{
    auto& lib = ContentLibrary::get();
    for (int fm = 0; fm < FmTypeCount; ++fm)
        for (int rt = 0; rt < RetrigCount; ++rt)
            for (int pd = 0; pd < PdCount; ++pd)
                for (int mw : { (int) ModSine, (int) ModSquare, (int) ModPinkNoise })
                {
                    auto sPtr = std::make_unique<SynthCore>();
                    SynthCore& s = *sPtr;
                    s.setSampleRate (44100.0f);
                    auto& e = s.params.engine[0];
                    e.type = EngineWavetable;
                    e.wt.table = lib.factoryWavetable (pd % lib.numFactoryWavetables());
                    e.wt.position = 0.6f;
                    e.wt.fmType = fm;
                    e.wt.fmAmount = 0.6f;
                    e.wt.pmAmount = 0.5f;
                    e.wt.retrig = rt;
                    e.wt.pdShape = pd;
                    e.wt.pdAmount = 0.5f;
                    e.wt.pdMod = 0.5f;
                    e.wt.foldShape = pd % FoldCount;
                    e.wt.foldAmount = 0.5f;
                    e.wt.foldMod = 0.5f;
                    e.wt.modWave = mw;
                    e.wt.modTuneMode = pd % ModTuneCount;
                    e.wt.modVolume = 0.3f;
                    e.unisonOn = true;
                    e.unisonMode = pd % UnisonModeCount;
                    s.noteOn (40 + pd * 5, 1.0f);
                    const auto st = renderStats (s, 4410);
                    CHECK (st.finite, "wavetable feature combination finite");
                    CHECK (st.peak < 2.0f, "wavetable feature combination bounded");
                }
}

static void testNoisesAndUtility()
{
    auto& lib = ContentLibrary::get();
    for (int k = 0; k < lib.numFactoryNoises(); ++k)
        for (bool loop : { true, false })
        {
            auto sPtr = std::make_unique<SynthCore>();
            SynthCore& s = *sPtr;
            s.setSampleRate (48000.0f);
            s.params.engine[0].on = false;
            auto& u = s.params.utility;
            u.noise[0].sample = lib.factoryNoise (k);
            u.noise[0].volume = 1.0f;
            u.noise[0].loop = loop;
            u.noise[0].length = 0.5f;
            u.noise[0].keytrack = true;
            u.noise[0].filter = (k % 3) - 1.0f;
            u.oscVolume = 0.3f;
            u.oscRoute = k % RouteCount;
            s.noteOn (36 + k, 1.0f);
            const auto st = renderStats (s, 48000 * 3);
            char msg[128];
            std::snprintf (msg, sizeof msg, "noise %d (%s) loop %d", k, lib.noiseEntry (k).name, (int) loop);
            CHECK (st.finite, msg);
            CHECK (st.peak < 1.5f, msg);
            CHECK (st.rms > 0.001f, msg);
        }
}

static void testCrossModBothWays()
{
    auto p = patches::crossMod();
    for (int mode = 0; mode < 3; ++mode)
    {
        auto sPtr = std::make_unique<SynthCore>();
        SynthCore& s = *sPtr;
        s.setSampleRate (48000.0f);
        s.params = p;
        s.params.engine[1].modFromEngine = mode >= 1; // mode 2: both directions
        s.params.engine[1].osc[0].fm = 0.5f;
        s.params.engine[0].modFromEngine = mode != 1;
        s.params.engine[1].volume = 0.4f;
        s.noteOn (60, 1.0f);
        s.noteOn (67, 1.0f);
        const auto st = renderStats (s, 48000);
        CHECK (st.finite && st.peak < 2.0f && st.rms > 0.005f, "cross-mod renders");
    }
}

static void testEngineTypeSwitchWhileHeld()
{
    auto sPtr = std::make_unique<SynthCore>();
    SynthCore& s = *sPtr;
    s.setSampleRate (48000.0f);
    s.noteOn (60, 1.0f);
    renderStats (s, 4800);
    s.params.engine[0].type = EngineWavetable;
    s.params.engine[0].wt.table = ContentLibrary::get().factoryWavetable (0);
    const auto st = renderStats (s, 4800);
    CHECK (st.finite && st.rms > 0.01f, "switching engine type on a held note keeps sounding");
    s.params.engine[0].wt.table = nullptr; // content still loading: silent, not a crash
    CHECK (renderStats (s, 4800).finite, "missing wavetable is safe");
}

static void testWavImportSlicing()
{
    std::vector<float> x (2048 * 3 + 100);
    for (size_t i = 0; i < x.size(); ++i)
    {
        const size_t frame = i / 2048;
        x[i] = (float) std::sin (2.0 * 3.141592653589793 * (double) ((frame + 1) * (i % 2048)) / 2048.0);
    }
    auto wt = Wavetable::fromSamples ("test", x.data(), x.size());
    CHECK (wt->getNumFrames() == 3, "WAV import makes one frame per 2048 samples");
    const float* f0 = wt->frame (0, 0);
    double err = 0.0;
    for (int i = 0; i < 2048; ++i)
        err = std::max (err, std::abs ((double) f0[i] - std::sin (2.0 * 3.141592653589793 * i / 2048.0)));
    CHECK (err < 1.0e-3, "imported frame reproduces the source cycle");
}

// --- Milestone 3: modulation ------------------------------------------------

// RMS of consecutive chunks; returns (max - min) in dB across chunks.
static float levelSwingDb (SynthCore& s, int chunks, int chunk)
{
    std::vector<float> l ((size_t) chunk), r ((size_t) chunk);
    float lo = 1.0e9f, hi = -1.0e9f;
    for (int c = 0; c < chunks; ++c)
    {
        s.render (l.data(), r.data(), chunk);
        double e = 0.0;
        for (float v : l) e += v * v;
        const float db = 10.0f * std::log10 ((float) (e / chunk) + 1.0e-12f);
        lo = std::min (lo, db);
        hi = std::max (hi, db);
    }
    return hi - lo;
}

static void addRoute (ModRouting& r, int src, const char* dest, float amount, int sc = -1, float scAmt = 1.0f)
{
    r.routes[r.count++] = { src, findModDest (dest), amount, sc, scAmt };
}

static void testLfoBasics()
{
    LfoParams p;
    p.rate.hz = 2.0f;
    Lfo lfo;
    Rng rng;
    lfo.reset (p, rng);
    int wraps = 0;
    float lo = 1, hi = -1;
    for (int i = 0; i < 48000 / kBlock; ++i)
    {
        lfo.tick (p, 2.0f, kBlock, 48000.0f, rng);
        wraps += lfo.wrapped ? 1 : 0;
        lo = std::min (lo, lfo.value);
        hi = std::max (hi, lfo.value);
    }
    CHECK (wraps >= 1 && wraps <= 3, "2 Hz LFO wraps twice a second");
    CHECK (lo < -0.95f && hi > 0.95f, "bipolar LFO spans -1..1");

    for (int w = 0; w < LfoWaveCount; ++w)
        for (float sym : { 0.1f, 0.5f, 0.9f })
        {
            p.wave = w;
            p.symmetry = sym;
            p.bipolar = false;
            for (int i = 0; i < 2000; ++i)
            {
                lfo.tick (p, 7.0f, kBlock, 48000.0f, rng);
                CHECK (lfo.value >= -1.0e-4f && lfo.value <= 1.0001f, "unipolar LFO stays in 0..1");
            }
        }

    Transport t;
    t.bpm = 120.0;
    RateParams r { 1.0f, true, 7 }; // 1/4
    CHECK (std::abs (rateToHz (r, t) - 2.0f) < 1.0e-4f, "1/4 at 120 BPM = 2 Hz");
    r.div = 12; // 1/8 T
    CHECK (std::abs (rateToHz (r, t) - 6.0f) < 1.0e-3f, "1/8 triplet at 120 BPM = 6 Hz");
}

static void testFunctionsAndPresets()
{
    for (int k = 0; k < kNumFunctionPresets; ++k)
        for (int i = 0; i <= 200; ++i)
        {
            const float v = functionPreset (k).eval ((float) i / 200.0f);
            CHECK (v >= -1.0e-4f && v <= 1.0001f, "function preset stays in 0..1");
        }
    for (int k = 0; k < kNumRemapShapes; ++k)
        CHECK (std::isfinite (remapShape (k).eval (0.37f)), "remap shape evaluates");

    // Envelope mode holds at the sustain point while the gate is on, then finishes.
    FunctionParams fp;
    fp.shape = &functionPreset (8); // Rise & Fall, sustain at point 2 (y = 1)
    fp.mode = FuncEnvelope;
    fp.rate.hz = 4.0f;
    FunctionGen fg;
    fg.reset();
    for (int i = 0; i < 48000 / kBlock; ++i)
        fg.tick (fp, 4.0f, true, kBlock, 48000.0f);
    CHECK (std::abs (fg.value - 1.0f) < 0.02f, "function envelope holds at sustain");
    for (int i = 0; i < 48000 / kBlock; ++i)
        fg.tick (fp, 4.0f, false, kBlock, 48000.0f);
    CHECK (fg.value < 0.02f, "function envelope finishes after release");
}

static void testRandomGenerators()
{
    Rng rng;
    for (int type = 0; type < RandTypeCount; ++type)
    {
        RandomParams rp;
        rp.type = type;
        rp.smooth = 0.5f;
        rp.jitter = 0.5f;
        RandomGen g;
        g.reset (rng);
        float src[NumModSources] {};
        for (int i = 0; i < 6000; ++i)
        {
            src[SrcLfo1] = std::sin ((float) i * 0.01f);
            g.tick (rp, 20.0f, kBlock, 48000.0f, rng, src, i % 8);
            CHECK (std::isfinite (g.value) && g.value >= -1.0001f && g.value <= 1.0001f, "random generator bounded");
        }
    }

    // Turing with flip 0 repeats with the chosen length.
    RandomParams tp;
    tp.type = RandTuring;
    tp.flip = 0.0f;
    tp.length = 5;
    RandomGen tg;
    tg.reset (rng);
    std::vector<float> seq;
    for (int i = 0; i < 4000 && seq.size() < 15; ++i)
    {
        tg.tick (tp, 50.0f, kBlock, 48000.0f, rng, nullptr, 0);
        if (tg.wrapped)
            seq.push_back (tg.value);
    }
    bool repeats = seq.size() >= 15;
    for (size_t i = 5; i < seq.size(); ++i)
        repeats = repeats && std::abs (seq[i] - seq[i - 5]) < 1.0e-6f;
    CHECK (repeats, "Turing at flip 0 loops with its length");

    // Binary probability.
    RandomParams bp;
    bp.type = RandBinary;
    bp.probability = 0.5f;
    bp.bipolar = false;
    RandomGen bg;
    bg.reset (rng);
    int ones = 0, steps = 0;
    for (int i = 0; i < 200000 && steps < 2000; ++i)
    {
        bg.tick (bp, 100.0f, kBlock, 48000.0f, rng, nullptr, 0);
        if (bg.wrapped) { ++steps; ones += bg.value > 0.5f ? 1 : 0; }
    }
    CHECK (steps == 2000 && ones > 850 && ones < 1150, "Binary at 50 % gives about half ones");
}

static void testCombinators()
{
    CombParams c;
    float lag = 0.0f;
    c.amount = 0.5f;
    c.type = CombSum;      CHECK (std::abs (combine (c, 0.2f, 0.4f, lag, 0, 0.001f) - 0.4f) < 1e-5f, "comb sum");
    c.type = CombDiff;     CHECK (std::abs (combine (c, 0.2f, 0.4f, lag, 0, 0.001f) - 0.0f) < 1e-5f, "comb diff");
    c.type = CombMultiply; CHECK (std::abs (combine (c, 0.5f, 0.5f, lag, 0, 0.001f) - 0.375f) < 1e-5f, "comb multiply");
    c.type = CombCrossfade;CHECK (std::abs (combine (c, 0.0f, 1.0f, lag, 0, 0.001f) - 0.5f) < 1e-5f, "comb crossfade");
    c.type = CombThreshold;CHECK (std::abs (combine (c, -0.5f, 0.0f, lag, 0, 0.001f) - 0.0f) < 1e-5f, "comb threshold");
    c.type = CombOffset;   CHECK (std::abs (combine (c, -0.5f, 0.0f, lag, 0, 0.001f) - 0.5f) < 1e-5f, "comb offset");
    c.type = CombDivide;   CHECK (std::isfinite (combine (c, 1.0f, -0.5f, lag, 0, 0.001f)), "comb divide is safe at zero");
    c.type = CombLag;
    lag = 0.0f;
    float v = 0.0f;
    for (int i = 0; i < 100; ++i) v = combine (c, 1.0f, 0.0f, lag, 0, 0.001f);
    CHECK (v > 0.0f && v < 0.5f, "comb lag rises slowly");
    c.type = CombRemap;
    c.remapShape = 7; // Invert
    c.amount = 1.0f;
    CHECK (std::abs (combine (c, 0.6f, 0.0f, lag, 0, 0.001f) + 0.6f) < 0.02f, "comb remap invert");
}

static void testRoutingModulates()
{
    // LFO -> cutoff makes the level move; no route keeps it steady.
    for (bool routed : { false, true })
    {
        auto sPtr = std::make_unique<SynthCore>();
        SynthCore& s = *sPtr;
        s.setSampleRate (48000.0f);
        s.params.filter[0].mode = LP24;
        s.params.filter[0].cutoff = 600.0f;
        s.params.lfo[0].rate.hz = 3.0f;
        ModRouting r;
        if (routed) addRoute (r, SrcLfo1, "f1_cutoff", 0.4f);
        s.setRouting (r);
        s.noteOn (48, 1.0f);
        renderStats (s, 4800);
        const float swing = levelSwingDb (s, 40, 1200);
        CHECK (routed ? swing > 6.0f : swing < 1.5f, routed ? "LFO->cutoff modulates" : "no route, no modulation");
    }

    // Sidechain: mod wheel at 0 disables the route, at 1 enables it.
    for (float wheel : { 0.0f, 1.0f })
    {
        auto sPtr = std::make_unique<SynthCore>();
        SynthCore& s = *sPtr;
        s.setSampleRate (48000.0f);
        s.params.lfo[0].rate.hz = 4.0f;
        ModRouting r;
        addRoute (r, SrcLfo1, "e1_vol", -0.6f, SrcModWheel, 1.0f);
        s.setRouting (r);
        s.setModWheel (wheel);
        s.noteOn (60, 1.0f);
        renderStats (s, 4800);
        const float swing = levelSwingDb (s, 40, 1200);
        CHECK (wheel > 0.5f ? swing > 6.0f : swing < 1.5f, "sidechain scales the route");
    }

    // Amp mod with an LFO as source makes a tremolo.
    {
        auto sPtr = std::make_unique<SynthCore>();
        SynthCore& s = *sPtr;
        s.setSampleRate (48000.0f);
        s.params.ampModSource = SrcLfo1;
        s.params.ampVelocity = 0.8f;
        s.params.lfo[0].rate.hz = 5.0f;
        s.noteOn (60, 1.0f);
        renderStats (s, 4800);
        CHECK (levelSwingDb (s, 40, 600) > 6.0f, "amp mod by LFO = tremolo");
    }
}

static void testGlobalVersusPolySources()
{
    for (int retrig : { (int) RetrigPolyKbd, (int) RetrigFreeRun })
    {
        auto sPtr = std::make_unique<SynthCore>();
        SynthCore& s = *sPtr;
        s.setSampleRate (48000.0f);
        s.params.voicing = VoicingPolyReassign;
        s.params.lfo[0].rate.hz = 1.3f;
        s.params.lfo[0].retrig = retrig;
        ModRouting r;
        addRoute (r, SrcLfo1, "voice_pan", 0.5f);
        s.setRouting (r);
        s.noteOn (60, 1.0f);   // voice 0
        renderStats (s, 14400); // 0.3 s later
        s.noteOn (64, 1.0f);   // voice 1
        renderStats (s, 4800);
        const float a = s.voiceSourceValue (0, SrcLfo1), b = s.voiceSourceValue (1, SrcLfo1);
        CHECK (retrig == RetrigFreeRun ? std::abs (a - b) < 1.0e-5f : std::abs (a - b) > 0.1f,
               retrig == RetrigFreeRun ? "free-running LFO is shared by voices" : "poly LFO restarts per voice");
    }
}

static void testVoiceModulatorCycles()
{
    auto sPtr = std::make_unique<SynthCore>();
    SynthCore& s = *sPtr;
    s.setSampleRate (48000.0f);
    s.params.random[0].type = RandVoiceMod;
    s.params.random[0].vmMode = VmCycle;
    s.params.random[0].vmVoices = 4;
    s.params.random[0].vmSteps = { 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f };
    ModRouting r;
    addRoute (r, SrcRand1, "voice_pan", 0.5f);
    s.setRouting (r);
    s.params.voicing = VoicingPolyReassign;
    for (int k = 0; k < 4; ++k)
    {
        s.noteOn (60 + k, 1.0f);
        renderStats (s, 256);
    }
    bool ok = true;
    for (int k = 0; k < 4; ++k)
        ok = ok && std::abs (s.voiceSourceValue (k, SrcRand1) - 0.1f * (float) (k + 1)) < 1.0e-5f;
    CHECK (ok, "voice modulator hands out steps in order");
}

static void testModQuantize()
{
    const int rootFifth = (1 << 0) | (1 << 7);
    CHECK (quantizeSemitones (4.2f, rootFifth) == 7.0f, "4 snaps to the fifth");
    CHECK (quantizeSemitones (2.0f, rootFifth) == 0.0f, "2 snaps to the root");
    CHECK (quantizeSemitones (-4.0f, rootFifth) == -5.0f, "-4 snaps to the fifth below");
    CHECK (quantizeSemitones (3.4f, 0xFFF) == 3.0f, "full mask just rounds");
}

static void testModulatedPatchSane()
{
    ModRouting r;
    auto sPtr = std::make_unique<SynthCore>();
    SynthCore& s = *sPtr;
    s.setSampleRate (48000.0f);
    s.params = patches::modulatedPad (r);
    s.setRouting (r);
    s.setModWheel (1.0f);
    for (int n : { 48, 55, 60, 64, 67, 71, 74, 79 })
        s.noteOn (n, 0.8f);
    const auto st = renderStats (s, 48000 * 2);
    CHECK (st.finite && st.peak < 2.0f && st.rms > 0.01f, "modulated pad renders sanely");
}

// --- Milestone 4: filters ----------------------------------------------------

static void testAllFilterTypesStable()
{
    for (int type = 0; type < FilterTypeCount; ++type)
        for (int sub = 0; sub < 7; ++sub)
            for (float reso : { 0.0f, 1.0f })
                for (float drive : { 0.0f, 1.0f })
                    for (float fm : { 0.0f, 0.8f })
                    {
                        if (type == FilterClassicType && sub > 0)
                            continue; // classic slopes are covered by testFiltersStable
                        auto sPtr = std::make_unique<SynthCore>();
                        SynthCore& s = *sPtr;
                        s.setSampleRate (48000.0f);
                        auto& e = s.params.engine[0];
                        e.noiseVolume = 0.5f;
                        auto& f = s.params.filter[0];
                        f.type = type;
                        f.submode = sub;
                        f.reso = reso;
                        f.drive = drive;
                        f.fm = fm;
                        f.fmSource = sub % FmSrcCount;
                        f.a = 0.3f + 0.1f * (float) sub;
                        f.b = 0.7f;
                        f.c = 0.4f;
                        s.noteOn (40, 1.0f);
                        Stats worst;
                        for (int k = 0; k < 16; ++k)
                        {
                            f.cutoff = 20.0f * std::pow (1000.0f, (float) k / 15.0f);
                            const auto st = renderStats (s, 1200);
                            worst.finite = worst.finite && st.finite;
                            worst.peak = std::max (worst.peak, st.peak);
                        }
                        char msg[160];
                        std::snprintf (msg, sizeof msg, "filter type %d sub %d reso %.0f drive %.0f fm %.1f", type, sub, reso, drive, fm);
                        CHECK (worst.finite, msg);
                        CHECK (worst.peak < 8.0f, msg);
                    }
}

// Level of white noise (low-pass test) or a low saw (high-pass test) through one filter setting.
static float filteredLevelDb (int type, int sub, float a, float cutoff, bool noise)
{
    auto sPtr = std::make_unique<SynthCore>();
    SynthCore& s = *sPtr;
    s.setSampleRate (48000.0f);
    auto& e = s.params.engine[0];
    if (noise)
    {
        e.osc[0].volume = 0.0f;
        e.noiseVolume = 1.0f;
    }
    auto& f = s.params.filter[0];
    f.type = type;
    f.submode = sub;
    f.a = a;
    f.cutoff = cutoff;
    f.reso = 0.1f;
    s.noteOn (36, 1.0f);
    renderStats (s, 4800);
    return 20.0f * std::log10 (renderStats (s, 24000).rms + 1.0e-9f);
}

static void testFilterResponses()
{
    struct Case { int type, sub; float a; bool lowpass; const char* name; };
    const Case cases[] = {
        { FilterLadder, 0, 0.5f, true, "Ladder" },
        { FilterSallenKey, 0, 0.5f, true, "Sallen-Key LP" },
        { FilterOta, 0, 0.5f, true, "OTA LP24" },
        { FilterOta, 1, 0.5f, true, "OTA LP12" },
        { FilterMultimode, 0, 0.5f, true, "Multimode LP24" },
        { FilterMultimode, 4, 0.5f, false, "Multimode HP24" },
        { FilterMultimode, 5, 0.5f, false, "Multimode HP12" },
        { FilterSvfMorph, 0, 0.1f, true, "SVF Morph LP" },
        { FilterSvfMorph, 0, 1.0f, false, "SVF Morph HP" },
        { FilterSurgeon, 0, 0.5f, true, "Surgeon LP" },
        { FilterSurgeon, 1, 0.5f, false, "Surgeon HP" },
        { FilterDrive, 1, 0.5f, true, "Drive" },
        { FilterSallenKey, 1, 0.5f, false, "Sallen-Key HP" },
    };
    for (const auto& c : cases)
    {
        // Low-pass: noise is much quieter at a low cutoff. High-pass: a low saw is much quieter at a high cutoff.
        const float lo = filteredLevelDb (c.type, c.sub, c.a, 300.0f, c.lowpass);
        const float hi = filteredLevelDb (c.type, c.sub, c.a, 8000.0f, c.lowpass);
        char msg[128];
        std::snprintf (msg, sizeof msg, "%s filters in the right direction (%.1f vs %.1f dB)", c.name, lo, hi);
        CHECK (c.lowpass ? hi - lo > 8.0f : lo - hi > 8.0f, msg);
    }
}

static void testLowpassGateCloses()
{
    for (int sub = 0; sub < 3; ++sub)
    {
        const float open = filteredLevelDb (FilterLpg, sub, 1.0f, 1000.0f, true);
        const float closed = filteredLevelDb (FilterLpg, sub, 0.0f, 1000.0f, true);
        CHECK (open - closed > 20.0f, "lowpass gate closes at level 0");
    }
}

static void testSplitRoutingKeepsBothFilters()
{
    // Split sends F1 and F2 to separate buses; summed output must still contain both.
    for (int mode = 0; mode < RoutingModeCount; ++mode)
    {
        auto sPtr = std::make_unique<SynthCore>();
        SynthCore& s = *sPtr;
        s.setSampleRate (48000.0f);
        s.params.filterOutput = mode;
        s.params.filter[1].on = true;
        s.params.engine[0].filterMix = 0.5f;
        s.noteOn (60, 1.0f);
        renderStats (s, 2400);
        const auto st = renderStats (s, 24000);
        CHECK (st.finite && st.rms > 0.02f, "filter output routing renders");
    }
}

// --- Milestone 5: FX ---------------------------------------------------------

struct FxRig
{
    std::unique_ptr<SynthCore> core = std::make_unique<SynthCore>();
    fx::FxHost host { core->getFxRack() };
    int types[fx::kNumFxSlots] {};

    explicit FxRig (float sr = 48000.0f) { core->setSampleRate (sr); }
    void setSlot (int slot, int type)
    {
        types[slot] = type;
        auto& sp = core->params.fx.slot[(size_t) slot];
        sp.type = type;
        const auto& info = fx::fxInfo (type);
        for (int k = 0; k < fx::kFxParams; ++k)
            sp.p[(size_t) k] = info.defaults[(size_t) k];
        host.rebuild (types, 48000.0f);
    }
};

static void testEveryEffectRenders()
{
    for (int type = 1; type < fx::FxTypeCount; ++type)
        for (int variant = 0; variant < 4; ++variant)
        {
            FxRig rig;
            auto& s = *rig.core;
            rig.setSlot (0, type);
            auto& sp = s.params.fx.slot[0];
            Rng rng;
            rng.seed ((uint32_t) (type * 31 + variant));
            if (variant > 0) // random settings, including extremes
                for (auto& v : sp.p)
                    v = variant == 3 ? (rng.uni() < 0.5f ? 0.0f : 1.0f) : rng.uni();
            sp.sync = variant == 2;
            s.params.engine[0].noiseVolume = 0.3f;
            s.noteOn (48, 1.0f);
            s.noteOn (55, 1.0f);
            const auto st = renderStats (s, 48000);
            s.allNotesOff();
            const auto tail = renderStats (s, 96000);
            char msg[128];
            std::snprintf (msg, sizeof msg, "effect %s variant %d", fx::fxInfo (type).name, variant);
            CHECK (st.finite && tail.finite, msg);
            CHECK (st.peak < 8.0f && tail.peak < 8.0f, msg);
        }
}

// Inserting an echo, reverb, chorus or EQ at its default settings must not
// change the loudness much (no surprise level drops when building a sound live).
static void testFxDefaultsKeepLevel()
{
    auto loudnessDb = [] (int type)
    {
        FxRig rig;
        auto& s = *rig.core;
        s.params = patches::pad();
        rig.setSlot (0, type);
        for (int n : { 48, 55, 60, 64, 67, 71, 74, 79 })
            s.noteOn (n, 0.85f);
        std::vector<float> l (48000 * 3), r (48000 * 3);
        for (size_t p = 0; p < l.size(); p += 128)
            s.render (l.data() + p, r.data() + p, 128);
        double e = 0.0;
        for (size_t i = 48000; i < l.size(); ++i)
            e += (double) l[i] * l[i] + (double) r[i] * r[i];
        return 10.0 * std::log10 (e + 1.0e-20);
    };
    const double dry = loudnessDb (fx::FxNone);
    for (int type : { fx::FxDelay, fx::FxTapeEcho, fx::FxPitchDelay, fx::FxReverb, fx::FxShimmer, fx::FxMultiband,
                      fx::FxParamEq, fx::FxBitcrusher, fx::FxSuperUnison, fx::FxChorus, fx::FxVintageChorus,
                      fx::FxFlanger, fx::FxPanner })
    {
        const double d = loudnessDb (type) - dry;
        char msg[128];
        std::snprintf (msg, sizeof msg, "%s at defaults changes level by %.1f dB", fx::fxInfo (type).name, d);
        CHECK (std::abs (d) < 1.5, msg);
    }
}

// Impulse through a slot: returns energy in [from, to) seconds.
static float impulseEnergy (int type, std::array<float, fx::kFxParams> p, float from, float to)
{
    fx::FxRack rack;
    fx::FxHost host (rack);
    int types[fx::kNumFxSlots] {};
    types[0] = type;
    host.rebuild (types, 48000.0f);
    fx::FxParams fp;
    fp.slot[0].type = type;
    fp.slot[0].p = p;
    const int n = 48000 * 3;
    std::vector<float> inL ((size_t) n, 0.0f), inR ((size_t) n, 0.0f), zero ((size_t) n, 0.0f), oL ((size_t) n), oR ((size_t) n);
    inL[0] = inR[0] = 1.0f;
    fx::FxContext ctx;
    for (int pos = 0; pos < n; pos += 512)
    {
        const fx::FxInputs in { inL.data() + pos, inR.data() + pos, zero.data() + pos, zero.data() + pos,
                                zero.data() + pos, zero.data() + pos, zero.data() + pos, zero.data() + pos };
        rack.process (fp, in, oL.data() + pos, oR.data() + pos, std::min (512, n - pos), ctx);
    }
    double e = 0.0;
    for (int i = (int) (from * 48000); i < (int) (to * 48000) && i < n; ++i)
        e += oL[(size_t) i] * oL[(size_t) i] + oR[(size_t) i] * oR[(size_t) i];
    return (float) e;
}

static void testDelayAndReverbBehave()
{
    auto d = fx::fxInfo (fx::FxDelay).defaults;
    d[0] = std::log (250.0f) / std::log (2000.0f); // 250 ms
    d[7] = 1.0f;
    CHECK (impulseEnergy (fx::FxDelay, d, 0.24f, 0.26f) > 0.1f, "delay repeats at 250 ms");
    CHECK (impulseEnergy (fx::FxDelay, d, 0.1f, 0.2f) < 1.0e-4f, "delay is silent before the first repeat");

    auto rv = fx::fxInfo (fx::FxReverb).defaults;
    rv[7] = 1.0f;
    const float early = impulseEnergy (fx::FxReverb, rv, 0.05f, 0.5f);
    const float late = impulseEnergy (fx::FxReverb, rv, 2.5f, 3.0f);
    CHECK (early > 1.0e-3f, "reverb produces a tail");
    CHECK (late < early * 0.05f, "reverb tail decays");

    auto sh = fx::fxInfo (fx::FxShimmer).defaults;
    sh[8] = 1.0f;
    CHECK (impulseEnergy (fx::FxShimmer, sh, 0.05f, 1.0f) > 1.0e-3f, "shimmer produces a tail");
}

static void testCompressorReducesPeaks()
{
    FxRig rig;
    auto& s = *rig.core;
    s.params.engine[0].osc[0].volume = 1.0f;
    s.noteOn (48, 1.0f);
    renderStats (s, 4800);
    const float dry = renderStats (s, 24000).rms;
    rig.setSlot (0, fx::FxCompressor);
    auto& p = s.params.fx.slot[0].p;
    p[0] = 0.3f;  // threshold -42 dB
    p[1] = 0.8f;  // high ratio
    p[5] = 0.0f;  // no auto makeup
    renderStats (s, 4800);
    const float comp = renderStats (s, 24000).rms;
    CHECK (comp < dry * 0.5f, "compressor turns loud signals down");
}

static void testFxRoutingAndAux()
{
    for (int routing = 0; routing < fx::FxRoutingCount; ++routing)
        for (bool pre : { false, true })
        {
            FxRig rig;
            auto& s = *rig.core;
            rig.setSlot (0, fx::FxChorus);
            rig.setSlot (3, fx::FxDelay);
            rig.setSlot (6, fx::FxReverb);
            s.params.fx.slot[6].p[7] = 1.0f;
            s.params.fx.routing = routing;
            s.params.fx.auxSend = 0.5f;
            s.params.fx.auxPre = pre;
            s.params.filterOutput = routing == 2 ? RoutingSplit : RoutingSum;
            s.params.filter[1].on = true;
            s.params.engine[0].filterMix = 0.5f;
            s.noteOn (60, 1.0f);
            const auto st = renderStats (s, 24000);
            CHECK (st.finite && st.rms > 0.01f && st.peak < 4.0f, "FX routing renders");
        }

    // Aux send off = no aux effect; on = reverb tail after the note stops.
    for (float send : { 0.0f, 0.8f })
    {
        FxRig rig;
        auto& s = *rig.core;
        rig.setSlot (6, fx::FxReverb);
        s.params.fx.slot[6].p[7] = 1.0f;
        s.params.fx.slot[6].p[2] = 0.7f;
        s.params.fx.auxSend = send;
        s.params.ampEnv.release = 0.01f;
        s.noteOn (60, 1.0f);
        renderStats (s, 24000);
        s.noteOff (60);
        renderStats (s, 4800);
        const float tail = renderStats (s, 12000).rms;
        CHECK (send > 0.0f ? tail > 1.0e-3f : tail < 1.0e-5f, "aux reverb tail follows the send");
    }
}

static void testFxModulationByMacro()
{
    FxRig rig;
    auto& s = *rig.core;
    rig.setSlot (0, fx::FxMultiFilter); // LP12 at default cutoff
    auto& p = s.params.fx.slot[0].p;
    p[1] = 0.2f; // dark
    ModRouting r;
    addRoute (r, SrcMacro1, "fx_a1_p2", 0.7f); // macro opens the FX filter
    s.setRouting (r);
    s.params.engine[0].osc[0].volume = 0.0f;
    s.params.engine[0].noiseVolume = 1.0f;
    s.noteOn (60, 1.0f);
    renderStats (s, 4800);
    const float closed = renderStats (s, 12000).rms;
    s.params.macro[0] = 1.0f;
    renderStats (s, 4800);
    const float open = renderStats (s, 12000).rms;
    CHECK (open > closed * 2.0f, "macro modulates an FX parameter");
}

static void testFxSwapIsSafe()
{
    // Changing effect types while audio runs: old effects are only freed after renders.
    FxRig rig;
    auto& s = *rig.core;
    s.noteOn (60, 1.0f);
    for (int k = 0; k < 40; ++k)
    {
        rig.types[0] = 1 + k % (fx::FxTypeCount - 1);
        s.params.fx.slot[0].type = rig.types[0];
        rig.host.sync (rig.types, 48000.0f, s.getRenderEpoch());
        CHECK (renderStats (s, 512).finite, "audio keeps running while effects change");
    }
    rig.host.sync (rig.types, 48000.0f, s.getRenderEpoch() + 10);
    CHECK (rig.host.pendingFrees() == 0, "replaced effects are freed after the audio moved on");
}


// --- Milestone 6: Seq / Arp ----------------------------------------------------

struct SeqEvent
{
    int64_t time;
    int note;
    bool on;
    int slideFrom;
};

struct SeqRecorder final : SeqArp::Sink
{
    std::vector<SeqEvent> events;
    int64_t now = 0;
    void seqNoteOn (int note, float, int slideFrom, float) noexcept override { events.push_back ({ now, note, true, slideFrom }); }
    void seqNoteOff (int note) noexcept override { events.push_back ({ now, note, false, -1 }); }
    std::vector<SeqEvent> ons() const
    {
        std::vector<SeqEvent> r;
        for (const auto& e : events)
            if (e.on)
                r.push_back (e);
        return r;
    }
};

struct SeqRig
{
    std::unique_ptr<SeqArp> seq = std::make_unique<SeqArp>();
    SeqParams p;
    Transport t;
    SeqRecorder rec;
    SeqRig()
    {
        seq->setSampleRate (48000.0f);
        p.on = true;
        t.bpm = 120.0;
        t.playing = false;
    }
    // Runs for `samples`, in host blocks of `block`.
    void run (int64_t samples, int block = 512)
    {
        const int64_t end = rec.now + samples;
        while (rec.now < end)
        {
            int left = (int) std::min<int64_t> (block, end - rec.now);
            while (left > 0)
            {
                const int n = seq->run (left, p, t, rec);
                rec.now += n;
                left -= n;
                if (t.playing)
                    t.ppq += (double) n * t.bpm / 60.0 / 48000.0;
            }
        }
    }
};

static std::vector<int> notesOf (const std::vector<SeqEvent>& ev)
{
    std::vector<int> r;
    for (const auto& e : ev)
        r.push_back (e.note);
    return r;
}

static void testArpOrdersAndTiming()
{
    struct Case { int order, octaves; std::vector<int> keys, expect; };
    const std::vector<Case> cases {
        { ArpUp, 1, { 64, 60, 67 }, { 60, 64, 67, 60, 64, 67 } },
        { ArpDown, 1, { 64, 60, 67 }, { 67, 64, 60, 67, 64, 60 } },
        { ArpUpDownIncl, 1, { 60, 64, 67 }, { 60, 64, 67, 67, 64, 60, 60 } },
        { ArpUpDownExcl, 1, { 60, 64, 67 }, { 60, 64, 67, 64, 60, 64, 67 } },
        { ArpAsPlayed, 1, { 67, 60, 64 }, { 67, 60, 64, 67 } },
        { ArpUp, 2, { 60, 64, 67 }, { 60, 64, 67, 72, 76, 79, 60 } },
    };
    for (const auto& c : cases)
    {
        SeqRig rig;
        rig.p.mode = SeqModeArp;
        rig.p.arpOrder = c.order;
        rig.p.arpOctaves = c.octaves;
        for (int k : c.keys)
            rig.seq->keyDown (k, 0.9f, false, false);
        rig.run (6000 * (int64_t) c.expect.size() - 10); // 1/16 at 120 BPM = 6000 samples
        const auto ons = rig.rec.ons();
        CHECK (notesOf (ons) == c.expect, "arp order");
        for (size_t i = 0; i < ons.size(); ++i)
            CHECK (ons[i].time == (int64_t) i * 6000, "arp steps land on the 1/16 grid");
    }

    // Gate 50 %: each note ends half a step later; on/off counts balance after release.
    SeqRig rig;
    rig.seq->keyDown (60, 0.9f, false, false);
    rig.run (6000 * 4);
    for (const auto& e : rig.rec.events)
        if (! e.on)
            CHECK (e.time % 6000 == 3000, "gate 50 % ends mid-step");
    rig.seq->keyUp (60, false, false);
    rig.run (48000);
    int bal = 0;
    for (const auto& e : rig.rec.events)
        bal += e.on ? 1 : -1;
    CHECK (bal == 0, "every arp note gets its note-off");
}

static void testSeqSwingAndPitch()
{
    SeqRig rig;
    rig.p.mode = SeqModeSeq;
    rig.p.swing = 0.75f;
    SeqPattern pat;
    const int pitches[4] = { 0, 7, 12, 3 };
    for (int i = 0; i < kSeqSteps; ++i)
        pat.steps[(size_t) i].pitch = pitches[i % 4];
    rig.seq->setPattern (pat);
    rig.seq->keyDown (48, 0.9f, false, false);
    rig.run (24000 - 10);
    const auto ons = rig.rec.ons();
    CHECK (notesOf (ons) == std::vector<int> ({ 48, 55, 60, 51 }), "seq pitch track transposes the held key");
    if (ons.size() == 4)
    {
        CHECK (ons[0].time == 0 && ons[2].time == 12000, "even steps stay on the grid");
        CHECK (ons[1].time == 9000 && ons[3].time == 21000, "75 % swing delays odd steps by half a step");
    }

    // Chord held in Seq mode: every step plays all keys.
    SeqRig chord;
    chord.p.mode = SeqModeSeq;
    chord.seq->keyDown (60, 0.9f, false, false);
    chord.seq->keyDown (64, 0.9f, false, false);
    chord.run (6000 * 2 - 10);
    CHECK (chord.rec.ons().size() == 4, "seq plays the whole held chord each step");

    // Scale snap: C# (1) snaps down to C in Major.
    CHECK (seqSnapToScale (1, kSeqScales[1].weights) == 0, "snap to scale");
    CHECK (seqSnapToScale (6, kSeqScales[1].weights) == 5, "snap ties go down");
    CHECK (seqSnapToScale (10, kSeqScales[1].weights) == 9, "snap to nearest");
}

static void testSeqStepsProbabilityAndPolymetry()
{
    {
        SeqRig rig;
        SeqPattern pat;
        for (int i = 0; i < kSeqSteps; ++i)
            pat.steps[(size_t) i].on = (i % 2) == 0;
        rig.seq->setPattern (pat);
        rig.seq->keyDown (60, 0.9f, false, false);
        rig.run (6000 * 8 - 10);
        CHECK (rig.rec.ons().size() == 4, "off steps are rests");
    }
    {
        SeqRig rig;
        SeqPattern pat;
        for (auto& st : pat.steps)
            st.prob = 0.0f;
        rig.seq->setPattern (pat);
        rig.seq->keyDown (60, 0.9f, false, false);
        rig.run (48000);
        CHECK (rig.rec.ons().empty(), "probability 0 never plays");
    }
    {
        SeqRig rig;
        SeqPattern pat;
        for (auto& st : pat.steps)
            st.prob = 0.5f;
        rig.seq->setPattern (pat);
        rig.seq->keyDown (60, 0.9f, false, false);
        rig.run (6000 * 400);
        const auto n = rig.rec.ons().size();
        CHECK (n > 150 && n < 250, "probability 50 % plays about half the steps");
    }
    {
        // Polymetry: pitch track 3 steps long against 16 for the rest.
        SeqRig rig;
        rig.p.mode = SeqModeSeq;
        rig.p.polymetry = true;
        SeqPattern pat;
        pat.tracks[TrackPitch].length = 3;
        pat.steps[0].pitch = 0;
        pat.steps[1].pitch = 4;
        pat.steps[2].pitch = 7;
        rig.seq->setPattern (pat);
        rig.seq->keyDown (60, 0.9f, false, false);
        rig.run (6000 * 7 - 10);
        CHECK (notesOf (rig.rec.ons()) == std::vector<int> ({ 60, 64, 67, 60, 64, 67, 60 }), "polymetry: track length 3");
    }
    {
        // Length 4 + Ping-Pong.
        SeqRig rig;
        rig.p.mode = SeqModeSeq;
        rig.p.length = 4;
        rig.p.direction = SeqPingPong;
        SeqPattern pat;
        for (int i = 0; i < 4; ++i)
            pat.steps[(size_t) i].pitch = i;
        rig.seq->setPattern (pat);
        rig.seq->keyDown (60, 0.9f, false, false);
        rig.run (6000 * 8 - 10);
        CHECK (notesOf (rig.rec.ons()) == std::vector<int> ({ 60, 61, 62, 63, 62, 61, 60, 61 }), "ping-pong");
    }
    {
        // Track rate divider: octave track moves every 2 steps.
        SeqRig rig;
        rig.p.mode = SeqModeSeq;
        SeqPattern pat;
        pat.tracks[TrackOctave].divider = 2;
        pat.steps[1].octave = 1;
        rig.seq->setPattern (pat);
        rig.seq->keyDown (60, 0.9f, false, false);
        rig.run (6000 * 4 - 10);
        CHECK (notesOf (rig.rec.ons()) == std::vector<int> ({ 60, 60, 72, 72 }), "track divider");
    }
}

static void testSeqHoldSlideAndHostSync()
{
    {
        SeqRig rig;
        rig.seq->keyDown (60, 0.9f, true, false);
        rig.seq->keyUp (60, true, false);
        rig.run (6000 * 3 - 10);
        CHECK (rig.rec.ons().size() == 3, "hold latches released keys");
        rig.seq->keyDown (67, 0.9f, true, false); // new chord after letting go replaces the latch
        rig.rec.events.clear();
        rig.run (6000 * 3);
        for (const auto& e : rig.rec.ons())
            CHECK (e.note == 67, "a new key replaces the latched one");
        rig.seq->pedalChanged (false, false); // Hold switched off
        rig.seq->keyUp (67, false, false);
        rig.rec.events.clear();
        rig.run (48000);
        CHECK (rig.rec.ons().empty(), "hold off: the latch is released");
    }
    {
        // Slide on step 2: the note glides from step 1 (no note-off in between).
        SeqRig rig;
        rig.p.mode = SeqModeSeq;
        SeqPattern pat;
        pat.steps[1].pitch = 5;
        pat.steps[1].slide = 0.5f;
        rig.seq->setPattern (pat);
        rig.seq->keyDown (60, 0.9f, false, false);
        rig.run (6000 * 2 - 10);
        const auto ons = rig.rec.ons();
        CHECK (ons.size() == 2 && ons[1].slideFrom == 60 && ons[1].note == 65, "slide step glides from the previous note");
        bool offBeforeSlide = false;
        for (const auto& e : rig.rec.events)
            if (! e.on && e.note == 60 && e.time < 6000)
                offBeforeSlide = true;
        CHECK (! offBeforeSlide, "the note before a slide is held into it");
    }
    {
        // Host playing: steps follow the song position.
        SeqRig rig;
        rig.t.playing = true;
        rig.t.ppq = 0.1; // 0.4 of a 1/16 step past the grid
        rig.seq->keyDown (60, 0.9f, false, false);
        rig.run (6000 * 2);
        const auto ons = rig.rec.ons();
        CHECK (! ons.empty() && ons[0].time == 3600, "host sync: first note on the next grid step");
        rig.rec.events.clear();
        rig.t.ppq = 8.0; // loop jump to bar 3
        rig.run (6000 * 2);
        CHECK (rig.rec.ons().size() == 2 && rig.rec.ons()[0].time % 6000 == 0, "host sync: follows a jump");
    }
    {
        // A key pressed a little after the beat still plays that step at once.
        SeqRig rig;
        rig.t.playing = true;
        rig.t.ppq = 0.04;
        rig.seq->keyDown (60, 0.9f, false, false);
        rig.run (100);
        CHECK (rig.rec.ons().size() == 1 && rig.rec.ons()[0].time == 0, "late key press plays the missed step");
    }
}

static void testSeqRandomness()
{
    SeqPattern pat;
    pat.tracks[TrackPitch].random = 1.0f;
    pat.tracks[TrackVelocity].random = 0.5f;
    SeqParams p;
    p.mode = SeqModeSeq;
    p.scale = 1; // Major
    auto pitches = [&] (uint32_t seed) {
        std::vector<int> v;
        for (int c = 0; c < kSeqSteps; ++c)
        {
            int cols[NumSeqTracks];
            for (auto& x : cols) x = c;
            v.push_back (seqResolve (pat, p, cols, seed).pitch);
        }
        return v;
    };
    CHECK (pitches (7) == pitches (7), "random values repeat for the same seed");
    CHECK (pitches (7) != pitches (seqNextSeed (7)), "Regen changes the random values");
    for (int x : pitches (99))
        CHECK (kSeqScales[1].weights[(size_t) (x % 12)] > 0.0f, "random pitches stay in the scale");

    // Locked column keeps its values on Regen.
    pat.steps[3].lockSeed = 7;
    CHECK (pitches (7)[3] == pitches (12345)[3], "locked column keeps its random values");

    // Apply random bakes the offsets in: resolves the same with random off.
    SeqPattern baked = pat;
    seqApplyRandom (baked, p, 7);
    CHECK (baked.tracks[TrackPitch].random == 0.0f, "apply random clears the amounts");
    std::vector<int> bakedPitches;
    for (int c = 0; c < kSeqSteps; ++c)
        bakedPitches.push_back (baked.steps[(size_t) c].pitch);
    CHECK (bakedPitches == pitches (7), "apply random keeps what was playing");

    // Auto-regen: values change from bar to bar.
    SeqRig rig;
    rig.p = p;
    rig.p.on = true;
    rig.p.autoRegen = 5; // 1 bar
    rig.seq->setPattern (pat);
    rig.seq->keyDown (60, 0.9f, false, false);
    const uint32_t s0 = rig.seq->uiSeed.load();
    rig.run (6000 * 17);
    CHECK (rig.seq->uiSeed.load() != s0, "auto-regen changes the seed each bar");
}

static void testSeqInTheSynth()
{
    auto sPtr = std::make_unique<SynthCore>();
    SynthCore& s = *sPtr;
    s.setSampleRate (48000.0f);
    s.params = patches::init();
    s.params.seq.on = true;
    s.params.seq.mode = SeqModeArp;
    s.noteOn (60, 0.9f);
    s.noteOn (64, 0.9f);
    s.noteOn (67, 0.9f);
    const auto st = renderStats (s, 48000);
    CHECK (st.finite && st.rms > 0.001f, "arp makes sound");
    CHECK (s.activeVoiceCount() <= 3, "arp at 50 % gate plays one note at a time (plus releases)");
    s.noteOff (60);
    s.noteOff (64);
    s.noteOff (67);
    renderStats (s, 48000 * 3);
    CHECK (s.activeVoiceCount() == 0, "arp: nothing left sounding after release");

    // Toggling the Seq/Arp with keys down leaves nothing stuck.
    s.noteOn (60, 0.9f);
    renderStats (s, 12000);
    s.params.seq.on = false;
    renderStats (s, 12000);
    CHECK (s.activeVoiceCount() >= 1, "seq off: the held key sounds normally");
    s.params.seq.on = true;
    renderStats (s, 12000);
    s.noteOff (60);
    renderStats (s, 48000 * 3);
    CHECK (s.activeVoiceCount() == 0, "seq toggles leave no stuck notes");

    // Chord arp: Unison Chord (major triad) is spread by the arp.
    s.params.engine[0].unisonOn = true;
    s.params.engine[0].unisonMode = UnisonChord;
    s.params.engine[0].chord = 3; // Major
    s.params.engine[0].unisonVoices = 3;
    s.params.seq.arpOrder = ArpUp;
    s.noteOn (60, 0.9f);
    const auto chordSt = renderStats (s, 24000);
    CHECK (chordSt.finite && chordSt.rms > 0.001f, "chord arp makes sound");
    s.noteOff (60);
    renderStats (s, 48000 * 3);
    CHECK (s.activeVoiceCount() == 0, "chord arp leaves nothing stuck");

    // Sustain pedal latches the arp's keys instead of the voices.
    s.params.engine[0].unisonOn = false;
    s.setSustain (true);
    s.noteOn (62, 0.9f);
    s.noteOff (62);
    renderStats (s, 24000);
    CHECK (s.activeVoiceCount() >= 1, "pedal keeps the arp running");
    s.setSustain (false);
    renderStats (s, 48000 * 3);
    CHECK (s.activeVoiceCount() == 0, "pedal up: arp stops");
}

static void testChordArpNotes()
{
    SeqRig rig;
    int iv[3] = { 0, 4, 7 };
    rig.seq->setChord (iv, 3);
    rig.p.arpOrder = ArpUp;
    rig.seq->keyDown (60, 0.9f, false, false);
    rig.run (6000 * 4 - 10);
    CHECK (notesOf (rig.rec.ons()) == std::vector<int> ({ 60, 64, 67, 60 }), "chord arp spreads the unison chord");
}

static void testSeqRateChangeAndOverlap()
{
    SeqRig rig;
    SeqPattern pat;
    for (auto& st : pat.steps)
        st.gate = 3.0f; // overlapping notes of the same pitch
    rig.seq->setPattern (pat);
    rig.seq->keyDown (60, 0.9f, false, false);
    rig.run (6000 * 5);
    rig.p.rateDiv = 10; // 1/8
    rig.run (48000);
    rig.p.rateDiv = 15; // 1/16 T
    rig.run (48000);
    rig.seq->keyUp (60, false, false);
    rig.run (48000 * 2);
    // Same-pitch overlaps end the old note first: on/off alternate per note.
    int depth = 0, maxDepth = 0;
    for (const auto& e : rig.rec.events)
    {
        depth += e.on ? 1 : -1;
        maxDepth = std::max (maxDepth, depth);
    }
    CHECK (depth == 0 && maxDepth == 1, "same-pitch overlaps never stack");
}


// --- Feedback round 1 -----------------------------------------------------

// FX power off: dry signal (no reverb tail); on again: effects come back.
static void testFxPowerSwitch()
{
    FxRig rig;
    auto& s = *rig.core;
    rig.setSlot (0, fx::FxReverb);
    s.params.fx.slot[0].p[7] = 1.0f; // fully wet
    s.params.ampEnv.release = 0.01f;
    auto tail = [&] {
        s.noteOn (60, 1.0f);
        renderStats (s, 12000);
        s.noteOff (60);
        renderStats (s, 4800);
        return renderStats (s, 9600).rms;
    };
    const float onTail = tail();
    s.params.fx.on = false;
    renderStats (s, 48000); // let the crossfade finish
    const float offTail = tail();
    CHECK (onTail > 1.0e-3f, "fx on: reverb tail");
    CHECK (offTail < 1.0e-5f, "fx off: dry, no tail");
    const auto st = renderStats (s, 4800);
    CHECK (st.finite, "fx off renders");
}

// The output never goes over 0 dBFS, even with a hot patch and full master volume.
static void testOutputLimiter()
{
    auto sPtr = std::make_unique<SynthCore>();
    SynthCore& s = *sPtr;
    s.setSampleRate (48000.0f);
    s.params = patches::supersaw();
    s.params.masterVolume = 1.0f;
    for (int n : { 36, 48, 55, 60, 64, 67, 71, 74, 79, 84 })
        s.noteOn (n, 1.0f);
    const auto st = renderStats (s, 48000);
    CHECK (st.finite, "limiter: finite");
    CHECK (st.peak <= 1.0f, "limiter: peak at most 0 dBFS");
    CHECK (st.rms > 0.1f, "limiter: still loud");
}

// Unison doesn't drop the level below a single voice (feedback: it used to).
static void testUnisonKeepsLevel()
{
    auto rmsDb = [] (bool uni, int voices, int mode) {
        auto sPtr = std::make_unique<SynthCore>();
        SynthCore& s = *sPtr;
        s.setSampleRate (48000.0f);
        s.params.masterVolume = 0.25f; // stay below the limiter
        auto& e = s.params.engine[0];
        e.unisonOn = uni;
        e.unisonMode = mode;
        e.unisonVoices = voices;
        s.noteOn (48, 1.0f);
        renderStats (s, 4800);
        std::vector<float> l (128), r (128);
        double sum = 0.0;
        for (int i = 0; i < 48000; i += 128)
        {
            s.render (l.data(), r.data(), 128);
            for (int k = 0; k < 128; ++k)
                sum += 0.5 * ((double) l[(size_t) k] * l[(size_t) k] + (double) r[(size_t) k] * r[(size_t) k]);
        }
        return (float) (10.0 * std::log10 (sum / 48000.0)); // both channels
    };
    const float single = rmsDb (false, 1, UnisonClassic);
    // (detuned voices partly add up in phase on low notes, so allow a little extra)
    for (int v = 2; v <= kMaxUnison; ++v)
    {
        const float d = rmsDb (true, v, UnisonClassic) - single;
        CHECK (d > -1.0f && d < 3.0f, "classic unison keeps the level");
    }
    const float d = rmsDb (true, 4, UnisonSuper) - single;
    CHECK (d > -1.0f && d < 3.0f, "super unison keeps the level");
}

// Vintage Chorus: wide (lots of side signal) at the default settings.
static void testVintageChorusIsWide()
{
    FxRig rig;
    auto& s = *rig.core;
    rig.setSlot (0, fx::FxVintageChorus);
    s.params.engine[0].stereo = 0.0f;
    s.noteOn (48, 1.0f);
    std::vector<float> l (96000), r (96000);
    for (int i = 0; i < 96000; i += 128)
        s.render (l.data() + i, r.data() + i, 128);
    double mid = 0, side = 0;
    for (size_t i = 24000; i < l.size(); ++i)
    {
        mid += 0.25 * (l[i] + r[i]) * (l[i] + r[i]);
        side += 0.25 * (l[i] - r[i]) * (l[i] - r[i]);
    }
    CHECK (10.0 * std::log10 (side / mid) > -3.0, "vintage chorus: side within 3 dB of mid");
}

int main()
{
    testDriveAddsHarmonics();
    testRotarySpeeds();
    testSeqGenerate();
    testFxPowerSwitch();
    testOutputLimiter();
    testUnisonKeepsLevel();
    testVintageChorusIsWide();
    testArpOrdersAndTiming();
    testSeqSwingAndPitch();
    testSeqStepsProbabilityAndPolymetry();
    testSeqHoldSlideAndHostSync();
    testSeqRandomness();
    testSeqInTheSynth();
    testChordArpNotes();
    testSeqRateChangeAndOverlap();
    testEveryEffectRenders();
    testDelayAndReverbBehave();
    testCompressorReducesPeaks();
    testFxRoutingAndAux();
    testFxModulationByMacro();
    testFxSwapIsSafe();
    testFxDefaultsKeepLevel();
    testAllFilterTypesStable();
    testFilterResponses();
    testLowpassGateCloses();
    testSplitRoutingKeepsBothFilters();
    testLfoBasics();
    testFunctionsAndPresets();
    testRandomGenerators();
    testCombinators();
    testRoutingModulates();
    testGlobalVersusPolySources();
    testVoiceModulatorCycles();
    testModQuantize();
    testModulatedPatchSane();
    testFactoryWavetablesPlay();
    testWavetableNoAliasing();
    testPhaseTransformAndFoldRanges();
    testAllWavetableFeatures();
    testNoisesAndUtility();
    testCrossModBothWays();
    testEngineTypeSwitchWhileHeld();
    testWavImportSlicing();
    testDriveLevelIsSmooth();
    testOscillatorsBoundedAndCentred();
    testFiltersStable();
    testPolyphonyLimitAndRelease();
    testStealIsRespectedOnQuickRelease();
    testSustainPedal();
    testMonoLegato();
    testHeavyPatchSane();
    if (failures == 0)
        std::printf ("All tests passed.\n");
    return failures == 0 ? 0 : 1;
}
