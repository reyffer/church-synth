// Sanity tests for the DSP core. Plain asserts, no framework.
// Run: ./cs_tests  (exit code 0 = pass)

#include "Patches.h"
#include "dsp/SynthCore.h"
#include "dsp/WavetableEngine.h"
#include "dsp/ModMatrix.h"
#include "dsp/ModSources.h"

#include <cmath>
#include <cstdio>
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
                SynthCore s;
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
                SynthCore s;
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
    SynthCore s;
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
    SynthCore s;
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
    SynthCore s;
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
    SynthCore s;
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
        SynthCore s;
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
static void testDriveLevelIsSmooth()
{
    auto levelDb = [] (float drive) {
        SynthCore s;
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
        CHECK (std::abs (lv - base) < 3.0f, "drive: level stays within 3 dB of undriven");
        prev = lv;
    }
}

// --- Milestone 2 ----------------------------------------------------------

static void testFactoryWavetablesPlay()
{
    auto& lib = ContentLibrary::get();
    for (int t = 0; t < lib.numFactoryWavetables(); ++t)
        for (float pos : { 0.0f, 0.5f, 1.0f })
        {
            SynthCore s;
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
    SynthCore s;
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
                    SynthCore s;
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
            SynthCore s;
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
        SynthCore s;
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
    SynthCore s;
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
        SynthCore s;
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
        SynthCore s;
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
        SynthCore s;
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
        SynthCore s;
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
    SynthCore s;
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
    SynthCore s;
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
                        SynthCore s;
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
    SynthCore s;
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
        SynthCore s;
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

int main()
{
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
