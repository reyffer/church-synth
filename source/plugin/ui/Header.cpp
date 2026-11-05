#include "Header.h"

#include "BinaryData.h"

#include "../PresetManager.h"
#include "dsp/ModMatrix.h"

namespace ui
{
namespace
{
void iconBrowser (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c)
{
    g.setColour (c);
    const float w = r.getWidth() / 5.0f;
    for (int i = 0; i < 3; ++i)
        g.fillRect (r.getX() + w * (float) i * 1.5f, r.getY(), w * 0.8f, r.getHeight());
    juce::Path p;
    p.addRectangle (0, 0, w * 0.8f, r.getHeight());
    p.applyTransform (juce::AffineTransform::rotation (-0.35f).translated (r.getRight() - w * 0.9f, r.getY() + 1.0f));
    g.fillPath (p);
}

void iconHeart (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c, bool filled)
{
    juce::Path p;
    const float x = r.getX(), y = r.getY(), w = r.getWidth(), h = r.getHeight();
    p.startNewSubPath (x + w * 0.5f, y + h);
    p.cubicTo (x - w * 0.15f, y + h * 0.45f, x + w * 0.15f, y - h * 0.15f, x + w * 0.5f, y + h * 0.25f);
    p.cubicTo (x + w * 0.85f, y - h * 0.15f, x + w * 1.15f, y + h * 0.45f, x + w * 0.5f, y + h);
    p.closeSubPath();
    g.setColour (c);
    if (filled)
        g.fillPath (p);
    else
        g.strokePath (p, juce::PathStrokeType (1.4f));
}

void iconMenu (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c)
{
    g.setColour (c);
    for (int i = 0; i < 3; ++i)
        g.fillRoundedRectangle (r.getX(), r.getY() + r.getHeight() * (0.1f + 0.38f * (float) i), r.getWidth(), 2.0f, 1.0f);
}

void iconArrow (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c, bool up)
{
    juce::Path p;
    if (up)
        p.addTriangle (r.getX(), r.getBottom() - r.getHeight() * 0.2f, r.getRight(), r.getBottom() - r.getHeight() * 0.2f, r.getCentreX(), r.getY() + r.getHeight() * 0.2f);
    else
        p.addTriangle (r.getX(), r.getY() + r.getHeight() * 0.2f, r.getRight(), r.getY() + r.getHeight() * 0.2f, r.getCentreX(), r.getBottom() - r.getHeight() * 0.2f);
    g.setColour (c);
    g.fillPath (p);
}

void iconSave (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c)
{
    g.setColour (c);
    g.drawRoundedRectangle (r, 2.0f, 1.4f);
    g.fillRect (r.getX() + r.getWidth() * 0.25f, r.getY(), r.getWidth() * 0.5f, r.getHeight() * 0.35f);
    g.drawRect (r.getX() + r.getWidth() * 0.22f, r.getY() + r.getHeight() * 0.6f, r.getWidth() * 0.56f, r.getHeight() * 0.4f, 1.0f);
}
} // namespace

// ===========================================================================
TopBar::TopBar (UiContext& uc) : ctx (uc), seqPower (uc, "seq_on", Toggle::Power), fxPower (uc, "fx_on", Toggle::Power), master (uc, "master_vol", {})
{
    menu.outline = false;
    menu.icon = iconMenu;
    menu.setTooltip ("Menu");
    menu.onClick = [this] { if (onMenu) onMenu(); };
    browser.icon = iconBrowser;
    browser.setTooltip ("Preset browser");
    browser.onClick = [this] { if (onBrowser) onBrowser(); };
    fav.outline = false;
    fav.setTooltip ("Favourite");
    fav.onClick = [this] {
        auto& pm = ctx.proc.presets();
        const auto info = pm.currentInfo();
        pm.setFavourite (info, ! pm.isFavourite (info));
        refresh();
    };
    prev.outline = next.outline = false;
    prev.icon = [] (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c) { iconArrow (g, r, c, true); };
    next.icon = [] (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c) { iconArrow (g, r, c, false); };
    prev.setTooltip ("Previous preset");
    next.setTooltip ("Next preset");
    prev.onClick = [this] { if (onPrev) onPrev(); };
    next.onClick = [this] { if (onNext) onNext(); };
    save.outline = false;
    save.icon = iconSave;
    save.setTooltip ("Save preset");
    save.onClick = [this] { if (onSave) onSave(); };
    for (auto* b : { &synthBtn, &fxBtn, &seqBtn })
    {
        b->outline = false;
        b->fontSize = 13.0f;
    }
    synthBtn.onClick = [this] { if (onPage) onPage (Page::Synth); };
    fxBtn.onClick = [this] { if (onPage) onPage (Page::Fx); };
    seqBtn.onClick = [this] { if (onPage) onPage (Page::Seq); };
    seqPower.setTooltip ("Seq/Arp on/off");
    fxBtn.justification = seqBtn.justification = juce::Justification::centredLeft; // hug their power switches
    fxPower.setTooltip ("FX section on/off");
    master.setTooltip ("Master volume");
    master.setLabel ({});
    for (auto* comp : std::initializer_list<juce::Component*> { &menu, &browser, &fav, &prev, &next, &save, &synthBtn, &fxBtn, &seqBtn, &seqPower, &fxPower, &master })
        addAndMakeVisible (comp);
    refresh();
}

void TopBar::refresh()
{
    auto& pm = ctx.proc.presets();
    name = ctx.proc.presetName();
    category = ctx.proc.presetCategory();
    modified = ctx.proc.presetModified();
    isFav = pm.isFavourite (pm.currentInfo());
    fav.icon = [f = isFav] (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour) { iconHeart (g, r, f ? col::gold : col::textDim, f); };
    synthBtn.lit = page == Page::Synth;
    fxBtn.lit = page == Page::Fx;
    seqBtn.lit = page == Page::Seq;
    browser.lit = browserOpen;
    for (auto* b : std::initializer_list<FlatButton*> { &synthBtn, &fxBtn, &seqBtn, &browser, &fav })
        b->repaint();
    repaint();
}

void TopBar::resized()
{
    auto r = getLocalBounds();
    menu.setBounds (r.removeFromLeft (44).reduced (12, 13));
    auto centre = juce::Rectangle<int> (440, 7, 400, 30);
    browser.setBounds (centre.removeFromLeft (34).reduced (3, 3));
    fav.setBounds (centre.removeFromLeft (28).reduced (6, 7));
    auto arrows = centre.removeFromRight (44);
    prev.setBounds (arrows.removeFromLeft (22).reduced (5, 8));
    next.setBounds (arrows.reduced (5, 8));
    save.setBounds (centre.removeFromRight (26).reduced (5, 7));
    nameArea = centre;
    auto right = r.removeFromRight (380);
    right.removeFromRight (10);
    meterArea = right.removeFromRight (8).reduced (0, 8);
    right.removeFromRight (4);
    master.setBounds (right.removeFromRight (40).reduced (2, 4));
    // Page tabs; each power switch sits right against its own label: SYNTH   (|)FX   (|)SEQ
    right.removeFromRight (16);
    seqBtn.setBounds (right.removeFromRight (40).reduced (0, 9));
    seqPower.setBounds (right.removeFromRight (18).reduced (1, 12));
    right.removeFromRight (18);
    fxBtn.setBounds (right.removeFromRight (30).reduced (0, 9));
    fxPower.setBounds (right.removeFromRight (18).reduced (1, 12));
    right.removeFromRight (18);
    synthBtn.setBounds (right.removeFromRight (56).reduced (0, 9));
}

void TopBar::tickMeter()
{
    const float pk = ctx.proc.takeOutputPeak();
    meter = juce::jmax (pk, meter * 0.85f);
    repaint (meterArea.expanded (2));
}

void TopBar::paint (juce::Graphics& g)
{
    g.fillAll (col::panelLo);
    {
        // Output meter (dBFS, -48..0)
        const auto m = meterArea.toFloat();
        g.setColour (col::bg);
        g.fillRoundedRectangle (m, 2.0f);
        const float db = juce::Decibels::gainToDecibels (meter, -60.0f);
        const float frac = juce::jlimit (0.0f, 1.0f, (db + 48.0f) / 48.0f);
        g.setColour (meter >= 0.99f ? col::warn : (db > -6.0f ? col::gold : col::accent));
        g.fillRoundedRectangle (m.withTop (m.getBottom() - m.getHeight() * frac), 2.0f);
    }
    g.setColour (col::border);
    g.drawHorizontalLine (getHeight() - 1, 0.0f, (float) getWidth());

    // Logo: the church lotus (light grey), then CHURCH and SYNTH, evenly spaced
    // (same gap between the three, measured from the actual glyph widths).
    {
        constexpr float gap = 9.0f, x0 = 48.0f, lh = 24.0f;
        float x = x0;
        const juce::Image lotus = juce::ImageCache::getFromMemory (BinaryData::lotus_png, BinaryData::lotus_pngSize);
        if (lotus.isValid())
        {
            const float lw = lh * (float) lotus.getWidth() / (float) lotus.getHeight();
            g.setColour (col::text.withAlpha (0.85f));
            g.drawImage (lotus, { x, ((float) getHeight() - lh) * 0.5f, lw, lh }, juce::RectanglePlacement::centred, true);
            x += lw + gap;
        }
        auto word = [&] (const juce::String& t, const juce::Font& f, juce::Colour c) {
            juce::GlyphArrangement ga;
            ga.addLineOfText (f, t, 0.0f, 0.0f);
            const auto bb = ga.getBoundingBox (0, -1, true); // ink bounds, so spacing follows the letters
            g.setColour (c);
            const float baseline = (float) getHeight() * 0.5f + f.getAscent() * 0.36f;
            ga.moveRangeOfGlyphs (0, -1, x - bb.getX(), baseline);
            ga.draw (g);
            x += bb.getWidth() + gap;
        };
        word ("CHURCH", font (20.0f, true), col::gold);
        word ("SYNTH", font (20.0f, false), col::text);
    }

    // Preset bar
    auto bar = juce::Rectangle<float> (440.0f, 7.0f, 400.0f, 30.0f);
    g.setColour (col::bg);
    g.fillRoundedRectangle (bar, 5.0f);
    g.setColour (col::border);
    g.drawRoundedRectangle (bar.reduced (0.5f), 5.0f, 1.0f);
    g.setColour (col::text);
    g.setFont (font (15.0f));
    g.drawFittedText (name + (modified ? " *" : ""), nameArea, juce::Justification::centred, 1);
    if (category.isNotEmpty())
    {
        g.setColour (col::textFaint);
        g.setFont (font (10.0f));
        g.drawText (category.toUpperCase(), nameArea.withTrimmedLeft (nameArea.getWidth() - 90).withTrimmedRight (4), juce::Justification::centredRight);
    }
}

void TopBar::mouseDown (const juce::MouseEvent& e)
{
    if (nameArea.contains (e.getPosition()) && onBrowser)
        onBrowser();
}

// ===========================================================================
Footer::Footer (UiContext& c) : ctx (c)
{
    playMode.setTooltip ("Play mode: voicing, polyphony, glide, bend, transpose, hold");
    playMode.onClick = [this] { showPlayMode(); };
    undo.text = "Undo";
    redo.text = "Redo";
    historyBtn.text = "History";
    routesBtn.text = "Mod routes";
    routesBtn.setTooltip ("List every modulation route (edit amounts and sidechains)");
    undo.onClick = [this] { ctx.proc.undo(); };
    redo.onClick = [this] { ctx.proc.redo(); };
    historyBtn.onClick = [this] { showHistory(); };
    routesBtn.onClick = [this] { if (onRoutes) onRoutes(); };
    for (auto* b : { &playMode, &undo, &redo, &historyBtn, &routesBtn })
    {
        b->fontSize = 11.0f;
        addAndMakeVisible (b);
    }
    done.text = "Done";
    done.fontSize = 11.0f;
    done.lit = true;
    done.setTooltip ("Finish editing this source's modulation (or click any empty area, or press Esc)");
    done.onClick = [this] { ctx.clearSource(); };
    addChildComponent (done);
    ctx.addChangeListener (this);
    refresh();
}

Footer::~Footer()
{
    ctx.removeChangeListener (this);
}

void Footer::refresh()
{
    const int v = ctx.intValue ("voicing");
    const int maxV = ctx.intValue ("max_voices");
    playMode.text = v <= 1 ? (v == 0 ? "Mono" : "Legato") : "Poly " + juce::String (maxV);
    undo.setEnabled (ctx.proc.canUndo());
    redo.setEnabled (ctx.proc.canRedo());
    cpu = ctx.proc.getCpuLoad();
    voices = ctx.proc.getSynth().uiVoices.load (std::memory_order_relaxed);
    const auto learn = ctx.proc.learningParam();
    learnText = learn.isNotEmpty() ? "MIDI learn: move a controller for " + (ctx.def (learn) != nullptr ? ctx.def (learn)->name : learn)
                                   : juce::String();
    for (auto* b : { &playMode, &undo, &redo })
        b->repaint();
    done.setVisible (ctx.selectedSource >= 0);
    if (ctx.selectedSource >= 0)
        done.litColour = sourceColour (ctx.selectedSource);
    repaint();
}

void Footer::resized()
{
    auto r = getLocalBounds().reduced (6, 4);
    auto right = r.removeFromRight (620);
    right.removeFromRight (124); // CPU / voices text
    historyBtn.setBounds (right.removeFromRight (64).reduced (2, 0));
    redo.setBounds (right.removeFromRight (52).reduced (2, 0));
    undo.setBounds (right.removeFromRight (52).reduced (2, 0));
    right.removeFromRight (14);
    playMode.setBounds (right.removeFromRight (78).reduced (2, 0));
    playCaption = right.removeFromRight (66);
    right.removeFromRight (14);
    routesBtn.setBounds (right.removeFromRight (90).reduced (2, 0));
    done.setBounds (r.getX() + 420, r.getY(), 60, r.getHeight());
}

void Footer::paint (juce::Graphics& g)
{
    g.fillAll (col::panelLo);
    g.setColour (col::border);
    g.drawHorizontalLine (0, 0.0f, (float) getWidth());
    auto r = getLocalBounds().reduced (10, 0);
    g.setFont (font (11.0f));
    g.setColour (col::textDim);
    g.drawText ("CPU " + juce::String (juce::roundToInt (cpu * 100.0f)) + " %   Voices " + juce::String (voices),
                r.removeFromRight (118), juce::Justification::centredRight);
    r.removeFromRight (520);
    if (ctx.selectedSource >= 0)
    {
        // Mod-edit mode banner.
        g.setColour (sourceColour (ctx.selectedSource));
        g.setFont (font (11.5f, true));
        g.drawText (juce::String ("Editing ") + cs::modSourceName (ctx.selectedSource)
                        + ": drag knobs to set amounts, double-click a knob to remove",
                    r.withWidth (414), juce::Justification::centredLeft);
    }
    else
    {
        g.setColour (learnText.isNotEmpty() ? col::gold : col::textFaint);
        g.drawText (learnText.isNotEmpty() ? learnText
                                           : juce::String ("Click a mod source to edit amounts on the knobs, or drag it onto a knob. Right-click a control for MIDI learn."),
                    r, juce::Justification::centredLeft);
    }
    g.setColour (col::textDim);
    g.drawText ("Play Mode", playCaption, juce::Justification::centredRight);
}

void Footer::showPlayMode()
{
    auto panel = std::make_unique<PlayModePanel> (ctx);
    panel->setSize (360, 150);
    juce::CallOutBox::launchAsynchronously (std::move (panel), playMode.getScreenBounds(), nullptr);
}

void Footer::showHistory()
{
    juce::PopupMenu m;
    const auto labels = ctx.proc.historyLabels();
    const int cur = ctx.proc.getHistoryIndex();
    m.addSectionHeader ("History");
    for (int i = labels.size() - 1; i >= juce::jmax (0, labels.size() - 40); --i)
        m.addItem (i + 1, labels[i], true, i == cur);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&historyBtn), [this] (int r) {
        if (r > 0)
            ctx.proc.jumpToHistory (r - 1);
    });
}

