#include "PluginEditor.h"

namespace
{
constexpr int kHeader = 40;
constexpr int kRowH = 28;
const char* const kRowTitles[] = { "Engine 1 wavetable", "Engine 2 wavetable", "Noise 1 sample", "Noise 2 sample" };
const juce::Colour kBg (0xff1e1f24), kAccent (0xff3fb6ff);

// Combo item IDs are index + 1 (0 means "nothing selected" in JUCE).
void fillSources (juce::ComboBox& box, bool withNone)
{
    if (withNone)
        box.addItem ("None", 1);
    for (int s = 0; s < cs::NumModSources; ++s)
        box.addItem (cs::modSourceName (s), s + 2);
}

void fillDestinations (juce::ComboBox& box)
{
    const auto& dests = cs::modDestinations();
    juce::String lastGroup;
    for (size_t i = 0; i < dests.size(); ++i)
    {
        // Group by the first two words ("Engine 1", "Filter 2", "LFO 3" ...).
        const juce::String name (dests[i].name);
        const auto group = name.upToFirstOccurrenceOf (" ", false, false) + " "
                           + name.fromFirstOccurrenceOf (" ", false, false).upToFirstOccurrenceOf (" ", false, false);
        if (group != lastGroup)
        {
            box.addSectionHeading (group);
            lastGroup = group;
        }
        box.addItem (name, (int) i + 1);
    }
}
} // namespace

// ---------------------------------------------------------------------------
RouteRow::RouteRow (std::function<void()> onChange, std::function<void()> onRemove)
{
    fillSources (source, false);
    fillDestinations (dest);
    fillSources (sidechain, true);
    sidechain.setTooltip ("Sidechain: a second source that scales this route (e.g. Mod Wheel scales an LFO).");

    for (auto* s : { &amount, &scAmount })
    {
        s->setSliderStyle (juce::Slider::LinearBar);
        s->setTextBoxStyle (juce::Slider::TextBoxLeft, false, 50, 20);
        s->setNumDecimalPlacesToDisplay (2);
    }
    amount.setRange (-1.0, 1.0, 0.001);
    amount.setDoubleClickReturnValue (true, 0.0);
    amount.setTooltip ("Amount: fraction of the destination's full range (negative = inverted).");
    scAmount.setRange (0.0, 1.0, 0.001);
    scAmount.setValue (1.0, juce::dontSendNotification);
    scAmount.setTooltip ("How strongly the sidechain scales this route.");

    source.onChange = dest.onChange = sidechain.onChange = onChange;
    amount.onValueChange = scAmount.onValueChange = onChange;
    remove.onClick = std::move (onRemove);

    for (auto* c : std::initializer_list<juce::Component*> { &source, &dest, &amount, &sidechain, &scAmount, &remove })
        addAndMakeVisible (c);
}

void RouteRow::set (const cs::ModRoute& r)
{
    source.setSelectedId (r.source + 2, juce::dontSendNotification);
    dest.setSelectedId (r.dest + 1, juce::dontSendNotification);
    sidechain.setSelectedId (r.sidechain < 0 ? 1 : r.sidechain + 2, juce::dontSendNotification);
    amount.setValue (r.amount, juce::dontSendNotification);
    scAmount.setValue (r.scAmount, juce::dontSendNotification);
}

cs::ModRoute RouteRow::get() const
{
    cs::ModRoute r;
    r.source = source.getSelectedId() - 2;
    r.dest = dest.getSelectedId() - 1;
    r.sidechain = sidechain.getSelectedId() <= 1 ? -1 : sidechain.getSelectedId() - 2;
    r.amount = (float) amount.getValue();
    r.scAmount = (float) scAmount.getValue();
    return r;
}

void RouteRow::resized()
{
    auto b = getLocalBounds().reduced (2);
    remove.setBounds (b.removeFromRight (28));
    b.removeFromRight (4);
    const int w = b.getWidth();
    source.setBounds (b.removeFromLeft (w * 18 / 100));
    b.removeFromLeft (4);
    dest.setBounds (b.removeFromLeft (w * 30 / 100));
    b.removeFromLeft (4);
    amount.setBounds (b.removeFromLeft (w * 18 / 100));
    b.removeFromLeft (8);
    sidechain.setBounds (b.removeFromLeft (w * 16 / 100));
    b.removeFromLeft (4);
    scAmount.setBounds (b);
}

// ---------------------------------------------------------------------------
RoutePanel::RoutePanel (ChurchSynthProcessor& p) : proc (p)
{
    help.setText ("Source  ->  Destination   |   Amount   |   Sidechain   |   SC amount", juce::dontSendNotification);
    help.setColour (juce::Label::textColourId, juce::Colours::grey);
    add.onClick = [this] {
        if (rows.size() >= (size_t) cs::kMaxRoutes)
            return;
        auto r = proc.getRoutes();
        cs::ModRoute nr;
        nr.source = cs::SrcLfo1;
        nr.dest = cs::findModDest ("f1_cutoff");
        nr.amount = 0.2f;
        r.routes[r.count++] = nr;
        proc.setRoutes (r);
        reload();
    };
    addAndMakeVisible (add);
    addAndMakeVisible (help);
    viewport.setViewedComponent (&list, false);
    viewport.setScrollBarsShown (true, false);
    addAndMakeVisible (viewport);
    reload();
}

