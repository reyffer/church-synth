#include "ModSection.h"
#include <array>

#include "dsp/ModMatrix.h"

namespace ui
{
namespace
{
const char* const kShortNames[cs::NumModSources] = { "VELO", "AT", "MW", "KBD", "EXP", "ENV AMP", "ENV 2", "ENV 3",
                                                     "LFO 1", "LFO 2", "LFO 3", "FUNC 1", "FUNC 2", "FUNC 3",
                                                     "RAND 1", "RAND 2", "RAND 3", "COMB 1", "COMB 2", "COMB 3",
                                                     "M 1", "M 2", "M 3", "M 4" };
const char* const kTabNames[6] = { "KEYBOARD", "ENVELOPES", "LFO", "FUNCTIONS", "RANDOM", "COMBINATE" };
const juce::Colour kTabColours[6] = { col::modKeyboard, col::modEnv, col::modLfo, col::modFunc, col::modRandom, col::modComb };

int tabForSource (int s)
{
    if (s <= cs::SrcExpression) return 0;
    if (s <= cs::SrcEnv3) return 1;
    if (s <= cs::SrcLfo3) return 2;
    if (s <= cs::SrcFunc3) return 3;
    if (s <= cs::SrcRand3) return 4;
    if (s <= cs::SrcComb3) return 5;
    return -1;
}

bool sourceIsBipolar (UiContext& ctx, int s)
{
    if (s >= cs::SrcLfo1 && s <= cs::SrcLfo3)
        return ctx.boolValue ("lfo" + juce::String (s - cs::SrcLfo1 + 1) + "_bipolar");
    if (s >= cs::SrcFunc1 && s <= cs::SrcFunc3)
        return ctx.boolValue ("func" + juce::String (s - cs::SrcFunc1 + 1) + "_bipolar");
    if (s >= cs::SrcRand1 && s <= cs::SrcRand3)
        return ctx.boolValue ("rand" + juce::String (s - cs::SrcRand1 + 1) + "_bipolar");
    return s >= cs::SrcComb1 && s <= cs::SrcComb3;
}
} // namespace

// ===========================================================================
class ModStrip::Tile final : public juce::Component, public juce::SettableTooltipClient
{
public:
    Tile (UiContext& c, int source) : ctx (c), src (source), scope (sourceColour (source))
    {
        scope.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (scope);
        setTooltip (juce::String (cs::modSourceName (src)) + ": click to select (then drag knobs to set amounts), or drag onto a knob");
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }
    void resized() override { scope.setBounds (getLocalBounds().withTrimmedTop (22).reduced (3, 4)); }
    void paint (juce::Graphics& g) override
    {
        const bool sel = ctx.selectedSource == src;
        const bool used = ctx.sourceUsed (src);
        auto r = getLocalBounds().toFloat().reduced (1.0f, 0.0f);
        g.setColour (sel ? col::panelHi.brighter (0.06f) : col::panel);
        g.fillRect (r);
        if (sel)
        {
            g.setColour (sourceColour (src));
            g.drawRect (r, 1.5f);
        }
        g.setColour (used || sel ? sourceColour (src) : col::textDim);
        g.setFont (font (11.5f, true));
        const juce::String name = src >= cs::SrcMacro1 ? juce::String ("M ") + juce::String (src - cs::SrcMacro1 + 1) : juce::String (kShortNames[src]);
        g.drawFittedText (name, getLocalBounds().removeFromTop (22).reduced (2, 0), juce::Justification::centred, 1, 0.7f);
    }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (e.getDistanceFromDragStart() > 6)
            if (auto* dnd = juce::DragAndDropContainer::findParentDragContainerFor (this))
                if (! dnd->isDragAndDropActive())
                {
                    juce::Image img (juce::Image::ARGB, 60, 22, true);
                    {
                        juce::Graphics g (img);
                        g.setColour (sourceColour (src).withAlpha (0.85f));
                        g.fillRoundedRectangle (0, 0, 60, 22, 5);
                        g.setColour (col::bg);
                        g.setFont (font (11.0f, true));
                        g.drawText (kShortNames[src], 0, 0, 60, 22, juce::Justification::centred);
                    }
                    dnd->startDragging ("modsrc:" + juce::String (src), this, juce::ScaledImage (img));
                    dragged = true;
                }
    }
    void mouseUp (const juce::MouseEvent& e) override
    {
        if (! dragged && ! e.mods.isPopupMenu())
        {
            ctx.selectSource (src);
            if (onClick)
                onClick (src);
        }
        dragged = false;
    }
    void push()
    {
        scope.bipolar = sourceIsBipolar (ctx, src);
        scope.push (ctx.proc.getSynth().uiSource[src].load (std::memory_order_relaxed));
    }
    std::function<void (int)> onClick;

private:
    UiContext& ctx;
    int src;
    Scope scope;
    bool dragged = false;
};

