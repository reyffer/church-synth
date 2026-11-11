#include "SynthPage.h"

namespace ui
{
// ===========================================================================
// ControlPanel helpers
// ===========================================================================
void ControlPanel::refreshView()
{
    for (auto* v : views)
        v->refreshView();
}

Knob& ControlPanel::knob (const juce::String& id, const juce::String& label, juce::Rectangle<int> r)
{
    return add<Knob> (r, ctx, id, label);
}
Pill& ControlPanel::pill (const juce::String& id, juce::Rectangle<int> r)
{
    return add<Pill> (r, ctx, id);
}
Toggle& ControlPanel::toggle (const juce::String& id, Toggle::Style s, const juce::String& text, juce::Rectangle<int> r)
{
    return add<Toggle> (r, ctx, id, s, text);
}
Stepper& ControlPanel::stepper (const juce::String& id, juce::Rectangle<int> r)
{
    return add<Stepper> (r, ctx, id);
}
void ControlPanel::paintCaptions (juce::Graphics& g)
{
    for (const auto& c : captions)
        if (c.title)
            drawSectionTitle (g, c.r, c.text);
        else
            drawCaption (g, c.r, c.text);
}

// ===========================================================================
NamedIntPill::NamedIntPill (UiContext& c, juce::String paramId) : ctx (c), id (std::move (paramId))
{
    if (auto* p = ctx.param (id))
    {
        att = std::make_unique<juce::ParameterAttachment> (*p, [this] (float v) { value = (int) std::lround (v); repaint(); });
        att->sendInitialUpdate();
    }
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void NamedIntPill::paint (juce::Graphics& g)
{
    if (names.isEmpty())
        return;
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (col::panelLo);
    g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
    g.setColour (col::border);
    g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.0f);
    g.setColour (col::accent);
    g.setFont (font (juce::jmin (12.0f, (float) getHeight() * 0.62f)));
    g.drawFittedText (names[juce::jlimit (0, names.size() - 1, value)], getLocalBounds().reduced (6, 0), juce::Justification::centred, 1, 0.7f);
}

void NamedIntPill::mouseDown (const juce::MouseEvent&)
{
    if (names.isEmpty() || att == nullptr)
        return;
    juce::PopupMenu m;
    for (int i = 0; i < names.size(); ++i)
        m.addItem (i + 1, names[i], true, i == value);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [safe = juce::Component::SafePointer<NamedIntPill> (this)] (int r) {
        if (safe != nullptr && r > 0)
            safe->att->setValueAsCompleteGesture ((float) (r - 1));
    });
}

// ===========================================================================
// Engines
// ===========================================================================
namespace
{
// Shared "Tune + Unison" column used by both engine types.
void buildTuneColumn (ControlPanel& p, UiContext& ctx, int e, bool wavetable,
                      std::function<Knob& (const juce::String&, const juce::String&, juce::Rectangle<int>)> knob,
                      std::function<Toggle& (const juce::String&, Toggle::Style, const juce::String&, juce::Rectangle<int>)> toggle,
                      std::function<Pill& (const juce::String&, juce::Rectangle<int>)> pill,
                      std::function<Stepper& (const juce::String&, juce::Rectangle<int>)> stepper,
                      std::function<void (juce::Rectangle<int>, const juce::String&)> title,
                      std::function<void (juce::Rectangle<int>, const juce::String&)> caption,
                      std::vector<juce::Component*>& chordOnly, std::vector<juce::Component*>& superOnly)
{
    juce::ignoreUnused (p, ctx, wavetable);
    const juce::String x = "e" + juce::String (e + 1) + "_";
    title ({ 12, 6, 80, 16 }, "Tune");
    knob (x + "coarse", "Coarse", { 10, 24, 64, 70 }).setFormatter ([] (float v) { return juce::String ((int) std::lround (v)) + " st"; });
    toggle (x + "kbd", Toggle::TextPill, "kbd", { 82, 30, 40, 16 }).setTooltip ("Key tracking (off = fixed pitch)");
    knob (x + "fine", "Fine", { 6, 98, 54, 60 });
    knob (x + "drift", "Drift", { 66, 98, 54, 60 });

    toggle (x + "uni_on", Toggle::Power, {}, { 10, 170, 16, 16 }).setTooltip ("Unison on/off");
    auto& mode = pill (x + "uni_mode", { 30, 168, 92, 20 });
    mode.arrows = true;
    caption ({ 6, 196, 56, 14 }, "Voices");
    stepper (x + "uni_voices", { 8, 212, 52, 20 });
    knob (x + "uni_detune", "Detune", { 66, 190, 56, 60 });
    knob (x + "uni_stereo", "Stereo", { 6, 242, 56, 60 });
    auto& chord = pill (x + "uni_chord", { 64, 262, 62, 20 });
    chordOnly.push_back (&chord);
    auto& mix = knob (x + "uni_mix", "Mix", { 66, 246, 56, 60 });
    superOnly.push_back (&mix);
    if (wavetable)
    {
        auto& ph = knob (x + "wt_uni_phase", "Phase", { 66, 246, 56, 60 });
        ph.setTooltip ("Unison phase spread (Wavetable)");
        chordOnly.push_back (&ph); // hidden in Chord/Super modes to make room
    }
}
} // namespace

