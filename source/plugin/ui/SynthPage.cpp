#include "SynthPage.h"
#include "dsp/fx/Effects.h"

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

// Rotary lamp (Organ panel): the speed of the first Rotary effect in the FX rack, live
// (so the mod wheel or a MIDI-learned button can be seen switching it). Click = switch.
class RotaryLamp final : public juce::Component, public juce::SettableTooltipClient, private juce::Timer
{
public:
    explicit RotaryLamp (UiContext& c) : ctx (c)
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
        startTimerHz (15);
        update();
    }
    void paint (juce::Graphics& g) override
    {
        update();
        auto r = getLocalBounds().toFloat();
        const auto lampR = juce::Rectangle<float> (4.0f, 4.0f, 16.0f, 16.0f);
        const juce::Colour fastCol (0xffff7a3d), slowCol (0xff5fd38a);
        if (slot < 0)
        {
            g.setColour (col::border);
            g.drawEllipse (lampR, 1.2f);
            g.setColour (col::textDim);
            g.setFont (font (11.0f, true));
            g.drawText ("Rotary: not in the FX", r.withTrimmedLeft (26.0f).removeFromTop (24.0f), juce::Justification::centredLeft);
            g.setFont (font (10.0f));
            g.drawFittedText ("Add a Rotary on the FX page", r.withTrimmedTop (24.0f).toNearestInt(), juce::Justification::centredLeft, 1);
            return;
        }
        const auto c = state == 2 ? col::textDim : state == 1 ? fastCol : slowCol;
        if (state == 1)
        {
            g.setColour (c.withAlpha (0.25f));
            g.fillEllipse (lampR.expanded (4.0f));
        }
        g.setColour (state == 2 ? col::panelLo : c);
        g.fillEllipse (lampR);
        g.setColour (c.brighter (0.3f));
        g.drawEllipse (lampR, 1.2f);
        g.setColour (state == 2 ? col::textDim : col::text);
        g.setFont (font (12.0f, true));
        g.drawText (juce::String ("Rotary: ") + (state == 2 ? "Stopped" : state == 1 ? "Fast" : "Slow"),
                    r.withTrimmedLeft (26.0f).removeFromTop (24.0f), juce::Justification::centredLeft);
        g.setColour (col::textDim);
        g.setFont (font (10.0f));
        g.drawFittedText (wheelRouted ? "Mod wheel past half = Fast" : "Click to switch", r.withTrimmedTop (24.0f).toNearestInt(),
                          juce::Justification::centredLeft, 1, 0.8f);
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (slot < 0 || e.mods.isPopupMenu())
            return;
        const auto id = speedId();
        ctx.setValue (id, ctx.value (id) >= 0.5f ? 0.0f : 1.0f);
        ctx.proc.checkpoint ("Rotary speed");
        if (update())
            repaint();
    }

private:
    UiContext& ctx;
    int slot = -1, shownSlot = -2, state = -1;
    bool wheelRouted = false;

    static const char* slotId (int s)
    {
        static const char* const ids[9] = { "a1", "a2", "a3", "b1", "b2", "b3", "x1", "x2", "x3" };
        return ids[s];
    }
    juce::String speedId() const { return juce::String ("fx_") + slotId (slot) + "_p1"; }
    void timerCallback() override
    {
        if (visibleInEditor (*this) && update())
            repaint();
    }
    bool update() // true when what's shown changed
    {
        int s = -1;
        for (int i = 0; i < 9 && s < 0; ++i)
            if (ctx.intValue (juce::String ("fx_") + slotId (i) + "_type") == cs::fx::FxRotary)
                s = i;
        int st = -1;
        bool wr = false;
        if (s >= 0)
        {
            slot = s;
            const bool brake = ctx.modulatedValue (juce::String ("fx_") + slotId (s) + "_p9") >= 0.5f;
            st = brake ? 2 : ctx.modulatedValue (speedId()) >= 0.5f ? 1 : 0;
            const int d = cs::findModDest (speedId().toStdString());
            wr = d >= 0 && ctx.hasRoute (cs::SrcModWheel, d);
        }
        const bool changed = s != shownSlot || st != state || wr != wheelRouted;
        slot = shownSlot = s;
        state = st;
        wheelRouted = wr;
        if (changed)
            setTooltip (slot < 0 ? "No Rotary effect in the FX rack"
                                 : "The Rotary effect's speed, live. Click to switch Slow/Fast. "
                                   "With the mod wheel connected (the organ presets), past halfway is Fast. "
                                   "For a button or footswitch: FX page, right-click Speed > MIDI learn");
        return changed;
    }
};