void RoutePanel::reload()
{
    rows.clear();
    const auto r = proc.getRoutes();
    for (int i = 0; i < r.count; ++i)
    {
        // The list is rebuilt after every removal, so the captured index stays valid.
        rows.push_back (std::make_unique<RouteRow> ([this] { push(); },
                                                     [this, i] {
                                                         juce::MessageManager::callAsync ([this, i] {
                                                             cs::ModRouting out;
                                                             for (size_t k = 0; k < rows.size(); ++k)
                                                                 if ((int) k != i)
                                                                     out.routes[out.count++] = rows[k]->get();
                                                             proc.setRoutes (out);
                                                             reload();
                                                         });
                                                     }));
        rows.back()->set (r.routes[i]);
        list.addAndMakeVisible (rows.back().get());
    }
    layoutRows();
    repaint();
}

void RoutePanel::push()
{
    cs::ModRouting out;
    for (auto& row : rows)
        if (out.count < cs::kMaxRoutes)
            out.routes[out.count++] = row->get();
    proc.setRoutes (out);
}

void RoutePanel::layoutRows()
{
    const int w = juce::jmax (100, viewport.getWidth() - viewport.getScrollBarThickness());
    list.setSize (w, (int) rows.size() * 30 + 4);
    for (size_t i = 0; i < rows.size(); ++i)
        rows[i]->setBounds (0, (int) i * 30, w, 30);
}

void RoutePanel::paint (juce::Graphics& g)
{
    if (rows.empty())
    {
        g.setColour (juce::Colours::grey);
        g.drawFittedText ("No modulation yet. Click \"+ Add route\" (default: LFO 1 -> Filter 1 Cutoff), then pick a source, "
                          "a destination and an amount. Up to 64 routes.",
                          getLocalBounds().reduced (24).withTrimmedTop (60), juce::Justification::centredTop, 4);
    }
}

void RoutePanel::resized()
{
    auto b = getLocalBounds().reduced (8);
    auto top = b.removeFromTop (28);
    add.setBounds (top.removeFromLeft (130));
    top.removeFromLeft (12);
    help.setBounds (top);
    b.removeFromTop (6);
    viewport.setBounds (b);
    layoutRows();
}

// ---------------------------------------------------------------------------
namespace
{
const char* const kFxSlotIds[cs::fx::kNumFxSlots] = { "a1", "a2", "a3", "b1", "b2", "b3", "x1", "x2", "x3" };
const char* const kFxSlotNames[cs::fx::kNumFxSlots] = { "A1", "A2", "A3", "B1", "B2", "B3", "Aux 1", "Aux 2", "Aux 3" };
juce::String fxId (int slot, const juce::String& suffix) { return juce::String ("fx_") + kFxSlotIds[slot] + "_" + suffix; }

void styleKnob (juce::Slider& k)
{
    k.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
}
} // namespace

FxPanel::FxPanel (ChurchSynthProcessor& p) : proc (p)
{
    auto& vts = proc.apvts;
    routing.addItemList ({ "A > B", "B > A", "A // B" }, 1);
    routingAtt = std::make_unique<ComboAtt> (vts, "fx_routing", routing);
    routing.setTooltip ("A > B: bus A feeds bus B.  A // B: the two buses run in parallel.");
    addAndMakeVisible (routing);

    const char* busOnIds[3] = { "fx_a_on", "fx_b_on", "fx_aux_on" };
    const char* busOnNames[3] = { "Bus A", "Bus B", "Aux" };
    for (int b = 0; b < 3; ++b)
    {
        busOn[b].setButtonText (busOnNames[b]);
        busOnAtt[b] = std::make_unique<ButtonAtt> (vts, busOnIds[b], busOn[b]);
        addAndMakeVisible (busOn[b]);
    }
    auxPreAtt = std::make_unique<ButtonAtt> (vts, "fx_aux_pre", auxPre);
    auxPre.setTooltip ("On: the Aux send is taken before buses A and B. Off: after them.");
    addAndMakeVisible (auxPre);

    juce::Slider* busSliders[4] = { &busVol[0], &busVol[1], &auxSend, &auxReturn };
    const char* busSliderIds[4] = { "fx_a_vol", "fx_b_vol", "fx_aux_send", "fx_aux_return" };
    const char* busSliderNames[4] = { "A volume", "B volume", "Aux send", "Aux return" };
    for (int i = 0; i < 4; ++i)
    {
        styleKnob (*busSliders[i]);
        addAndMakeVisible (busSliders[i]);
        busLabels[i].setText (busSliderNames[i], juce::dontSendNotification);
        busLabels[i].setJustificationType (juce::Justification::centred);
        addAndMakeVisible (busLabels[i]);
    }
    busVolAtt[0] = std::make_unique<SliderAtt> (vts, busSliderIds[0], busVol[0]);
    busVolAtt[1] = std::make_unique<SliderAtt> (vts, busSliderIds[1], busVol[1]);
    auxSendAtt = std::make_unique<SliderAtt> (vts, busSliderIds[2], auxSend);
    auxReturnAtt = std::make_unique<SliderAtt> (vts, busSliderIds[3], auxReturn);
    for (auto* sl : busSliders)
    {
        sl->textFromValueFunction = [] (double v) { return juce::String (juce::roundToInt (v * 100.0)) + " %"; };
        sl->updateText();
    }

    for (int sl = 0; sl < cs::fx::kNumFxSlots; ++sl)
    {
        slotButtons[sl].setClickingTogglesState (false);
        slotButtons[sl].onClick = [this, sl] { select (sl); };
        addAndMakeVisible (slotButtons[sl]);
    }
    for (int t = 0; t < cs::fx::FxTypeCount; ++t)
        type.addItem (cs::fx::fxInfo (t).name, t + 1);
    addAndMakeVisible (type);
    addAndMakeVisible (slotOn);
    sync.setTooltip ("Lock the time / rate control to the host tempo.");
    addAndMakeVisible (sync);
    for (int k = 0; k < cs::fx::kFxParams; ++k)
    {
        styleKnob (knobs[k]);
        knobLabels[k].setJustificationType (juce::Justification::centred);
        addChildComponent (knobs[k]);
        addChildComponent (knobLabels[k]);
    }
    select (0);
}

