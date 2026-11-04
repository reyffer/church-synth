#pragma once

#include <cmath>

// Owns the effect objects for the 9 FX slots and hands them to the rack.
// Call sync() from a non-audio thread (it allocates). Replaced effects are kept
// until the audio thread has finished a full render after the swap ("epoch").

#include "FxRack.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace cs::fx
{
class FxHost
{
public:
    explicit FxHost (FxRack& r) : rack (r) {}

    // Creates effects for slots whose type changed. renderEpoch = the audio
    // thread's render counter now. Returns true if anything changed.
    bool sync (const int* types, float sampleRate, uint64_t renderEpoch)
    {
        collect (renderEpoch);
        bool changed = std::abs (sampleRate - rate) > 0.5f;
        if (changed)
            rate = sampleRate;
        bool any = false;
        for (int s = 0; s < kNumFxSlots; ++s)
        {
            if (! changed && current[s] != nullptr && current[s]->type == types[s])
                continue;
            if (! changed && current[s] == nullptr && types[s] == FxNone)
                continue;
            auto next = makeEffect (types[s], rate);
            rack.setEffect (s, next.get());
            if (current[s] != nullptr)
                retired.push_back ({ std::move (current[s]), renderEpoch });
            current[s] = std::move (next);
            any = true;
        }
        return any;
    }

    // Audio is stopped (prepareToPlay / tests): rebuild everything immediately.
    void rebuild (const int* types, float sampleRate)
    {
        rate = sampleRate;
        for (int s = 0; s < kNumFxSlots; ++s)
        {
            rack.setEffect (s, nullptr);
            current[s] = makeEffect (types[s], rate);
            rack.setEffect (s, current[s].get());
        }
        retired.clear();
    }

    int pendingFrees() const noexcept { return (int) retired.size(); }

private:
    struct Retired
    {
        std::unique_ptr<Effect> e;
        uint64_t epoch;
    };
    FxRack& rack;
    std::unique_ptr<Effect> current[kNumFxSlots];
    std::vector<Retired> retired;
    float rate = 0.0f;

    void collect (uint64_t renderEpoch)
    {
        // Safe once a whole render has started and finished after the swap.
        for (size_t i = 0; i < retired.size();)
            if (renderEpoch >= retired[i].epoch + 2)
                retired.erase (retired.begin() + (long) i);
            else
                ++i;
    }
};
} // namespace cs::fx
