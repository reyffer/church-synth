#pragma once

#include "Widgets.h"

namespace ui
{
enum class Page { Synth, Fx, Seq };

class TopBar final : public juce::Component
{
public:
    explicit TopBar (UiContext& ctx);
    void resized() override;
    void paint (juce::Graphics&) override;
    void refresh(); // preset name, page buttons (editor timer)
    void tickMeter();

    std::function<void (Page)> onPage;
    std::function<void()> onBrowser, onPrev, onNext, onMenu, onSave;
    Page page = Page::Synth;
    bool browserOpen = false;

private:
    UiContext& ctx;
    FlatButton menu, browser, fav, prev, next, save;
    FlatButton synthBtn { "SYNTH" }, fxBtn { "FX" }, seqBtn { "SEQ" };
    Toggle seqPower, fxPower;
    Knob master;
    juce::String name, category;
    bool modified = false, isFav = false;
    float meter = 0.0f;
    juce::Rectangle<int> nameArea, meterArea;
    void mouseDown (const juce::MouseEvent&) override;
};

class Footer final : public juce::Component, private juce::ChangeListener
{
public:
    explicit Footer (UiContext& ctx);
    ~Footer() override;
    void resized() override;
    void paint (juce::Graphics&) override;
    void refresh();
    std::function<void()> onRoutes;

private:
    UiContext& ctx;
    FlatButton playMode, undo, redo, historyBtn, routesBtn, done;
    void changeListenerCallback (juce::ChangeBroadcaster*) override { refresh(); }
    float cpu = 0.0f;
    int voices = 0;
    juce::String learnText;
    juce::Rectangle<int> playCaption;
    void showPlayMode();
    void showHistory();
};

// Play mode settings (voicing, polyphony, glide, bend...), shown in a call-out.
class PlayModePanel final : public juce::Component
{
public:
    explicit PlayModePanel (UiContext& ctx);
    void resized() override;
    void paint (juce::Graphics&) override;

private:
    Pill voicing;
    Stepper voices, bendUp, bendDown, transpose;
    Knob glide, fine;
    Toggle legato, hold;
};
} // namespace ui