ModStrip::ModStrip (UiContext& c) : ctx (c)
{
    for (int s = 0; s < cs::NumModSources; ++s)
    {
        tiles.push_back (std::make_unique<Tile> (ctx, s));
        tiles.back()->onClick = [this] (int src) { if (onSourceClicked) onSourceClicked (src); };
        addAndMakeVisible (*tiles.back());
    }
    ctx.addChangeListener (this);
}

ModStrip::~ModStrip()
{
    ctx.removeChangeListener (this);
}

void ModStrip::resized()
{
    const float w = (float) getWidth() / (float) cs::NumModSources;
    for (int s = 0; s < cs::NumModSources; ++s)
        tiles[(size_t) s]->setBounds ((int) std::round ((float) s * w), 0, (int) std::round ((float) (s + 1) * w) - (int) std::round ((float) s * w), getHeight());
}

void ModStrip::paint (juce::Graphics& g)
{
    g.fillAll (col::bg);
}

void ModStrip::tick()
{
    for (auto& t : tiles)
        t->push();
}

// ===========================================================================
// Tab pages
// ===========================================================================
namespace
{
class KeyboardPage final : public ControlPanel
{
public:
    explicit KeyboardPage (UiContext& c) : ControlPanel (c)
    {
        add<CurveEditor> ({ 12, 12, 240, 200 }, ctx, "curve_velo", "Velocity", col::modKeyboard);
        add<CurveEditor> ({ 264, 12, 240, 200 }, ctx, "curve_at", "Aftertouch", col::modKeyboard);
        add<CurveEditor> ({ 516, 12, 240, 200 }, ctx, "curve_kbd", "Keytrack", col::modKeyboard);
        title ({ 780, 12, 260, 16 }, "Mod Quantize (Coarse)");
        caption ({ 772, 40, 80, 14 }, "Engine 1");
        add<NoteMask> ({ 856, 34, 180, 30 }, ctx, "e1_q_mask");
        caption ({ 772, 80, 80, 14 }, "Engine 2");
        add<NoteMask> ({ 856, 74, 180, 30 }, ctx, "e2_q_mask");
        caption ({ 772, 120, 80, 14 }, "Utility Osc");
        add<NoteMask> ({ 856, 114, 180, 30 }, ctx, "u_osc_q_mask");
        caption ({ 772, 160, 270, 40 }, "Modulated Coarse snaps to the lit intervals.");
    }
    void paint (juce::Graphics& g) override { paintCaptions (g); }
};

class EnvelopesPage final : public ControlPanel
{
public:
    explicit EnvelopesPage (UiContext& c) : ControlPanel (c)
    {
        const char* names[3] = { "Env Amp", "Env 2", "Env 3" };
        for (int k = 0; k < 3; ++k)
        {
            const juce::String p = "env" + juce::String (k + 1);
            const int bx = 8 + k * 350;
            title ({ bx + 6, 6, 120, 16 }, names[k]);
            add<EnvDisplay> ({ bx + 4, 26, 330, 84 }, ctx, p, col::modEnv);
            for (auto* kn : { &knob (p + "_a", "Attack", { bx, 114, 60, 66 }), &knob (p + "_d", "Decay", { bx + 64, 114, 60, 66 }),
                              &knob (p + "_s", "Sustain", { bx + 128, 114, 60, 66 }), &knob (p + "_r", "Release", { bx + 192, 114, 60, 66 }),
                              &knob (p + "_acurve", "Att Curve", { bx, 178, 60, 60 }), &knob (p + "_dcurve", "Dec Curve", { bx + 64, 178, 60, 60 }) })
                kn->setValueColour (col::modEnv);
            toggle (p + "_link", Toggle::TextPill, "link", { bx + 258, 132, 44, 16 }).setTooltip ("Release uses the Decay time");
            toggle (p + "_adr", Toggle::TextPill, "ADR", { bx + 258, 154, 44, 16 }).setTooltip ("ADR mode: no sustain stage");
            if (k > 0)
            {
                caption ({ bx + 128, 230, 120, 12 }, "Retrig Source");
                pill (p + "_retrig", { bx + 136, 196, 110, 18 });
            }
        }
    }
    void paint (juce::Graphics& g) override
    {
        g.setColour (col::border);
        g.drawVerticalLine (352, 8.0f, (float) getHeight() - 8.0f);
        g.drawVerticalLine (702, 8.0f, (float) getHeight() - 8.0f);
        paintCaptions (g);
    }
};

// Rate knob in Hz, or a division menu when synced.
class RateControl final : public juce::Component
{
public:
    RateControl (UiContext& c, const juce::String& prefix, juce::Colour colour)
        : ctx (c), p (prefix), rate (c, prefix + "_rate", "Rate"), div (c, prefix + "_div"),
          sync (c, prefix + "_sync", Toggle::TextPill, "sync")
    {
        rate.setValueColour (colour);
        sync.onColour = colour;
        sync.onChange = [this] { update(); };
        addAndMakeVisible (rate);
        addAndMakeVisible (div);
        addAndMakeVisible (sync);
        update();
    }
    void resized() override
    {
        rate.setBounds (0, 0, 60, 66);
        div.setBounds (2, 22, 56, 18);
        sync.setBounds (8, 68, 44, 15);
    }
    void paint (juce::Graphics& g) override
    {
        if (div.isVisible())
            drawCaption (g, { 0, 46, 60, 14 }, "Rate");
    }
    void update()
    {
        const bool s = ctx.boolValue (p + "_sync");
        rate.setVisible (! s);
        div.setVisible (s);
        repaint();
    }

private:
    UiContext& ctx;
    juce::String p;
    Knob rate;
    Pill div;
    Toggle sync;
};

class LfoPage final : public ControlPanel
{
public:
    explicit LfoPage (UiContext& c) : ControlPanel (c)
    {
        for (int k = 0; k < 3; ++k)
        {
            const juce::String p = "lfo" + juce::String (k + 1);
            const int bx = 8 + k * 350;
            title ({ bx + 6, 6, 80, 16 }, "LFO " + juce::String (k + 1));
            auto& wave = pill (p + "_wave", { bx + 220, 4, 110, 18 });
            wave.textColour = col::modLfo;
            add<LfoDisplay> ({ bx + 4, 26, 330, 82 }, ctx, k);
            add<RateControl> ({ bx, 112, 60, 86 }, ctx, p, col::modLfo);
            for (auto* kn : { &knob (p + "_sym", "Symmetry", { bx + 64, 112, 60, 66 }), &knob (p + "_phase", "Phase", { bx + 128, 112, 60, 66 }),
                              &knob (p + "_fade", "Fade", { bx + 192, 112, 60, 66 }), &knob (p + "_smooth", "Smooth", { bx + 256, 112, 60, 66 }),
                              &knob (p + "_kbd", "Kbd", { bx + 64, 180, 56, 58 }) })
                kn->setValueColour (col::modLfo);
            caption ({ bx + 126, 222, 112, 12 }, "Retrig Source");
            pill (p + "_retrig", { bx + 128, 198, 110, 18 }).textColour = col::modLfo;
            auto& bi = toggle (p + "_bipolar", Toggle::TextPill, "+/-", { bx + 260, 198, 44, 18 });
            bi.onColour = col::modLfo;
            bi.setTooltip ("Bipolar (-1..1) or unipolar (0..1)");
        }
    }
    void paint (juce::Graphics& g) override
    {
        g.setColour (col::border);
        g.drawVerticalLine (352, 8.0f, (float) getHeight() - 8.0f);
        g.drawVerticalLine (702, 8.0f, (float) getHeight() - 8.0f);
        paintCaptions (g);
    }
};

class FunctionsPage final : public ControlPanel
{
public:
    explicit FunctionsPage (UiContext& c) : ControlPanel (c)
    {
        for (int k = 0; k < 3; ++k)
        {
            const juce::String p = "func" + juce::String (k + 1);
            const int bx = 8 + k * 350;
            title ({ bx + 6, 6, 100, 16 }, "Function " + juce::String (k + 1));
            auto& shape = pill (p + "_shape", { bx + 196, 4, 134, 18 });
            shape.textColour = col::modFunc;
            shape.setTooltip ("Shape preset (edit the curve to make it Custom)");
            auto& ed = add<FunctionEditor> ({ bx + 4, 26, 330, 96 }, ctx, k);
            ed.setTooltip ("Drag points; drag between points to bend; double-click to add / remove; right-click a point for sustain / loop");
            add<RateControl> ({ bx, 126, 60, 86 }, ctx, p, col::modFunc);
            knob (p + "_smooth", "Smooth", { bx + 64, 126, 60, 66 }).setValueColour (col::modFunc);
            knob (p + "_scale", "Scale", { bx + 128, 126, 60, 66 }).setValueColour (col::modFunc);
            caption ({ bx + 192, 146, 110, 12 }, "Mode");
            pill (p + "_mode", { bx + 196, 126, 110, 18 }).textColour = col::modFunc;
            caption ({ bx + 64, 222, 120, 12 }, "Retrig Source");
            pill (p + "_retrig", { bx + 68, 198, 110, 18 }).textColour = col::modFunc;
            auto& bi = toggle (p + "_bipolar", Toggle::TextPill, "+/-", { bx + 196, 198, 44, 18 });
            bi.onColour = col::modFunc;
        }
    }
    void paint (juce::Graphics& g) override
    {
        g.setColour (col::border);
        g.drawVerticalLine (352, 8.0f, (float) getHeight() - 8.0f);
        g.drawVerticalLine (702, 8.0f, (float) getHeight() - 8.0f);
        paintCaptions (g);
    }
};

// Random: common controls + the controls of the chosen type.
class RandomPage final : public ControlPanel
{
public:
    explicit RandomPage (UiContext& c) : ControlPanel (c)
    {
        for (int k = 0; k < 3; ++k)
        {
            const juce::String p = "rand" + juce::String (k + 1);
            const int bx = 8 + k * 350;
            title ({ bx + 6, 6, 100, 16 }, "Random " + juce::String (k + 1));
            pill (p + "_type", { bx + 196, 4, 134, 18 }).textColour = col::modRandom;
            auto& sc = add<Scope> ({ bx + 4, 26, 330, 60 }, col::modRandom);
            scopes[k] = &sc;
            add<RateControl> ({ bx, 90, 60, 86 }, ctx, p, col::modRandom);
            knob (p + "_smooth", "Smooth", { bx + 64, 90, 56, 62 }).setValueColour (col::modRandom);
            knob (p + "_dist", "Distance", { bx + 122, 90, 56, 62 }).setValueColour (col::modRandom);
            knob (p + "_jitter", "Jitter", { bx + 180, 90, 56, 62 }).setValueColour (col::modRandom);
            auto& bi = toggle (p + "_bipolar", Toggle::TextPill, "+/-", { bx + 246, 92, 44, 18 });
            bi.onColour = col::modRandom;
            pill (p + "_retrig", { bx + 240, 116, 96, 18 }).textColour = col::modRandom;

            // Type-specific rows (y 180..240)
            auto& vm = typeRows[k][cs::RandVoiceMod];
            vm.push_back (&stepper (p + "_vm_voices", { bx + 4, 186, 50, 18 }));
            vm.push_back (&pill (p + "_vm_mode", { bx + 4, 210, 70, 18 }));
            for (int st = 0; st < 8; ++st)
            {
                auto& kn = knob (p + "_vm" + juce::String (st + 1), juce::String (st + 1), { bx + 80 + st * 32, 182, 32, 52 });
                kn.setValueColour (col::modRandom);
                vm.push_back (&kn);
            }
            auto& sh = typeRows[k][cs::RandSampleHold];
            sh.push_back (&pill (p + "_sh_src", { bx + 4, 196, 110, 18 }));
            sh.push_back (&knob (p + "_sh_rise", "Rise", { bx + 122, 180, 56, 60 }));
            sh.push_back (&knob (p + "_sh_fall", "Fall", { bx + 180, 180, 56, 60 }));
            sh.push_back (&toggle (p + "_sh_link", Toggle::TextPill, "link", { bx + 244, 196, 44, 16 }));
            auto& tu = typeRows[k][cs::RandTuring];
            tu.push_back (&knob (p + "_flip", "Flip", { bx + 4, 180, 56, 60 }));
            tu.push_back (&stepper (p + "_len", { bx + 70, 198, 56, 18 }));
            auto& bin = typeRows[k][cs::RandBinary];
            bin.push_back (&knob (p + "_proba", "Probability", { bx + 4, 180, 60, 60 }));
            bin.push_back (&knob (p + "_correl", "Correlation", { bx + 68, 180, 60, 60 }));
        }
    }
    void paint (juce::Graphics& g) override
    {
        g.setColour (col::border);
        g.drawVerticalLine (352, 8.0f, (float) getHeight() - 8.0f);
        g.drawVerticalLine (702, 8.0f, (float) getHeight() - 8.0f);
        for (int k = 0; k < 3; ++k)
        {
            const int type = ctx.intValue ("rand" + juce::String (k + 1) + "_type");
            const int bx = 8 + k * 350;
            if (type == cs::RandVoiceMod)
                drawCaption (g, { bx + 4, 230, 70, 12 }, "VM steps / mode");
            if (type == cs::RandTuring)
                drawCaption (g, { bx + 70, 220, 56, 12 }, "Length");
        }
        paintCaptions (g);
    }
    void refreshView() override
    {
        ControlPanel::refreshView();
        for (int k = 0; k < 3; ++k)
        {
            const int type = ctx.intValue ("rand" + juce::String (k + 1) + "_type");
            for (int t = 0; t < cs::RandTypeCount; ++t)
                for (auto* comp : typeRows[k][t])
                    comp->setVisible (t == type);
        }
        repaint();
    }
    void tick()
    {
        for (int k = 0; k < 3; ++k)
        {
            scopes[k]->bipolar = ctx.boolValue ("rand" + juce::String (k + 1) + "_bipolar");
            scopes[k]->push (ctx.proc.getSynth().uiSource[cs::SrcRand1 + k].load (std::memory_order_relaxed));
        }
    }

private:
    Scope* scopes[3] {};
    std::vector<juce::Component*> typeRows[3][cs::RandTypeCount];
};

class CombinatePage final : public ControlPanel
{
public:
    explicit CombinatePage (UiContext& c) : ControlPanel (c)
    {
        for (int k = 0; k < 3; ++k)
        {
            const juce::String p = "comb" + juce::String (k + 1);
            const int bx = 8 + k * 350;
            title ({ bx + 6, 6, 120, 16 }, "Combinator " + juce::String (k + 1));
            pill (p + "_type", { bx + 196, 4, 134, 18 }).textColour = col::modComb;
            auto& sc = add<Scope> ({ bx + 4, 26, 330, 56 }, col::modComb);
            scopes[k] = &sc;
            caption ({ bx + 4, 108, 110, 12 }, "Source");
            pill (p + "_src", { bx + 6, 88, 106, 18 });
            caption ({ bx + 118, 108, 110, 12 }, "Modulator");
            modPills[k] = &pill (p + "_mod", { bx + 120, 88, 106, 18 });
            knob (p + "_amount", "Amount", { bx + 240, 86, 60, 66 }).setValueColour (col::modComb);
            remap[k] = &pill (p + "_remap", { bx + 120, 128, 106, 18 });
            auto& ef = follower[k];
            ef.push_back (&pill (p + "_ef_input", { bx + 6, 128, 106, 18 }));
            ef.push_back (&knob (p + "_ef_gain", "Gain", { bx + 2, 160, 56, 62 }));
            ef.push_back (&knob (p + "_ef_thresh", "Threshold", { bx + 62, 160, 56, 62 }));
            ef.push_back (&knob (p + "_ef_att", "Attack", { bx + 122, 160, 56, 62 }));
            ef.push_back (&knob (p + "_ef_rel", "Release", { bx + 182, 160, 56, 62 }));
        }
    }
    void paint (juce::Graphics& g) override
    {
        g.setColour (col::border);
        g.drawVerticalLine (352, 8.0f, (float) getHeight() - 8.0f);
        g.drawVerticalLine (702, 8.0f, (float) getHeight() - 8.0f);
        paintCaptions (g);
    }
    void refreshView() override
    {
        ControlPanel::refreshView();
        for (int k = 0; k < 3; ++k)
        {
            const int type = ctx.intValue ("comb" + juce::String (k + 1) + "_type");
            remap[k]->setVisible (type == cs::CombRemap);
            for (auto* comp : follower[k])
                comp->setVisible (type == cs::CombEnvFollow);
            modPills[k]->setVisible (type != cs::CombRemap && type != cs::CombEnvFollow);
        }
    }
    void tick()
    {
        for (int k = 0; k < 3; ++k)
            scopes[k]->push (ctx.proc.getSynth().uiSource[cs::SrcComb1 + k].load (std::memory_order_relaxed));
    }

private:
    Scope* scopes[3] {};
    Pill* remap[3] {};
    Pill* modPills[3] {};
    std::vector<juce::Component*> follower[3];
};

// Macros: 4 big knobs with editable names (double-click the name).
class MacroPanel final : public ControlPanel
{
public:
    explicit MacroPanel (UiContext& c) : ControlPanel (c)
    {
        for (int m = 0; m < 4; ++m)
        {
            auto& k = knob ("macro" + juce::String (m + 1), ctx.proc.getMacroName (m), { 14 + (m % 2) * 100, 34 + (m / 2) * 108, 86, 98 });
            k.setValueColour (col::modMacro);
            k.setTooltip ("Macro " + juce::String (m + 1) + ": route it from the M " + juce::String (m + 1) + " tile. Double-click the name below to rename.");
            knobs[m] = &k;
            auto& ed = add<juce::Label> ({ 14 + (m % 2) * 100, 34 + (m / 2) * 108 + 84, 86, 14 });
            ed.setJustificationType (juce::Justification::centred);
            ed.setFont (font (11.0f));
            ed.setColour (juce::Label::textColourId, juce::Colours::transparentBlack);
            ed.setEditable (false, true, false);
            ed.onTextChange = [this, m, &ed] {
                ctx.proc.setMacroName (m, ed.getText());
                knobs[m]->setLabel (ctx.proc.getMacroName (m));
                ed.setText ({}, juce::dontSendNotification);
                ctx.proc.checkpoint ("Rename macro");
            };
            ed.onEditorShow = [&ed, this, m] {
                if (auto* te = ed.getCurrentTextEditor())
                {
                    te->setText (ctx.proc.getMacroName (m), false);
                    te->selectAll();
                }
            };
        }
    }
    void paint (juce::Graphics& g) override
    {
        drawPanel (g, getLocalBounds().toFloat());
        drawSectionTitle (g, { 0, 6, getWidth(), 18 }, "Macros", col::text, juce::Justification::centred);
    }
    void refreshView() override
    {
        ControlPanel::refreshView();
        for (int m = 0; m < 4; ++m)
            knobs[m]->setLabel (ctx.proc.getMacroName (m));
    }

private:
    Knob* knobs[4] {};
};
} // namespace

