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
ChurchSynthEditor::ChurchSynthEditor (ChurchSynthProcessor& p)
    : AudioProcessorEditor (p),
      proc (p),
      darkLook (juce::LookAndFeel_V4::getMidnightColourScheme()),
      generic (p),
      routes (p),
      fxPanel (p)
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
    tabs.addTab ("Files", kBg, &files, false);
    addAndMakeVisible (tabs);

    setResizable (true, true);
    setResizeLimits (640, 500, 1800, 1600);
    setSize (820, 860);
    timerCallback();
    startTimerHz (10);
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
}

void ChurchSynthEditor::paint (juce::Graphics& g)
{
    g.fillAll (kBg);
    g.setColour (kAccent);
    g.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    g.drawText ("CHURCH SYNTH", 16, 0, getWidth() - 32, kHeader, juce::Justification::centredLeft);
    g.setColour (juce::Colours::grey);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText ("v" JucePlugin_VersionString " - milestone 5", 16, 0, getWidth() - 32, kHeader, juce::Justification::centredRight);
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
