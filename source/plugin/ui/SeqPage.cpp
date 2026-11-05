#include "SeqPage.h"
#include <cstring>

#include "../PresetManager.h"

namespace ui
{
namespace
{
constexpr int kGridX = 268;    // grid starts here (left column before)
constexpr int kHeaderW = 230;  // track header column inside the grid
constexpr int kTop = 24;       // header row
constexpr int kLaneHeights[SeqPage::NumLanes] = { 26, 28, 64, 40, 42, 42, 36, 18 };
const char* const kLaneNames[SeqPage::NumLanes] = { "ON / OFF", "PROBABILITY", "PITCH", "VELOCITY", "OCTAVE", "GATE LENGTH", "SLIDE", "LOCK" };
constexpr int kLaneTrack[SeqPage::NumLanes] = { cs::TrackSteps, cs::TrackSteps, cs::TrackPitch, cs::TrackVelocity,
                                                cs::TrackOctave, cs::TrackGate, cs::TrackSlide, -1 };
const juce::Colour kOn (0xff2fc6b8), kGhost (0xfff0b44c);
} // namespace

// ===========================================================================
class SeqPage::Lane final : public juce::Component, public juce::SettableTooltipClient
{
public:
    Lane (SeqPage& o, LaneKind k) : owner (o), kind (k)
    {
        setTooltip (k == LaneLock ? "Lock: the step keeps its random values on Regen"
                                  : "Drag to draw. Shift-drag to scale the whole lane.");
    }

