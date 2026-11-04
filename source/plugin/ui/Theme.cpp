#include "Theme.h"

#include "BinaryData.h"
#include "dsp/Params.h"

namespace ui
{
juce::Colour sourceColour (int s)
{
    using namespace cs;
    if (s < 0)
        return col::textDim;
    if (s <= SrcExpression) return col::modKeyboard;
    if (s <= SrcEnv3) return col::modEnv;
    if (s <= SrcLfo3) return col::modLfo;
    if (s <= SrcFunc3) return col::modFunc;
    if (s <= SrcRand3) return col::modRandom;
    if (s <= SrcComb3) return col::modComb;
    return col::modMacro;
}

namespace
{
juce::Typeface::Ptr typeface (bool b)
{
    static auto reg = juce::Typeface::createSystemTypefaceFor (BinaryData::Inter400_ttf, BinaryData::Inter400_ttfSize);
    static auto bld = juce::Typeface::createSystemTypefaceFor (BinaryData::Inter600_ttf, BinaryData::Inter600_ttfSize);
    return b ? bld : reg;
}
} // namespace

juce::Font font (float height, bool bold)
{
    return juce::Font (juce::FontOptions (typeface (bold)).withHeight (height));
}

LookAndFeel::LookAndFeel()
{
    regular = typeface (false);
    bold = typeface (true);
    setColour (juce::ResizableWindow::backgroundColourId, col::bg);
    setColour (juce::Label::textColourId, col::text);
    setColour (juce::Slider::textBoxTextColourId, col::text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, col::panelLo);
    setColour (juce::TextButton::buttonColourId, col::panelHi);
    setColour (juce::TextButton::buttonOnColourId, col::accentDim);
    setColour (juce::TextButton::textColourOffId, col::text);
    setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    setColour (juce::ComboBox::backgroundColourId, col::panelLo);
    setColour (juce::ComboBox::textColourId, col::accent);
    setColour (juce::ComboBox::outlineColourId, col::border);
    setColour (juce::ComboBox::arrowColourId, col::textDim);
    setColour (juce::PopupMenu::backgroundColourId, col::panelHi);
    setColour (juce::PopupMenu::textColourId, col::text);
    setColour (juce::PopupMenu::headerTextColourId, col::gold);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, col::accentDim);
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour (juce::TextEditor::backgroundColourId, col::panelLo);
    setColour (juce::TextEditor::textColourId, col::text);
    setColour (juce::TextEditor::outlineColourId, col::border);
    setColour (juce::TextEditor::focusedOutlineColourId, col::accent);
    setColour (juce::TextEditor::highlightColourId, col::accentDim);
    setColour (juce::CaretComponent::caretColourId, col::accent);
    setColour (juce::ListBox::backgroundColourId, col::panelLo);
    setColour (juce::ScrollBar::thumbColourId, col::border);
    setColour (juce::TooltipWindow::backgroundColourId, col::panelHi);
    setColour (juce::TooltipWindow::textColourId, col::text);
    setColour (juce::AlertWindow::backgroundColourId, col::panel);
    setColour (juce::AlertWindow::textColourId, col::text);
    setColour (juce::AlertWindow::outlineColourId, col::border);
    setColour (juce::ToggleButton::textColourId, col::text);
    setColour (juce::ToggleButton::tickColourId, col::accent);
    setColour (juce::ToggleButton::tickDisabledColourId, col::textFaint);
    setColour (juce::TabbedButtonBar::tabTextColourId, col::textDim);
    setColour (juce::TabbedButtonBar::frontTextColourId, col::text);
    setColour (juce::DocumentWindow::textColourId, col::text);
}

juce::Typeface::Ptr LookAndFeel::getTypefaceForFont (const juce::Font& f)
{
    return f.isBold() ? bold : regular;
}

void LookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float start, float end,
                                    juce::Slider& s)
{
    const auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat();
    const float d = juce::jmin (bounds.getWidth(), bounds.getHeight());
    const auto r = bounds.withSizeKeepingCentre (d, d).reduced (d * 0.06f);
    const float radius = r.getWidth() * 0.5f;
    const auto c = r.getCentre();
    const float trackW = juce::jmax (2.0f, radius * 0.13f);
    const bool enabled = s.isEnabled();
    const auto valueCol = s.findColour (juce::Slider::rotarySliderFillColourId);

    // Track
    juce::Path track;
    track.addCentredArc (c.x, c.y, radius - trackW * 0.5f, radius - trackW * 0.5f, 0.0f, start, end, true);
    g.setColour (col::knobTrack);
    g.strokePath (track, juce::PathStrokeType (trackW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Value arc: from the centre for bipolar ranges, else from the start.
    const bool bipolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;
    const float zeroPos = bipolar ? (float) s.valueToProportionOfLength (0.0) : 0.0f;
    const float a0 = start + zeroPos * (end - start), a1 = start + pos * (end - start);
    if (std::abs (a1 - a0) > 0.001f)
    {
        juce::Path arc;
        arc.addCentredArc (c.x, c.y, radius - trackW * 0.5f, radius - trackW * 0.5f, 0.0f, juce::jmin (a0, a1), juce::jmax (a0, a1), true);
        g.setColour (enabled ? valueCol : col::textFaint);
        g.strokePath (arc, juce::PathStrokeType (trackW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Body
    const float bodyR = radius - trackW * 1.9f;
    juce::ColourGradient grad (col::knobBody.brighter (0.25f), c.x, c.y - bodyR, col::knobBody.darker (0.35f), c.x, c.y + bodyR, false);
    g.setGradientFill (grad);
    g.fillEllipse (c.x - bodyR, c.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawEllipse (c.x - bodyR, c.y - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.0f);

    // Pointer
    const float ang = a1 - juce::MathConstants<float>::halfPi;
    const float p0 = bodyR * 0.25f, p1 = bodyR * 0.85f;
    g.setColour (enabled ? col::text : col::textFaint);
    g.drawLine (c.x + std::cos (ang) * p0, c.y + std::sin (ang) * p0, c.x + std::cos (ang) * p1, c.y + std::sin (ang) * p1,
                juce::jmax (1.5f, radius * 0.08f));
}

void LookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                                    juce::Slider::SliderStyle style, juce::Slider& s)
{
    if (style == juce::Slider::LinearBar || style == juce::Slider::LinearBarVertical)
    {
        const auto r = juce::Rectangle<int> (x, y, w, h).toFloat();
        g.setColour (col::panelLo);
        g.fillRoundedRectangle (r, 3.0f);
        auto fill = r;
        if (style == juce::Slider::LinearBar)
            fill = fill.withWidth (pos - (float) x);
        else
            fill = fill.withTop (pos);
        g.setColour (col::accentDim);
        g.fillRoundedRectangle (fill, 3.0f);
        g.setColour (col::border);
        g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, 1.0f);
        return;
    }
    juce::LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, pos, 0, 0, style, s);
}

void LookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = b.getToggleState();
    auto fill = on ? col::accentDim : col::panelHi;
    if (over)
        fill = fill.brighter (0.08f);
    if (down)
        fill = fill.darker (0.15f);
    g.setColour (fill);
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (on ? col::accent.withAlpha (0.6f) : col::border);
    g.drawRoundedRectangle (r, 4.0f, 1.0f);
}

void LookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    g.setFont (font (juce::jmin (12.0f, (float) b.getHeight() * 0.55f), true));
    g.setColour (b.isEnabled() ? (b.getToggleState() ? juce::Colours::white : col::text) : col::textFaint);
    g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced (4, 1), juce::Justification::centred, 1);
}

void LookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool over, bool)
{
    auto r = b.getLocalBounds().toFloat();
    const float box = juce::jmin (14.0f, r.getHeight() - 2.0f);
    auto tick = r.removeFromLeft (box + 6.0f).withSizeKeepingCentre (box, box);
    g.setColour (b.getToggleState() ? col::accent : (over ? col::panelHi.brighter (0.2f) : col::panelHi));
    g.fillRoundedRectangle (tick, 3.0f);
    g.setColour (col::border);
    g.drawRoundedRectangle (tick, 3.0f, 1.0f);
    if (b.getToggleState())
    {
        juce::Path p;
        p.startNewSubPath (tick.getX() + box * 0.22f, tick.getCentreY());
        p.lineTo (tick.getX() + box * 0.42f, tick.getBottom() - box * 0.25f);
        p.lineTo (tick.getRight() - box * 0.2f, tick.getY() + box * 0.25f);
        g.setColour (col::bg);
        g.strokePath (p, juce::PathStrokeType (2.0f));
    }
    g.setColour (b.isEnabled() ? col::text : col::textFaint);
    g.setFont (font (12.0f));
    g.drawFittedText (b.getButtonText(), r.toNearestInt(), juce::Justification::centredLeft, 1);
}

void LookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&)
{
    const auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setColour (col::panelLo);
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (col::border);
    g.drawRoundedRectangle (r, 4.0f, 1.0f);
    juce::Path arrow;
    const float ax = (float) w - 12.0f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
    g.setColour (col::textDim);
    g.fillPath (arrow);
}

void LookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
{
    g.fillAll (col::panelHi);
    g.setColour (col::border);
    g.drawRect (0, 0, w, h);
}

void LookAndFeel::drawTooltip (juce::Graphics& g, const juce::String& text, int w, int h)
{
    const auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h);
    g.setColour (col::panelHi);
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (col::border);
    g.drawRoundedRectangle (r.reduced (0.5f), 4.0f, 1.0f);
    g.setColour (col::text);
    g.setFont (font (12.0f));
    g.drawFittedText (text, juce::Rectangle<int> (w, h).reduced (6, 3), juce::Justification::centredLeft, 4);
}

juce::Rectangle<int> LookAndFeel::getTooltipBounds (const juce::String& text, juce::Point<int> pos, juce::Rectangle<int> parent)
{
    const int w = juce::jmin (320, (int) juce::GlyphArrangement::getStringWidth (font (12.0f), text) + 16);
    const int lines = 1 + (int) juce::GlyphArrangement::getStringWidth (font (12.0f), text) / 300;
    const int h = 8 + lines * 15;
    return juce::Rectangle<int> (pos.x > parent.getCentreX() ? pos.x - (w + 12) : pos.x + 12, pos.y + 16, w, h)
        .constrainedWithin (parent);
}

void LookAndFeel::drawScrollbar (juce::Graphics& g, juce::ScrollBar&, int x, int y, int w, int h, bool vertical, int thumbStart,
                                 int thumbSize, bool over, bool)
{
    auto thumb = vertical ? juce::Rectangle<int> (x + 1, thumbStart, w - 2, thumbSize) : juce::Rectangle<int> (thumbStart, y + 1, thumbSize, h - 2);
    g.setColour (over ? col::textFaint : col::border);
    g.fillRoundedRectangle (thumb.toFloat(), 3.0f);
}

void drawPanel (juce::Graphics& g, juce::Rectangle<float> r, float corner, juce::Colour fill)
{
    g.setColour (fill);
    g.fillRoundedRectangle (r, corner);
}

void drawSectionTitle (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& text, juce::Colour c, juce::Justification j)
{
    g.setColour (c);
    g.setFont (font (11.5f, true));
    g.drawText (text.toUpperCase(), r, j, true);
}

void drawPowerIcon (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c)
{
    const auto s = juce::jmin (r.getWidth(), r.getHeight()) * 0.8f;
    const auto b = r.withSizeKeepingCentre (s, s);
    juce::Path p;
    p.addCentredArc (b.getCentreX(), b.getCentreY() + s * 0.04f, s * 0.4f, s * 0.4f, 0.0f, 0.65f, juce::MathConstants<float>::twoPi - 0.65f, true);
    g.setColour (c);
    g.strokePath (p, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.drawLine (b.getCentreX(), b.getY(), b.getCentreX(), b.getCentreY(), 1.6f);
}
} // namespace ui
