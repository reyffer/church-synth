#pragma once

#include "Parameters.h"
#include "dsp/Content.h"
#include "dsp/ModMatrix.h"
#include "dsp/SynthCore.h"

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>

class ChurchSynthProcessor final : public juce::AudioProcessor
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

private:
    cs::SynthCore synth;
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

    class Loader;
    std::unique_ptr<Loader> loader;

    void refreshContent();
    juce::String desiredKey (int slot) const;
    void pullParameters() noexcept;
    void handleMidi (const juce::MidiMessage& m) noexcept;
    void storeUserNamesInState();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChurchSynthProcessor)
};