    void paint (juce::Graphics& g) override
    {
        const auto& pat = owner.pat;
        const auto& sp = owner.seqParams;
        const float w = (float) getWidth() / (float) cs::kSeqSteps, h = (float) getHeight();
        const int track = kLaneTrack[kind];
        const bool enabled = track < 0 || pat.tracks[(size_t) track].enabled;
        const int trackLen = track >= 0 && sp.polymetry ? pat.tracks[(size_t) track].length : sp.length;
        const uint32_t seed = owner.shownSeed;
        for (int c = 0; c < cs::kSeqSteps; ++c)
        {
            const auto& st = pat.steps[(size_t) c];
            auto cell = juce::Rectangle<float> ((float) c * w, 0.0f, w, h).reduced (2.0f, 1.5f);
            const bool inRange = kind == LaneLock || c < trackLen;
            g.setColour (c == owner.playStep ? col::panelHi.brighter (0.12f) : col::panelLo);
            g.fillRoundedRectangle (cell, 3.0f);
            if (! inRange)
            {
                g.setColour (juce::Colours::black.withAlpha (0.45f));
                g.fillRoundedRectangle (cell, 3.0f);
                continue;
            }
            const auto colour = (enabled ? kOn : col::textFaint).withAlpha (st.on || kind == LaneOnOff ? 1.0f : 0.4f);
            int cols[cs::NumSeqTracks];
            for (auto& x : cols)
                x = c;
            const auto r = cs::seqResolve (pat, sp, cols, seed);
            float frac = 0.0f, ghost = -1.0f;
            juce::String text;
            switch (kind)
            {
                case LaneOnOff:
                    if (st.on)
                    {
                        g.setColour (colour);
                        g.fillRoundedRectangle (cell.reduced (3.0f, 4.0f), 2.0f);
                    }
                    if (r.on != st.on)
                    {
                        g.setColour (kGhost);
                        g.drawRoundedRectangle (cell.reduced (2.0f), 3.0f, 1.5f);
                    }
                    continue;
                case LaneLock:
                    g.setColour (st.lockSeed != 0 ? kGhost : col::border);
                    g.fillEllipse (cell.withSizeKeepingCentre (7.0f, 7.0f));
                    continue;
                case LaneProb: frac = st.prob; text = juce::String (juce::roundToInt (st.prob * 100.0f)); break;
                case LanePitch: frac = (float) st.pitch / 12.0f; text = juce::String (st.pitch); ghost = (float) r.pitch / 12.0f; break;
                case LaneVelocity: frac = st.velocity; ghost = r.velocity; break;
                case LaneOctave:
                {
                    const float cy = cell.getCentreY(), half = cell.getHeight() * 0.5f - 2.0f;
                    const float y = cy - (float) st.octave / 2.0f * half;
                    g.setColour (colour);
                    g.fillRect (juce::Rectangle<float>::leftTopRightBottom (cell.getX() + 3.0f, std::min (y, cy), cell.getRight() - 3.0f, std::max (y, cy) + 1.5f));
                    g.setColour (col::text.withAlpha (0.8f));
                    g.setFont (font (10.0f));
                    g.drawText ((st.octave > 0 ? "+" : "") + juce::String (st.octave), cell, juce::Justification::centredRight);
                    if (r.octave != st.octave)
                    {
                        g.setColour (kGhost);
                        g.fillRect (cell.getX() + 4.0f, cy - (float) r.octave / 2.0f * half - 1.0f, cell.getWidth() - 8.0f, 2.0f);
                    }
                    continue;
                }
                case LaneGate:
                    frac = std::sqrt (st.gate / 4.0f);
                    text = juce::String (juce::roundToInt (st.gate * 100.0f));
                    ghost = std::sqrt (r.gate / 4.0f);
                    break;
                case LaneSlide:
                    frac = st.slide;
                    text = st.slide > 0.0f ? juce::String (juce::roundToInt (cs::seqSlideSeconds (st.slide) * 1000.0f)) : juce::String();
                    break;
                case NumLanes:
                default: break;
            }
            const float barH = cell.getHeight() * juce::jlimit (0.0f, 1.0f, frac);
            g.setColour (colour.withAlpha (colour.getFloatAlpha() * 0.85f));
            g.fillRoundedRectangle (cell.withTop (cell.getBottom() - barH), 2.0f);
            if (ghost >= 0.0f && std::abs (ghost - frac) > 1.0e-3f)
            {
                g.setColour (kGhost);
                g.fillRect (cell.getX() + 4.0f, cell.getBottom() - cell.getHeight() * juce::jlimit (0.0f, 1.0f, ghost) - 1.0f, cell.getWidth() - 8.0f, 2.0f);
            }
            if (text.isNotEmpty())
            {
                g.setColour (col::text.withAlpha (0.8f));
                g.setFont (font (10.0f));
                g.drawText (text, cell.removeFromTop (12.0f), juce::Justification::centred);
            }
        }
        if (kind == LaneGate)
        {
            g.setColour (juce::Colours::white.withAlpha (0.15f));
            g.drawHorizontalLine ((int) (h - h * 0.5f), 0.0f, (float) getWidth());
        }
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        owner.editing = true;
        scaling = e.mods.isShiftDown() && kind != LaneOnOff && kind != LaneLock;
        before = owner.pat;
        if (! scaling)
            edit (e, true);
    }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (scaling)
            scale (e);
        else
            edit (e, false);
    }
    void mouseUp (const juce::MouseEvent&) override
    {
        owner.editing = false;
        owner.commit (juce::String ("Seq ") + kLaneNames[kind]);
    }

private:
    SeqPage& owner;
    LaneKind kind;
    bool paintOn = true, scaling = false;
    cs::SeqPattern before;

    // Shift-drag: scale the whole lane up or down (relative to its values when the drag started).
    void scale (const juce::MouseEvent& e)
    {
        const float f = std::exp2 ((float) -e.getDistanceFromDragStartY() / 60.0f);
        for (int c = 0; c < cs::kSeqSteps; ++c)
        {
            const auto& b = before.steps[(size_t) c];
            auto& st = owner.pat.steps[(size_t) c];
            switch (kind)
            {
                case LaneProb: st.prob = juce::jlimit (0.0f, 1.0f, b.prob * f); break;
                case LanePitch: st.pitch = juce::jlimit (0, 12, juce::roundToInt ((float) b.pitch * f)); break;
                case LaneVelocity: st.velocity = juce::jlimit (0.0f, 1.0f, b.velocity * f); break;
                case LaneOctave: st.octave = juce::jlimit (-2, 2, juce::roundToInt ((float) b.octave * f)); break;
                case LaneGate: st.gate = juce::jlimit (0.05f, 4.0f, b.gate * f); break;
                case LaneSlide: st.slide = b.slide > 0.0f ? juce::jlimit (0.08f, 1.0f, b.slide * f) : 0.0f; break;
                case LaneOnOff:
                case LaneLock:
                case NumLanes:
                default: break;
            }
        }
        owner.ctx.proc.setPattern (owner.pat);
        owner.seenVersion = owner.ctx.proc.patternVersion();
        repaint();
    }

