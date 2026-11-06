#include "FxPage.h"

#include "../PresetManager.h"
#include "dsp/fx/Effects.h"

namespace ui
{
namespace
{
const char* const kSlotIds[9] = { "a1", "a2", "a3", "b1", "b2", "b3", "x1", "x2", "x3" };
const char* const kSlotNames[9] = { "A1", "A2", "A3", "B1", "B2", "B3", "Aux 1", "Aux 2", "Aux 3" };
juce::String slotParam (int slot, const juce::String& s) { return juce::String ("fx_") + kSlotIds[slot] + "_" + s; }

// Effect controls that are really menus or switches (empty = continuous knob).
juce::StringArray fxChoices (int type, int k)
{
    using namespace cs::fx;
    const juce::String name = fxInfo (type).params[(size_t) k] != nullptr ? fxInfo (type).params[(size_t) k] : "";
    if (name == "Ping-Pong" || name == "Auto Makeup" || name == "Auto Gain" || name == "Dark" || name == "Invert"
        || name == "Mono Bass" || name == "Natural Law" || name == "Wide" || name == "Mono Input" || name == "Brake")
        return { "Off", "On" };
    switch (type)
    {
        case FxDistortion:
            if (k == 0) return { "Soft Clip", "Hard Clip", "Tube", "Tape", "Overdrive", "Fuzz", "Crunch", "Diode", "Sine Fold",
                                 "Triangle Fold", "Half Rectify", "Full Rectify", "Asymmetric", "Bit Shape", "Saturate", "Destroy" };
            if (k == 4) return { "Filter Off", "Pre", "Post" };
            if (k == 5) return { "Lowpass", "Highpass", "Bandpass" };
            if (k == 8) return { "12 dB", "24 dB", "36 dB" };
            break;
        case FxMultiFilter:
            if (k == 0) return { "LP 12", "LP 24", "LP 36", "HP 12", "HP 24", "HP 36", "BP 12", "BP 24", "BP 36", "Notch", "Comb FF", "Comb FB" };
            break;
        case FxErode:
            if (k == 0) return { "Sine", "Triangle", "Saw", "Square", "Noise" };
            break;
        case FxVintageChorus:
            if (k == 0) return { "I", "II", "I + II" };
            break;
        case FxPhaser:
            if (k == 2) return { "Sine", "Triangle", "Square", "Saw", "Ramp", "S&H" };
            if (k == 5) return { "2 poles", "4 poles", "6 poles", "8 poles", "10 poles", "12 poles" };
            break;
        case FxChorus:
            if (k == 4) return { "1 voice", "2 voices", "3 voices", "4 voices" };
            if (k == 5) return { "Sine", "Square" };
            break;
        case FxFlanger:
            if (k == 7) return { "Sine", "Triangle" };
            break;
        case FxPanner:
            if (k == 6) return { "Sine", "Triangle", "Square" };
            break;
        case FxSuperUnison:
            if (k == 0) return { "2 voices", "3 voices", "4 voices", "5 voices", "6 voices", "7 voices", "8 voices" };
            break;
        case FxPitchDelay:
            if (k == 3) return { "Normal", "+ Oct Up", "+ Oct Down" };
            break;
        case FxRotary:
            if (k == 0) return { "Slow", "Fast" };
            break;
        default: break;
    }
    return {};
}

int choiceIndex (float v, int n) { return juce::jlimit (0, n - 1, (int) (v * (float) n)); }

// Readable value text for a knob.
juce::String fxValueText (int type, int k, float v, bool sync)
{
    using namespace cs::fx;
    if (sync && k == fxSyncParam (type))
        return cs::kSyncDivs[(size_t) choiceIndex (v, cs::kNumSyncDivs)].name;
    const auto c = fxChoices (type, k);
    if (! c.isEmpty())
        return c.size() == 2 ? (v >= 0.5f ? c[1] : c[0]) : c[choiceIndex (v, c.size())];
    if (type == FxPitchDelay && k == 1)
        return juce::String ((int) std::round (-12.0f + v * 24.0f)) + " st";
    return juce::String (juce::roundToInt (v * 100.0f)) + " %";
}

// A discrete effect control as a menu (or an on/off pill for switches).
class FxMenu final : public juce::Component, public juce::SettableTooltipClient
{
public:
    FxMenu (UiContext& c, const juce::String& paramId) : ctx (c), id (paramId)
    {
        getProperties().set ("paramId", paramId); // for the help bar
        if (auto* p = ctx.param (id))
        {
            att = std::make_unique<juce::ParameterAttachment> (*p, [this] (float v) { value = v; repaint(); });
            att->sendInitialUpdate();
        }
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }
    void setChoices (const juce::String& label, const juce::StringArray& c)
    {
        if (caption == label && choices == c)
            return;
        caption = label;
        choices = c;
        repaint();
    }
    void paint (juce::Graphics& g) override
    {
        drawCaption (g, getLocalBounds().removeFromTop (16), caption);
        auto r = getLocalBounds().withTrimmedTop (22).withHeight (22).reduced (2, 0).toFloat();
        const bool isSwitch = choices.size() == 2;
        const bool on = isSwitch && value >= 0.5f;
        g.setColour (on ? col::accent.withAlpha (0.18f) : col::panelLo);
        g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
        g.setColour (on ? col::accent.withAlpha (0.7f) : col::border);
        g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.0f);
        g.setColour (isSwitch && ! on ? col::textDim : col::accent);
        g.setFont (font (12.0f));
        const juce::String t = isSwitch ? (on ? choices[1] : choices[0]) : choices[choiceIndex (value, choices.size())];
        g.drawFittedText (t, r.toNearestInt().reduced (6, 0), juce::Justification::centred, 1, 0.7f);
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (att == nullptr || choices.isEmpty())
            return;
        if (e.mods.isPopupMenu())
        {
            Knob::showContextMenu (ctx, id, this);
            return;
        }
        if (choices.size() == 2)
        {
            att->setValueAsCompleteGesture (value >= 0.5f ? 0.0f : 1.0f);
            return;
        }
        juce::PopupMenu m;
        const int cur = choiceIndex (value, choices.size());
        for (int i = 0; i < choices.size(); ++i)
            m.addItem (i + 1, choices[i], true, i == cur);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [safe = juce::Component::SafePointer<FxMenu> (this)] (int r) {
            if (safe != nullptr && r > 0)
                safe->att->setValueAsCompleteGesture (((float) (r - 1) + 0.5f) / (float) safe->choices.size());
        });
    }

private:
    UiContext& ctx;
    juce::String id, caption;
    juce::StringArray choices;
    std::unique_ptr<juce::ParameterAttachment> att;
    float value = 0.0f;
};

