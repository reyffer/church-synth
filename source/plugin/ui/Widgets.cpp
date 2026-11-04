#include "Widgets.h"
#include <map>

#include "dsp/ModMatrix.h"

namespace ui
{
// ===========================================================================
// UiContext
// ===========================================================================
UiContext::UiContext (ChurchSynthProcessor& p) : proc (p)
{
    refreshRoutes();
}

void UiContext::selectSource (int s)
{
    selectedSource = (s == selectedSource) ? -1 : s;
    sendChangeMessage();
}

void UiContext::refreshRoutes()
{
    cached = proc.getRoutes();
    sendChangeMessage();
}

float UiContext::routeAmount (int src, int dest) const noexcept
{
    for (int i = 0; i < cached.count; ++i)
        if (cached.routes[i].source == src && cached.routes[i].dest == dest)
            return cached.routes[i].amount;
    return 0.0f;
}

bool UiContext::hasRoute (int src, int dest) const noexcept
{
    for (int i = 0; i < cached.count; ++i)
        if (cached.routes[i].source == src && cached.routes[i].dest == dest)
            return true;
    return false;
}

bool UiContext::sourceUsed (int src) const noexcept
{
    for (int i = 0; i < cached.count; ++i)
        if (cached.routes[i].source == src || cached.routes[i].sidechain == src)
            return true;
    return false;
}

std::vector<std::pair<int, float>> UiContext::routesTo (int dest) const
{
    std::vector<std::pair<int, float>> r;
    for (int i = 0; i < cached.count; ++i)
        if (cached.routes[i].dest == dest)
            r.push_back ({ cached.routes[i].source, cached.routes[i].amount });
    return r;
}

void UiContext::setRouteAmount (int src, int dest, float amount, bool record)
{
    if (src < 0 || dest < 0)
        return;
    amount = juce::jlimit (-1.0f, 1.0f, amount);
    auto r = proc.getRoutes();
    bool found = false;
    for (int i = 0; i < r.count; ++i)
        if (r.routes[i].source == src && r.routes[i].dest == dest)
        {
            r.routes[i].amount = amount;
            found = true;
        }
    if (! found)
    {
        if (r.count >= cs::kMaxRoutes)
            return;
        cs::ModRoute nr;
        nr.source = src;
        nr.dest = dest;
        nr.amount = amount;
        r.routes[r.count++] = nr;
    }
    proc.setRoutes (r);
    if (record)
        proc.checkpoint (juce::String ("Mod ") + cs::modSourceName (src) + " > " + cs::modDestinations()[(size_t) dest].name);
    refreshRoutes();
}

void UiContext::removeRoute (int src, int dest)
{
    auto r = proc.getRoutes();
    cs::ModRouting out;
    for (int i = 0; i < r.count; ++i)
        if (! (r.routes[i].source == src && r.routes[i].dest == dest))
            out.routes[out.count++] = r.routes[i];
    proc.setRoutes (out);
    proc.checkpoint (juce::String ("Remove mod ") + cs::modSourceName (src));
    refreshRoutes();
}

const cs::ParamDef* UiContext::def (const juce::String& id) const
{
    for (const auto& d : cs::parameterDefs())
        if (d.id == id)
            return &d;
    return nullptr;
}

float UiContext::value (const juce::String& id) const
{
    if (auto* v = proc.apvts.getRawParameterValue (id))
        return v->load();
    return 0.0f;
}

void UiContext::setValue (const juce::String& id, float realValue)
{
    if (auto* p = param (id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (realValue));
        p->endChangeGesture();
    }
}

juce::String formatValue (const cs::ParamDef& d, float v)
{
    if (d.kind == cs::ParamDef::Choice)
        return d.choices[juce::jlimit (0, d.choices.size() - 1, (int) std::lround (v))];
    if (d.kind == cs::ParamDef::Bool)
        return v >= 0.5f ? "On" : "Off";
    if (d.kind == cs::ParamDef::Int)
        return juce::String ((int) std::lround (v)) + (d.unit.isNotEmpty() ? " " + d.unit : juce::String());
    if (d.unit == "Hz")
        return v >= 1000.0f ? juce::String (v / 1000.0f, v >= 10000.0f ? 1 : 2) + " kHz"
                            : juce::String (v, v < 10.0f ? 2 : (v < 100.0f ? 1 : 0)) + " Hz";
    if (d.unit == "s")
        return v < 1.0f ? juce::String (juce::roundToInt (v * 1000.0f)) + " ms" : juce::String (v, 2) + " s";
    if (d.unit == "st")
        return juce::String (v, std::abs (v) < 1.0f ? 2 : 1) + " st";
    if (d.unit == "deg")
        return juce::String (juce::roundToInt (v)) + juce::String::fromUTF8 ("\xc2\xb0");
    if (d.unit == "BPM")
        return juce::String (v, 1) + " BPM";
    if (juce::approximatelyEqual (d.min, 0.0f) && juce::approximatelyEqual (d.max, 1.0f))
        return juce::String (juce::roundToInt (v * 100.0f)) + " %";
    if (juce::approximatelyEqual (d.min, -1.0f) && juce::approximatelyEqual (d.max, 1.0f))
        return juce::String (juce::roundToInt (v * 100.0f)) + " %";
    if (d.max - d.min >= 20.0f && std::abs (v - std::round (v)) < 1.0e-3f)
        return juce::String ((int) std::lround (v));
    return juce::String (v, 2);
}

// ===========================================================================
// Knob
// ===========================================================================
class Knob::ModOverlay final : public juce::Component
{
public:
    explicit ModOverlay (Knob& k) : knob (k) { setRepaintsOnMouseActivity (false); }
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu())
        {
            Knob::showContextMenu (knob.ctx, knob.paramId, &knob);
            return;
        }
        startAmount = knob.ctx.routeAmount (knob.ctx.selectedSource, knob.dest);
        knob.editingMod = true;
        knob.modDragAmount = startAmount;
    }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (! knob.editingMod)
            return;
        const float delta = (float) (-e.getDistanceFromDragStartY() + e.getDistanceFromDragStartX()) / 150.0f
                            * (e.mods.isShiftDown() ? 0.2f : 1.0f);
        knob.modDragAmount = juce::jlimit (-1.0f, 1.0f, startAmount + delta);
        knob.ctx.setRouteAmount (knob.ctx.selectedSource, knob.dest, knob.modDragAmount, false);
        knob.repaint();
    }
    void mouseUp (const juce::MouseEvent&) override
    {
        if (knob.editingMod)
            knob.ctx.setRouteAmount (knob.ctx.selectedSource, knob.dest, knob.modDragAmount, true);
        knob.editingMod = false;
        knob.repaint();
    }
    void mouseDoubleClick (const juce::MouseEvent&) override
    {
        knob.ctx.removeRoute (knob.ctx.selectedSource, knob.dest);
    }