    void edit (const juce::MouseEvent& e, bool first)
    {
        const int c = juce::jlimit (0, cs::kSeqSteps - 1, e.x * cs::kSeqSteps / juce::jmax (1, getWidth()));
        auto& st = owner.pat.steps[(size_t) c];
        const float v = juce::jlimit (0.0f, 1.0f, 1.0f - (float) e.y / juce::jmax (1.0f, (float) getHeight()));
        switch (kind)
        {
            case LaneOnOff:
                if (first) paintOn = ! st.on;
                st.on = paintOn;
                break;
            case LaneLock:
                if (first) paintOn = st.lockSeed == 0;
                st.lockSeed = paintOn ? owner.ctx.proc.getSeq().uiSeed.load() : 0u;
                break;
            case LaneProb: st.prob = std::round (v * 20.0f) / 20.0f; break;
            case LanePitch: st.pitch = juce::jlimit (0, 12, juce::roundToInt (v * 12.0f)); break;
            case LaneVelocity: st.velocity = v; break;
            case LaneOctave: st.octave = juce::jlimit (-2, 2, juce::roundToInt ((v - 0.5f) * 4.0f)); break;
            case LaneGate: st.gate = juce::jlimit (0.05f, 4.0f, v * v * 4.0f); break;
            case LaneSlide: st.slide = v < 0.08f ? 0.0f : v; break;
            case NumLanes:
            default: break;
        }
        owner.ctx.proc.setPattern (owner.pat);
        owner.seenVersion = owner.ctx.proc.patternVersion();
        repaint();
    }
};

