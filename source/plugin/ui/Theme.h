#pragma once

// Church Synth look: colours, fonts and the LookAndFeel. Our own artwork
// (dark only): charcoal panels, teal values, gold brand, a colour per
// modulation-source group.

#include <juce_gui_basics/juce_gui_basics.h>

namespace ui
{
namespace col
{
const juce::Colour bg (0xff141519);
const juce::Colour panel (0xff1f2126);
const juce::Colour panelHi (0xff272a31);
const juce::Colour panelLo (0xff191a1e);
const juce::Colour border (0xff30333b);
const juce::Colour text (0xffdfe1e6);
const juce::Colour textDim (0xff8b8f99);
const juce::Colour textFaint (0xff5c606a);
const juce::Colour accent (0xff2fc6b8);     // values, active controls
const juce::Colour accentDim (0xff1d7c74);
const juce::Colour gold (0xfff0b44c);       // brand, highlights
const juce::Colour knobBody (0xff2d3038);
const juce::Colour knobTrack (0xff3a3e47);
const juce::Colour warn (0xffe8575a);
const juce::Colour led (0xff5ee07f);        // sound LEDs

// Modulation source groups.
const juce::Colour modKeyboard (0xffe264a8);
const juce::Colour modEnv (0xfff08c4c);
const juce::Colour modLfo (0xffe8d05a);
const juce::Colour modFunc (0xff6fcf6b);
const juce::Colour modRandom (0xff9b80f2);
const juce::Colour modComb (0xffea5d5f);
const juce::Colour modMacro (0xff4cb8e8);
} // namespace col

juce::Colour sourceColour (int source);

juce::Font font (float height, bool bold = false);

class LookAndFeel final : public juce::LookAndFeel_V4
{
public:
    LookAndFeel();
    juce::Typeface::Ptr getTypefaceForFont (const juce::Font&) override;

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float minPos, float maxPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool over, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return font (12.0f); }
    juce::Font getPopupMenuFont() override { return font (13.0f); }
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return font (12.0f, true); }
    juce::Font getLabelFont (juce::Label&) override { return font (12.0f); }
    void drawPopupMenuBackground (juce::Graphics&, int w, int h) override;
    void drawTooltip (juce::Graphics&, const juce::String&, int w, int h) override;
    juce::Rectangle<int> getTooltipBounds (const juce::String&, juce::Point<int>, juce::Rectangle<int>) override;
    void drawScrollbar (juce::Graphics&, juce::ScrollBar&, int x, int y, int w, int h, bool vertical, int thumbStart,
                        int thumbSize, bool over, bool down) override;
    int getDefaultScrollbarWidth() override { return 8; }

private:
    juce::Typeface::Ptr regular, bold;
};

// Panel background with an optional title (top-left, small caps).
void drawPanel (juce::Graphics&, juce::Rectangle<float> r, float corner = 6.0f, juce::Colour fill = col::panel);
void drawSectionTitle (juce::Graphics&, juce::Rectangle<int> r, const juce::String& text,
                       juce::Colour c = col::text, juce::Justification j = juce::Justification::centredLeft);
void drawPowerIcon (juce::Graphics&, juce::Rectangle<float> r, juce::Colour c);
} // namespace ui