class AnalogView final : public ControlPanel
{
public:
    AnalogView (UiContext& c, int engine) : ControlPanel (c), e (engine)
    {
        const juce::String x = "e" + juce::String (e + 1) + "_";
        const int meterBase = e == 0 ? cs::MeterE1Osc1 : cs::MeterE2Osc1;
        buildTuneColumn (*this, ctx, e, false,
                         [this] (auto& id, auto& l, auto r) -> Knob& { return knob (id, l, r); },
                         [this] (auto& id, auto s, auto& t, auto r) -> Toggle& { return toggle (id, s, t, r); },
                         [this] (auto& id, auto r) -> Pill& { return pill (id, r); },
                         [this] (auto& id, auto r) -> Stepper& { return stepper (id, r); },
                         [this] (auto r, auto& t) { title (r, t); }, [this] (auto r, auto& t) { caption (r, t); }, chordOnly, superOnly);

        for (int o = 0; o < 3; ++o)
        {
            const juce::String op = x + "o" + juce::String (o + 1) + "_";
            const int y = 4 + o * 102, bx = 140;
            add<SoundLed> ({ bx + 6, y + 7, 14, 14 }, ctx, meterBase + o, "Oscillator " + juce::String (o + 1));
            title ({ bx + 22, y + 6, 80, 16 }, "Oscillator " + juce::String (o + 1));
            knob (op + "coarse", "Coarse", { bx + 6, y + 26, 54, 66 }).setFormatter ([] (float v) { return juce::String ((int) std::lround (v)) + " st"; });
            if (o > 0)
            {
                knob (op + "fine", "Fine", { bx + 62, y + 26, 54, 66 });
                toggle (op + "kbd", Toggle::TextPill, "kbd", { bx + 104, y + 6, 36, 16 }).setTooltip ("Key tracking");
            }
            if (o == 1)
                toggle (x + "sync", Toggle::TextPill, "sync", { bx + 144, y + 6, 44, 16 }).setTooltip ("Hard sync Osc 2 to Osc 1");
            add<OscDisplay> ({ bx + 124, y + 24, 160, 46 }, ctx, op + "wave", op + "width", op + "vol");
            add<WaveSelector> ({ bx + 124, y + 72, 160, 22 }, ctx, op + "wave");
            widths[o] = &knob (op + "width", "Width", { bx + 290, y + 26, 54, 66 });
            widths[o]->setTooltip ("Pulse width (Square) / slope (Triangle)");
            knob (op + "vol", "Volume", { bx + 346, y + 26, 54, 66 });
            if (o < 2)
                knob (op + "fm", "FM", { bx + 404, y + 30, 46, 58 }).setTooltip (o == 0 ? "Osc 3 / Noise FMs Osc 1" : "Osc 3 / Noise FMs Osc 2");
        }
        const int rx = 610;
        add<SoundLed> ({ rx + 4, 7, 14, 14 }, ctx, meterBase + 3, "the noise");
        title ({ rx + 20, 6, 80, 16 }, "Noise");
        knob (x + "noise_color", "Colour", { rx + 4, 26, 56, 66 });
        knob (x + "noise_vol", "Volume", { rx + 62, 26, 56, 66 });
        title ({ rx + 6, 104, 80, 16 }, "Output");
        knob (x + "vol", "Volume", { rx + 4, 124, 56, 66 });
        knob (x + "fmix", "F1 / F2", { rx + 62, 124, 56, 66 }).setTooltip ("Filter mix: how much goes to Filter 1 vs Filter 2");
        title ({ rx + 6, 206, 100, 16 }, "Modulator");
        knob (x + "mod_src", "Osc3 / Noise", { rx + 4, 226, 64, 70 }).setTooltip ("FM modulator: Osc 3, Noise or a blend");
        toggle (x + "mod_eng", Toggle::TextPill, "other engine", { rx + 74, 244, 90, 18 }).setTooltip ("Use the other engine as the FM modulator (cross-mod)");
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (col::border);
        g.drawVerticalLine (132, 6.0f, (float) getHeight() - 6.0f);
        g.drawHorizontalLine (160, 6.0f, 126.0f);
        g.drawVerticalLine (604, 6.0f, (float) getHeight() - 6.0f);
        for (int o = 1; o < 3; ++o)
            g.drawHorizontalLine (4 + o * 102 - 2, 146.0f, 596.0f);
        paintCaptions (g);
    }