// ===========================================================================
class SeqPage::TrackHeader final : public juce::Component
{
public:
    TrackHeader (SeqPage& o, int t) : owner (o), track (t)
    {
        on.setClickingTogglesState (true);
        on.setTooltip ("Track on/off");
        on.onClick = [this] { owner.pat.tracks[(size_t) track].enabled = on.getToggleState(); owner.commit ("Seq track on/off"); };
        random.setSliderStyle (juce::Slider::LinearBar);
        random.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        random.setRange (0.0, 1.0, 0.01);
        random.setTooltip ("Random amount (Regen picks new random values)");
        random.onValueChange = [this] { owner.pat.tracks[(size_t) track].random = (float) random.getValue(); owner.ctx.proc.setPattern (owner.pat); owner.seenVersion = owner.ctx.proc.patternVersion(); repaint(); };
        random.onDragEnd = [this] { owner.commit ("Seq random"); };
        div.onClick = [this] {
            juce::PopupMenu m;
            for (int d = 0; d < cs::kNumSeqDividers; ++d)
                m.addItem (d + 1, d == 0 ? juce::String ("off") : "/" + juce::String (cs::kSeqDividers[d]), true,
                           owner.pat.tracks[(size_t) track].divider == cs::kSeqDividers[d]);
            m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&div), [this] (int r) {
                if (r > 0)
                {
                    owner.pat.tracks[(size_t) track].divider = cs::kSeqDividers[r - 1];
                    owner.commit ("Seq track divider");
                }
            });
        };
        div.setTooltip ("Rate divider: the track moves once every N steps");
        len.onClick = [this] {
            juce::PopupMenu m;
            for (int l = 1; l <= cs::kSeqSteps; ++l)
                m.addItem (l, juce::String (l), true, owner.pat.tracks[(size_t) track].length == l);
            m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&len), [this] (int r) {
                if (r > 0)
                {
                    owner.pat.tracks[(size_t) track].length = r;
                    owner.commit ("Seq track length");
                }
            });
        };
        len.setTooltip ("Track length (used with Polymetry)");
        left.text = "<";
        right.text = ">";
        left.outline = right.outline = false;
        left.setTooltip ("Rotate left");
        right.setTooltip ("Rotate right");
        left.onClick = [this] { cs::seqRotate (owner.pat, track, -1); owner.commit ("Seq rotate"); };
        right.onClick = [this] { cs::seqRotate (owner.pat, track, 1); owner.commit ("Seq rotate"); };
        for (auto* c : std::initializer_list<juce::Component*> { &on, &random, &div, &len, &left, &right })
            addAndMakeVisible (c);
    }
    void update()
    {
        const auto& tr = owner.pat.tracks[(size_t) track];
        on.setToggleState (tr.enabled, juce::dontSendNotification);
        random.setValue (tr.random, juce::dontSendNotification);
        div.text = tr.divider == 1 ? juce::String ("off") : "/" + juce::String (tr.divider);
        len.text = juce::String (tr.length);
        len.setVisible (owner.seqParams.polymetry);
        div.repaint();
        len.repaint();
        repaint();
    }
    void resized() override
    {
        // name (drawn) | on | random | div | len | < >
        on.setBounds (92, 4, 18, 18);
        random.setBounds (114, 6, 44, 14);
        div.setBounds (162, 4, 30, 18);
        len.setBounds (114, 24, 44, 14);
        left.setBounds (194, 4, 16, 18);
        right.setBounds (210, 4, 16, 18);
    }
    void paint (juce::Graphics& g) override
    {
        g.setColour (col::text);
        g.setFont (font (11.0f, true));
        g.drawText (track == cs::TrackSteps ? "STEPS" : kLaneNames[track + 1], 8, 4, 84, 18, juce::Justification::centredLeft);
        if (random.getValue() > 0.0)
        {
            g.setColour (kGhost);
            g.setFont (font (9.5f));
            g.drawText (juce::String (juce::roundToInt (random.getValue() * 100.0)) + "%", random.getBounds().translated (0, 12), juce::Justification::centred);
        }
    }

private:
    SeqPage& owner;
    int track;
    juce::ToggleButton on;
    juce::Slider random;
    FlatButton div, len, left, right;
};

