#pragma once

// Effect interface, the effect list and each effect's controls.
//
// Every slot has 12 generic 0..1 controls (P1..P12) plus a Sync switch; each
// effect type gives them meaning (see kFxInfo in Effects.cpp and SPEC §6.3).
// TYPE ORDER IS PERMANENT (stored in presets): append only.

#include "FxParams.h"

#include <array>
#include <memory>

namespace cs::fx
{
enum FxType : int
{
    FxNone = 0,
    FxDelay, FxTapeEcho, FxPitchDelay, FxReverb, FxShimmer,
    FxCompressor, FxMultiband,
    FxMultiFilter, FxParamEq,
    FxDistortion, FxBitcrusher, FxErode,
    FxSuperUnison, FxChorus, FxVintageChorus, FxFlanger, FxVintageFlanger, FxPhaser, FxPanner,
    FxRotary, // appended in feedback round 2
    FxTypeCount
};

struct FxInfo
{
    const char* name;
    const char* group;
    std::array<const char*, kFxParams> params; // nullptr = unused
    std::array<float, kFxParams> defaults;
    bool usesSync;
};

const FxInfo& fxInfo (int type) noexcept;
// Which control becomes a note division when Sync is on (-1 = none).
int fxSyncParam (int type) noexcept;

struct FxContext
{
    double bpm = 120.0;
};

class Effect
{
public:
    virtual ~Effect() = default;
    int type = 0; // FxType, set by makeEffect
    virtual void prepare (float sampleRate) = 0; // allocates: call off the audio thread
    virtual void reset() = 0;
    // In-place stereo processing. p = the slot's 12 controls (0..1).
    virtual void process (float* l, float* r, int n, const float* p, bool sync, const FxContext& ctx) noexcept = 0;
};

// Creates and prepares an effect (nullptr for FxNone). Call off the audio thread.
std::unique_ptr<Effect> makeEffect (int type, float sampleRate);
} // namespace cs::fx