    void refreshView() override
    {
        ControlPanel::refreshView();
        const juce::String x = "e" + juce::String (e + 1) + "_";
        const int mode = ctx.intValue (x + "uni_mode");
        for (auto* c : chordOnly)
            c->setVisible (mode == cs::UnisonChord);
        for (auto* c : superOnly)
            c->setVisible (mode == cs::UnisonSuper);
        // Width only shapes Square and Triangle.
        for (int o = 0; o < 3; ++o)
        {
            const int w = ctx.intValue (x + "o" + juce::String (o + 1) + "_wave");
            widths[o]->setEnabled (w == cs::WaveSquare || w == cs::WaveTriangle);
        }
    }

private:
    int e;
    std::vector<juce::Component*> chordOnly, superOnly;
    Knob* widths[3] {};
};

class WavetableView final : public ControlPanel
{
public:
    WavetableView (UiContext& c, int engine) : ControlPanel (c), e (engine)
    {
        const juce::String x = "e" + juce::String (e + 1) + "_", w = x + "wt_";
        buildTuneColumn (*this, ctx, e, true,
                         [this] (auto& id, auto& l, auto r) -> Knob& { return knob (id, l, r); },
                         [this] (auto& id, auto s, auto& t, auto r) -> Toggle& { return toggle (id, s, t, r); },
                         [this] (auto& id, auto r) -> Pill& { return pill (id, r); },
                         [this] (auto& id, auto r) -> Stepper& { return stepper (id, r); },
                         [this] (auto r, auto& t) { title (r, t); }, [this] (auto r, auto& t) { caption (r, t); }, chordOnly, superOnly);
        phaseOnly = chordOnly.back();
        chordOnly.pop_back();

        const int bx = 140;
        auto& table = pill (w + "table", { bx + 8, 6, 230, 20 });
        table.extraMenu = [] (juce::PopupMenu& m) {
            m.addItem (10001, "Load your own wavetable...");
            m.addItem (10002, "Back to the factory table");
        };
        table.onExtra = [this] (int r) {
            if (r == 10001)
                chooseFile();
            else
                ctx.proc.clearUserFile (e);
        };
        tableRef = &table;
        add<SoundLed> ({ bx + 242, 9, 14, 14 }, ctx, e == 0 ? cs::MeterE1Osc1 : cs::MeterE2Osc1, "the wavetable oscillator");
        table.displayName = [this] (int) { return ctx.proc.getSlotDisplayName (e).fromFirstOccurrenceOf (": ", false, false); };
        add<WavetableDisplay> ({ bx + 8, 30, 300, 156 }, ctx, e);
        knob (w + "pos", "Position", { bx + 314, 28, 70, 78 }).setValueColour (col::accent);
        toggle (w + "morph", Toggle::TextPill, "morph", { bx + 322, 110, 56, 16 }).setTooltip ("Smooth morph between frames");
        knob (w + "vol", "Level", { bx + 318, 130, 62, 66 });

        title ({ bx + 8, 196, 60, 16 }, "FM");
        pill (w + "fm_type", { bx + 40, 196, 80, 16 });
        knob (w + "fm", "Amount", { bx + 4, 216, 54, 64 });
        knob (w + "pm", "Phase Mod", { bx + 62, 216, 54, 64 });
        title ({ bx + 128, 196, 120, 16 }, "Phase");
        pill (w + "pd_type", { bx + 180, 196, 84, 16 });
        knob (w + "pd", "Amount", { bx + 124, 216, 54, 64 });
        knob (w + "pd_mod", "Mod", { bx + 180, 216, 54, 64 });
        title ({ bx + 268, 196, 60, 16 }, "Fold");
        pill (w + "fold_type", { bx + 310, 196, 70, 16 });
        knob (w + "fold", "Amount", { bx + 264, 216, 54, 64 });
        knob (w + "fold_mod", "Mod", { bx + 320, 216, 54, 64 });
        caption ({ bx + 4, 286, 380, 14 }, "Mod = how much the modulator osc moves Phase / Fold");

        const int rx = 540;
        title ({ rx + 6, 6, 80, 16 }, "Output");
        knob (x + "vol", "Volume", { rx + 4, 26, 56, 66 });
        knob (x + "fmix", "F1 / F2", { rx + 62, 26, 56, 66 });
        title ({ rx + 128, 6, 100, 16 }, "Start phase");
        pill (w + "retrig", { rx + 128, 26, 100, 18 });
        knob (w + "phase", "Phase", { rx + 150, 48, 56, 60 });
        add<SoundLed> ({ rx + 4, 119, 14, 14 }, ctx, e == 0 ? cs::MeterE1Osc2 : cs::MeterE2Osc2, "the modulator oscillator (heard through its Volume)");
        title ({ rx + 20, 118, 160, 16 }, "Modulator Osc");
        pill (w + "mod_tune_mode", { rx + 8, 140, 100, 18 });
        pill (w + "mod_wave", { rx + 114, 140, 108, 18 });
        tuneCoarse = &knob (w + "mod_coarse", "Coarse", { rx + 4, 166, 56, 66 });
        tuneRatio = &knob (w + "mod_ratio", "Ratio", { rx + 4, 166, 56, 66 });
        tuneHz = &knob (w + "mod_hz", "Freq", { rx + 4, 166, 56, 66 });
        knob (w + "mod_fine", "Fine", { rx + 62, 166, 56, 66 });
        knob (w + "mod_vol", "Volume", { rx + 120, 166, 56, 66 }).setTooltip ("Modulator heard directly (0 = only modulates)");
        toggle (x + "mod_eng", Toggle::TextPill, "other engine", { rx + 8, 248, 100, 18 }).setTooltip ("Use the other engine as the modulator (cross-mod)");
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (col::border);
        g.drawVerticalLine (132, 6.0f, (float) getHeight() - 6.0f);
        g.drawHorizontalLine (160, 6.0f, 126.0f);
        g.drawVerticalLine (534, 6.0f, (float) getHeight() - 6.0f);
        g.drawHorizontalLine (112, 540.0f, (float) getWidth() - 8.0f);
        paintCaptions (g);
    }

