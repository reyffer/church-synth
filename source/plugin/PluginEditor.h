#pragma once

// Temporary editor (until the UI milestone):
//   Sound      - every parameter, grouped
//   Modulation - the routing matrix (source -> destination, amount, sidechain)
//   Files      - user wavetables / noise samples

#include "PluginProcessor.h"

#include <juce_audio_utils/juce_audio_utils.h>

class ChurchSynthEditor;

// One row of the routing list.
class RouteRow final : public juce::Component
{
public:
    explicit RouteRow (std::function<void()> onChange, std::function<void()> onRemove);
    void set (const cs::ModRoute& r);
    cs::ModRoute get() const;
    void resized() override;

private:
    juce::ComboBox source, dest, sidechain;
    juce::Slider amount, scAmount;
    juce::TextButton remove { "X" };
};

class RoutePanel final : public juce::Component
{
public:
    explicit RoutePanel (ChurchSynthProcessor& p);
    void resized() override;
    void paint (juce::Graphics&) override;
    void reload(); // rebuild rows from the processor

private:
    ChurchSynthProcessor& proc;
    juce::TextButton add { "+ Add route" };
    juce::Label help;
    juce::Viewport viewport;
    juce::Component list;
    std::vector<std::unique_ptr<RouteRow>> rows;

    void push(); // rows -> processor
    void layoutRows();
};

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
    juce::TabbedComponent tabs { juce::TabbedButtonBar::TabsAtTop };
    juce::GenericAudioProcessorEditor generic;
    RoutePanel routes;
    juce::Component files;
    ContentRow rows[ChurchSynthProcessor::NumContentSlots];
    std::unique_ptr<juce::FileChooser> chooser;

    void choose (int slot);
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChurchSynthEditor)
};
