#pragma once

#include "SynthPage.h"

namespace ui
{
// The colour-coded strip of 24 modulation sources (click to select, drag onto a knob).
class ModStrip final : public juce::Component, private juce::ChangeListener
{
public:
    explicit ModStrip (UiContext& ctx);
    ~ModStrip() override;
    void resized() override;
    void paint (juce::Graphics&) override;
    void tick(); // push live values (editor timer)
    std::function<void (int)> onSourceClicked; // opens the matching tab

private:
    class Tile;
    UiContext& ctx;
    std::vector<std::unique_ptr<Tile>> tiles;
    void changeListenerCallback (juce::ChangeBroadcaster*) override { repaint(); for (auto& t : tiles) ((juce::Component*) t.get())->repaint(); }
};

// Tabs under the strip (Keyboard, Envelopes, LFO, Functions, Random, Combinate) + Macros.
class ModTabs final : public juce::Component, public Refreshable
{
public:
    explicit ModTabs (UiContext& ctx);
    ~ModTabs() override;
    void resized() override;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void refreshView() override;
    void tick();
    void showTab (int t);
    void showTabForSource (int source);

private:
    UiContext& ctx;
    int current = 1;
    std::vector<std::unique_ptr<ControlPanel>> pages;
    std::unique_ptr<ControlPanel> macros;
    juce::Rectangle<int> tabRect (int t) const;
};
} // namespace ui