// Param EQ: response curve with draggable band points.
class EqGraph final : public juce::Component, public juce::SettableTooltipClient
{
public:
    EqGraph (UiContext& c, int s) : ctx (c), slot (s)
    {
        setTooltip ("Drag a point to set its frequency and gain; mouse wheel on a point sets the Q");
    }
    void refreshView()
    {
        juce::String key;
        for (int k = 0; k < 12; ++k)
            key << juce::String (value (k), 4) << ",";
        if (key == lastKey)
            return;
        lastKey = key;
        auto e = cs::fx::makeEffect (cs::fx::FxParamEq, 48000.0f);
        float p[12];
        for (int k = 0; k < 12; ++k)
            p[k] = value (k);
        constexpr int n = 4096;
        std::vector<float> l (n, 0.0f), r (n, 0.0f);
        l[0] = r[0] = 1.0f;
        cs::fx::FxContext fc;
        for (int i = 0; i < n; i += 256)
            e->process (l.data() + i, r.data() + i, 256, p, false, fc);
        mags.assign (kBins, 0.0f);
        for (int b = 0; b < kBins; ++b)
        {
            const double w = juce::MathConstants<double>::twoPi * freqAt ((float) b / (kBins - 1)) / 48000.0;
            // DFT of the impulse response at one frequency (rotating phasor, no trig per sample).
            const double cr = std::cos (w), ci = -std::sin (w);
            double re = 0.0, im = 0.0, pr = 1.0, pi = 0.0;
            for (int i = 0; i < n; ++i)
            {
                re += l[(size_t) i] * pr;
                im += l[(size_t) i] * pi;
                const double t = pr * cr - pi * ci;
                pi = pr * ci + pi * cr;
                pr = t;
            }
            mags[(size_t) b] = (float) (20.0 * std::log10 (std::sqrt (re * re + im * im) + 1.0e-9));
        }
        repaint();
    }
    void paint (juce::Graphics& g) override
    {
        const auto r = plot();
        g.setColour (col::panelLo);
        g.fillRoundedRectangle (getLocalBounds().toFloat(), 4.0f);
        g.setColour (col::border.withAlpha (0.5f));
        for (float f : { 100.0f, 1000.0f, 10000.0f })
            g.drawVerticalLine ((int) xOf (f), r.getY(), r.getBottom());
        for (float db : { -12.0f, 0.0f, 12.0f })
            g.drawHorizontalLine ((int) yOf (db), r.getX(), r.getRight());
        if (! mags.empty())
        {
            juce::Path path;
            for (int b = 0; b < kBins; ++b)
            {
                const float x = r.getX() + r.getWidth() * (float) b / (kBins - 1);
                const float y = yOf (juce::jlimit (-kRange, kRange, mags[(size_t) b]));
                b == 0 ? path.startNewSubPath (x, y) : path.lineTo (x, y);
            }
            g.setColour (col::accent);
            g.strokePath (path, juce::PathStrokeType (1.8f));
        }
        static const char* const kNames[5] = { "L", "1", "2", "3", "H" };
        for (int band = 0; band < 5; ++band)
        {
            const auto pt = nodePos (band);
            g.setColour (band == dragBand ? col::gold : col::text);
            g.fillEllipse (pt.x - 7.0f, pt.y - 7.0f, 14.0f, 14.0f);
            g.setColour (col::bg);
            g.setFont (font (10.0f, true));
            g.drawText (kNames[band], juce::Rectangle<float> (pt.x - 7.0f, pt.y - 7.0f, 14.0f, 14.0f), juce::Justification::centred);
        }
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        dragBand = hit (e.position);
        if (dragBand >= 0)
            for (int k : { freqParam (dragBand), freqParam (dragBand) + 1 })
                if (auto* p = param (k))
                    p->beginChangeGesture();
        repaint();
    }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragBand < 0)
            return;
        const auto r = plot();
        const float t = juce::jlimit (0.0f, 1.0f, (e.position.x - r.getX()) / r.getWidth());
        const float f = freqAt (t);
        const float lo = bandLo (dragBand), hi = bandHi (dragBand);
        setValue (freqParam (dragBand), std::log (juce::jlimit (lo, hi, f) / lo) / std::log (hi / lo));
        const float db = juce::jlimit (-kRange, kRange, kRange - 2.0f * kRange * (e.position.y - r.getY()) / r.getHeight());
        const float scale = juce::jmax (0.05f, value (11) * 2.0f);
        setValue (freqParam (dragBand) + 1, juce::jlimit (0.0f, 1.0f, (db / scale + 15.0f) / 30.0f));
    }
    void mouseUp (const juce::MouseEvent&) override
    {
        if (dragBand >= 0)
            for (int k : { freqParam (dragBand), freqParam (dragBand) + 1 })
                if (auto* p = param (k))
                    p->endChangeGesture();
        dragBand = -1;
        repaint();
    }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override
    {
        if (auto* p = param (10))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, p->getValue() + w.deltaY * 0.15f));
            p->endChangeGesture();
        }
    }