private:
    Knob& knob;
    float startAmount = 0.0f;
};

Knob::Knob (UiContext& c, const juce::String& id, const juce::String& l) : ctx (c), paramId (id), label (l)
{
    pdef = ctx.def (id);
    dest = cs::findModDest (id.toStdString());
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    slider.setColour (juce::Slider::rotarySliderFillColourId, col::accent);
    slider.setMouseDragSensitivity (180);
    slider.setVelocityBasedMode (false);
    slider.addMouseListener (this, false);
    slider.setPopupMenuEnabled (false);
    addAndMakeVisible (slider);
    att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (ctx.proc.apvts, id, slider);
    if (pdef != nullptr)
        slider.setDoubleClickReturnValue (true, pdef->def);
    slider.onValueChange = [this] { repaint(); };
    slider.onDragStart = [this] { repaint(); };
    slider.onDragEnd = [this] { repaint(); };
    if (label.isEmpty() && pdef != nullptr)
        label = pdef->name;
    if (pdef != nullptr)
        setTooltip (pdef->name + (dest >= 0 ? "  (drag a mod source here, right-click for MIDI learn)" : "  (right-click for MIDI learn)"));

    overlay = std::make_unique<ModOverlay> (*this);
    addChildComponent (*overlay);
    ctx.addChangeListener (this);
    changeListenerCallback (nullptr);
}