void FxPanel::select (int slot)
{
    selected = slot;
    auto& vts = proc.apvts;
    // Detach before re-attaching so the old slot's parameters are left untouched.
    typeAtt.reset();
    slotOnAtt.reset();
    syncAtt.reset();
    for (auto& a : knobAtt)
        a.reset();
    typeAtt = std::make_unique<ComboAtt> (vts, fxId (slot, "type"), type);
    slotOnAtt = std::make_unique<ButtonAtt> (vts, fxId (slot, "on"), slotOn);
    syncAtt = std::make_unique<ButtonAtt> (vts, fxId (slot, "sync"), sync);
    for (int k = 0; k < cs::fx::kFxParams; ++k)
        knobAtt[k] = std::make_unique<SliderAtt> (vts, fxId (slot, "p" + juce::String (k + 1)), knobs[k]);
    shownType = shownSync = -1;
    refresh();
    resized();
}

void FxPanel::refresh()
{
    for (int sl = 0; sl < cs::fx::kNumFxSlots; ++sl)
    {
        const int t = (int) std::lround (proc.apvts.getRawParameterValue (fxId (sl, "type"))->load());
        slotButtons[sl].setButtonText (juce::String (kFxSlotNames[sl]) + ": " + (t == 0 ? juce::String ("-") : juce::String (cs::fx::fxInfo (t).name)));
        slotButtons[sl].setToggleState (sl == selected, juce::dontSendNotification);
    }
    const int t = (int) std::lround (proc.apvts.getRawParameterValue (fxId (selected, "type"))->load());
    const int sy = proc.apvts.getRawParameterValue (fxId (selected, "sync"))->load() >= 0.5f ? 1 : 0;
    if (t != shownType || sy != shownSync)
    {
        shownType = t;
        shownSync = sy;
        updateLabels();
    }
}

void FxPanel::updateLabels()
{
    const auto& info = cs::fx::fxInfo (shownType);
    const int syncParam = shownSync != 0 ? cs::fx::fxSyncParam (shownType) : -1;
    sync.setVisible (info.usesSync);
    slotOn.setVisible (shownType != cs::fx::FxNone);
    for (int k = 0; k < cs::fx::kFxParams; ++k)
    {
        const bool used = info.params[(size_t) k] != nullptr;
        knobs[k].setVisible (used);
        knobLabels[k].setVisible (used);
        if (! used)
            continue;
        knobLabels[k].setText (info.params[(size_t) k], juce::dontSendNotification);
        if (k == syncParam)
            knobs[k].textFromValueFunction = [] (double v) {
                return juce::String (cs::kSyncDivs[(size_t) juce::jlimit (0, cs::kNumSyncDivs - 1, (int) (v * cs::kNumSyncDivs))].name);
            };
        else
            knobs[k].textFromValueFunction = [] (double v) { return juce::String (juce::roundToInt (v * 100.0)) + " %"; };
        knobs[k].updateText();
    }
    resized();
    repaint();
}

void FxPanel::paint (juce::Graphics& g)
{
    g.setColour (juce::Colours::grey);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText ("Interim FX page: the full Pigments-style FX view comes with the UI milestone.",
                getLocalBounds().removeFromBottom (24).reduced (12, 0), juce::Justification::centredLeft);
    if (shownType == cs::fx::FxNone)
        g.drawFittedText ("Pick an effect for this slot. Settings start at the effect's defaults; Aux effects start 100 % wet.",
                          getLocalBounds().reduced (24).withTrimmedTop (230), juce::Justification::centredTop, 3);
}