private:
    static constexpr int kBins = 120;
    static constexpr float kRange = 18.0f;
    UiContext& ctx;
    int slot, dragBand = -1;
    std::vector<float> mags;
    juce::String lastKey;

    juce::RangedAudioParameter* param (int k) const { return ctx.param (slotParam (slot, "p" + juce::String (k + 1))); }
    float value (int k) const { return ctx.value (slotParam (slot, "p" + juce::String (k + 1))); }
    void setValue (int k, float v)
    {
        if (auto* p = param (k))
            p->setValueNotifyingHost (v);
    }
    juce::Rectangle<float> plot() const { return getLocalBounds().toFloat().reduced (8.0f, 8.0f); }
    static float freqAt (float t) { return 20.0f * std::pow (1000.0f, t); }
    float xOf (float f) const { const auto r = plot(); return r.getX() + r.getWidth() * std::log (f / 20.0f) / std::log (1000.0f); }
    float yOf (float db) const { const auto r = plot(); return r.getY() + r.getHeight() * (kRange - db) / (2.0f * kRange); }
    static int freqParam (int band) { return band * 2; }
    static float bandLo (int band) { return band == 0 ? 30.0f : band == 4 ? 2000.0f : 50.0f; }
    static float bandHi (int band) { return band == 0 ? 500.0f : 16000.0f; }
    juce::Point<float> nodePos (int band) const
    {
        const float f = bandLo (band) * std::pow (bandHi (band) / bandLo (band), value (freqParam (band)));
        const float db = (value (freqParam (band) + 1) * 30.0f - 15.0f) * value (11) * 2.0f;
        return { xOf (f), yOf (juce::jlimit (-kRange, kRange, db)) };
    }
    int hit (juce::Point<float> p) const
    {
        int best = -1;
        float bestD = 14.0f;
        for (int band = 0; band < 5; ++band)
            if (const float d = nodePos (band).getDistanceFrom (p); d < bestD)
            {
                bestD = d;
                best = band;
            }
        return best;
    }
};
} // namespace

