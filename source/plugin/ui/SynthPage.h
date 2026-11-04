#pragma once

#include "Visuals.h"

namespace ui
{
// Base for panels made of many parameter controls laid out by hand.
class ControlPanel : public juce::Component, public Refreshable
{
public:
    explicit ControlPanel (UiContext& c) : ctx (c) {}
    void refreshView() override;

protected:
    UiContext& ctx;
    std::vector<std::unique_ptr<juce::Component>> owned;
    std::vector<Refreshable*> views;

    Knob& knob (const juce::String& id, const juce::String& label = {}, juce::Rectangle<int> r = {});
    Pill& pill (const juce::String& id, juce::Rectangle<int> r = {});
    Toggle& toggle (const juce::String& id, Toggle::Style s, const juce::String& text = {}, juce::Rectangle<int> r = {});
    Stepper& stepper (const juce::String& id, juce::Rectangle<int> r = {});
    template <typename T, typename... Args>
    T& add (juce::Rectangle<int> r, Args&&... args)
    {
        auto c = std::make_unique<T> (std::forward<Args> (args)...);
        auto& ref = *c;
        c->setBounds (r);
        addAndMakeVisible (*c);
        if constexpr (std::is_base_of_v<Refreshable, T>)
            views.push_back (&ref);
        owned.push_back (std::move (c));
        return ref;
    }
    struct Caption
    {
        juce::Rectangle<int> r;
        juce::String text;
        bool title;
    };
    std::vector<Caption> captions;
    void caption (juce::Rectangle<int> r, const juce::String& t) { captions.push_back ({ r, t, false }); }
    void title (juce::Rectangle<int> r, const juce::String& t) { captions.push_back ({ r, t, true }); }
    void paintCaptions (juce::Graphics& g);
};

// Pill-like menu for an integer "mode" parameter whose names depend on context.
class NamedIntPill final : public juce::Component, public juce::SettableTooltipClient
{
public:
    NamedIntPill (UiContext& c, juce::String paramId);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    juce::StringArray names;

private:
    UiContext& ctx;
    juce::String id;
    std::unique_ptr<juce::ParameterAttachment> att;
    int value = 0;
};

class SynthPage final : public juce::Component, public Refreshable
{
public:
    explicit SynthPage (UiContext& ctx);
    ~SynthPage() override;
    void resized() override;
    void paint (juce::Graphics&) override;
    void refreshView() override;
    void showEngineTab (int t);

private:
    class EngineTabs;
    class FilterPanel;
    class RoutingColumn;
    std::unique_ptr<EngineTabs> engines;
    std::unique_ptr<FilterPanel> filters[2];
    std::unique_ptr<RoutingColumn> routing;
};
} // namespace ui
