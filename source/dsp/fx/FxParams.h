#pragma once

// FX section parameters (plain data, part of SynthParams).

#include <array>

namespace cs::fx
{
constexpr int kFxParams = 12;
constexpr int kSlotsPerBus = 3;
enum FxBusId : int { FxBusA = 0, FxBusB, FxBusAux, NumFxBuses };
constexpr int kNumFxSlots = kSlotsPerBus * NumFxBuses;
enum FxRouting : int { RouteAthenB = 0, RouteBthenA, RouteParallel, FxRoutingCount };

struct FxSlotParams
{
    bool on = true;
    int type = 0; // FxType
    bool sync = false;
    std::array<float, kFxParams> p {};
};

struct FxParams
{
    std::array<FxSlotParams, kNumFxSlots> slot {}; // A1 A2 A3 B1 B2 B3 Aux1 Aux2 Aux3
    int routing = RouteAthenB;
    bool busOn[NumFxBuses] { true, true, true };
    float busVolume[2] { 1.0f, 1.0f }; // A, B (linear, 0..1.5)
    float auxSend = 0.0f, auxReturn = 1.0f;
    bool auxPre = false; // send taken before (true) or after (false) the A/B buses
    bool on = true;      // whole FX section (off = dry, effects not processed)
};
} // namespace cs::fx