// ===========================================================================
PlayModePanel::PlayModePanel (UiContext& c)
    : voicing (c, "voicing"), voices (c, "max_voices"), bendUp (c, "bend_up"), bendDown (c, "bend_down"),
      transpose (c, "transpose"), glide (c, "glide", "Glide"), fine (c, "fine_tune", "Fine Tune"),
      legato (c, "glide_legato", Toggle::TextPill, "legato"), hold (c, "hold", Toggle::TextPill, "Hold")
{
    transpose.displayName = [] (int v) { return (v > 0 ? "+" : "") + juce::String (v) + " oct"; };
    for (auto* comp : std::initializer_list<juce::Component*> { &voicing, &voices, &bendUp, &bendDown, &transpose, &glide, &fine, &legato, &hold })
        addAndMakeVisible (comp);
}

void PlayModePanel::paint (juce::Graphics& g)
{
    g.fillAll (col::panel);
    drawCaption (g, { 10, 6, 120, 14 }, "Voicing");
    drawCaption (g, { 140, 6, 70, 14 }, "Voices");
    drawCaption (g, { 10, 58, 60, 14 }, "Bend up");
    drawCaption (g, { 80, 58, 60, 14 }, "Bend down");
    drawCaption (g, { 150, 58, 70, 14 }, "Transpose");
}

void PlayModePanel::resized()
{
    voicing.setBounds (10, 22, 120, 22);
    voices.setBounds (140, 22, 70, 22);
    hold.setBounds (220, 22, 60, 22);
    bendUp.setBounds (10, 74, 60, 22);
    bendDown.setBounds (80, 74, 60, 22);
    transpose.setBounds (150, 74, 70, 22);
    glide.setBounds (290, 4, 60, 64);
    legato.setBounds (292, 70, 56, 18);
    fine.setBounds (230, 76, 60, 64);
}
} // namespace ui