// ===========================================================================
// One effect slot editor: type menu, presets, sync and its controls.
class FxPage::SlotEditor final : public juce::Component, public Refreshable
{
public:
    SlotEditor (UiContext& c) : ctx (c) {}

    void setSlot (int s)
    {
        if (s == slot)
            return;
        slot = s;
        power = std::make_unique<Toggle> (ctx, slotParam (s, "on"), Toggle::Power);
        type = std::make_unique<Pill> (ctx, slotParam (s, "type"));
        type->textColour = col::text;
        type->displayName = [] (int i) { return juce::String (cs::fx::fxInfo (i).name).toUpperCase(); };
        sync = std::make_unique<Toggle> (ctx, slotParam (s, "sync"), Toggle::TextPill, "sync");
        sync->onChange = [this] { for (auto& k : knobs) k->repaint(); };
        presets.text = "presets";
        presets.fontSize = 11.0f;
        presets.onClick = [this] { showPresets(); };
        knobs.clear();
        menus.clear();
        eq = std::make_unique<EqGraph> (ctx, s);
        addChildComponent (*eq);
        for (int k = 0; k < cs::fx::kFxParams; ++k)
        {
            auto mn = std::make_unique<FxMenu> (ctx, slotParam (s, "p" + juce::String (k + 1)));
            addChildComponent (*mn);
            menus.push_back (std::move (mn));
            auto kn = std::make_unique<Knob> (ctx, slotParam (s, "p" + juce::String (k + 1)), "");
            kn->setFormatter ([this, k] (float v) {
                return fxValueText (ctx.intValue (slotParam (slot, "type")), k, v, ctx.boolValue (slotParam (slot, "sync")));
            });
            addChildComponent (*kn);
            knobs.push_back (std::move (kn));
        }
        for (auto* comp : std::initializer_list<juce::Component*> { power.get(), type.get(), sync.get(), &presets })
            addAndMakeVisible (comp);
        shownType = -1;
        refreshView();
        resized();
    }

