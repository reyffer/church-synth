// cs_fxcli <type> [p1,p2,...] : runs one effect offline (48 kHz).
// Reads raw float32 stereo interleaved from stdin, writes the same to stdout.
// Controls are 0..1 knob values in order; "-" (or a missing entry) keeps the
// effect's default. Used by the FX analysis scripts (see SPEC §6.3 notes).
#include "fx/Effects.h"

#include <cstdio>
#include <sstream>
#include <string>
#include <vector>

using namespace cs::fx;

int main (int argc, char** argv)
{
    if (argc < 2)
    {
        std::fprintf (stderr, "usage: cs_fxcli <type> [p1,p2,...]\n");
        return 1;
    }
    const int type = std::stoi (argv[1]);
    float p[kFxParams];
    const auto& info = fxInfo (type);
    for (int k = 0; k < kFxParams; ++k)
        p[k] = info.defaults[(size_t) k];
    if (argc > 2)
    {
        std::stringstream ss (argv[2]);
        std::string t;
        int k = 0;
        while (std::getline (ss, t, ',') && k < kFxParams)
        {
            if (t != "-" && ! t.empty())
                p[k] = std::stof (t);
            ++k;
        }
    }
    std::vector<float> in;
    float buf[4096];
    size_t n;
    while ((n = std::fread (buf, 4, 4096, stdin)) > 0)
        in.insert (in.end(), buf, buf + n);
    const size_t frames = in.size() / 2;
    auto fx = makeEffect (type, 48000.0f);
    if (fx == nullptr)
        return 1;
    FxContext ctx;
    std::vector<float> L (256), R (256);
    for (size_t i = 0; i < frames; i += 256)
    {
        const int m = (int) std::min<size_t> (256, frames - i);
        for (int j = 0; j < m; ++j) { L[(size_t) j] = in[2 * (i + (size_t) j)]; R[(size_t) j] = in[2 * (i + (size_t) j) + 1]; }
        fx->process (L.data(), R.data(), m, p, false, ctx);
        for (int j = 0; j < m; ++j) { in[2 * (i + (size_t) j)] = L[(size_t) j]; in[2 * (i + (size_t) j) + 1] = R[(size_t) j]; }
    }
    std::fwrite (in.data(), 4, in.size(), stdout);
    return 0;
}