    void refreshView() override
    {
        ControlPanel::refreshView();
        const juce::String x = "e" + juce::String (e + 1) + "_";
        const int mode = ctx.intValue (x + "uni_mode");
        for (auto* c : chordOnly)
            c->setVisible (mode == cs::UnisonChord);
        for (auto* c : superOnly)
            c->setVisible (mode == cs::UnisonSuper);
        phaseOnly->setVisible (mode == cs::UnisonClassic);
        const int tm = ctx.intValue (x + "wt_mod_tune_mode");
        tuneRatio->setVisible (tm == 0);
        tuneCoarse->setVisible (tm == 1 || tm == 2);
        tuneHz->setVisible (tm == 3);
        tableRef->repaint();
    }

private:
    int e;
    std::vector<juce::Component*> chordOnly, superOnly;
    juce::Component* phaseOnly = nullptr;
    Knob *tuneCoarse = nullptr, *tuneRatio = nullptr, *tuneHz = nullptr;
    Pill* tableRef = nullptr;
    std::unique_ptr<juce::FileChooser> chooser;

    void chooseFile()
    {
        chooser = std::make_unique<juce::FileChooser> ("Choose a wavetable (2048 samples per frame)", ChurchSynthProcessor::userFolder (true),
                                                       "*.wav;*.aif;*.aiff;*.flac");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& fc) {
                                  const auto f = fc.getResult();
                                  juce::String err;
                                  if (f != juce::File() && ! ctx.proc.importUserFile (e, f, err))
                                      juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Church Synth", err);
                              });
    }
};