Knob::~Knob()
{
    ctx.removeChangeListener (this);
}

void Knob::changeListenerCallback (juce::ChangeBroadcaster*)
{
    const bool modMode = ctx.selectedSource >= 0 && dest >= 0;
    overlay->setVisible (modMode);
    overlay->setMouseCursor (modMode ? juce::MouseCursor::UpDownResizeCursor : juce::MouseCursor::NormalCursor);
    repaint();
}

juce::Rectangle<float> Knob::dialBounds() const
{
    auto r = getLocalBounds().toFloat();
    const float labelH = label.isEmpty() ? 0.0f : 15.0f;
    const float d = juce::jmin (r.getWidth(), r.getHeight() - labelH);
    return juce::Rectangle<float> (r.getCentreX() - d * 0.5f, r.getY(), d, d);
}

void Knob::resized()
{
    const auto d = dialBounds().toNearestInt();
    slider.setBounds (d);
    overlay->setBounds (d);
}

juce::String Knob::valueText() const
{
    const float v = (float) slider.getValue();
    if (formatter)
        return formatter (v);
    return pdef != nullptr ? formatValue (*pdef, v) : juce::String (v, 2);
}

void Knob::paint (juce::Graphics& g)
{
    if (label.isEmpty())
        return;
    auto r = getLocalBounds();
    auto text = r.removeFromBottom (15);
    juce::String t = label;
    auto c = col::textDim;
    if (editingMod)
    {
        t = (modDragAmount >= 0 ? "+" : "") + juce::String (juce::roundToInt (modDragAmount * 100.0f)) + " %";
        c = sourceColour (ctx.selectedSource);
    }
    else if (slider.isMouseButtonDown() || hover)
    {
        t = valueText();
        c = col::text;
    }
    g.setColour (c);
    g.setFont (font (11.0f));
    g.drawFittedText (t, text.expanded (6, 0), juce::Justification::centred, 1, 0.8f);
}

