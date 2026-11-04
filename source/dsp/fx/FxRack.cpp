#include "FxRack.h"

#include <algorithm>
#include <cstring>

namespace cs::fx
{
void FxRack::runBus (const FxParams& p, int bus, float* l, float* r, int n, const FxContext& ctx) noexcept
{
    if (! p.busOn[bus])
        return;
    for (int k = 0; k < kSlotsPerBus; ++k)
    {
        const int s = bus * kSlotsPerBus + k;
        const auto& sp = p.slot[(size_t) s];
        Effect* e = effects[s].load (std::memory_order_acquire);
        if (! sp.on || sp.type == FxNone || e == nullptr || e->type != sp.type)
            continue;
        e->process (l, r, n, sp.p.data(), sp.sync, ctx);
    }
}

void FxRack::process (const FxParams& p, const FxInputs& in, float* outL, float* outR, int n, const FxContext& ctx) noexcept
{
    n = std::min (n, kMaxBlock);
    const float volA = p.busVolume[0], volB = p.busVolume[1];

    // Aux send taken pre-A/B: from everything entering the FX.
    if (p.auxPre)
        for (int i = 0; i < n; ++i)
        {
            xL[i] = (in.mainL[i] + in.toAL[i] + in.toBL[i]) * p.auxSend;
            xR[i] = (in.mainR[i] + in.toAR[i] + in.toBR[i]) * p.auxSend;
        }

    switch (p.routing)
    {
        case RouteParallel:
            // The summed signal feeds both buses (half each, so dry level is kept).
            for (int i = 0; i < n; ++i)
            {
                aL[i] = 0.5f * in.mainL[i] + in.toAL[i];
                aR[i] = 0.5f * in.mainR[i] + in.toAR[i];
                bL[i] = 0.5f * in.mainL[i] + in.toBL[i];
                bR[i] = 0.5f * in.mainR[i] + in.toBR[i];
            }
            runBus (p, FxBusA, aL, aR, n, ctx);
            runBus (p, FxBusB, bL, bR, n, ctx);
            for (int i = 0; i < n; ++i)
            {
                outL[i] = aL[i] * volA + bL[i] * volB;
                outR[i] = aR[i] * volA + bR[i] * volB;
            }
            break;

        case RouteBthenA:
            for (int i = 0; i < n; ++i)
            {
                bL[i] = in.mainL[i] + in.toBL[i];
                bR[i] = in.mainR[i] + in.toBR[i];
            }
            runBus (p, FxBusB, bL, bR, n, ctx);
            for (int i = 0; i < n; ++i)
            {
                aL[i] = bL[i] * volB + in.toAL[i];
                aR[i] = bR[i] * volB + in.toAR[i];
            }
            runBus (p, FxBusA, aL, aR, n, ctx);
            for (int i = 0; i < n; ++i)
            {
                outL[i] = aL[i] * volA;
                outR[i] = aR[i] * volA;
            }
            break;

        case RouteAthenB:
        default:
            for (int i = 0; i < n; ++i)
            {
                aL[i] = in.mainL[i] + in.toAL[i];
                aR[i] = in.mainR[i] + in.toAR[i];
            }
            runBus (p, FxBusA, aL, aR, n, ctx);
            for (int i = 0; i < n; ++i)
            {
                bL[i] = aL[i] * volA + in.toBL[i];
                bR[i] = aR[i] * volA + in.toBR[i];
            }
            runBus (p, FxBusB, bL, bR, n, ctx);
            for (int i = 0; i < n; ++i)
            {
                outL[i] = bL[i] * volB;
                outR[i] = bR[i] * volB;
            }
            break;
    }

    // Aux: post-A/B send taken from the bus output; return added on top.
    if (p.busOn[FxBusAux] && p.auxSend > 0.0f)
    {
        if (! p.auxPre)
            for (int i = 0; i < n; ++i)
            {
                xL[i] = outL[i] * p.auxSend;
                xR[i] = outR[i] * p.auxSend;
            }
        runBus (p, FxBusAux, xL, xR, n, ctx);
        for (int i = 0; i < n; ++i)
        {
            outL[i] += xL[i] * p.auxReturn;
            outR[i] += xR[i] * p.auxReturn;
        }
    }

    for (int i = 0; i < n; ++i)
    {
        outL[i] += in.directL[i];
        outR[i] += in.directR[i];
    }
}
} // namespace cs::fx