void FxPanel::resized()
{
    auto b = getLocalBounds().reduced (12);
    // Bus row
    auto row = b.removeFromTop (28);
    routing.setBounds (row.removeFromLeft (110));
    row.removeFromLeft (12);
    for (auto& t : busOn)
        t.setBounds (row.removeFromLeft (80));
    auxPre.setBounds (row.removeFromLeft (120));
    b.removeFromTop (6);
    auto knobsRow = b.removeFromTop (92);
    juce::Slider* busSliders[4] = { &busVol[0], &busVol[1], &auxSend, &auxReturn };
    for (int i = 0; i < 4; ++i)
    {
        auto cell = knobsRow.removeFromLeft (100);
        busLabels[i].setBounds (cell.removeFromTop (18));
        busSliders[i]->setBounds (cell);
    }
    b.removeFromTop (10);
    // Slot buttons: three rows of three (bus A, bus B, aux)
    for (int r = 0; r < 3; ++r)
    {
        auto line = b.removeFromTop (26);
        const int w = line.getWidth() / 3;
        for (int c = 0; c < 3; ++c)
            slotButtons[r * 3 + c].setBounds (line.removeFromLeft (w).reduced (2, 1));
    }
    b.removeFromTop (10);
    auto head = b.removeFromTop (28);
    type.setBounds (head.removeFromLeft (200));
    head.removeFromLeft (12);
    slotOn.setBounds (head.removeFromLeft (60));
    sync.setBounds (head.removeFromLeft (80));
    b.removeFromTop (8);
    // Knobs: up to 12, in rows of 6
    const int kw = juce::jmax (80, b.getWidth() / 6);
    int i = 0;
    for (int k = 0; k < cs::fx::kFxParams; ++k)
    {
        if (! knobs[k].isVisible())
            continue;
        const int x = b.getX() + (i % 6) * kw, y = b.getY() + (i / 6) * 112;
        knobLabels[k].setBounds (x, y, kw, 18);
        knobs[k].setBounds (x, y + 18, kw, 90);
        ++i;
    }
}

// ---------------------------------------------------------------------------
namespace
{
constexpr int kLaneHeights[SeqPanel::NumLanes] = { 22, 34, 60, 42, 46, 46, 42, 18 };
const char* const kLaneNames[SeqPanel::NumLanes] = { "Steps", "Probability", "Pitch", "Velocity", "Octave", "Gate", "Slide", "Lock" };
// Track that owns each lane (Probability belongs to Steps; Lock to none).
constexpr int kLaneTrack[SeqPanel::NumLanes] = { cs::TrackSteps, cs::TrackSteps, cs::TrackPitch, cs::TrackVelocity,
                                                 cs::TrackOctave, cs::TrackGate, cs::TrackSlide, -1 };
const juce::Colour kLaneOn (0xff3fb6ff), kLaneOff (0xff34363d), kLaneGhost (0xffffc04d);
} // namespace

