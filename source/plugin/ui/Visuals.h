#pragma once

// Displays: oscillator and wavetable views, filter response, envelope / LFO
// shapes, the function editor, keyboard curves and small scopes. They redraw
// at a low rate from the editor timer and do nothing while the editor is closed.

#include "Widgets.h"

#include "dsp/ModSources.h"

namespace ui
{
// Draws an analog waveform (Sine, Triangle, Saw, Ramp, Square) with pulse width.
void drawWave (juce::Graphics& g, juce::Rectangle<float> r, int wave, float width, juce::Colour c, float thickness = 1.5f, bool fill = false);

// Something that redraws when the sound changes (called by the editor timer).
class Refreshable
{
public:
    virtual ~Refreshable() = default;
    virtual void refreshView() {}
};

class OscDisplay final : public juce::Component, public Refreshable
{
public:
    OscDisplay (UiContext& c, juce::String waveId, juce::String widthId, juce::String volId)
        : ctx (c), wave (std::move (waveId)), width (std::move (widthId)), vol (std::move (volId)) {}
    void paint (juce::Graphics&) override;
    void refreshView() override { repaint(); }

private:
    UiContext& ctx;
    juce::String wave, width, vol;
};

// Wave choice icons under an oscillator display (click to pick).
class WaveSelector final : public juce::Component, public juce::SettableTooltipClient
{
public:
    WaveSelector (UiContext& c, juce::String waveId);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    UiContext& ctx;
    juce::String id;
    std::unique_ptr<juce::ParameterAttachment> att;
    int current = 0;
};

class WavetableDisplay final : public juce::Component, public Refreshable
{
public:
    WavetableDisplay (UiContext& c, int engine) : ctx (c), eng (engine) {}
    void paint (juce::Graphics&) override;
    void refreshView() override { repaint(); }

private:
    UiContext& ctx;
    int eng;
};

// Filter response computed from the real filter code (impulse -> magnitude).
class FilterDisplay final : public juce::Component, public Refreshable
{
public:
    FilterDisplay (UiContext& c, int filter) : ctx (c), index (filter) {}
    void paint (juce::Graphics&) override;
    void refreshView() override;

private:
    UiContext& ctx;
    int index;
    std::vector<float> mags; // dB at log-spaced frequencies
    juce::String lastKey;
    bool active = true;
};

class EnvDisplay final : public juce::Component, public Refreshable
{
public:
    EnvDisplay (UiContext& c, juce::String prefix, juce::Colour colour) : ctx (c), p (std::move (prefix)), colourUsed (colour) {}
    void paint (juce::Graphics&) override;
    void refreshView() override { repaint(); }

private:
    UiContext& ctx;
    juce::String p;
    juce::Colour colourUsed;
};

class LfoDisplay final : public juce::Component, public Refreshable
{
public:
    LfoDisplay (UiContext& c, int lfo) : ctx (c), k (lfo) {}
    void paint (juce::Graphics&) override;
    void refreshView() override { repaint(); }

private:
    UiContext& ctx;
    int k;
};

// Draw / edit a function shape. Editing a preset shape turns it into "Custom".
class FunctionEditor final : public juce::Component, public Refreshable, public juce::SettableTooltipClient
{
public:
    FunctionEditor (UiContext& c, int func) : ctx (c), k (func) {}
    void paint (juce::Graphics&) override;
    void refreshView() override { repaint(); }
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    UiContext& ctx;
    int k;
    int dragPoint = -1, dragSegment = -1;
    float dragCurveStart = 0.0f;
    cs::FunctionShape working;
    cs::FunctionShape current() const; // the shape playing now
    void makeCustom (const cs::FunctionShape& s, bool record);
    juce::Point<float> toScreen (float x, float y) const;
    juce::Point<float> fromScreen (juce::Point<float> p) const;
    juce::Rectangle<float> plot() const { return getLocalBounds().toFloat().reduced (6.0f, 6.0f); }
};

// Keyboard response curve (velocity / aftertouch / keytrack): drag to bend.
class CurveEditor final : public juce::Component, public Refreshable, public juce::SettableTooltipClient
{
public:
    CurveEditor (UiContext& c, juce::String paramId, juce::String title, juce::Colour colour);
    void paint (juce::Graphics&) override;
    void refreshView() override { repaint(); }
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    UiContext& ctx;
    juce::String id, title;
    juce::Colour colourUsed;
    float startValue = 0.0f;
    std::unique_ptr<juce::ParameterAttachment> att;
};

// Rolling trace of a modulation source (mod strip tiles, random view).
class Scope final : public juce::Component
{
public:
    explicit Scope (juce::Colour c) : colourUsed (c) {}
    void push (float v);
    void paint (juce::Graphics&) override;
    bool bipolar = true;
    juce::Colour colourUsed;

private:
    std::array<float, 96> buf {};
    int pos = 0;
};

// Mini keyboard: 12 toggles for a 12-bit note mask parameter (Mod Quantize).
class NoteMask final : public juce::Component, public juce::SettableTooltipClient
{
public:
    NoteMask (UiContext& c, juce::String paramId);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    UiContext& ctx;
    juce::String id;
    std::unique_ptr<juce::ParameterAttachment> att;
    int mask = 0xFFF;
    juce::Rectangle<float> keyRect (int note) const;
};
} // namespace ui
