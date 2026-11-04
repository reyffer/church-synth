// Sanity tests for the DSP core. Plain asserts, no framework.
// Run: ./cs_tests  (exit code 0 = pass)

#include "Patches.h"
#include "dsp/SynthCore.h"

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

int main()
{
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