// ===========================================================================
class SeqPage::LeftColumn final : public ControlPanel
{
public:
    LeftColumn (SeqPage& o, UiContext& c) : ControlPanel (c), owner (o)
    {
        toggle ("seq_on", Toggle::Power, {}, { 10, 8, 16, 16 }).setTooltip ("Seq/Arp on/off");
        seqBtn.text = "sequencer";
        arpBtn.text = "arpeggiator";
        seqBtn.onClick = [this] { ctx.setValue ("seq_mode", 1.0f); };
        arpBtn.onClick = [this] { ctx.setValue ("seq_mode", 0.0f); };
        seqBtn.fontSize = arpBtn.fontSize = 11.0f;
        for (auto* b : { &seqBtn, &arpBtn })
            addAndMakeVisible (b);
        seqBtn.setBounds (32, 6, 92, 20);
        arpBtn.setBounds (128, 6, 92, 20);
        toggle ("seq_lock", Toggle::TextPill, "lock", { 224, 6, 38, 20 }).setTooltip ("Lock: keep this pattern when loading presets");

        presets.text = "PATTERNS";
        presets.fontSize = 11.0f;
        presets.onClick = [this] { showPatternMenu(); };
        addAndMakeVisible (presets);
        presets.setBounds (40, 32, 186, 20);

        title ({ 8, 58, 250, 14 }, "Playback");
        pill ("seq_direction", { 8, 78, 92, 18 });
        caption ({ 8, 96, 92, 12 }, "Direction");
        pill ("seq_rate", { 104, 78, 64, 18 });
        caption ({ 104, 96, 64, 12 }, "Rate");
        auto& sw = knob ("seq_swing", "Swing", { 186, 64, 62, 66 });
        sw.setFormatter ([] (float v) { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; });
        caption ({ 8, 130, 54, 12 }, "Length");
        stepper ("seq_length", { 8, 112, 54, 18 });
        toggle ("seq_polymetry", Toggle::TextPill, "polymetry", { 66, 112, 70, 18 });
        caption ({ 140, 130, 50, 12 }, "Reset");
        stepper ("seq_reset", { 140, 112, 50, 18 }).displayName = [] (int v) { return v == 0 ? juce::String ("off") : juce::String (v); };

        title ({ 8, 150, 250, 14 }, "Arpeggiator");
        pill ("seq_arp_order", { 8, 168, 120, 18 });
        caption ({ 132, 186, 60, 12 }, "Octaves");
        stepper ("seq_arp_octaves", { 132, 168, 60, 18 });

        title ({ 8, 202, 250, 14 }, "Generation");
        pill ("seq_scale", { 8, 220, 130, 18 });
        toggle ("seq_snap", Toggle::TextPill, "snap", { 142, 220, 48, 18 }).setTooltip ("Snap pitches to the scale");
        clear.text = "x";
        regen.text = "REGEN";
        apply.text = "apply";
        clear.setTooltip ("Clear random: all Random amounts to 0, unlock every step");
        regen.setTooltip ("New random values (locked steps keep theirs). With no Random amounts set, Regen sets some first");
        apply.setTooltip ("Make the random values you hear the pattern itself");
        clear.onClick = [this] { cs::seqClearRandom (owner.pat); owner.commit ("Seq clear random"); };
        regen.onClick = [this] {
            // With no Random amounts there is nothing to regenerate: start from
            // sensible amounts so every press gives a new variation.
            bool anyRandom = false;
            for (const auto& t : owner.pat.tracks)
                anyRandom = anyRandom || t.random > 0.0f;
            if (! anyRandom)
            {
                owner.pat.tracks[cs::TrackSteps].random = 0.25f;
                owner.pat.tracks[cs::TrackVelocity].random = 0.3f;
                owner.pat.tracks[cs::TrackOctave].random = 0.25f;
                owner.pat.tracks[cs::TrackGate].random = 0.2f;
                if (ctx.intValue ("seq_mode") == cs::SeqModeSeq)
                    owner.pat.tracks[cs::TrackPitch].random = 0.4f;
            }
            owner.pat.seed = cs::seqNextSeed (ctx.proc.getSeq().uiSeed.load());
            owner.commit ("Seq regen");
        };
        apply.onClick = [this] {
            cs::seqApplyRandom (owner.pat, owner.seqParams, ctx.proc.getSeq().uiSeed.load());
            owner.commit ("Seq apply random");
        };
        regen.litColour = col::gold;
        generate.text = "generate";
        generate.fontSize = 11.0f;
        generate.setTooltip ("Write a new pattern: rhythm, notes in the chosen scale, accents and octaves (locked steps stay). "
                             "The arrow sets how busy it is, the range, octave jumps and slides");
        generate.onClick = [this] {
            cs::seqGenerate (owner.pat, owner.seqParams, (uint32_t) juce::Random::getSystemRandom().nextInt() | 1u, genSettings());
            owner.commit ("Seq generate");
        };
        genMenu.icon = [] (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c) {
            juce::Path p;
            const auto cx = r.getCentreX(), cy = r.getCentreY();
            p.addTriangle (cx - 3.5f, cy - 1.8f, cx + 3.5f, cy - 1.8f, cx, cy + 2.2f);
            g.setColour (c);
            g.fillPath (p);
        };
        genMenu.setTooltip ("Generate settings");
        genMenu.onClick = [this] { showGenerateMenu(); };
        generate.setBounds (194, 220, 48, 18);
        genMenu.setBounds (244, 220, 16, 18);
        for (auto* b : { &clear, &regen, &apply, &generate, &genMenu })
            addAndMakeVisible (b);
        clear.setBounds (8, 244, 26, 22);
        regen.setBounds (38, 244, 80, 22);
        apply.setBounds (122, 244, 54, 22);
        pill ("seq_autoregen", { 180, 246, 80, 18 }).setTooltip ("Auto Regen: new random values every ...");

        knob ("seq_vel_blend", "Vel Blend", { 2, 270, 58, 64 }).setTooltip ("Velocity: played (0 %) or step value (100 %)");
        caption ({ 66, 312, 66, 12 }, "Transpose");
        stepper ("seq_transpose", { 66, 292, 66, 18 }).displayName = [] (int v) { return (v > 0 ? "+" : "") + juce::String (v) + " st"; };
        toggle ("hold", Toggle::TextPill, "hold", { 140, 292, 50, 18 }).setTooltip ("Hold: latch the keys");
        auto& bpm = knob ("seq_bpm", "Tempo", { 198, 270, 58, 64 });
        bpm.setTooltip ("Tempo when the host gives none (standalone)");
    }

