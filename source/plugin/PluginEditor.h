#pragma once

// Temporary editor (until the UI milestone): content pickers for user
// wavetables/noises, then a dark, grouped list of every parameter.

#include "PluginProcessor.h"

#include <juce_audio_utils/juce_audio_utils.h>

class ChurchSynthEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit ChurchSynthEditor (ChurchSynthProcessor&);
    ~ChurchSynthEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct ContentRow
    {
        juce::Label title, current;
        juce::TextButton load { "Load file..." }, factory { "Use factory" };
    };

    ChurchSynthProcessor& proc;
    juce::LookAndFeel_V4 darkLook;
    ContentRow rows[ChurchSynthProcessor::NumContentSlots];
    std::unique_ptr<juce::FileChooser> chooser;
    juce::GenericAudioProcessorEditor generic;

    void choose (int slot);
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChurchSynthEditor)
};