void SeqPanel::Lane::paint (juce::Graphics& g)
{
    const auto& pat = owner.pat;
    const float w = (float) getWidth() / (float) cs::kSeqSteps, h = (float) getHeight();
    const bool enabled = kLaneTrack[kind] < 0 || pat.tracks[(size_t) kLaneTrack[kind]].enabled;
    // Random preview: what each column plays with the current seed.
    const auto& sp = owner.seqParams;
    const uint32_t seed = owner.proc.getSeq().uiSeed.load (std::memory_order_relaxed);
    const int trackLen = kLaneTrack[kind] >= 0 && sp.polymetry ? pat.tracks[(size_t) kLaneTrack[kind]].length : sp.length;

    for (int c = 0; c < cs::kSeqSteps; ++c)
    {
        const auto& st = pat.steps[(size_t) c];
        auto cell = juce::Rectangle<float> ((float) c * w, 0.0f, w, h).reduced (1.5f, 1.0f);
        const bool inRange = kind == LaneLock || c < trackLen;
        g.setColour (c == owner.playStep ? juce::Colour (0xff4a4d57) : juce::Colour (0xff26282e));
        g.fillRect (cell);
        if (! inRange)
        {
            g.setColour (juce::Colour (0x99000000));
            g.fillRect (cell);
            continue;
        }
        const auto col = (enabled ? kLaneOn : juce::Colours::grey).withAlpha (st.on || kind == LaneOnOff ? 1.0f : 0.45f);
        float frac = 0.0f;
        juce::String text;
        int cols[cs::NumSeqTracks];
        for (auto& x : cols)
            x = c;
        const auto r = cs::seqResolve (pat, sp, cols, seed);
        float ghost = -1.0f;
        switch (kind)
        {
            case LaneOnOff:
                g.setColour (st.on ? col : kLaneOff);
                g.fillRect (cell.reduced (2.0f));
                if (r.on != st.on)
                {
                    g.setColour (kLaneGhost);
                    g.drawRect (cell.reduced (2.0f), 2.0f);
                }
                continue;
            case LaneLock:
                g.setColour (st.lockSeed != 0 ? kLaneGhost : kLaneOff);
                g.fillEllipse (cell.withSizeKeepingCentre (8.0f, 8.0f));
                continue;
            case LaneProb: frac = st.prob; text = juce::String (juce::roundToInt (st.prob * 100.0f)); break;
            case LanePitch:
                frac = (float) st.pitch / 12.0f;
                text = juce::String (st.pitch);
                ghost = (float) r.pitch / 12.0f;
                break;
            case LaneVelocity: frac = st.velocity; ghost = r.velocity; break;
            case LaneOctave:
            {
                // Bipolar: bar from the centre line.
                const float cy = cell.getCentreY();
                const float y = cy - (float) st.octave / 2.0f * (cell.getHeight() * 0.5f);
                g.setColour (col);
                g.fillRect (juce::Rectangle<float>::leftTopRightBottom (cell.getX() + 2, std::min (y, cy), cell.getRight() - 2, std::max (y, cy) + 1.0f));
                g.setColour (juce::Colours::white.withAlpha (0.7f));
                g.setFont (juce::FontOptions (10.0f));
                g.drawText (st.octave == 0 ? juce::String ("0") : juce::String (st.octave > 0 ? "+" : "") + juce::String (st.octave), cell, juce::Justification::centred);
                if (r.octave != st.octave)
                {
                    const float gy = cy - (float) r.octave / 2.0f * (cell.getHeight() * 0.5f);
                    g.setColour (kLaneGhost);
                    g.fillRect (cell.getX() + 3.0f, gy - 1.0f, cell.getWidth() - 6.0f, 2.0f);
                }
                continue;
            }
            case LaneGate:
                frac = std::sqrt (st.gate / 4.0f);
                text = juce::String (juce::roundToInt (st.gate * 100.0f));
                ghost = std::sqrt (r.gate / 4.0f);
                break;
            case LaneSlide: frac = st.slide; text = st.slide > 0.0f ? juce::String (juce::roundToInt (cs::seqSlideSeconds (st.slide) * 1000.0f)) : juce::String(); break;
            case NumLanes:
            default: break;
        }
        const float barH = cell.getHeight() * juce::jlimit (0.0f, 1.0f, frac);
        g.setColour (col);
        g.fillRect (cell.withTop (cell.getBottom() - barH));
        if (ghost >= 0.0f && std::abs (ghost - frac) > 1.0e-3f)
        {
            g.setColour (kLaneGhost);
            const float gy = cell.getBottom() - cell.getHeight() * juce::jlimit (0.0f, 1.0f, ghost);
            g.fillRect (cell.getX() + 3.0f, gy - 1.0f, cell.getWidth() - 6.0f, 2.0f);
        }
        if (text.isNotEmpty())
        {
            g.setColour (juce::Colours::white.withAlpha (0.75f));
            g.setFont (juce::FontOptions (10.0f));
            g.drawText (text, cell.removeFromTop (12.0f), juce::Justification::centred);
        }
    }
    if (kind == LaneGate) // 100 % line
    {
        g.setColour (juce::Colours::white.withAlpha (0.15f));
        g.drawHorizontalLine ((int) (h - h * std::sqrt (0.25f)), 0.0f, (float) getWidth());
    }
}

void SeqPanel::Lane::mouseDown (const juce::MouseEvent& e)
{
    owner.editing = true;
    lastCol = -1;
    edit (e, true);
}

void SeqPanel::Lane::mouseDrag (const juce::MouseEvent& e) { edit (e, false); }

void SeqPanel::Lane::mouseUp (const juce::MouseEvent&)
{
    owner.editing = false;
    owner.commit();
}