void Knob::paintOverChildren (juce::Graphics& g)
{
    if (dest < 0)
        return;
    const auto d = dialBounds();
    const auto c = d.getCentre();
    const float radius = d.getWidth() * 0.5f;
    const float start = juce::MathConstants<float>::pi * 1.25f, end = juce::MathConstants<float>::pi * 2.75f;
    const float pos = (float) slider.valueToProportionOfLength (slider.getValue());

    if (dropHover)
    {
        g.setColour (col::gold.withAlpha (0.6f));
        g.drawEllipse (d.reduced (1.0f), 2.0f);
    }
    const auto mods = ctx.routesTo (dest);
    float ringR = radius + 1.0f;
    for (const auto& m : mods)
    {
        const bool sel = m.first == ctx.selectedSource;
        const float a0 = start + pos * (end - start);
        const float a1 = start + juce::jlimit (0.0f, 1.0f, pos + m.second) * (end - start);
        juce::Path arc;
        arc.addCentredArc (c.x, c.y, ringR, ringR, 0.0f, juce::jmin (a0, a1), juce::jmax (a0, a1), true);
        g.setColour (sourceColour (m.first).withAlpha (ctx.selectedSource < 0 || sel ? 1.0f : 0.35f));
        g.strokePath (arc, juce::PathStrokeType (sel ? 2.6f : 1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        ringR += 0.0f;
    }
    // Mod-edit mode: a dot shows this knob can take the selected source.
    if (ctx.selectedSource >= 0 && ! ctx.hasRoute (ctx.selectedSource, dest))
    {
        g.setColour (sourceColour (ctx.selectedSource).withAlpha (0.8f));
        g.fillEllipse (d.getRight() - 6.0f, d.getY(), 5.0f, 5.0f);
    }
}

bool Knob::isInterestedInDragSource (const SourceDetails& s)
{
    return dest >= 0 && s.description.toString().startsWith ("modsrc:");
}

void Knob::itemDropped (const SourceDetails& s)
{
    dropHover = false;
    const int src = s.description.toString().fromFirstOccurrenceOf ("modsrc:", false, false).getIntValue();
    if (! ctx.hasRoute (src, dest))
        ctx.setRouteAmount (src, dest, 0.25f);
    if (ctx.selectedSource != src)
        ctx.selectSource (src);
    repaint();
}

void Knob::showContextMenu (UiContext& ctx, const juce::String& paramId, juce::Component* target)
{
    juce::PopupMenu m;
    const int cc = ctx.proc.ccForParam (paramId);
    const bool learning = ctx.proc.learningParam() == paramId;
    m.addSectionHeader (ctx.def (paramId) != nullptr ? ctx.def (paramId)->name : paramId);
    m.addItem (1, learning ? "Cancel MIDI learn" : "MIDI learn (move a controller)");
    if (cc >= 0)
        m.addItem (2, "Forget CC " + juce::String (cc));
    m.addItem (3, "Reset to default");
    const int dest = cs::findModDest (paramId.toStdString());
    const auto mods = ctx.routesTo (dest);
    if (! mods.empty())
    {
        m.addSeparator();
        m.addSectionHeader ("Modulation");
        for (size_t i = 0; i < mods.size(); ++i)
            m.addItem (100 + (int) i, juce::String ("Remove ") + cs::modSourceName (mods[i].first) + "  ("
                                          + juce::String (juce::roundToInt (mods[i].second * 100.0f)) + " %)");
    }
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (target), [&ctx, paramId, mods, dest, learning] (int r) {
        if (r == 1)
            learning ? ctx.proc.cancelMidiLearn() : ctx.proc.startMidiLearn (paramId);
        else if (r == 2)
            ctx.proc.clearMidiMapping (paramId);
        else if (r == 3)
        {
            if (auto* d = ctx.def (paramId))
                ctx.setValue (paramId, d->def);
        }
        else if (r >= 100 && r - 100 < (int) mods.size())
            ctx.removeRoute (mods[(size_t) (r - 100)].first, dest);
    });
}

// ===========================================================================
// Pill
// ===========================================================================
Pill::Pill (UiContext& c, const juce::String& id) : ctx (c), paramId (id)
{
    pdef = ctx.def (id);
    if (auto* p = ctx.param (id))
    {
        att = std::make_unique<juce::ParameterAttachment> (*p, [this] (float v) { index = (int) std::lround (v); repaint(); });
        att->sendInitialUpdate();
    }
    if (pdef != nullptr)
        setTooltip (pdef->name);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void Pill::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (col::panelLo);
    g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
    g.setColour (col::border);
    g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.0f);
    juce::String t;
    if (displayName)
        t = displayName (index);
    else if (pdef != nullptr && pdef->choices.size() > 0)
    {
        t = pdef->choices[juce::jlimit (0, pdef->choices.size() - 1, index)];
        if (t.contains (": "))
            t = t.fromFirstOccurrenceOf (": ", false, false);
    }
    auto tr = getLocalBounds().reduced (6, 0);
    if (arrows)
    {
        g.setColour (col::textDim);
        g.setFont (font (12.0f, true));
        g.drawText ("<", tr.removeFromLeft (10), juce::Justification::centred);
        g.drawText (">", tr.removeFromRight (10), juce::Justification::centred);
    }
    g.setColour (isEnabled() ? textColour : col::textFaint);
    g.setFont (font (juce::jmin (12.0f, (float) getHeight() * 0.62f)));
    g.drawFittedText (t.toLowerCase() == t ? t : t, tr, juce::Justification::centred, 1, 0.75f);
}

