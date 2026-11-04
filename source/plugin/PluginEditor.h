#pragma once

// The editor: Pigments-style layout (top bar, Synth / FX / Seq pages, mod
// strip, mod tabs + macros, footer, preset browser), drawn at 1280 x 800 and
// scaled to the window. RouteRow / RoutePanel: the "Mod routes" list.

#include "PluginProcessor.h"
#include "ui/FxPage.h"
#include "ui/Header.h"
#include "ui/ModSection.h"
#include "ui/PresetBrowser.h"
#include "ui/SeqPage.h"
#include "ui/SynthPage.h"

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

class ChurchSynthEditor final : public juce::AudioProcessorEditor, public juce::DragAndDropContainer, private juce::Timer
{
public:
    explicit ChurchSynthEditor (ChurchSynthProcessor&);
    ~ChurchSynthEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;

    // For screenshots / tests: 0 Synth, 1 FX, 2 Seq, 3 preset browser; modTab 0..5.
    void showView (int view, int modTab = -1, int engineTab = -1);

private:
    class Root;
    ChurchSynthProcessor& proc;
    juce::SharedResourcePointer<ui::LookAndFeel> lnf; // shared by all open editors
    ui::UiContext ctx;
    std::unique_ptr<Root> root;
    std::unique_ptr<juce::TooltipWindow> tooltips;
    uint32_t lastEdit = 0;
    int ticks = 0, saveTimer = 0;

    void timerCallback() override;
    void showMenu();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChurchSynthEditor)
};
