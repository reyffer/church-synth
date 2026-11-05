#pragma once

// Downloadable preset packs: built in code like the factory set, exported as
// .csp files by `cs_state_test --export-pack <dir>` (not compiled into the plugin).

#include "plugin/PresetManager.h"

#include <vector>

struct PresetPack
{
    const char* folder; // folder name inside Documents/Church Synth/Presets
    const std::vector<FactoryPreset>& (*presets)();
    juce::StringArray (*tags) (const juce::String& presetName);
};

const std::vector<FactoryPreset>& pack1Presets();
juce::StringArray pack1Tags (const juce::String& presetName);

inline const std::vector<PresetPack>& presetPacks()
{
    static const std::vector<PresetPack> packs { { "Pack 1 - Keys, Strings and Synths", &pack1Presets, &pack1Tags } };
    return packs;
}
