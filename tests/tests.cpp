// Sanity tests for the DSP core. Plain asserts, no framework.
// Run: ./cs_tests  (exit code 0 = pass)

#include "Patches.h"
#include "dsp/SynthCore.h"
#include "dsp/WavetableEngine.h"

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

int main()
{
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