// Organ (tonewheel) engine: drawbars in the middle, wheels / click / percussion on the right.
class OrganView final : public ControlPanel
{
public:
    OrganView (UiContext& c, int engine) : ControlPanel (c), e (engine)
    {
        const juce::String x = "e" + juce::String (e + 1) + "_", o = x + "org_";
        const int meterBase = e == 0 ? cs::MeterE1Osc1 : cs::MeterE2Osc1;

        // Tune + output (no unison: one tone per key, like the real organ).
        title ({ 12, 6, 80, 16 }, "Tune");
        knob (x + "coarse", "Coarse", { 10, 24, 64, 70 }).setFormatter ([] (float v) { return juce::String ((int) std::lround (v)) + " st"; });
        toggle (x + "kbd", Toggle::TextPill, "kbd", { 82, 30, 40, 16 }).setTooltip ("Key tracking (off = fixed pitch)");
        knob (x + "fine", "Fine", { 6, 98, 54, 60 });
        title ({ 12, 170, 80, 16 }, "Output");
        knob (x + "vol", "Volume", { 6, 190, 56, 66 }).setTooltip ("Organ volume. The swell pedal is Expression (CC11), via Amp Mod in the presets");
        knob (x + "fmix", "F1 / F2", { 66, 190, 56, 66 }).setTooltip ("Filter mix: how much goes to Filter 1 vs Filter 2");
        add<RotaryLamp> ({ 4, 262, 126, 40 }, ctx);

        // Drawbars: brown = sub-harmonics, white = octaves, black = the other harmonics.
        add<SoundLed> ({ 144, 9, 14, 14 }, ctx, meterBase + 0, "the tonewheels (drawbars)");
        title ({ 160, 8, 120, 16 }, "Drawbars");
        auto& reg = add<FlatButton> ({ 400, 5, 120, 20 }, "Registrations...");
        reg.setTooltip ("Classic drawbar settings to start from");
        reg.onClick = [this, &reg] { showRegistrations (reg); };
        static const char* const kFeet[cs::kNumDrawbars] = { "16'", "5 1/3'", "8'", "4'", "2 2/3'", "2'", "1 3/5'", "1 1/3'", "1'" };
        const juce::Colour brown (0xff8a4b2a), white (0xffe6e1d6), black (0xff26262a);
        const juce::Colour heads[cs::kNumDrawbars] = { brown, brown, white, white, black, white, black, black, white };
        for (int d = 0; d < cs::kNumDrawbars; ++d)
            knob (o + "db" + juce::String (d + 1), kFeet[d], { 146 + d * 41, 30, 40, 258 }).setDrawbar (heads[d]);
        caption ({ 146, 290, 380, 14 }, "Drag a bar down to pull it out (8 = full, 0 = off). Shift = fine");

        const int rx = 534;
        title ({ rx + 6, 6, 100, 16 }, "Tonewheels");
        knob (o + "char", "Character", { rx + 2, 26, 56, 66 }).setTooltip ("Clean sine wheels (0) to old, slightly gritty wheels (1)");
        knob (o + "leak", "Leakage", { rx + 60, 26, 56, 66 });
        pill (o + "tuning", { rx + 122, 30, 116, 18 }).setTooltip ("Tonewheel gear-ratio tuning or equal temperament");
        toggle (o + "bass", Toggle::TextPill, "bass all the way down", { rx + 122, 56, 116, 18 })
            .setTooltip ("Off: the 16' folds up an octave in the lowest octave");

        add<SoundLed> ({ rx + 4, 105, 14, 14 }, ctx, meterBase + 3, "the key click");
        title ({ rx + 20, 104, 100, 16 }, "Key Click");
        knob (o + "click", "Click", { rx + 2, 124, 56, 66 });
        knob (o + "click_tone", "Tone", { rx + 60, 124, 56, 66 });
        caption ({ rx + 122, 132, 118, 14 }, "Each drawbar contact");
        caption ({ rx + 122, 146, 118, 14 }, "closes at its own time.");

        toggle (o + "perc", Toggle::Power, {}, { rx + 6, 207, 16, 16 }).setTooltip ("Percussion on/off");
        add<SoundLed> ({ rx + 26, 208, 14, 14 }, ctx, meterBase + 1, "the percussion");
        title ({ rx + 42, 207, 100, 16 }, "Percussion");
        percControls.push_back (&pill (o + "perc_harm", { rx + 6, 232, 74, 18 }));
        percControls.push_back (&pill (o + "perc_decay", { rx + 84, 232, 74, 18 }));
        percControls.push_back (&pill (o + "perc_vol", { rx + 162, 232, 74, 18 }));
        percControls.push_back (&toggle (o + "perc_cut", Toggle::TextPill, "mutes the 1'", { rx + 6, 258, 100, 18 }));
        caption ({ rx + 6, 284, 236, 14 }, "Plays only from all keys up: legato won't retrigger it");
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (col::border);
        g.drawVerticalLine (132, 6.0f, (float) getHeight() - 6.0f);
        g.drawHorizontalLine (162, 6.0f, 126.0f);
        g.drawVerticalLine (528, 6.0f, (float) getHeight() - 6.0f);
        g.drawHorizontalLine (98, 534.0f, (float) getWidth() - 8.0f);
        g.drawHorizontalLine (200, 534.0f, (float) getWidth() - 8.0f);
        paintCaptions (g);
    }