    void paint (juce::Graphics& g) override
    {
        drawPanel (g, getLocalBounds().toFloat());
        paintCaptions (g);
        g.setColour (col::textDim);
        g.setFont (font (12.0f, true));
        g.drawText ("<", 18, 32, 20, 20, juce::Justification::centred);
        g.drawText (">", 230, 32, 20, 20, juce::Justification::centred);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.y >= 30 && e.y <= 54 && (e.x < 40 || e.x > 226))
            stepPattern (e.x < 40 ? -1 : 1);
    }

    void refreshView() override
    {
        ControlPanel::refreshView();
        const bool seqMode = ctx.intValue ("seq_mode") == cs::SeqModeSeq;
        seqBtn.lit = seqMode;
        arpBtn.lit = ! seqMode;
        seqBtn.repaint();
        arpBtn.repaint();
        presets.text = patternName.isEmpty() ? juce::String ("PATTERNS") : patternName;
        presets.repaint();
    }

private:
    SeqPage& owner;
    FlatButton seqBtn, arpBtn, presets, clear, regen, apply, generate, genMenu;
    static cs::SeqGenerateSettings& genSettings()
    {
        static cs::SeqGenerateSettings g; // per session, shared by every editor
        return g;
    }
    void showGenerateMenu()
    {
        auto& g = genSettings();
        juce::PopupMenu m;
        m.addSectionHeader ("Generate");
        m.addItem (1, "Sparse", true, g.density < 0.5f);
        m.addItem (2, "Medium", true, g.density >= 0.5f && g.density < 0.8f);
        m.addItem (3, "Busy", true, g.density >= 0.8f);
        m.addSeparator();
        m.addItem (11, "Range: a fifth", true, g.range == 7);
        m.addItem (12, "Range: an octave", true, g.range == 12);
        m.addSeparator();
        m.addItem (21, "No octave jumps", true, g.octaves <= 0.0f);
        m.addItem (22, "Some octave jumps", true, g.octaves > 0.0f && g.octaves < 0.3f);
        m.addItem (23, "Lots of octave jumps", true, g.octaves >= 0.3f);
        m.addSeparator();
        m.addItem (31, "Slides", true, g.slides > 0.0f);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&genMenu), [] (int r) {
            auto& gs = genSettings();
            switch (r)
            {
                case 1: gs.density = 0.35f; break;
                case 2: gs.density = 0.65f; break;
                case 3: gs.density = 0.9f; break;
                case 11: gs.range = 7; break;
                case 12: gs.range = 12; break;
                case 21: gs.octaves = 0.0f; break;
                case 22: gs.octaves = 0.15f; break;
                case 23: gs.octaves = 0.4f; break;
                case 31: gs.slides = gs.slides > 0.0f ? 0.0f : 0.15f; break;
                default: break;
            }
        });
    }
    juce::String patternName;
    int patternIndex = -1;

    void stepPattern (int delta)
    {
        auto list = ctx.proc.presets().patterns (0);
        if (list.empty())
            return;
        patternIndex = (patternIndex + delta + (int) list.size()) % (int) list.size();
        if (ctx.proc.presets().loadPattern (list[(size_t) patternIndex]))
            patternName = list[(size_t) patternIndex].name;
        owner.load();
        refreshView();
    }

    void showPatternMenu()
    {
        auto& pm = ctx.proc.presets();
        auto factory = pm.patterns (0), fromPreset = pm.patterns (1), user = pm.patterns (2);
        juce::PopupMenu m, f, p, u, del;
        for (size_t i = 0; i < factory.size(); ++i)
            f.addItem (1 + (int) i, factory[i].name);
        for (size_t i = 0; i < fromPreset.size(); ++i)
            p.addItem (1000 + (int) i, fromPreset[i].name);
        for (size_t i = 0; i < user.size(); ++i)
        {
            u.addItem (2000 + (int) i, user[i].name);
            del.addItem (3000 + (int) i, user[i].name);
        }
        m.addSubMenu ("Factory", f);
        m.addSubMenu ("From preset", p, ! fromPreset.empty());
        m.addSubMenu ("User", u, ! user.empty());
        m.addSeparator();
        m.addItem (900, "Save pattern...");
        m.addSubMenu ("Delete user pattern", del, ! user.empty());
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&presets), [this, factory, fromPreset, user] (int r) {
            auto& pmgr = ctx.proc.presets();
            const PresetManager::PatternInfo* chosen = nullptr;
            if (r >= 1 && r <= (int) factory.size())
            {
                chosen = &factory[(size_t) r - 1];
                patternIndex = r - 1;
            }
            else if (r >= 1000 && r - 1000 < (int) fromPreset.size())
                chosen = &fromPreset[(size_t) r - 1000];
            else if (r >= 2000 && r - 2000 < (int) user.size())
                chosen = &user[(size_t) r - 2000];
            else if (r >= 3000 && r - 3000 < (int) user.size())
                pmgr.deletePattern (user[(size_t) r - 3000]);
            else if (r == 900)
            {
                auto* w = new juce::AlertWindow ("Save pattern", "Name for this pattern:", juce::MessageBoxIconType::NoIcon);
                w->addTextEditor ("name", patternName.isEmpty() ? "My pattern" : patternName);
                w->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
                w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
                w->enterModalState (true, juce::ModalCallbackFunction::create ([this, w] (int result) {
                    if (result == 1)
                    {
                        juce::String err;
                        if (! ctx.proc.presets().savePattern (w->getTextEditorContents ("name"), err))
                            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Church Synth", err);
                        else
                            patternName = w->getTextEditorContents ("name");
                        refreshView();
                    }
                }), true);
            }
            if (chosen != nullptr && pmgr.loadPattern (*chosen))
            {
                patternName = chosen->name;
                owner.load();
                refreshView();
            }
        });
    }
};

