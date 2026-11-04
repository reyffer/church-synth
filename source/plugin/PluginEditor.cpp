#include "PluginEditor.h"

namespace
{
constexpr int kHeader = 40;
constexpr int kRowH = 28;
const char* const kRowTitles[] = { "Engine 1 wavetable", "Engine 2 wavetable", "Noise 1 sample", "Noise 2 sample" };
} // namespace

ChurchSynthEditor::ChurchSynthEditor (ChurchSynthProcessor& p)
    : AudioProcessorEditor (p),
      proc (p),
      darkLook (juce::LookAndFeel_V4::getMidnightColourScheme()),
      generic (p)
{
    setLookAndFeel (&darkLook);

    for (int slot = 0; slot < ChurchSynthProcessor::NumContentSlots; ++slot)
    {
        auto& r = rows[slot];
        r.title.setText (kRowTitles[slot], juce::dontSendNotification);
        r.title.setColour (juce::Label::textColourId, juce::Colours::grey);
        r.current.setColour (juce::Label::textColourId, juce::Colour (0xff3fb6ff));
        r.load.onClick = [this, slot] { choose (slot); };
        r.factory.onClick = [this, slot] { proc.clearUserFile (slot); timerCallback(); };
        r.load.setTooltip ("Import your own WAV/AIFF/FLAC. It's copied to Documents/Church Synth.");
        r.factory.setTooltip ("Go back to the factory choice selected below.");
        for (auto* c : std::initializer_list<juce::Component*> { &r.title, &r.current, &r.load, &r.factory })
            addAndMakeVisible (c);
    }

    addAndMakeVisible (generic);
    setResizable (true, true);
    setResizeLimits (560, 500, 1600, 1600);
    setSize (680, 860);
    timerCallback();
    startTimerHz (4);
}

ChurchSynthEditor::~ChurchSynthEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void ChurchSynthEditor::choose (int slot)
{
    const bool wt = slot < 2;
    chooser = std::make_unique<juce::FileChooser> (wt ? "Choose a wavetable (2048 samples per frame)" : "Choose a noise / texture sample",
                                                   ChurchSynthProcessor::userFolder (wt), "*.wav;*.aif;*.aiff;*.flac");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this, slot] (const juce::FileChooser& fc) {
                              const auto file = fc.getResult();
                              if (file == juce::File())
                                  return;
                              juce::String error;
                              if (! proc.importUserFile (slot, file, error))
                                  juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Church Synth", error);
                              timerCallback();
                          });
}

void ChurchSynthEditor::timerCallback()
{
    for (int slot = 0; slot < ChurchSynthProcessor::NumContentSlots; ++slot)
        rows[slot].current.setText (proc.getSlotDisplayName (slot), juce::dontSendNotification);
}

void ChurchSynthEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1e1f24));
    g.setColour (juce::Colour (0xff3fb6ff));
    g.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    g.drawText ("CHURCH SYNTH", 16, 0, getWidth() - 32, kHeader, juce::Justification::centredLeft);
    g.setColour (juce::Colours::grey);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText ("v" JucePlugin_VersionString " - milestone 2", 16, 0, getWidth() - 32, kHeader, juce::Justification::centredRight);
}

void ChurchSynthEditor::resized()
{
    auto area = getLocalBounds().withTrimmedTop (kHeader).reduced (12, 0);
    for (auto& r : rows)
    {
        auto row = area.removeFromTop (kRowH).reduced (0, 2);
        r.title.setBounds (row.removeFromLeft (140));
        r.factory.setBounds (row.removeFromRight (100));
        row.removeFromRight (6);
        r.load.setBounds (row.removeFromRight (100));
        row.removeFromRight (6);
        r.current.setBounds (row);
    }
    area.removeFromTop (8);
    generic.setBounds (getLocalBounds().withTop (area.getY()));
}
