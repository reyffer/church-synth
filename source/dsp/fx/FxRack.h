#pragma once

// FX section: bus A and bus B (3 insert slots each) and an Aux send/return bus
// (3 slots). Effect objects are created off the audio thread and handed over
// as raw pointers (see FxSlotHandle); the rack never allocates.

#include "Effects.h"
#include "FxParams.h"

#include <atomic>

namespace cs::fx
{
// Inputs coming from the voices.
struct FxInputs
{
    const float* mainL; const float* mainR;  // Sum routing
    const float* toAL;  const float* toAR;   // Split / utility -> FX A
    const float* toBL;  const float* toBR;   // Split / utility -> FX B
    const float* directL; const float* directR; // bypasses the FX
};

class FxRack
{
public:
    // Effect pointer per slot (owned by FxHost). nullptr = empty slot. The rack only
    // runs an effect whose type matches the slot's current type parameter.
    void setEffect (int slot, Effect* e) noexcept { effects[slot].store (e, std::memory_order_release); }

    // Renders n <= kMaxBlock samples into outL/outR (overwrites).
    void process (const FxParams& p, const FxInputs& in, float* outL, float* outR, int n, const FxContext& ctx) noexcept;

    static constexpr int kMaxBlock = 2048;

private:
    std::atomic<Effect*> effects[kNumFxSlots] {};
    float aL[kMaxBlock] {}, aR[kMaxBlock] {}, bL[kMaxBlock] {}, bR[kMaxBlock] {};
    float xL[kMaxBlock] {}, xR[kMaxBlock] {};

    void runBus (const FxParams& p, int bus, float* l, float* r, int n, const FxContext& ctx) noexcept;
};
} // namespace cs::fx