// ===========================================================================
ModTabs::ModTabs (UiContext& c) : ctx (c)
{
    pages.push_back (std::make_unique<KeyboardPage> (ctx));
    pages.push_back (std::make_unique<EnvelopesPage> (ctx));
    pages.push_back (std::make_unique<LfoPage> (ctx));
    pages.push_back (std::make_unique<FunctionsPage> (ctx));
    pages.push_back (std::make_unique<RandomPage> (ctx));
    pages.push_back (std::make_unique<CombinatePage> (ctx));
    for (auto& p : pages)
        addChildComponent (*p);
    macros = std::make_unique<MacroPanel> (ctx);
    addAndMakeVisible (*macros);
    showTab (1);
}

ModTabs::~ModTabs() = default;

juce::Rectangle<int> ModTabs::tabRect (int t) const
{
    const int w = 1058 / 6;
    return { t * w, 0, t == 5 ? 1058 - 5 * w : w, 26 };
}

void ModTabs::showTab (int t)
{
    current = juce::jlimit (0, 5, t);
    for (size_t i = 0; i < pages.size(); ++i)
        pages[i]->setVisible ((int) i == current);
    pages[(size_t) current]->refreshView();
    repaint();
}

void ModTabs::showTabForSource (int source)
{
    const int t = tabForSource (source);
    if (t >= 0)
        showTab (t);
}

