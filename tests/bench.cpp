// CPU benchmark + reference renders.
//
//   ./cs_bench            -> prints CPU use per patch (% of one core, real time)
//   ./cs_bench renders/   -> also writes WAV files of each patch to that folder
//
// "% of one core" = time spent rendering / duration of audio rendered.
// MainStage runs each channel strip on one core, so this is the number that
// matters for crackles.

#include "Patches.h"
#include "WavWriter.h"
#include "dsp/SynthCore.h"

#include <chrono>
#include <cstdio>
#include <string>
#include <vector>

using namespace cs;

struct Case
{
    const char* name;
    SynthParams params;
    std::vector<int> notes;
    ModRouting routing {};
};

static Case modulated (const std::vector<int>& notes)
{
    Case c { "modulated_pad_8voices", {}, notes };
    c.params = patches::modulatedPad (c.routing);
    return c;
}

int main (int argc, char** argv)
{
    const std::string outDir = argc > 1 ? argv[1] : "";
    constexpr int sr = 48000;
    constexpr int block = 128;
    constexpr float seconds = 8.0f;

    {
        // First-use generation cost of all factory content (happens once per process, off the audio thread).
        const auto g0 = std::chrono::steady_clock::now();
        auto& lib = ContentLibrary::get();
        for (int i = 0; i < lib.numFactoryWavetables(); ++i) lib.factoryWavetable (i);
        for (int i = 0; i < lib.numFactoryNoises(); ++i) lib.factoryNoise (i);
        const double ms = std::chrono::duration<double, std::milli> (std::chrono::steady_clock::now() - g0).count();
        std::printf ("content generation (all %d wavetables + %d noises): %.0f ms\n\n",
                     lib.numFactoryWavetables(), lib.numFactoryNoises(), ms);
    }
    const std::vector<int> chord8 { 48, 55, 60, 64, 67, 71, 74, 79 };
    const std::vector<Case> cases {
        { "init_1voice", patches::init(), { 60 } },
        { "init_8voices", patches::init(), chord8 },
        { "pad_8voices", patches::pad(), chord8 },
        { "supersaw_8voices", patches::supersaw(), chord8 },
        { "wavetable_pad_8voices", patches::wavetablePad(), chord8 },
        modulated (chord8),
        { "utility_8voices", patches::withUtility(), chord8 },
        { "crossmod_8voices", patches::crossMod(), chord8 },
        { "wavetable_heavy_16voices", patches::wavetableHeavy(),
          { 36, 43, 48, 52, 55, 59, 60, 62, 64, 67, 69, 71, 72, 74, 76, 79 } },
        { "worstcase_16voices", patches::worstCase(),
          { 36, 43, 48, 52, 55, 59, 60, 62, 64, 67, 69, 71, 72, 74, 76, 79 } },
    };

    std::printf ("%-26s %10s %12s\n", "patch", "us/block", "% one core");
    for (const auto& c : cases)
    {
        SynthCore s;
        s.setSampleRate ((float) sr);
        s.params = c.params;
        s.setRouting (c.routing);
        s.setModWheel (0.5f);

        const int frames = (int) (seconds * sr);
        std::vector<float> l ((size_t) frames), r ((size_t) frames);

        for (int n : c.notes)
            s.noteOn (n, 0.85f);

        // Warm up (caches, attack), then time the sustained part.
        s.render (l.data(), r.data(), sr / 2);

        const auto t0 = std::chrono::steady_clock::now();
        for (int pos = 0; pos < frames; pos += block)
            s.render (l.data() + pos, r.data() + pos, std::min (block, frames - pos));
        const auto t1 = std::chrono::steady_clock::now();

        const double secs = std::chrono::duration<double> (t1 - t0).count();
        const double usPerBlock = secs * 1.0e6 / (double) (frames / block);
        std::printf ("%-26s %10.1f %11.2f%%\n", c.name, usPerBlock, 100.0 * secs / seconds);

        if (! outDir.empty())
        {
            // Add a release tail for listening.
            for (int n : c.notes)
                s.noteOff (n);
            std::vector<float> tl ((size_t) sr * 3), tr ((size_t) sr * 3);
            s.render (tl.data(), tr.data(), sr * 3);
            l.insert (l.end(), tl.begin(), tl.end());
            r.insert (r.end(), tr.begin(), tr.end());
            writeWav (outDir + "/" + c.name + ".wav", l, r, sr);
        }
    }
    return 0;
}
