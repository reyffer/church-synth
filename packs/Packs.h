#pragma once

// Downloadable preset packs: built in code like the factory set, exported as
// .csp files by `cs_state_test --export-pack <dir>` (not compiled into the plugin).

#include "plugin/PresetManager.h"

#include <vector>

struct PresetPack
{
    const char* folder; // folder name inside Documents/Church Synth/Presets
    const std::vector<FactoryPreset>& (*presets)();
};

const std::vector<FactoryPreset>& pack1Presets();

inline const std::vector<PresetPack>& presetPacks()
{
    static const std::vector<PresetPack> packs { { "Pack 1 - Keys, Strings and Synths", &pack1Presets } };
    return packs;
}
