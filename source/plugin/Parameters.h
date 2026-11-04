#pragma once

// Table of host parameters. Each entry knows how to write its value into the
// DSP core's SynthParams, so the processor just loops over the table.
// Parameter IDs are permanent: never rename one, or saved MainStage
// concerts lose that setting. Add new parameters with a new ID.

#include "dsp/Params.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <functional>
#include <vector>

namespace cs
{
struct ParamDef
{
    enum Kind { Float, Int, Choice, Bool };

    juce::String id, name, group, unit;
    Kind kind = Float;
    float min = 0, max = 1, def = 0, centre = -1; // centre >= 0 sets a skew so that value sits mid-knob
    juce::StringArray choices;
    std::function<void (SynthParams&, float)> apply;
};

const std::vector<ParamDef>& parameterDefs();
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
} // namespace cs