void SeqPanel::Lane::edit (const juce::MouseEvent& e, bool first)
{
    const int c = owner.columnAt (e.x, getWidth());
    auto& st = owner.pat.steps[(size_t) c];
    const float h = (float) getHeight();
    const float v = juce::jlimit (0.0f, 1.0f, 1.0f - (float) e.y / juce::jmax (1.0f, h));
    switch (kind)
    {
        case LaneOnOff:
            if (first)
                paintOn = ! st.on; // drag paints the same state across steps
            st.on = paintOn;
            break;
        case LaneLock:
            if (first)
                paintOn = st.lockSeed == 0;
            st.lockSeed = paintOn ? owner.proc.getSeq().uiSeed.load() : 0u;
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
    lastCol = c;
    owner.commit();
    repaint();
}

SeqPanel::SeqPanel (ChurchSynthProcessor& p) : proc (p)
{
    for (auto* id : { "seq_on", "seq_mode", "seq_rate", "seq_swing", "seq_length", "seq_direction", "seq_arp_order",
                      "seq_arp_octaves", "seq_vel_blend", "seq_transpose", "seq_scale", "seq_snap", "seq_polymetry",
                      "seq_reset", "seq_autoregen", "seq_bpm", "hold" })
        addControl (id, {});

    regen.setTooltip ("New random values for every track with a Random amount (locked steps keep theirs).");
    applyRandom.setTooltip ("Make the random values you hear the pattern itself.");
    clearRandom.setTooltip ("Set every track's Random amount to 0 and unlock all steps.");
    regen.onClick = [this] {
        pat.seed = cs::seqNextSeed (proc.getSeq().uiSeed.load());
        commit();
    };
    applyRandom.onClick = [this] {
        cs::seqApplyRandom (pat, proc.currentSeqParams(), proc.getSeq().uiSeed.load());
        commit();
        updateHeaders();
    };
    clearRandom.onClick = [this] { cs::seqClearRandom (pat); commit(); updateHeaders(); };
    rotLeft.onClick = [this] { cs::seqRotate (pat, -1, -1); commit(); };
    rotRight.onClick = [this] { cs::seqRotate (pat, -1, 1); commit(); };
    clearPattern.onClick = [this] { pat = cs::SeqPattern(); commit(); updateHeaders(); };
    for (auto* b : { &regen, &applyRandom, &clearRandom, &rotLeft, &rotRight, &clearPattern })
        addAndMakeVisible (b);
    help.setText ("Drag in the lanes to draw. Arp: Pitch is ignored. Hold (or the sustain pedal) latches the keys. "
                  "Yellow marks show what the random amounts make each step play.",
                  juce::dontSendNotification);
    help.setColour (juce::Label::textColourId, juce::Colours::grey);
    help.setFont (juce::FontOptions (12.0f));
    addAndMakeVisible (help);

    for (int l = 0; l < NumLanes; ++l)
    {
        lanes[l] = std::make_unique<Lane> (*this, (LaneKind) l);
        addAndMakeVisible (*lanes[l]);
        laneNames[l].setText (kLaneNames[l], juce::dontSendNotification);
        laneNames[l].setFont (juce::FontOptions (12.0f));
        addAndMakeVisible (laneNames[l]);
    }
    for (int t = 0; t < cs::NumSeqTracks; ++t)
    {
        auto& h = headers[t];
        h.on.setTooltip ("Track on/off");
        h.on.onClick = [this, t] { pat.tracks[(size_t) t].enabled = headers[t].on.getToggleState(); commit(); };
        h.length.setSliderStyle (juce::Slider::LinearBar);
        h.length.setRange (1, cs::kSeqSteps, 1);
        h.length.setTooltip ("Track length (with Polymetry on)");
        h.length.onValueChange = [this, t] { pat.tracks[(size_t) t].length = (int) headers[t].length.getValue(); commit(); };
        h.random.setSliderStyle (juce::Slider::LinearBar);
        h.random.setRange (0.0, 1.0, 0.01);
        h.random.textFromValueFunction = [] (double v) { return "Rnd " + juce::String (juce::roundToInt (v * 100.0)) + "%"; };
        h.random.setTooltip ("Random amount for this track");
        h.random.onValueChange = [this, t] { pat.tracks[(size_t) t].random = (float) headers[t].random.getValue(); commit(); };
        for (int d = 0; d < cs::kNumSeqDividers; ++d)
            h.divider.addItem ("/" + juce::String (cs::kSeqDividers[d]), d + 1);
        h.divider.setTooltip ("Rate divider: the track moves once every N steps");
        h.divider.onChange = [this, t] {
            pat.tracks[(size_t) t].divider = cs::kSeqDividers[juce::jlimit (0, cs::kNumSeqDividers - 1, headers[t].divider.getSelectedId() - 1)];
            commit();
        };
        h.left.onClick = [this, t] { cs::seqRotate (pat, t, -1); commit(); };
        h.right.onClick = [this, t] { cs::seqRotate (pat, t, 1); commit(); };
        for (auto* c : std::initializer_list<juce::Component*> { &h.on, &h.length, &h.random, &h.divider, &h.left, &h.right })
            addAndMakeVisible (c);
    }
    loadFromProcessor();
}

SeqPanel::~SeqPanel() = default;

void SeqPanel::addControl (const juce::String& id, const juce::String&)
{
    auto c = std::make_unique<Control>();
    auto* prm = proc.apvts.getParameter (id);
    if (prm == nullptr)
        return;
    c->label.setText (prm->getName (40).replace ("Seq/Arp ", "").replace ("Seq ", ""), juce::dontSendNotification);
    c->label.setFont (juce::FontOptions (11.0f));
    c->label.setColour (juce::Label::textColourId, juce::Colours::grey);
    if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (prm))
    {
        auto box = std::make_unique<juce::ComboBox>();
        box->addItemList (choice->choices, 1);
        c->cAtt = std::make_unique<ComboAtt> (proc.apvts, id, *box);
        c->widget = std::move (box);
    }
    else if (dynamic_cast<juce::AudioParameterBool*> (prm) != nullptr)
    {
        auto b = std::make_unique<juce::ToggleButton> (c->label.getText());
        c->bAtt = std::make_unique<ButtonAtt> (proc.apvts, id, *b);
        c->label.setText ({}, juce::dontSendNotification);
        c->widget = std::move (b);
    }
    else
    {
        auto sl = std::make_unique<juce::Slider> (juce::Slider::LinearBar, juce::Slider::TextBoxLeft);
        c->sAtt = std::make_unique<SliderAtt> (proc.apvts, id, *sl);
        if (id == "seq_swing" || id == "seq_vel_blend")
            sl->textFromValueFunction = [] (double v) { return juce::String (juce::roundToInt (v * 100.0)) + " %"; };
        if (id == "seq_bpm")
            sl->textFromValueFunction = [] (double v) { return juce::String (v, 1) + " BPM"; };
        sl->updateText();
        c->widget = std::move (sl);
    }
    addAndMakeVisible (c->label);
    addAndMakeVisible (*c->widget);
    controls.push_back (std::move (c));
}