    void refreshView() override
    {
        ControlPanel::refreshView();
        const bool perc = ctx.intValue ("e" + juce::String (e + 1) + "_org_perc") != 0;
        for (auto* c : percControls)
            c->setAlpha (perc ? 1.0f : 0.45f);
    }

private:
    int e;
    std::vector<juce::Component*> percControls;

    void showRegistrations (juce::Component& target)
    {
        // Common starting registrations (16' 5 1/3' 8' 4' 2 2/3' 2' 1 3/5' 1 1/3' 1').
        struct Reg { const char* name; const char* bars; };
        static const Reg kRegs[] = {
            { "Jazz Trio (888 000 000)", "888000000" },
            { "Full Gospel (888 800 008)", "888800008" },
            { "Soft Ballad (008 500 000)", "008500000" },
            { "Warm Worship (838 000 000)", "838000000" },
            { "Flutes (006 040 000)", "006040000" },
            { "Bright Lead (800 000 888)", "800000888" },
            { "Sweet Choir (006 876 540)", "006876540" },
            { "Rock Grind (888 600 000)", "888600000" },
            { "All Out (888 888 888)", "888888888" },
        };
        juce::PopupMenu m;
        for (int i = 0; i < (int) std::size (kRegs); ++i)
            m.addItem (i + 1, kRegs[i].name);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&target),
                         [this, safe = juce::Component::SafePointer<OrganView> (this)] (int r) {
                             if (safe == nullptr || r <= 0)
                                 return;
                             const char* bars = kRegs[r - 1].bars;
                             for (int d = 0; d < cs::kNumDrawbars; ++d)
                                 ctx.setValue ("e" + juce::String (e + 1) + "_org_db" + juce::String (d + 1), (float) (bars[d] - '0'));
                         });
    }
};