class UtilityView final : public ControlPanel
{
public:
    explicit UtilityView (UiContext& c) : ControlPanel (c)
    {
        for (int n = 0; n < 2; ++n)
        {
            const juce::String p = "u_n" + juce::String (n + 1) + "_";
            const int bx = n * 300;
            toggle (p + "on", Toggle::Power, {}, { bx + 10, 8, 16, 16 });
            add<SoundLed> ({ bx + 30, 9, 14, 14 }, ctx, cs::MeterUtilNoise1 + n, "Noise " + juce::String (n + 1));
            title ({ bx + 46, 8, 100, 16 }, "Noise " + juce::String (n + 1));
            auto& sample = pill (p + "type", { bx + 10, 32, 278, 20 });
            const int slot = ChurchSynthProcessor::Noise1 + n;
            sample.displayName = [this, slot] (int) { return ctx.proc.getSlotDisplayName (slot); };
            sample.extraMenu = [] (juce::PopupMenu& m) {
                m.addItem (10001, "Load your own sample...");
                m.addItem (10002, "Back to the factory sample");
            };
            sample.onExtra = [this, slot] (int r) {
                if (r == 10001)
                    chooseFile (slot);
                else
                    ctx.proc.clearUserFile (slot);
            };
            samples[n] = &sample;
            pill (p + "phase", { bx + 10, 60, 80, 18 }).setTooltip ("Sample start: from the beginning (Key) or a random point");
            toggle (p + "kbd", Toggle::TextPill, "kbd", { bx + 96, 60, 40, 18 }).setTooltip ("Pitch follows the keyboard");
            toggle (p + "loop", Toggle::TextPill, "loop", { bx + 142, 60, 44, 18 });
            knob (p + "tune", "Tune", { bx + 6, 92, 56, 66 });
            knob (p + "filter", "LP / HP", { bx + 64, 92, 56, 66 }).setTooltip ("Tone filter: left = low-pass, right = high-pass");
            knob (p + "len", "Length", { bx + 122, 92, 56, 66 });
            knob (p + "fmix", "F1 / F2", { bx + 180, 92, 56, 66 });
            knob (p + "vol", "Volume", { bx + 238, 92, 56, 66 });
        }
        const int bx = 604;
        toggle ("u_osc_on", Toggle::Power, {}, { bx + 6, 8, 16, 16 });
        add<SoundLed> ({ bx + 26, 9, 14, 14 }, ctx, cs::MeterUtilOsc, "the Utility oscillator");
        title ({ bx + 42, 8, 120, 16 }, "Oscillator");
        add<OscDisplay> ({ bx + 6, 32, 164, 44 }, ctx, "u_osc_wave", "u_osc_width", "u_osc_vol");
        add<WaveSelector> ({ bx + 6, 78, 164, 20 }, ctx, "u_osc_wave");
        knob ("u_osc_coarse", "Coarse", { bx + 2, 104, 56, 66 }).setFormatter ([] (float v) { return juce::String ((int) std::lround (v)) + " st"; });
        knob ("u_osc_fine", "Fine", { bx + 60, 104, 56, 66 });
        uWidth = &knob ("u_osc_width", "Width", { bx + 118, 104, 56, 66 });
        knob ("u_osc_vol", "Volume", { bx + 2, 176, 56, 66 });
        knob ("u_osc_fmix", "F1 / F2", { bx + 60, 176, 56, 66 });
        toggle ("u_osc_kbd", Toggle::TextPill, "kbd", { bx + 122, 184, 44, 16 });
        caption ({ bx + 4, 252, 60, 14 }, "Output");
        pill ("u_osc_route", { bx + 64, 250, 104, 18 }).setTooltip ("Through the filters, or straight to an FX bus / the output");
        caption ({ bx + 4, 278, 60, 14 }, "Quantize");
        add<NoteMask> ({ bx + 64, 274, 104, 24 }, ctx, "u_osc_q_mask");
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (col::border);
        g.drawVerticalLine (298, 6.0f, (float) getHeight() - 6.0f);
        g.drawVerticalLine (598, 6.0f, (float) getHeight() - 6.0f);
        paintCaptions (g);
    }

    void refreshView() override
    {
        ControlPanel::refreshView();
        for (auto* s : samples)
            s->repaint();
        const int w = ctx.intValue ("u_osc_wave");
        uWidth->setEnabled (w == cs::WaveSquare || w == cs::WaveTriangle);
    }

private:
    Pill* samples[2] {};
    Knob* uWidth = nullptr;
    std::unique_ptr<juce::FileChooser> chooser;

    void chooseFile (int slot)
    {
        chooser = std::make_unique<juce::FileChooser> ("Choose a noise / texture sample", ChurchSynthProcessor::userFolder (false),
                                                       "*.wav;*.aif;*.aiff;*.flac");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this, slot] (const juce::FileChooser& fc) {
                                  const auto f = fc.getResult();
                                  juce::String err;
                                  if (f != juce::File() && ! ctx.proc.importUserFile (slot, f, err))
                                      juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Church Synth", err);
                              });
    }
};

// Tabs: ENGINE 1 | ENGINE 2 | UTILITY, each with a power button.
class SynthPage::EngineTabs final : public juce::Component, public Refreshable
{
public:
    explicit EngineTabs (UiContext& c) : ctx (c)
    {
        for (int e = 0; e < 2; ++e)
        {
            analog[e] = std::make_unique<AnalogView> (ctx, e);
            wavetable[e] = std::make_unique<WavetableView> (ctx, e);
            addChildComponent (*analog[e]);
            addChildComponent (*wavetable[e]);
            power[e] = std::make_unique<Toggle> (ctx, "e" + juce::String (e + 1) + "_on", Toggle::Power);
            type[e] = std::make_unique<Pill> (ctx, "e" + juce::String (e + 1) + "_type");
            type[e]->textColour = col::text;
            addAndMakeVisible (*power[e]);
            addAndMakeVisible (*type[e]);
        }
        utility = std::make_unique<UtilityView> (ctx);
        addChildComponent (*utility);
        power[2] = std::make_unique<Toggle> (ctx, "u_on", Toggle::Power);
        addAndMakeVisible (*power[2]);
        leds[0] = std::make_unique<SoundLed> (ctx, cs::MeterEngine1, "Engine 1");
        leds[1] = std::make_unique<SoundLed> (ctx, cs::MeterEngine2, "Engine 2");
        leds[2] = std::make_unique<SoundLed> (ctx, cs::MeterUtility, "the Utility engine");
        for (auto& l : leds)
            addAndMakeVisible (*l);
        refreshView();
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        for (int t = 0; t < 3; ++t)
            if (tabRect (t).contains (e.getPosition()))
                show (t);
    }

