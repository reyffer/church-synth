#pragma once

#include "Parameters.h"
#include "dsp/Content.h"
#include "dsp/ModMatrix.h"
#include "dsp/SynthCore.h"
#include "dsp/fx/FxHost.h"

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>

class PresetManager;

class ChurchSynthProcessor final : public juce::AudioProcessor,
                                   private juce::Timer,
                                   private juce::AudioProcessorParameter::Listener,
                                   private juce::AsyncUpdater
{
public:
    ChurchSynthProcessor();
    ~ChurchSynthProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // --- Wavetable / noise content -------------------------------------------
    // Slots 0/1 = engine 1/2 wavetable; slots 2/3 = utility noise 1/2.
    enum ContentSlot { Wt1 = 0, Wt2, Noise1, Noise2, NumContentSlots };

    static juce::File userFolder (bool wavetables);

    // Copies the file into the user folder and selects it for the slot (message thread).
    bool importUserFile (int slot, const juce::File& file, juce::String& error);
    void clearUserFile (int slot); // back to the factory selection
    juce::String getSlotDisplayName (int slot) const;

    // --- Modulation routes (message thread) -----------------------------------
    cs::ModRouting getRoutes() const;
    void setRoutes (const cs::ModRouting& r);

    // --- Seq/Arp pattern (message thread) -------------------------------------
    cs::SeqPattern getPattern() const;
    void setPattern (const cs::SeqPattern& p);
    const cs::SeqArp& getSeq() const noexcept { return synth.getSeq(); }
    cs::SeqParams currentSeqParams() const; // host parameter values (for the editor's random preview)
    uint32_t patternVersion() const noexcept { return patternSeq.load (std::memory_order_acquire); }

    // FX: create effects for the current slot types (message thread). Exposed for tests.
    void syncEffects();
    // Copy / swap two FX slots (type, on, sync and all controls).
    void copyFxSlot (int from, int to);
    void swapFxSlots (int a, int b);

    // --- Whole state as a tree (plugin state, presets and undo use it) --------
    enum class LoadKind { HostState, Preset, Undo };
    juce::ValueTree createStateTree();
    void restoreStateTree (const juce::ValueTree& tree, LoadKind kind);
    static juce::ValueTree patternTreeOf (const cs::SeqPattern& p) { return patternToTree (p); }
    static cs::SeqPattern patternOf (const juce::ValueTree& t) { return patternFromTree (t); }

    // --- Function shapes drawn in the editor (shape menu entry "Custom") -------
    cs::FunctionShape getCustomShape (int f) const;
    void setCustomShape (int f, const cs::FunctionShape& shape);

    // --- Macro names --------------------------------------------------------------
    juce::String getMacroName (int m) const;
    void setMacroName (int m, const juce::String& name);

    // --- MIDI learn (mappings are saved with the host state, not in presets) ----
    void startMidiLearn (const juce::String& paramId);
    void cancelMidiLearn() { learnIndex.store (-1); }
    juce::String learningParam() const;
    int ccForParam (const juce::String& paramId) const; // -1 = none
    void clearMidiMapping (const juce::String& paramId);

    // --- Telemetry for the editor ---------------------------------------------------
    float getCpuLoad() const noexcept { return cpuLoad.load (std::memory_order_relaxed); }
    float takeOutputPeak() noexcept { return outPeak.exchange (0.0f, std::memory_order_relaxed); } // since the last call
    const cs::SynthCore& getSynth() const noexcept { return synth; }
    void setEditorOpen (bool open) noexcept { synth.uiActive.store (open); }
    const cs::Wavetable* currentWavetable (int engine) const noexcept { return wtPtr[juce::jlimit (0, 1, engine)].load(); }
    const cs::NoiseSample* currentNoise (int slot) const noexcept { return noisePtr[juce::jlimit (0, 1, slot)].load(); }

    // --- Undo / redo / history (message thread) -----------------------------------
    void checkpoint (const juce::String& label); // call after an edit that isn't a parameter gesture
    bool canUndo() const noexcept { return historyIndex > 0; }
    bool canRedo() const noexcept { return historyIndex + 1 < (int) history.size(); }
    void undo() { jumpToHistory (historyIndex - 1); }
    void redo() { jumpToHistory (historyIndex + 1); }
    void jumpToHistory (int index);
    juce::StringArray historyLabels() const;
    int getHistoryIndex() const noexcept { return historyIndex; }
    uint32_t getEditCounter() const noexcept { return editCounter; } // changes whenever the sound changes (UI refresh)

    // --- Presets --------------------------------------------------------------------
    PresetManager& presets() noexcept { return *presetManager; }
    juce::String presetName() const { return currentPreset; }
    juce::String presetCategory() const { return currentCategory; }
    bool presetModified() const noexcept { return modified; }
    void setPresetInfo (const juce::String& name, const juce::String& category, bool isModified);

private:
    std::unique_ptr<cs::SynthCore> synthPtr = std::make_unique<cs::SynthCore>(); // ~1 MB: heap
    cs::SynthCore& synth = *synthPtr;
    cs::fx::FxHost fxHost { synth.getFxRack() };
    juce::CriticalSection fxLock; // FxHost is touched from the timer, prepareToPlay and state loads (never the audio thread)
    int lastFxType[cs::fx::kNumFxSlots] {};
    double currentRate = 48000.0;
    void timerCallback() override;
    void readFxTypes (int* types) const;
    void applyFxDefaults (int slot, int type);
    std::vector<std::atomic<float>*> rawParams;
    juce::AudioFormatManager formats;

    std::atomic<const cs::Wavetable*> wtPtr[2] {};
    std::atomic<const cs::NoiseSample*> noisePtr[2] {};

    mutable juce::CriticalSection contentLock; // guards the fields below
    juce::String userFile[NumContentSlots];    // file name in the user folder, or empty
    juce::String loadedKey[NumContentSlots];   // what is currently resolved
    bool missing[NumContentSlots] {};

    // Routes: edited on the message thread, handed to the audio thread through a seqlock.
    mutable juce::CriticalSection routeLock;
    cs::ModRouting routesEdit;           // message-thread copy (guarded by routeLock)
    cs::ModRouting routesShared;         // published copy
    std::atomic<uint32_t> routeSeq { 0 };
    uint32_t routeSeqSeen = ~0u;         // audio thread only
    cs::ModRouting routesAudio;          // audio-thread scratch
    void publishRoutes (const cs::ModRouting& r);
    void pullRoutes() noexcept;
    static juce::ValueTree routesToTree (const cs::ModRouting& r);
    static cs::ModRouting routesFromTree (const juce::ValueTree& t);

    // Seq pattern: same seqlock hand-off as the routes.
    mutable juce::CriticalSection patternLock;
    cs::SeqPattern patternEdit, patternShared, patternAudio;
    std::atomic<uint32_t> patternSeq { 0 };
    uint32_t patternSeqSeen = ~0u;
    void pullPattern() noexcept;
    static juce::ValueTree patternToTree (const cs::SeqPattern& p);
    static cs::SeqPattern patternFromTree (const juce::ValueTree& t);

    class Loader;
    std::unique_ptr<Loader> loader;

    // Custom function shapes: seqlock hand-off.
    mutable juce::CriticalSection funcLock;
    cs::FunctionShape funcEdit[cs::kNumFuncs], funcShared[cs::kNumFuncs], funcAudio[cs::kNumFuncs];
    std::atomic<uint32_t> funcSeq { 0 };
    uint32_t funcSeqSeen = ~0u;
    int funcShapeParam[cs::kNumFuncs] {}; // index in rawParams
    void pullFuncs() noexcept;

    juce::String macroNames[4] { "Macro 1", "Macro 2", "Macro 3", "Macro 4" };

    // MIDI learn
    std::vector<juce::RangedAudioParameter*> paramPtrs;
    std::atomic<int> ccMap[128];
    std::atomic<int> learnIndex { -1 };
    int paramIndex (const juce::String& id) const;

    // CPU
    std::atomic<float> cpuLoad { 0.0f }, outPeak { 0.0f };

    // History
    struct HistoryEntry
    {
        juce::String label;
        juce::ValueTree state;
        juce::int64 time;
    };
    std::vector<HistoryEntry> history;
    int historyIndex = -1;
    bool restoring = false;
    uint32_t editCounter = 0;
    juce::String pendingGestureLabel;
    void resetHistory (const juce::String& label);
    void parameterValueChanged (int, float) override {}
    void parameterGestureChanged (int index, bool starting) override;
    void handleAsyncUpdate() override;

    // Presets
    std::unique_ptr<PresetManager> presetManager;
    juce::String currentPreset { "Init" }, currentCategory;
    bool modified = false;

    void refreshContent();
    juce::String desiredKey (int slot) const;
    void pullParameters() noexcept;
    void handleMidi (const juce::MidiMessage& m) noexcept;
    void storeUserNamesInState();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChurchSynthProcessor)
};