void Pill::step (int delta)
{
    if (pdef == nullptr || att == nullptr)
        return;
    const int n = pdef->choices.size();
    att->setValueAsCompleteGesture ((float) ((index + delta + n) % n));
}

void Pill::mouseDown (const juce::MouseEvent& e)
{
    if (pdef == nullptr || att == nullptr)
        return;
    if (arrows && e.x < 16)
    {
        step (-1);
        return;
    }
    if (arrows && e.x > getWidth() - 16)
    {
        step (1);
        return;
    }
    juce::PopupMenu menu;
    std::map<juce::String, juce::PopupMenu> subs;
    juce::StringArray order;
    for (int i = 0; i < pdef->choices.size(); ++i)
    {
        const auto& c = pdef->choices[i];
        const juce::String shown = displayName ? displayName (i) : c;
        if (c.contains (": "))
        {
            const auto cat = c.upToFirstOccurrenceOf (": ", false, false);
            if (! order.contains (cat))
                order.add (cat);
            subs[cat].addItem (i + 1, c.fromFirstOccurrenceOf (": ", false, false), true, i == index);
        }
        else
            menu.addItem (i + 1, shown, true, i == index);
    }
    for (const auto& cat : order)
        menu.addSubMenu (cat, subs[cat], true, nullptr, pdef->choices[index].startsWith (cat + ": "));
    if (extraMenu)
    {
        menu.addSeparator();
        extraMenu (menu);
    }
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMinimumWidth (getWidth()),
                        [safe = juce::Component::SafePointer<Pill> (this)] (int r) {
                            if (safe == nullptr || r == 0)
                                return;
                            if (r >= 10000)
                            {
                                if (safe->onExtra)
                                    safe->onExtra (r);
                                return;
                            }
                            safe->att->setValueAsCompleteGesture ((float) (r - 1));
                        });
}

void Pill::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w)
{
    if (std::abs (w.deltaY) > 0.0f)
        step (w.deltaY < 0 ? 1 : -1);
}

// ===========================================================================
// Toggle
// ===========================================================================
Toggle::Toggle (UiContext& c, const juce::String& id, Style s, const juce::String& t) : ctx (c), paramId (id), text (t), style (s)
{
    if (auto* p = ctx.param (id))
    {
        att = std::make_unique<juce::ParameterAttachment> (*p, [this] (float v) {
            on = v >= 0.5f;
            repaint();
            if (onChange)
                onChange();
        });
        att->sendInitialUpdate();
    }
    if (auto* d = ctx.def (id))
        setTooltip (d->name);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void Toggle::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    switch (style)
    {
        case Power:
            drawPowerIcon (g, r.reduced (1.0f), on ? onColour : col::textFaint);
            break;
        case TextPill:
        {
            auto b = r.reduced (0.5f);
            g.setColour (on ? onColour.withAlpha (0.18f) : col::panelLo);
            g.fillRoundedRectangle (b, b.getHeight() * 0.5f);
            g.setColour (on ? onColour.withAlpha (0.7f) : col::border);
            g.drawRoundedRectangle (b, b.getHeight() * 0.5f, 1.0f);
            g.setColour (on ? onColour : col::textDim);
            g.setFont (font (juce::jmin (12.0f, r.getHeight() * 0.6f)));
            g.drawFittedText (text, getLocalBounds().reduced (4, 0), juce::Justification::centred, 1, 0.7f);
            break;
        }
        case Check:
        {
            const float box = juce::jmin (13.0f, r.getHeight() - 2.0f);
            auto tick = r.removeFromLeft (box + 5.0f).withSizeKeepingCentre (box, box);
            g.setColour (on ? onColour : col::panelLo);
            g.fillRoundedRectangle (tick, 3.0f);
            g.setColour (col::border);
            g.drawRoundedRectangle (tick, 3.0f, 1.0f);
            g.setColour (on ? col::text : col::textDim);
            g.setFont (font (11.5f));
            g.drawFittedText (text, r.toNearestInt(), juce::Justification::centredLeft, 1, 0.8f);
            break;
        }
    }
}

