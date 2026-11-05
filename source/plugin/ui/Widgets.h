#pragma once

// Parameter-bound controls shared by every page: knobs (with modulation rings,
// drag-and-drop routing and MIDI learn), menu pills, toggles and steppers.

#include "../PluginProcessor.h"
#include "Theme.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace ui
{
// Shared editor state: the selected modulation source and route helpers.
class UiContext final : public juce::ChangeBroadcaster
{
public:
    explicit UiContext (ChurchSynthProcessor& p);
    ChurchSynthProcessor& proc;

    int selectedSource = -1; // >= 0: knobs edit this source's route amounts
    void selectSource (int s); // toggles
    void clearSource() { if (selectedSource >= 0) { selectedSource = -1; sendChangeMessage(); } }

    void refreshRoutes(); // re-read from the processor (call when it may have changed)
    const cs::ModRouting& routes() const noexcept { return cached; }
    float routeAmount (int src, int dest) const noexcept;
    bool hasRoute (int src, int dest) const noexcept;
    void setRouteAmount (int src, int dest, float amount, bool record = true);
    void removeRoute (int src, int dest);
    std::vector<std::pair<int, float>> routesTo (int dest) const;
    bool sourceUsed (int src) const noexcept;

    const cs::ParamDef* def (const juce::String& id) const;
    juce::RangedAudioParameter* param (const juce::String& id) const { return proc.apvts.getParameter (id); }
    float value (const juce::String& id) const;  // real units
    int intValue (const juce::String& id) const { return (int) std::lround (value (id)); }
    bool boolValue (const juce::String& id) const { return value (id) >= 0.5f; }
    void setValue (const juce::String& id, float realValue); // as a complete gesture

    uint32_t lastEdit = 0;

private:
    cs::ModRouting cached;
};

juce::String formatValue (const cs::ParamDef& d, float v);

// ---------------------------------------------------------------------------
class Knob : public juce::Component,
             public juce::DragAndDropTarget,
             public juce::SettableTooltipClient,
             private juce::ChangeListener
{
public:
    Knob (UiContext& ctx, const juce::String& paramId, const juce::String& label = {});
    ~Knob() override;

    void setLabel (const juce::String& l) { label = l; repaint(); }
    void setValueColour (juce::Colour c) { slider.setColour (juce::Slider::rotarySliderFillColourId, c); }
    void setFormatter (std::function<juce::String (float)> f) { formatter = std::move (f); }
    const juce::String& getParamId() const noexcept { return paramId; }
    juce::Slider& getSlider() noexcept { return slider; }

    void resized() override;
    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }

    bool isInterestedInDragSource (const SourceDetails&) override;
    void itemDragEnter (const SourceDetails&) override { dropHover = true; repaint(); }
    void itemDragExit (const SourceDetails&) override { dropHover = false; repaint(); }
    void itemDropped (const SourceDetails&) override;

    static void showContextMenu (UiContext& ctx, const juce::String& paramId, juce::Component* target);

private:
    class ModOverlay;

    UiContext& ctx;
    juce::String paramId, label;
    const cs::ParamDef* pdef = nullptr;
    int dest = -1;
    juce::Slider slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    std::unique_ptr<ModOverlay> overlay;
    std::function<juce::String (float)> formatter;
    bool hover = false, dropHover = false, editingMod = false;
    float modDragAmount = 0.0f;

    juce::Rectangle<float> dialBounds() const;
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    juce::String valueText() const;
};

// Menu pill for a choice parameter (blue text on a dark pill, click for the menu).
class Pill : public juce::Component, public juce::SettableTooltipClient
{
public:
    Pill (UiContext& ctx, const juce::String& paramId);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    std::function<juce::String (int)> displayName; // optional shorter names
    std::function<juce::String (int)> menuName;    // optional longer names in the menu
    std::function<void (int)> onUserChange;        // after the user picks a new value
    std::function<void (juce::PopupMenu&)> extraMenu; // appended (ids >= 10000 go to onExtra)
    std::function<void (int)> onExtra;
    juce::Colour textColour = col::accent;
    bool arrows = false; // draw < > to step through the list
    int getIndex() const noexcept { return index; }

private:
    UiContext& ctx;
    juce::String paramId;
    const cs::ParamDef* pdef = nullptr;
    std::unique_ptr<juce::ParameterAttachment> att;
    int index = 0;
    void step (int delta);
};

// Bool parameter as a power icon, a lit text pill or a check box.
class Toggle : public juce::Component, public juce::SettableTooltipClient
{
public:
    enum Style { Power, TextPill, Check };
    Toggle (UiContext& ctx, const juce::String& paramId, Style style, const juce::String& text = {});
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    bool isOn() const noexcept { return on; }
    std::function<void()> onChange;
    juce::Colour onColour = col::accent;

private:
    UiContext& ctx;
    juce::String paramId, text;
    Style style;
    bool on = false;
    std::unique_ptr<juce::ParameterAttachment> att;
};

// Integer parameter: value with < > (click) or vertical drag.
class Stepper : public juce::Component, public juce::SettableTooltipClient
{
public:
    Stepper (UiContext& ctx, const juce::String& paramId);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    std::function<juce::String (int)> displayName;

private:
    UiContext& ctx;
    juce::String paramId;
    const cs::ParamDef* pdef = nullptr;
    std::unique_ptr<juce::ParameterAttachment> att;
    int value = 0, dragStart = 0;
    bool dragging = false;
};

// Small text button with an optional lit state (not a parameter).
class FlatButton : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit FlatButton (const juce::String& t = {}) : text (t) {}
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent& e) override
    {
        if (isEnabled() && getLocalBounds().contains (e.getPosition()) && onClick)
            onClick();
    }
    void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }
    juce::String text;
    bool lit = false, outline = true;
    juce::Colour litColour = col::accent;
    float fontSize = 12.0f;
    std::function<void()> onClick;
    std::function<void (juce::Graphics&, juce::Rectangle<float>, juce::Colour)> icon;

private:
    bool hover = false;
};

// Label that shows a parameter-style caption under / beside controls.
inline void drawCaption (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& t, juce::Colour c = col::textDim)
{
    g.setColour (c);
    g.setFont (font (11.0f));
    g.drawFittedText (t, r, juce::Justification::centred, 1);
}
} // namespace ui
