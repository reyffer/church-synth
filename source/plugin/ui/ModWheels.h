#pragma once

// Hover wheels: rest the mouse ~0.5 s on a knob (or FX menu) that is modulated and a
// small panel opens beside it with one wheel per incoming connection, in the source's
// colour (outlined in the first one's colour), plus a line from each source tile to the control itself. Hover or click a wheel: the
// lines go and the panel shows the source, the amount and the range it sweeps in real
// units ("Env 2  +30 %  /  629 Hz -> 5.27 kHz"). Drag a wheel up/down to change the
// amount (Shift = fine), mouse wheel to nudge, double-click or right-click to remove.
// Dropping a source on a knob opens it straight away on the new (0 %) connection.

#include "Widgets.h"

namespace ui
{

class ModWheels final : public juce::Component
{
public:
    explicit ModWheels (UiContext& c);

    // Where a source tile is, in this component's coordinates (set by the editor).
    std::function<juce::Rectangle<float> (int src)> tileBounds;
    // Mouse position in this component's coordinates (the editor scales us).
    std::function<juce::Component*()> componentUnderMouse;
    std::function<bool()> blocked; // browser open, a drag going on...

    void tick();                                        // ~30 Hz from the editor
    void open (juce::Component& target, int activeSource); // now, without the hover delay
    void close();
    bool isOpen() const noexcept { return target != nullptr; }
    juce::Component* getTarget() const noexcept { return target.getComponent(); }
    bool panelContains (juce::Point<int> p) const { return isOpen() && panel.contains (p); }

    // Esc / click elsewhere: close, and don't re-open on this target until the mouse leaves it.
    void dismiss();

    bool hitTest (int x, int y) override { return panelContains ({ x, y }); }
    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    // For tests / snapshots.
    int activeSource() const noexcept;
    juce::String rangeTextForTest() const;

private:
    struct Wheel
    {
        int src = -1;
        float amount = 0.0f;
        juce::Rectangle<float> r;
    };

    UiContext& ctx;
    juce::Component::SafePointer<juce::Component> target, candidate, dismissedOn;
    uint32_t candidateSince = 0, leftAt = 0;
    std::vector<Wheel> wheels;
    juce::Rectangle<int> panel;
    int hoverSrc = -1, clickedSrc = -1;
    int dragSrc = -1;
    float dragStart = 0.0f;
    bool dragMoved = false;
    uint32_t wheelEditAt = 0;
    int wheelEditSrc = -1;

    ModTarget* modTarget() const;
    void rebuild();      // routes + layout
    void layout();
    int wheelAt (juce::Point<float> p) const;
    const Wheel* findWheel (int src) const;
    void setAmount (int src, float amount, bool record);
    void showMenu (int src);
    void drawRange (juce::Graphics&, juce::Rectangle<float> area, const Wheel&) const;
    void rangeTexts (const Wheel&, juce::String& a, juce::String& b, bool& bipolar) const;
};

} // namespace ui