    int currentTab() const noexcept { return current; }

    void show (int t)
    {
        current = juce::jlimit (0, 2, t);
        refreshView();
        repaint();
    }

    juce::Rectangle<int> tabRect (int t) const
    {
        const int w[3] = { 300, 300, getWidth() - 600 };
        return { t == 0 ? 0 : (t == 1 ? 300 : 600), 0, w[t], 30 };
    }

    void resized() override
    {
        for (int t = 0; t < 3; ++t)
        {
            const auto r = tabRect (t);
            power[t]->setBounds (r.getX() + 10, 8, 15, 15);
            if (t < 2)
                type[t]->setBounds (r.getX() + 130, 6, 110, 19);
            leds[t]->setBounds (t < 2 ? r.getX() + 250 : r.getX() + 160, 8, 14, 14);
        }
        const auto content = getLocalBounds().withTrimmedTop (30);
        for (int e = 0; e < 2; ++e)
        {
            analog[e]->setBounds (content);
            wavetable[e]->setBounds (content);
        }
        utility->setBounds (content);
    }

    void paint (juce::Graphics& g) override
    {
        drawPanel (g, getLocalBounds().toFloat());
        for (int t = 0; t < 3; ++t)
        {
            const auto r = tabRect (t);
            g.setColour (t == current ? col::panel : col::panelLo);
            g.fillRect (r.reduced (1, 0).withTrimmedBottom (t == current ? 0 : 1));
            if (t == current)
            {
                g.setColour (col::gold);
                g.fillRect (r.getX() + 1, 0, r.getWidth() - 2, 2);
            }
            const juce::String name = t < 2 ? "ENGINE " + juce::String (t + 1) + " :" : "UTILITY ENGINE";
            g.setColour (t == current ? col::text : col::textDim);
            g.setFont (font (12.5f, true));
            g.drawText (name, r.getX() + 32, 0, 120, 30, juce::Justification::centredLeft);
        }
        g.setColour (col::border);
        g.drawHorizontalLine (30, 0.0f, (float) getWidth());
    }

    void refreshView() override
    {
        for (int e = 0; e < 2; ++e)
        {
            const bool wt = ctx.intValue ("e" + juce::String (e + 1) + "_type") == cs::EngineWavetable;
            analog[e]->setVisible (current == e && ! wt);
            wavetable[e]->setVisible (current == e && wt);
            if (current == e)
                (wt ? (ControlPanel*) wavetable[e].get() : (ControlPanel*) analog[e].get())->refreshView();
        }
        utility->setVisible (current == 2);
        if (current == 2)
            utility->refreshView();
    }

private:
    UiContext& ctx;
    int current = 0;
    std::unique_ptr<AnalogView> analog[2];
    std::unique_ptr<WavetableView> wavetable[2];
    std::unique_ptr<UtilityView> utility;
    std::unique_ptr<Toggle> power[3];
    std::unique_ptr<Pill> type[2];
    std::unique_ptr<SoundLed> leds[3];
};

