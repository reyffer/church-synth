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

// Readable values for effect controls that are really menus or switches.
juce::String fxValueText (int type, int k, float v, bool sync)
{
    using namespace cs::fx;
    auto pick = [v] (std::initializer_list<const char*> names) {
        const int n = (int) names.size();
        return juce::String (*(names.begin() + juce::jlimit (0, n - 1, (int) (v * (float) n))));
    };
    const juce::String name = fxInfo (type).params[(size_t) k] != nullptr ? fxInfo (type).params[(size_t) k] : "";
    if (sync && k == fxSyncParam (type))
        return cs::kSyncDivs[(size_t) juce::jlimit (0, cs::kNumSyncDivs - 1, (int) (v * cs::kNumSyncDivs))].name;
    if (name == "Ping-Pong" || name == "Auto Makeup" || name == "Auto Gain" || name == "Dark" || name == "Invert"
        || name == "Mono Bass" || name == "Natural Law" || name == "Wide" || name == "Mono Input")
        return v >= 0.5f ? "On" : "Off";
    switch (type)
    {
        case FxDistortion:
            if (k == 0) return pick ({ "Soft Clip", "Hard Clip", "Tube", "Tape", "Overdrive", "Fuzz", "Crunch", "Diode", "Sine Fold",
                                       "Triangle Fold", "Half Rectify", "Full Rectify", "Asymmetric", "Bit Shape", "Saturate", "Destroy" });
            if (k == 4) return pick ({ "Off", "Pre", "Post" });
            if (k == 5) return pick ({ "Lowpass", "Highpass", "Bandpass" });
            if (k == 8) return pick ({ "12 dB", "24 dB", "36 dB" });
            break;
        case FxMultiFilter:
            if (k == 0) return pick ({ "LP 12", "LP 24", "LP 36", "HP 12", "HP 24", "HP 36", "BP 12", "BP 24", "BP 36", "Notch", "Comb FF", "Comb FB" });
            break;
        case FxErode:
            if (k == 0) return pick ({ "Sine", "Triangle", "Saw", "Square", "Noise" });
            break;
        case FxVintageChorus:
            if (k == 0) return pick ({ "I", "II", "I + II" });
            break;
        case FxPhaser:
            if (k == 2) return pick ({ "Sine", "Triangle", "Square", "Saw", "Ramp", "S&H" });
            if (k == 5) return pick ({ "2", "4", "6", "8", "10", "12" }) + " poles";
            break;
        case FxChorus:
            if (k == 4) return pick ({ "1", "2", "3", "4" }) + " voices";
            if (k == 5) return pick ({ "Sine", "Square" });
            break;
        case FxFlanger:
            if (k == 7) return pick ({ "Sine", "Triangle" });
            break;
        case FxPanner:
            if (k == 6) return pick ({ "Sine", "Triangle", "Square" });
            break;
        case FxSuperUnison:
            if (k == 0) return pick ({ "2", "3", "4", "5", "6", "7", "8" }) + " voices";
            break;
        case FxPitchDelay:
            if (k == 1) return juce::String ((int) std::round (-12.0f + v * 24.0f)) + " st";
            if (k == 3) return pick ({ "Normal", "+ Oct Up", "+ Oct Down" });
            break;
        default: break;
    }
    return juce::String (juce::roundToInt (v * 100.0f)) + " %";
}
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
        for (int k = 0; k < cs::fx::kFxParams; ++k)
        {
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
            knobs[(size_t) k]->setVisible (used);
            if (used)
                knobs[(size_t) k]->setLabel (info.params[(size_t) k]);
        }
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
        int i = 0;
        for (auto& k : knobs)
            if (k->isVisible())
            {
                const int col = i % 4, row = i / 4;
                k->setBounds (6 + col * 80, 38 + row * 96, 76, 88);
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