void SeqPanel::commit()
{
    proc.setPattern (pat);
    seenVersion = proc.patternVersion();
    for (auto& l : lanes)
        l->repaint();
}

void SeqPanel::loadFromProcessor()
{
    pat = proc.getPattern();
    seenVersion = proc.patternVersion();
    seqParams = proc.currentSeqParams();
    updateHeaders();
    for (auto& l : lanes)
        l->repaint();
}

void SeqPanel::updateHeaders()
{
    for (int t = 0; t < cs::NumSeqTracks; ++t)
    {
        const auto& tr = pat.tracks[(size_t) t];
        auto& h = headers[t];
        h.on.setToggleState (tr.enabled, juce::dontSendNotification);
        h.length.setValue (tr.length, juce::dontSendNotification);
        h.random.setValue (tr.random, juce::dontSendNotification);
        h.random.updateText();
        int d = 0;
        for (int i = 0; i < cs::kNumSeqDividers; ++i)
            if (cs::kSeqDividers[i] == tr.divider)
                d = i;
        h.divider.setSelectedId (d + 1, juce::dontSendNotification);
    }
}

void SeqPanel::refresh()
{
    if (! editing && proc.patternVersion() != seenVersion)
        loadFromProcessor(); // a state load or another editor changed it
    const int step = proc.getSeq().uiStep.load (std::memory_order_relaxed);
    const uint32_t seed = proc.getSeq().uiSeed.load (std::memory_order_relaxed);
    const auto sp = proc.currentSeqParams();
    const bool paramsChanged = std::memcmp (&sp, &seqParams, sizeof (sp)) != 0;
    if (step != playStep || seed != shownSeed || paramsChanged)
    {
        playStep = step;
        shownSeed = seed;
        seqParams = sp;
        for (auto& l : lanes)
            l->repaint();
    }
}

void SeqPanel::paint (juce::Graphics& g)
{
    g.setColour (juce::Colours::grey);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText ("Interim Seq/Arp page: the Pigments-style view and the pattern browser come with the UI milestone.",
                getLocalBounds().removeFromBottom (22).reduced (12, 0), juce::Justification::centredLeft);
}

void SeqPanel::resized()
{
    auto b = getLocalBounds().reduced (10);
    // Settings: a grid of label + control, 6 per row.
    const int perRow = 6, cw = b.getWidth() / perRow;
    for (size_t i = 0; i < controls.size(); ++i)
    {
        const int r = (int) i / perRow, c = (int) i % perRow;
        auto cell = juce::Rectangle<int> (b.getX() + c * cw, b.getY() + r * 40, cw, 40).reduced (3, 1);
        controls[i]->label.setBounds (cell.removeFromTop (14));
        controls[i]->widget->setBounds (cell.withHeight (22));
    }
    const int rows = ((int) controls.size() + perRow - 1) / perRow;
    b.removeFromTop (rows * 40 + 6);

    auto buttons = b.removeFromTop (26);
    const int bw = buttons.getWidth() / 6;
    for (auto* btn : { &regen, &applyRandom, &clearRandom, &rotLeft, &rotRight, &clearPattern })
        btn->setBounds (buttons.removeFromLeft (bw).reduced (2, 0));
    b.removeFromTop (4);
    help.setBounds (b.removeFromTop (32));
    b.removeFromTop (4);
    b.removeFromBottom (24);

    // Lanes: header column on the left.
    const int headerW = 230;
    for (int l = 0; l < NumLanes; ++l)
    {
        auto row = b.removeFromTop (kLaneHeights[l]);
        b.removeFromTop (l == LaneProb || l == LaneLock - 1 ? 8 : 4);
        auto head = row.removeFromLeft (headerW);
        lanes[l]->setBounds (row);
        laneNames[l].setBounds (head.removeFromLeft (70).withHeight (juce::jmin (20, row.getHeight())));
        const int t = kLaneTrack[l];
        if (t >= 0 && l != LaneProb)
        {
            auto& h = headers[t];
            auto top = head.withHeight (20);
            h.on.setBounds (top.removeFromLeft (24));
            h.divider.setBounds (top.removeFromLeft (52));
            top.removeFromLeft (2);
            h.length.setBounds (top.removeFromLeft (34));
            top.removeFromLeft (2);
            h.left.setBounds (top.removeFromLeft (22));
            h.right.setBounds (top.removeFromLeft (22));
            if (l != LaneOnOff) // the Steps track's Random sits on the Probability row
                h.random.setBounds (head.withTop (head.getY() + 22).withHeight (18).withWidth (head.getWidth() - 4));
        }
        if (l == LaneProb)
            headers[cs::TrackSteps].random.setBounds (head.withHeight (18).withWidth (head.getWidth() - 4));
    }
}

