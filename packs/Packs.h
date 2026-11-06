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
    juce::String (*notes) (const juce::String& presetName);   // "How it's built"
};

const std::vector<FactoryPreset>& pack1Presets();
juce::StringArray pack1Tags (const juce::String& presetName);
juce::String pack1Notes (const juce::String& presetName);
const std::vector<FactoryPreset>& pack2Presets();
juce::StringArray pack2Tags (const juce::String& presetName);
juce::String pack2Notes (const juce::String& presetName);

inline const std::vector<PresetPack>& presetPacks()
{
    static const std::vector<PresetPack> packs {
        { "Pack 1 - Keys, Strings and Synths", &pack1Presets, &pack1Tags, &pack1Notes },
        { "Pack 2 - Worship Elements", &pack2Presets, &pack2Tags, &pack2Notes },
    };
    return packs;
}