// ===========================================================================
// Filters
// ===========================================================================
namespace
{
struct FilterUi
{
    const char* cutoff;
    const char* reso;
    const char* drive;
    const char* a;
    const char* b;
    const char* c;
    juce::StringArray submodes;
};

const FilterUi& filterUi (int type)
{
    static const std::vector<FilterUi> t {
        { "Cutoff", "Resonance", "Drive", nullptr, nullptr, nullptr, {} },                                       // Classic
        { "Cutoff", "Resonance", "Drive", "Spread", "Peaks", nullptr, { "Peak", "Bandpass", "Notch", "LP + HP" } }, // Cluster
        { "Cutoff", "Feedback", "Drive", "Poles", nullptr, nullptr, {} },                                         // Phaser
        { "Shift", "Q", "Drive", "Vowel", "Blend", nullptr, {} },                                                 // Formant
        { "Cutoff", "Resonance", "Drive", "Spread", nullptr, nullptr, { "Lowpass", "Highpass", "Notch", "Bandpass" } }, // Surgeon
        { "Rate", "Resonance", "Drive", "Jitter", "Pre Offset", "Post Offset", { "No filter", "Pre", "Post", "Pre + Post" } }, // LoFi
        { "Pitch", "Gain", "Drive", "Damping", "All-pass", nullptr, { "Lowpass", "Highpass", "Bandpass", "Feed-forward" } }, // Comb
        { "Cutoff", "Resonance", "Drive", nullptr, nullptr, nullptr, { "Diode", "Overdrive", "Soft Clip", "Tape", "Transistor" } }, // Drive
        { "Cutoff", "Bandwidth", "Shaper", "Dry", "Feedback", nullptr, { "Subtle", "Medium", "Hard" } },           // Allpass
        { "Cutoff", "Resonance", "Drive", nullptr, nullptr, nullptr, {} },                                         // Ladder
        { "Cutoff", "Resonance", "Drive", nullptr, nullptr, nullptr, { "Lowpass 12", "Highpass 6" } },             // Sallen-Key
        { "Cutoff", "Resonance", "Drive", nullptr, nullptr, nullptr, { "Lowpass 24", "Lowpass 12" } },             // OTA
        { "Cutoff", "Resonance", "Drive", "Morph", nullptr, nullptr, {} },                                         // SVF Morph
        { "Cutoff", "Resonance", "Drive", nullptr, nullptr, nullptr, { "LP24", "LP12", "BP24", "BP12", "HP24", "HP12", "Notch" } }, // Multimode
        { "Cutoff", "Resonance", "Drive", "Level", "Decay", nullptr, { "LPG", "VCA", "Both" } },                   // LPG
    };
    return t[(size_t) juce::jlimit (0, (int) t.size() - 1, type)];
}
} // namespace

class SynthPage::FilterPanel final : public ControlPanel
{
public:
    FilterPanel (UiContext& c, int filter) : ControlPanel (c), f (filter)
    {
        const juce::String p = "f" + juce::String (f + 1) + "_";
        toggle (p + "on", Toggle::Power, {}, { 10, 8, 15, 15 });
        auto& t = pill (p + "type", { 104, 5, 140, 20 });
        t.arrows = true;
        t.textColour = col::text;
        t.menuName = [] (int i) {
            static const char* const kDesc[] = {
                "Classic  -  clean multimode (LP / BP / HP / notch...)",
                "Cluster  -  several resonant peaks",
                "Phaser  -  notches that sweep with cutoff",
                "Formant  -  vowels",
                "Surgeon  -  very steep, precise",
                "LoFi  -  sample-rate reduction",
                "Comb  -  tuned delay (follows the keys)",
                "Drive  -  aggressive saturated filter",
                "Allpass  -  phase ripples",
                "Ladder  -  fat 24 dB transistor ladder (classic mono-synth)",
                "Sallen-Key  -  biting 12 dB, screams with resonance (semi-modular style)",
                "OTA 4-Pole  -  smooth 24 dB (classic polysynth)",
                "SVF Morph  -  morph LP > BP > HP",
                "Multimode  -  24 / 12 dB LP, BP, HP, notch",
                "Lowpass Gate  -  plucky, woody decay",
            };
            return juce::String (kDesc[juce::jlimit (0, 14, i)]);
        };
        // Comb is a tuned delay: when picked, make it follow the keyboard in tune.
        t.onUserChange = [this, p] (int type) {
            if (type == cs::FilterComb)
            {
                ctx.setValue (p + "cutoff", 261.6f);
                ctx.setValue (p + "kbd", 1.0f);
            }
        };
        cutoff = &knob (p + "cutoff", "Cutoff", { 4, 32, 66, 74 });
        reso = &knob (p + "reso", "Resonance", { 72, 34, 58, 66 });
        drive = &knob (p + "drive", "Drive", { 132, 34, 58, 66 });
        knob (p + "kbd", "Kbd", { 6, 110, 46, 56 }).setTooltip ("Cutoff key tracking");
        a = &knob (p + "a", "A", { 52, 110, 46, 56 });
        b = &knob (p + "b", "B", { 98, 110, 46, 56 });
        cc = &knob (p + "c", "C", { 144, 110, 46, 56 });
        add<FilterDisplay> ({ 196, 32, 112, 72 }, ctx, f);
        classicMode = &pill (p + "mode", { 196, 108, 112, 18 });
        sub = &add<NamedIntPill> ({ 196, 108, 112, 18 }, ctx, p + "submode");
        analog = &toggle (p + "analog", Toggle::TextPill, "analog", { 196, 130, 54, 16 });
        analog->setTooltip ("Analog-style nonlinear resonance");
        knob (p + "fm", "FM", { 252, 128, 34, 38 }).setTooltip ("Audio-rate cutoff FM amount");
        fmSrc = &pill (p + "fm_src", { 196, 150, 112, 14 });
        fmSrc->setTooltip ("Filter FM source");
        knob (p + "vol", "Volume", { 312, 30, 54, 64 }).setTooltip ("Filter output: 0 dB at the centre, up to +12 dB");
        knob (p + "pan", "Pan", { 312, 98, 54, 64 });
        cutoff->setFormatter ([] (float v) { return v >= 1000.0f ? juce::String (v / 1000.0f, 2) + " kHz" : juce::String (juce::roundToInt (v)) + " Hz"; });
    }