// ---------------------------------------------------------------------------
ChurchSynthEditor::ChurchSynthEditor (ChurchSynthProcessor& p)
    : AudioProcessorEditor (p),
      proc (p),
      darkLook (juce::LookAndFeel_V4::getMidnightColourScheme()),
      generic (p),
      routes (p),
      fxPanel (p),
      seqPanel (p)
{
    setLookAndFeel (&darkLook);

    for (int slot = 0; slot < ChurchSynthProcessor::NumContentSlots; ++slot)
    {
        auto& r = rows[slot];
        r.title.setText (kRowTitles[slot], juce::dontSendNotification);
        r.title.setColour (juce::Label::textColourId, juce::Colours::grey);
        r.current.setColour (juce::Label::textColourId, kAccent);
        r.load.onClick = [this, slot] { choose (slot); };
        r.factory.onClick = [this, slot] { proc.clearUserFile (slot); timerCallback(); };
        r.load.setTooltip ("Import your own WAV/AIFF/FLAC. It's copied to Documents/Church Synth.");
        r.factory.setTooltip ("Go back to the factory choice (set on the Sound tab).");
        for (auto* c : std::initializer_list<juce::Component*> { &r.title, &r.current, &r.load, &r.factory })
            files.addAndMakeVisible (c);
    }

    tabs.addTab ("Sound", kBg, &generic, false);
    tabs.addTab ("Modulation", kBg, &routes, false);
    tabs.addTab ("FX", kBg, &fxPanel, false);
    tabs.addTab ("Seq/Arp", kBg, &seqPanel, false);
    tabs.addTab ("Files", kBg, &files, false);
    addAndMakeVisible (tabs);

    setResizable (true, true);
    setResizeLimits (640, 500, 1800, 1600);
    setSize (820, 860);
    timerCallback();
    startTimerHz (20);
}

ChurchSynthEditor::~ChurchSynthEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void ChurchSynthEditor::choose (int slot)
{
    const bool wt = slot < 2;
    chooser = std::make_unique<juce::FileChooser> (wt ? "Choose a wavetable (2048 samples per frame)" : "Choose a noise / texture sample",
                                                   ChurchSynthProcessor::userFolder (wt), "*.wav;*.aif;*.aiff;*.flac");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this, slot] (const juce::FileChooser& fc) {
                              const auto file = fc.getResult();
                              if (file == juce::File())
                                  return;
                              juce::String error;
                              if (! proc.importUserFile (slot, file, error))
                                  juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Church Synth", error);
                              timerCallback();
                          });
}

void ChurchSynthEditor::timerCallback()
{
    for (int slot = 0; slot < ChurchSynthProcessor::NumContentSlots; ++slot)
        rows[slot].current.setText (proc.getSlotDisplayName (slot), juce::dontSendNotification);
    fxPanel.refresh();
    if (tabs.getCurrentContentComponent() == &seqPanel)
        seqPanel.refresh();
}

void ChurchSynthEditor::paint (juce::Graphics& g)
{
    g.fillAll (kBg);
    g.setColour (kAccent);
    g.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    g.drawText ("CHURCH SYNTH", 16, 0, getWidth() - 32, kHeader, juce::Justification::centredLeft);
    g.setColour (juce::Colours::grey);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText ("v" JucePlugin_VersionString " - milestone 6", 16, 0, getWidth() - 32, kHeader, juce::Justification::centredRight);
}

void ChurchSynthEditor::resized()
{
    tabs.setBounds (getLocalBounds().withTrimmedTop (kHeader));

    auto area = files.getLocalBounds().reduced (12, 12);
    for (auto& r : rows)
    {
        auto row = area.removeFromTop (kRowH).reduced (0, 2);
        r.title.setBounds (row.removeFromLeft (150));
        r.factory.setBounds (row.removeFromRight (100));
        row.removeFromRight (6);
        r.load.setBounds (row.removeFromRight (100));
        row.removeFromRight (6);
        r.current.setBounds (row);
    }
}
