#pragma once

// Milestone 1 editor: a dark, grouped list of every parameter, good enough
// to program and test sounds. The full Pigments-style layout arrives with
// the UI milestone; the parameters underneath will not change.

#include "PluginProcessor.h"

#include <juce_audio_utils/juce_audio_utils.h>

class ChurchSynthEditor final : public juce::AudioProcessorEditor
{
public:
    explicit ChurchSynthEditor (ChurchSynthProcessor&);
    ~ChurchSynthEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::LookAndFeel_V4 darkLook;
    juce::GenericAudioProcessorEditor generic;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChurchSynthEditor)
};