    void refreshView() override
    {
        if (slot < 0)
            return;
        const int t = ctx.intValue (slotParam (slot, "type"));
        const auto& info = cs::fx::fxInfo (t);
        sync->setVisible (info.usesSync);
        presets.setVisible (t != cs::fx::FxNone);
        power->setVisible (true);
        for (int k = 0; k < cs::fx::kFxParams; ++k)
        {
            const bool used = t != cs::fx::FxNone && info.params[(size_t) k] != nullptr;
            const auto choices = used ? fxChoices (t, k) : juce::StringArray();
            const bool asMenu = ! choices.isEmpty();
            knobs[(size_t) k]->setVisible (used && ! asMenu);
            menus[(size_t) k]->setVisible (used && asMenu);
            if (used)
            {
                static const char* const kEqShort[12] = { "LS Freq", "LS Gain", "P1 Freq", "P1 Gain", "P2 Freq", "P2 Gain",
                                                          "P3 Freq", "P3 Gain", "HS Freq", "HS Gain", "Peak Q", "Scale" };
                knobs[(size_t) k]->setLabel (t == cs::fx::FxParamEq ? kEqShort[k] : info.params[(size_t) k]);
                if (asMenu)
                    menus[(size_t) k]->setChoices (info.params[(size_t) k], choices);
            }
        }
        eq->setVisible (t == cs::fx::FxParamEq);
        if (t == cs::fx::FxParamEq)
            eq->refreshView();
        if (t != shownType)
        {
            shownType = t;
            resized();
            repaint();
        }
    }

    void resized() override
    {
        if (slot < 0)
            return;
        power->setBounds (8, 7, 15, 15);
        type->setBounds (28, 4, 160, 22);
        presets.setBounds (194, 5, 66, 20);
        sync->setBounds (266, 6, 50, 18);
        const bool isEq = eq->isVisible();
        if (isEq)
            eq->setBounds (6, 36, getWidth() - 12, 124);
        int i = 0;
        for (int k = 0; k < cs::fx::kFxParams; ++k)
        {
            juce::Component* c = knobs[(size_t) k]->isVisible() ? (juce::Component*) knobs[(size_t) k].get()
                                                                : (menus[(size_t) k]->isVisible() ? (juce::Component*) menus[(size_t) k].get() : nullptr);
            if (c == nullptr)
                continue;
            if (isEq)
                c->setBounds (4 + (i % 6) * 54, 166 + (i / 6) * 84, 54, 80);
            else
            {
                const int col = i % 4, row = i / 4;
                c->setBounds (6 + col * 80, 38 + row * 96, 76, c == menus[(size_t) k].get() ? 50 : 88);
            }
            ++i;
        }
    }

    void paint (juce::Graphics& g) override
    {
        drawPanel (g, getLocalBounds().toFloat(), 6.0f, col::panel);
        g.setColour (col::border);
        g.drawHorizontalLine (30, 6.0f, (float) getWidth() - 6.0f);
        if (shownType == cs::fx::FxNone)
        {
            g.setColour (col::textFaint);
            g.setFont (font (12.0f));
            g.drawText ("Click the menu above to add an effect", getLocalBounds().withTrimmedTop (30), juce::Justification::centred);
        }
        g.setColour (col::textFaint);
        g.setFont (font (10.5f));
        g.drawText (kSlotNames[juce::jmax (0, slot)], getLocalBounds().reduced (8, 6), juce::Justification::bottomRight);
    }

private:
    UiContext& ctx;
    int slot = -1, shownType = -1;
    std::unique_ptr<Toggle> power, sync;
    std::unique_ptr<Pill> type;
    FlatButton presets;
    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<std::unique_ptr<FxMenu>> menus;
    std::unique_ptr<EqGraph> eq;