void Toggle::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        Knob::showContextMenu (ctx, paramId, this);
        return;
    }
    if (att != nullptr)
        att->setValueAsCompleteGesture (on ? 0.0f : 1.0f);
}

// ===========================================================================
// Stepper
// ===========================================================================
Stepper::Stepper (UiContext& c, const juce::String& id) : ctx (c), paramId (id)
{
    pdef = ctx.def (id);
    if (auto* p = ctx.param (id))
    {
        att = std::make_unique<juce::ParameterAttachment> (*p, [this] (float v) { value = (int) std::lround (v); repaint(); });
        att->sendInitialUpdate();
    }
    if (pdef != nullptr)
        setTooltip (pdef->name + "  (click the arrows or drag)");
    setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
}

void Stepper::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (col::panelLo);
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (col::border);
    g.drawRoundedRectangle (r, 4.0f, 1.0f);
    auto t = getLocalBounds();
    g.setColour (col::textDim);
    g.setFont (font (11.0f, true));
    g.drawText ("<", t.removeFromLeft (14), juce::Justification::centred);
    g.drawText (">", t.removeFromRight (14), juce::Justification::centred);
    g.setColour (col::accent);
    g.setFont (font (12.0f));
    g.drawFittedText (displayName ? displayName (value) : juce::String (value), t, juce::Justification::centred, 1, 0.8f);
}

void Stepper::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        Knob::showContextMenu (ctx, paramId, this);
        return;
    }
    dragStart = value;
    dragging = false;
}

void Stepper::mouseDrag (const juce::MouseEvent& e)
{
    if (pdef == nullptr || att == nullptr || std::abs (e.getDistanceFromDragStartY()) < 4)
        return;
    if (! dragging)
    {
        att->beginGesture();
        dragging = true;
    }
    const int v = juce::jlimit ((int) pdef->min, (int) pdef->max, dragStart - e.getDistanceFromDragStartY() / 8);
    att->setValueAsPartOfGesture ((float) v);
}

void Stepper::mouseUp (const juce::MouseEvent& e)
{
    if (pdef == nullptr || att == nullptr)
        return;
    if (dragging)
    {
        att->endGesture();
        dragging = false;
        return;
    }
    if (e.mods.isPopupMenu())
        return;
    const int delta = e.x < getWidth() / 2 ? -1 : 1;
    att->setValueAsCompleteGesture ((float) juce::jlimit ((int) pdef->min, (int) pdef->max, value + delta));
}

// ===========================================================================
// FlatButton
// ===========================================================================
void FlatButton::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    if (outline)
    {
        g.setColour (lit ? litColour.withAlpha (0.18f) : (hover ? col::panelHi.brighter (0.05f) : col::panelHi));
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (lit ? litColour.withAlpha (0.7f) : col::border);
        g.drawRoundedRectangle (r, 4.0f, 1.0f);
    }
    const auto c = ! isEnabled() ? col::textFaint : (lit ? litColour : (hover ? juce::Colours::white : col::text));
    if (icon)
    {
        auto ir = text.isEmpty() ? r : r.removeFromLeft (r.getHeight());
        icon (g, ir.reduced (r.getHeight() * 0.22f), c);
    }
    if (text.isNotEmpty())
    {
        g.setColour (c);
        g.setFont (font (fontSize, true));
        g.drawFittedText (text, r.toNearestInt().reduced (3, 0), juce::Justification::centred, 1, 0.7f);
    }
}
} // namespace ui
