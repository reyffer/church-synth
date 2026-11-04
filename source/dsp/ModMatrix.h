#pragma once

// Modulation routing: which source moves which parameter, by how much,
// optionally scaled by a second "sidechain" source.
//
// Destinations use the same IDs as the host parameters, and routes are saved
// by ID / source name, so adding destinations later never breaks presets.

#include "Params.h"

#include <cmath>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace cs
{
struct ModDest
{
    const char* id;   // == host parameter ID
    std::string name; // display name
    float min, max;
    bool logScale; // modulate in log space (frequencies, times)
    std::function<float& (SynthParams&)> ref;

    float toNorm (float v) const noexcept;
    float fromNorm (float n) const noexcept;
};

const std::vector<ModDest>& modDestinations();
int findModDest (const std::string& id) noexcept;

const char* modSourceName (int source) noexcept;
int findModSource (const std::string& name) noexcept;
bool isGlobalSource (int source) noexcept; // same value for every voice

// Mod Quantize: snap a (modulated) semitone offset to the intervals in a 12-bit mask
// (bit 0 = root, always allowed). Full mask = plain rounding to semitones.
inline float quantizeSemitones (float semis, int mask) noexcept
{
    mask |= 1;
    const float r = std::round (semis);
    if ((mask & 0xFFF) == 0xFFF)
        return r;
    for (int dist = 0; dist <= 6; ++dist)
        for (int sgn : { -1, 1 })
        {
            const int cand = (int) r + sgn * dist;
            const int pc = ((cand % 12) + 12) % 12;
            if (mask & (1 << pc))
                return (float) cand;
        }
    return r;
}

struct ModRoute
{
    int source = -1;
    int dest = -1;
    float amount = 0.0f;  // -1..1 of the destination's full range
    int sidechain = -1;   // optional ModSource that scales this route
    float scAmount = 1.0f; // 0..1
};

constexpr int kMaxRoutes = 64;

struct ModRouting
{
    ModRoute routes[kMaxRoutes];
    int count = 0;

    // Derived: unique destinations touched, and each route's slot in that list.
    int touched[kMaxRoutes] {};
    int numTouched = 0;
    int slotOf[kMaxRoutes] {};

    void prepare() noexcept;
};
} // namespace cs