    void showPresets()
    {
        auto& pm = ctx.proc.presets();
        const int t = ctx.intValue (slotParam (slot, "type"));
        const auto list = pm.fxPresets (t);
        juce::PopupMenu m, del;
        m.addSectionHeader (juce::String (cs::fx::fxInfo (t).name) + " presets");
        for (size_t i = 0; i < list.size(); ++i)
        {
            m.addItem (1 + (int) i, list[i].name + (list[i].factory ? "" : "  (user)"));
            if (! list[i].factory)
                del.addItem (1000 + (int) i, list[i].name);
        }
        m.addSeparator();
        m.addItem (900, "Save current settings...");
        if (del.getNumItems() > 0)
            m.addSubMenu ("Delete user preset", del);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&presets), [this, list] (int r) {
            auto& pmgr = ctx.proc.presets();
            if (r > 0 && r <= (int) list.size())
                pmgr.applyFxPreset (slot, list[(size_t) r - 1]);
            else if (r >= 1000 && r - 1000 < (int) list.size())
                pmgr.deleteFxPreset (list[(size_t) r - 1000]);
            else if (r == 900)
            {
                auto* w = new juce::AlertWindow ("Save FX preset", "Name for this " + juce::String (cs::fx::fxInfo (ctx.intValue (slotParam (slot, "type"))).name) + " preset:",
                                                 juce::MessageBoxIconType::NoIcon);
                w->addTextEditor ("name", "My preset");
                w->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
                w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
                w->enterModalState (true, juce::ModalCallbackFunction::create ([this, w] (int result) {
                    if (result == 1)
                    {
                        juce::String err;
                        if (! ctx.proc.presets().saveFxPreset (slot, w->getTextEditorContents ("name"), err))
                            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Church Synth", err);
                    }
                }), true);
            }
        });
    }
};

// Slot name in a bus box: click to show the bus, drag onto another slot to swap,
// right-click to copy.
class FxPage::SlotButton final : public juce::Component, public juce::DragAndDropTarget, public juce::SettableTooltipClient
{
public:
    SlotButton (FxPage& o, int s) : owner (o), slot (s)
    {
        setTooltip ("Click to edit. Drag onto another slot to swap; right-click to copy.");
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }
    void paint (juce::Graphics& g) override
    {
        const int t = owner.ctx.intValue (slotParam (slot, "type"));
        const bool on = owner.ctx.boolValue (slotParam (slot, "on"));
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (hover ? col::panelHi.brighter (0.05f) : col::panelLo);
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (owner.bus == slot / 3 ? col::gold.withAlpha (0.5f) : col::border);
        g.drawRoundedRectangle (r, 4.0f, 1.0f);
        g.setColour (t == cs::fx::FxNone ? col::textFaint : (on ? col::accent : col::textDim));
        g.setFont (font (11.5f));
        g.drawFittedText (t == cs::fx::FxNone ? juce::String ("-") : juce::String (cs::fx::fxInfo (t).name).toLowerCase(),
                          getLocalBounds().reduced (6, 0), juce::Justification::centred, 1, 0.7f);
    }
    void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (e.getDistanceFromDragStart() > 6)
            if (auto* dnd = juce::DragAndDropContainer::findParentDragContainerFor (this))
                if (! dnd->isDragAndDropActive())
                {
                    dnd->startDragging ("fxslot:" + juce::String (slot), this);
                    dragged = true;
                }
    }
    void mouseUp (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu())
        {
            juce::PopupMenu m;
            m.addSectionHeader ("Copy " + juce::String (kSlotNames[slot]) + " to");
            for (int s = 0; s < 9; ++s)
                if (s != slot)
                    m.addItem (s + 1, kSlotNames[s]);
            m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [this] (int r) {
                if (r > 0)
                {
                    owner.ctx.proc.copyFxSlot (slot, r - 1);
                    owner.ctx.proc.checkpoint ("Copy FX slot");
                }
            });
        }
        else if (! dragged)
            owner.selectBus (slot / 3);
        dragged = false;
    }
    bool isInterestedInDragSource (const SourceDetails& d) override { return d.description.toString().startsWith ("fxslot:"); }
    void itemDropped (const SourceDetails& d) override
    {
        const int from = d.description.toString().fromFirstOccurrenceOf ("fxslot:", false, false).getIntValue();
        owner.ctx.proc.swapFxSlots (from, slot);
        owner.ctx.proc.checkpoint ("Swap FX slots");
        owner.refreshView();
    }

