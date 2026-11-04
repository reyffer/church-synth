#include "PluginEditor.h"

ChurchSynthEditor::ChurchSynthEditor (ChurchSynthProcessor& p)
    : AudioProcessorEditor (p),
      darkLook (juce::LookAndFeel_V4::getMidnightColourScheme()),
      generic (p)
{
    setLookAndFeel (&darkLook);
    addAndMakeVisible (generic);
    setResizable (true, true);
    setResizeLimits (480, 400, 1600, 1400);
    setSize (640, 760);
}

ChurchSynthEditor::~ChurchSynthEditor()
{
    setLookAndFeel (nullptr);
}

void ChurchSynthEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1e1f24));
    g.setColour (juce::Colour (0xff3fb6ff));
    g.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    g.drawText ("CHURCH SYNTH", 16, 0, getWidth() - 32, 40, juce::Justification::centredLeft);
    g.setColour (juce::Colours::grey);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText ("v" JucePlugin_VersionString " - milestone 1", 16, 0, getWidth() - 32, 40, juce::Justification::centredRight);
}

void ChurchSynthEditor::resized()
{
    generic.setBounds (getLocalBounds().withTrimmedTop (40));
}