// Seating diagram for the Section engine: one dot per player on a stage arc, placed
// by Players / Width / Position (the same seats the engine uses, less the shuffle).
class SeatingView final : public juce::Component, public Refreshable
{
public:
    SeatingView (UiContext& c, int engine) : ctx (c), e (engine) {}
    void refreshView() override
    {
        const juce::String x = "e" + juce::String (e + 1) + "_sec_";
        const int np = juce::jlimit (1, 8, ctx.intValue (x + "players"));
        const float w = ctx.value (x + "width"), pos = ctx.value (x + "pos");
        const bool brass = ctx.intValue (x + "family") == cs::SectionBrass;
        const auto key = juce::String (np) + "/" + juce::String (w, 2) + "/" + juce::String (pos, 2) + (brass ? "b" : "s");
        if (key != lastKey)
        {
            lastKey = key;
            players = np;
            width = w;
            position = pos;
            isBrass = brass;
            repaint();
        }
    }
    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (6.0f, 4.0f);
        g.setColour (col::panelLo);
        g.fillRoundedRectangle (r, 6.0f);
        const float cx = r.getCentreX(), base = r.getBottom() - 8.0f, rad = r.getWidth() * 0.42f;
        juce::Path arc;
        arc.addCentredArc (cx, base, rad, rad * 0.55f, 0.0f, -juce::MathConstants<float>::halfPi, juce::MathConstants<float>::halfPi, true);
        g.setColour (col::border);
        g.strokePath (arc, juce::PathStrokeType (1.0f));
        g.setColour (col::textDim);
        g.setFont (font (10.0f));
        g.drawText ("L", juce::Rectangle<float> (r.getX() + 4.0f, base - 14.0f, 12.0f, 12.0f), juce::Justification::centred);
        g.drawText ("R", juce::Rectangle<float> (r.getRight() - 16.0f, base - 14.0f, 12.0f, 12.0f), juce::Justification::centred);
        g.drawText ("you", juce::Rectangle<float> (cx - 20.0f, base - 4.0f, 40.0f, 12.0f), juce::Justification::centred);
        for (int k = 0; k < players; ++k)
        {
            const float sNorm = players == 1 ? 0.0f : 2.0f * (float) k / (float) (players - 1) - 1.0f;
            const float pan = juce::jlimit (-1.0f, 1.0f, position + width * 0.85f * sNorm);
            const float ang = pan * juce::MathConstants<float>::halfPi * 0.95f;
            const float px = cx + rad * std::sin (ang), py = base - rad * 0.55f * std::cos (ang);
            g.setColour ((isBrass ? col::gold : col::accent).withAlpha (0.9f));
            g.fillEllipse (px - 5.0f, py - 5.0f, 10.0f, 10.0f);
        }
        g.setColour (col::textDim);
        g.drawText (juce::String (players) + (players == 1 ? " player" : " players"), r.removeFromTop (14.0f).reduced (6.0f, 0.0f),
                    juce::Justification::centredLeft);
    }

private:
    UiContext& ctx;
    int e, players = 6;
    float width = 0.7f, position = 0.0f;
    bool isBrass = false;
    juce::String lastKey;
};

// Section (ensemble strings / brass) engine.
class SectionView final : public ControlPanel
{
public:
    SectionView (UiContext& c, int engine) : ControlPanel (c), e (engine)
    {
        const juce::String x = "e" + juce::String (e + 1) + "_", s = x + "sec_";
        const int meterBase = e == 0 ? cs::MeterE1Osc1 : cs::MeterE2Osc1;

        title ({ 12, 6, 80, 16 }, "Tune");
        knob (x + "coarse", "Coarse", { 10, 24, 64, 70 }).setFormatter ([] (float v) { return juce::String ((int) std::lround (v)) + " st"; });
        toggle (x + "kbd", Toggle::TextPill, "kbd", { 82, 30, 40, 16 }).setTooltip ("Key tracking (off = fixed pitch)");
        knob (x + "fine", "Fine", { 6, 98, 54, 60 });
        title ({ 12, 170, 80, 16 }, "Output");
        knob (x + "vol", "Volume", { 6, 190, 56, 66 });
        knob (x + "fmix", "F1 / F2", { 66, 190, 56, 66 }).setTooltip ("Filter mix: how much goes to Filter 1 vs Filter 2");

        // Players
        add<SoundLed> ({ 142, 9, 14, 14 }, ctx, meterBase + 0, "the players");
        title ({ 158, 8, 80, 16 }, "Players");
        pill (s + "family", { 232, 5, 96, 18 }).setTooltip ("Strings or Brass");
        knob (s + "players", "Players", { 138, 26, 58, 68 });
        knob (s + "into", "Intonation", { 196, 26, 58, 68 });
        knob (s + "loose", "Looseness", { 254, 26, 58, 68 });
        auto& size = add<FlatButton> ({ 140, 98, 70, 18 }, "Size...");
        size.setTooltip ("Solo, chamber, section or full");
        size.onClick = [this, &size] { showSizes (size); };
        knob (s + "width", "Width", { 196, 96, 58, 66 });
        knob (s + "pos", "Position", { 254, 96, 58, 66 });
        add<SeatingView> ({ 136, 168, 196, 120 }, ctx, e);

        // Dynamics and vibrato
        title ({ 344, 8, 120, 16 }, "Dynamics");
        knob (s + "dyn", "Dynamics", { 338, 24, 70, 76 }).setTooltip ("Soft to loud: level, brightness and vibrato together (mod wheel in the presets)");
        knob (s + "bright", "Brightness", { 410, 26, 58, 68 });
        add<SoundLed> ({ 474, 9, 14, 14 }, ctx, meterBase + 3, "the bow / breath noise");
        knob (s + "noise", "Noise", { 468, 26, 58, 68 });
        title ({ 344, 106, 120, 16 }, "Vibrato");
        knob (s + "vib_depth", "Depth", { 338, 124, 58, 68 });
        knob (s + "vib_rate", "Rate", { 396, 124, 58, 68 });
        knob (s + "vib_delay", "Delay", { 454, 124, 58, 68 }).setFormatter ([] (float v) { return juce::String (v, 2) + " s"; });
        caption ({ 340, 200, 190, 14 }, "Each player has their own vibrato,");
        caption ({ 340, 214, 190, 14 }, "speed and tuning around these.");

        // Attack and body
        title ({ 546, 8, 120, 16 }, "Attack");
        knob (s + "attack", "Attack", { 538, 26, 58, 68 });
        biteKnob = &knob (s + "bite", "Bite", { 596, 26, 58, 68 });
        scoopKnob = &knob (s + "scoop", "Scoop", { 654, 26, 58, 68 });
        add<SoundLed> ({ 542, 107, 14, 14 }, ctx, meterBase + 1, "the body / bell resonance");
        title ({ 560, 106, 120, 16 }, "Body");
        pill (s + "body", { 542, 130, 110, 18 }).setTooltip ("Instrument body or bell: fixed resonances, like the wood of a violin or the bell of a horn");
        knob (s + "body_mix", "Body Mix", { 654, 118, 58, 68 });
        caption ({ 540, 200, 230, 14 }, "Vibrato moves the harmonics through the");
        caption ({ 540, 214, 230, 14 }, "body's fixed peaks: that shimmer is the");
        caption ({ 540, 228, 230, 14 }, "difference from a plain synth string.");
        caption ({ 340, 262, 430, 14 }, "Legato (Play Mode: Mono Legato) slurs between notes. Dynamics on the mod wheel, as orchestral libraries do.");
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (col::border);
        g.drawVerticalLine (132, 6.0f, (float) getHeight() - 6.0f);
        g.drawHorizontalLine (162, 6.0f, 126.0f);
        g.drawVerticalLine (334, 6.0f, 250.0f);
        g.drawVerticalLine (532, 6.0f, 250.0f);
        g.drawHorizontalLine (100, 338.0f, 528.0f);
        g.drawHorizontalLine (100, 536.0f, (float) getWidth() - 8.0f);
        paintCaptions (g);
    }