private:
    FxPage& owner;
    int slot;
    bool hover = false, dragged = false;
};

// ===========================================================================
namespace
{
class BusColumn final : public ControlPanel
{
public:
    explicit BusColumn (UiContext& c) : ControlPanel (c)
    {
        title ({ 0, 6, 128, 16 }, "Insert");
        pill ("fx_routing", { 20, 24, 88, 18 }).displayName = [] (int i) { return i == 0 ? "a > b" : i == 1 ? "b > a" : "a // b"; };
        knob ("fx_a_vol", "FX A", { 30, 48, 68, 74 });
        knob ("fx_b_vol", "FX B", { 30, 124, 68, 74 });
        title ({ 0, 206, 128, 16 }, "Aux");
        toggle ("fx_aux_pre", Toggle::TextPill, "pre fx", { 30, 224, 68, 16 }).setTooltip ("Aux send taken before (on) or after (off) buses A and B");
        knob ("fx_aux_send", "Send", { 2, 250, 62, 72 });
        knob ("fx_aux_return", "Return", { 64, 250, 62, 72 });
    }
    void paint (juce::Graphics& g) override
    {
        drawPanel (g, getLocalBounds().toFloat());
        for (auto& c : captions)
            drawSectionTitle (g, c.r, c.text, col::text, juce::Justification::centred);
    }
};
} // namespace

FxPage::FxPage (UiContext& c) : ctx (c)
{
    left = std::make_unique<BusColumn> (ctx);
    addAndMakeVisible (*left);
    const char* busOn[3] = { "fx_a_on", "fx_b_on", "fx_aux_on" };
    for (int b = 0; b < 3; ++b)
    {
        busPower[b] = std::make_unique<Toggle> (ctx, busOn[b], Toggle::Power);
        addAndMakeVisible (*busPower[b]);
    }
    for (int s = 0; s < 9; ++s)
    {
        slotButtons.push_back (std::make_unique<SlotButton> (*this, s));
        addAndMakeVisible (*slotButtons.back());
    }
    for (auto& e : editors)
    {
        e = std::make_unique<SlotEditor> (ctx);
        addAndMakeVisible (*e);
    }
    selectBus (0);
}

FxPage::~FxPage() = default;

void FxPage::selectBus (int b)
{
    bus = juce::jlimit (0, 2, b);
    for (int i = 0; i < 3; ++i)
        editors[i]->setSlot (bus * 3 + i);
    refreshView();
    repaint();
}

void FxPage::resized()
{
    left->setBounds (0, 0, 128, 340);
    for (int b = 0; b < 3; ++b)
    {
        const int y = 4 + b * 112;
        busPower[b]->setBounds (140, y + 6, 14, 14);
        for (int i = 0; i < 3; ++i)
            slotButtons[(size_t) (b * 3 + i)]->setBounds (138, y + 26 + i * 26, 122, 22);
    }
    for (int i = 0; i < 3; ++i)
        editors[i]->setBounds (270 + i * 336, 0, 332, 340);
}

void FxPage::paint (juce::Graphics& g)
{
    const char* names[3] = { "FX A", "FX B", "AUX" };
    for (int b = 0; b < 3; ++b)
    {
        const auto r = juce::Rectangle<float> (132.0f, 4.0f + (float) b * 112.0f, 134.0f, 106.0f);
        drawPanel (g, r, 6.0f, b == bus ? col::panelHi : col::panel);
        if (b == bus)
        {
            g.setColour (col::gold);
            g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.2f);
        }
        drawSectionTitle (g, { 160, 6 + b * 112, 90, 14 }, names[b]);
    }
}

void FxPage::refreshView()
{
    left->refreshView();
    for (auto& e : editors)
        e->refreshView();
    for (auto& s : slotButtons)
        s->repaint();
}
} // namespace ui