// ===========================================================================
SeqPage::SeqPage (UiContext& c) : ctx (c)
{
    left = std::make_unique<LeftColumn> (*this, ctx);
    addAndMakeVisible (*left);
    for (int l = 0; l < NumLanes; ++l)
    {
        lanes[l] = std::make_unique<Lane> (*this, (LaneKind) l);
        addAndMakeVisible (*lanes[l]);
    }
    for (int t = 0; t < cs::NumSeqTracks; ++t)
    {
        headers[t] = std::make_unique<TrackHeader> (*this, t);
        addAndMakeVisible (*headers[t]);
    }
    load();
}

SeqPage::~SeqPage() = default;

juce::Rectangle<int> SeqPage::laneRow (int lane) const
{
    int y = kTop + 2;
    for (int l = 0; l < lane; ++l)
        y += kLaneHeights[l] + 2;
    return { kGridX, y, getWidth() - kGridX, kLaneHeights[lane] };
}

void SeqPage::resized()
{
    left->setBounds (0, 0, 264, 340);
    for (int l = 0; l < NumLanes; ++l)
        lanes[l]->setBounds (laneRow (l).withTrimmedLeft (kHeaderW).withTrimmedRight (4));
    // Steps header spans On/Off + Probability; the others sit on their lane.
    headers[cs::TrackSteps]->setBounds (laneRow (LaneOnOff).withWidth (kHeaderW).withHeight (kLaneHeights[0] + kLaneHeights[1] + 2));
    for (int t = 1; t < cs::NumSeqTracks; ++t)
        headers[t]->setBounds (laneRow (t + 1).withWidth (kHeaderW).withHeight (juce::jmax (kLaneHeights[t + 1], 40)));
}