    void refreshView() override
    {
        ControlPanel::refreshView();
        const int brass = ctx.intValue ("e" + juce::String (e + 1) + "_sec_family") == cs::SectionBrass ? 1 : 0;
        if (brass == shownFamily)
            return;
        shownFamily = brass;
        biteKnob->setLabel (brass ? "Blat" : "Bite");
        scoopKnob->setAlpha (brass ? 1.0f : 0.4f);
    }

private:
    int e;
    Knob* biteKnob = nullptr;
    Knob* scoopKnob = nullptr;
    int shownFamily = -1;

    void showSizes (juce::Component& target)
    {
        juce::PopupMenu m;
        m.addItem (1, "Solo (1)");
        m.addItem (3, "Chamber (3)");
        m.addItem (6, "Section (6)");
        m.addItem (8, "Full (8)");
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&target),
                         [this, safe = juce::Component::SafePointer<SectionView> (this)] (int r) {
                             if (safe != nullptr && r > 0)
                                 ctx.setValue ("e" + juce::String (e + 1) + "_sec_players", (float) r);
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
            organ[e] = std::make_unique<OrganView> (ctx, e);
            section[e] = std::make_unique<SectionView> (ctx, e);
            addChildComponent (*analog[e]);
            addChildComponent (*wavetable[e]);
            addChildComponent (*organ[e]);
            addChildComponent (*section[e]);
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
            organ[e]->setBounds (content);
            section[e]->setBounds (content);
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
            const int engineType = ctx.intValue ("e" + juce::String (e + 1) + "_type");
            ControlPanel* views[4] = { analog[e].get(), wavetable[e].get(), organ[e].get(), section[e].get() };
            const int shown = juce::jlimit (0, 3, engineType);
            for (int k = 0; k < 4; ++k)
                views[k]->setVisible (current == e && k == shown);
            if (current == e)
                views[shown]->refreshView();
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
    std::unique_ptr<OrganView> organ[2];
    std::unique_ptr<SectionView> section[2];
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
