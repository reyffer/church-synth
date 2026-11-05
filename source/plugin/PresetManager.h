#pragma once

// Presets in our own file format (.csp, XML), so a preset saved on Windows
// loads identically in MainStage. Factory presets are built in code (no files
// to install); user presets live in Documents/Church Synth/Presets.
//
// Also: sequencer pattern presets (Factory / From preset / User) and
// per-effect FX presets (Factory / User).

#include "PluginProcessor.h"
#include <array>

#include <juce_audio_processors/juce_audio_processors.h>

#include <functional>
#include <map>
#include <set>
#include <vector>

struct PresetInfo
{
    juce::String name, category, author, description;
    bool factory = true;
    int factoryIndex = -1; // factory presets
    juce::File file;       // user presets
    juce::StringArray tags; // effective tags (the preset's own, or the user's edits)
    juce::String key() const { return (factory ? "F/" : "U/") + category + "/" + name; }
};

class PresetManager
{
public:
    explicit PresetManager (ChurchSynthProcessor& p);

    static juce::File rootFolder();
    static juce::File userPresetFolder() { return rootFolder().getChildFile ("Presets"); }
    static juce::File patternFolder() { return rootFolder().getChildFile ("Patterns"); }
    static juce::File fxPresetFolder() { return rootFolder().getChildFile ("FX Presets"); }
    static const juce::StringArray& categories();
    static constexpr const char* kExtension = ".csp";

    // --- Synth presets ---------------------------------------------------------
    const std::vector<PresetInfo>& all();   // factory first, then user (scanned when needed)
    void rescan();
    bool load (const PresetInfo& p);
    void loadInit();
    bool save (const juce::String& name, const juce::String& category, const juce::String& author,
               const juce::String& description, juce::String& error);
    bool remove (const PresetInfo& p);
    // Writes the current sound to a .csp file (used by save and by the preset-pack exporter).
    static juce::String safeFileName (const juce::String& name);
    bool writePresetFile (const juce::File& file, const juce::String& name, const juce::String& category,
                          const juce::String& author, const juce::String& description,
                          const juce::StringArray& tags = {});
    juce::ValueTree treeFor (const PresetInfo& p); // the preset's state tree (empty if unreadable)
    int indexOfCurrent();                          // in all(), or -1
    PresetInfo currentInfo();

    // --- Tags ------------------------------------------------------------------------------
    // Every preset has default tags (factory: built in; user files: a "tags" attribute).
    // The user's edits are kept in "Preset Tags.xml" next to the Presets folder, so tags
    // work the same on factory and user presets and survive re-copying preset files.
    juce::StringArray allTags();                                    // sorted, no duplicates
    void setTags (const PresetInfo& p, const juce::StringArray& tags);
    void renameTag (const juce::String& from, const juce::String& to);
    void deleteTag (const juce::String& tag);
    static juce::StringArray parseTags (const juce::String& commaList);

    bool isFavourite (const PresetInfo& p) const { return favourites.count (p.key()) > 0; }
    void setFavourite (const PresetInfo& p, bool fav);

    // Writes the current sound as a preset tree (no MIDI mappings).
    juce::ValueTree currentAsPresetTree (const juce::String& name, const juce::String& category,
                                         const juce::String& author, const juce::String& description);

    // --- Sequencer patterns --------------------------------------------------------
    struct PatternInfo
    {
        juce::String name;
        int source = 0; // 0 factory, 1 from preset, 2 user
        int index = -1; // factory pattern / preset index
        juce::File file;
    };
    std::vector<PatternInfo> patterns (int source);
    bool loadPattern (const PatternInfo& p);
    bool savePattern (const juce::String& name, juce::String& error);
    bool deletePattern (const PatternInfo& p);

    // --- FX presets (per effect type) ---------------------------------------------------
    struct FxPresetInfo
    {
        juce::String name;
        bool factory = true;
        std::array<float, 12> values {};
        juce::File file;
    };
    std::vector<FxPresetInfo> fxPresets (int type);
    void applyFxPreset (int slot, const FxPresetInfo& p);
    bool saveFxPreset (int slot, const juce::String& name, juce::String& error);
    bool deleteFxPreset (const FxPresetInfo& p);

private:
    ChurchSynthProcessor& proc;
    std::vector<PresetInfo> list;
    bool scanned = false;
    std::set<juce::String> favourites;
    std::map<juce::String, juce::StringArray> tagEdits; // preset key -> tags chosen by the user

    void loadFavourites();
    void loadTagEdits();
    void saveTagEdits() const;
    void saveFavourites() const;
};

// --- Factory content built in code -------------------------------------------------
// Builds a full state tree from parameter defaults plus overrides (values in
// real units: Hz, seconds, choice index...).
class PresetBuilder
{
public:
    PresetBuilder();
    PresetBuilder& set (const juce::String& id, float value);
    PresetBuilder& choice (const juce::String& id, const juce::String& itemName); // by menu text
    PresetBuilder& route (const juce::String& source, const juce::String& destId, float amount,
                          const juce::String& sidechain = {}, float scAmount = 1.0f);
    // FX slot: 0..2 = A1..A3, 3..5 = B1..B3, 6..8 = Aux 1..3. Starts from the effect's defaults.
    PresetBuilder& fx (int slot, int type, std::initializer_list<std::pair<int, float>> knobs = {});
    PresetBuilder& pattern (const cs::SeqPattern& p);
    PresetBuilder& macroName (int m, const juce::String& name);
    juce::ValueTree build (const juce::String& name, const juce::String& category) const;
    juce::StringArray errors; // unknown IDs, menu items or routes (checked by the state test)

private:
    juce::ValueTree params; // PARAM children by id
    juce::ValueTree routes { "ROUTES" };
    juce::ValueTree seq;
    juce::String macroNames[4];
    juce::ValueTree param (const juce::String& id);
};

struct FactoryPreset
{
    const char* name;
    const char* category;
    const char* description;
    std::function<void (PresetBuilder&)> build;
};
const std::vector<FactoryPreset>& factoryPresets();
juce::StringArray factoryPresetTags (const juce::String& presetName); // default tags of a factory preset

struct FactoryPattern
{
    const char* name;
    std::function<void (cs::SeqPattern&, std::vector<std::pair<juce::String, float>>& params)> build;
};
const std::vector<FactoryPattern>& factoryPatterns();

struct FactoryFxPreset
{
    int type;
    const char* name;
    std::vector<std::pair<int, float>> knobs; // knob index -> 0..1 (others: the effect's defaults)
};
const std::vector<FactoryFxPreset>& factoryFxPresets();