void SeqPage::paint (juce::Graphics& g)
{
    drawPanel (g, juce::Rectangle<float> ((float) kGridX, 0.0f, (float) (getWidth() - kGridX), 340.0f));
    g.setFont (font (11.0f, true));
    g.setColour (col::textDim);
    g.drawText ("TRACK", kGridX + 8, 4, 80, 18, juce::Justification::centredLeft);
    g.drawText ("ON", kGridX + 90, 4, 24, 18, juce::Justification::centred);
    g.drawText ("RANDOM", kGridX + 112, 4, 50, 18, juce::Justification::centred);
    g.drawText ("DIV", kGridX + 160, 4, 34, 18, juce::Justification::centred);
    g.drawText ("ROT", kGridX + 194, 4, 34, 18, juce::Justification::centred);
    const auto lane = laneRow (0).withTrimmedLeft (kHeaderW).withTrimmedRight (4);
    const float w = (float) lane.getWidth() / (float) cs::kSeqSteps;
    for (int c = 0; c < cs::kSeqSteps; ++c)
    {
        g.setColour (c == playStep ? col::gold : col::textDim);
        g.drawText (juce::String (c + 1), (int) ((float) lane.getX() + (float) c * w), 4, (int) w, 18, juce::Justification::centred);
    }
    g.setColour (col::textDim);
    g.setFont (font (10.5f, true));
    g.drawText ("PROB", kGridX + 8, laneRow (LaneProb).getY() + 6, 60, 14, juce::Justification::centredLeft);
    g.drawText ("LOCK", kGridX + 8, laneRow (LaneLock).getY(), 60, 18, juce::Justification::centredLeft);
}

void SeqPage::commit (const juce::String& label)
{
    ctx.proc.setPattern (pat);
    seenVersion = ctx.proc.patternVersion();
    ctx.proc.checkpoint (label);
    for (auto& h : headers)
        h->update();
    for (auto& l : lanes)
        l->repaint();
}

void SeqPage::load()
{
    pat = ctx.proc.getPattern();
    seenVersion = ctx.proc.patternVersion();
    seqParams = ctx.proc.currentSeqParams();
    shownSeed = ctx.proc.getSeq().uiSeed.load();
    for (auto& h : headers)
        h->update();
    for (auto& l : lanes)
        l->repaint();
}

void SeqPage::refreshView()
{
    if (! editing && ctx.proc.patternVersion() != seenVersion)
        load();
    const auto sp = ctx.proc.currentSeqParams();
    auto same = [] (const cs::SeqParams& a, const cs::SeqParams& b) {
        return a.on == b.on && a.mode == b.mode && a.rateDiv == b.rateDiv && juce::approximatelyEqual (a.swing, b.swing)
               && a.direction == b.direction && a.length == b.length && a.polymetry == b.polymetry && a.resetSteps == b.resetSteps
               && a.arpOrder == b.arpOrder && a.arpOctaves == b.arpOctaves && a.scale == b.scale && a.snap == b.snap
               && juce::approximatelyEqual (a.transpose, b.transpose);
    };
    if (! same (sp, seqParams))
    {
        seqParams = sp;
        for (auto& h : headers)
            h->update();
        for (auto& l : lanes)
            l->repaint();
    }
    left->refreshView();
}

void SeqPage::tick()
{
    const int step = ctx.proc.getSeq().uiStep.load (std::memory_order_relaxed);
    const uint32_t seed = ctx.proc.getSeq().uiSeed.load (std::memory_order_relaxed);
    if (step != playStep || seed != shownSeed)
    {
        playStep = step;
        shownSeed = seed;
        for (auto& l : lanes)
            l->repaint();
        repaint (kGridX, 0, getWidth() - kGridX, kTop);
    }
}
} // namespace ui