    void paint (juce::Graphics& g) override
    {
        drawPanel (g, getLocalBounds().toFloat());
        g.setColour (col::text);
        g.setFont (font (12.5f, true));
        g.drawText ("FILTER " + juce::String (f + 1) + " :", 30, 0, 80, 30, juce::Justification::centredLeft);
        paintCaptions (g);
    }

    void refreshView() override
    {
        ControlPanel::refreshView();
        const juce::String p = "f" + juce::String (f + 1) + "_";
        const int type = ctx.intValue (p + "type");
        const auto& ui = filterUi (type);
        cutoff->setLabel (ui.cutoff);
        reso->setLabel (ui.reso);
        drive->setLabel (ui.drive);
        a->setVisible (ui.a != nullptr);
        b->setVisible (ui.b != nullptr);
        cc->setVisible (ui.c != nullptr);
        if (ui.a != nullptr) a->setLabel (ui.a);
        if (ui.b != nullptr) b->setLabel (ui.b);
        if (ui.c != nullptr) cc->setLabel (ui.c);
        classicMode->setVisible (type == cs::FilterClassicType);
        sub->names = ui.submodes;
        sub->setVisible (type != cs::FilterClassicType && ! ui.submodes.isEmpty());
        sub->repaint();
        analog->setVisible (type == cs::FilterClassicType);
    }

private:
    int f;
    Knob *cutoff, *reso, *drive, *a, *b, *cc;
    Pill *classicMode, *fmSrc;
    NamedIntPill* sub;
    Toggle* analog;
};

// ===========================================================================
class SynthPage::RoutingColumn final : public ControlPanel
{
public:
    explicit RoutingColumn (UiContext& c) : ControlPanel (c)
    {
        title ({ 0, 8, 116, 16 }, "Filter Routing");
        knob ("f_routing", "Ser / Par", { 28, 28, 60, 72 }).setFormatter ([] (float v) {
            return v < 0.02f ? juce::String ("series") : v > 0.98f ? juce::String ("parallel") : juce::String (juce::roundToInt (v * 100.0f)) + " % par";
        });
        pill ("f_output", { 8, 102, 100, 18 }).displayName = [] (int i) { return i == 0 ? "sum" : i == 1 ? "split" : "split rev"; };
        title ({ 0, 132, 116, 16 }, "Amp Mod");
        knob ("amp_velo", "Amount", { 28, 150, 60, 66 });
        pill ("amp_mod_src", { 8, 216, 100, 18 });
        knob ("voice_pan", "Voice Pan", { 28, 240, 60, 66 });
    }

    void paint (juce::Graphics& g) override
    {
        drawPanel (g, getLocalBounds().toFloat());
        for (auto& c : captions)
            if (c.title)
                drawSectionTitle (g, c.r, c.text, col::text, juce::Justification::centred);
    }
};

// ===========================================================================
SynthPage::SynthPage (UiContext& ctx)
{
    engines = std::make_unique<EngineTabs> (ctx);
    addAndMakeVisible (*engines);
    for (int f = 0; f < 2; ++f)
    {
        filters[f] = std::make_unique<FilterPanel> (ctx, f);
        addAndMakeVisible (*filters[f]);
    }
    routing = std::make_unique<RoutingColumn> (ctx);
    addAndMakeVisible (*routing);
    refreshView();
}

SynthPage::~SynthPage() = default;

void SynthPage::resized()
{
    engines->setBounds (4, 0, 780, 340);
    filters[0]->setBounds (788, 0, 368, 168);
    filters[1]->setBounds (788, 172, 368, 168);
    routing->setBounds (1160, 0, 116, 340);
}

void SynthPage::paint (juce::Graphics&) {}

void SynthPage::showEngineTab (int t)
{
    engines->show (t);
}

int SynthPage::engineTab() const
{
    return engines->currentTab();
}

juce::Rectangle<int> SynthPage::engineTabBounds (int t) const
{
    return engines->tabRect (t) + engines->getPosition();
}

void SynthPage::refreshView()
{
    engines->refreshView();
    for (auto& f : filters)
        f->refreshView();
    routing->refreshView();
}
} // namespace ui