void ModTabs::resized()
{
    for (auto& p : pages)
        p->setBounds (0, 28, 1058, getHeight() - 28);
    macros->setBounds (1062, 0, getWidth() - 1062, getHeight());
}

void ModTabs::paint (juce::Graphics& g)
{
    drawPanel (g, juce::Rectangle<float> (0.0f, 0.0f, 1058.0f, (float) getHeight()));
    for (int t = 0; t < 6; ++t)
    {
        const auto r = tabRect (t);
        g.setColour (t == current ? col::panel : col::panelLo);
        g.fillRect (r.reduced (1, 0));
        g.setColour (kTabColours[t].withAlpha (t == current ? 1.0f : 0.45f));
        g.fillRect (r.getX() + 1, 0, r.getWidth() - 2, 2);
        g.setColour (t == current ? col::text : col::textDim);
        g.setFont (font (12.0f, true));
        g.drawText (kTabNames[t], r, juce::Justification::centred);
    }
}

void ModTabs::mouseDown (const juce::MouseEvent& e)
{
    for (int t = 0; t < 6; ++t)
        if (tabRect (t).contains (e.getPosition()))
            showTab (t);
}

void ModTabs::refreshView()
{
    pages[(size_t) current]->refreshView();
    macros->refreshView();
}

void ModTabs::tick()
{
    if (current == 4)
        static_cast<RandomPage*> (pages[4].get())->tick();
    if (current == 5)
        static_cast<CombinatePage*> (pages[5].get())->tick();
    if (current == 2 || current == 3)
        pages[(size_t) current]->refreshView();
}
} // namespace ui
