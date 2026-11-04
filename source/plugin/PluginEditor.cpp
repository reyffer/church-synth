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
ChurchSynthEditor::ChurchSynthEditor (ChurchSynthProcessor& p)
    : AudioProcessorEditor (p),
      proc (p),
      darkLook (juce::LookAndFeel_V4::getMidnightColourScheme()),
      generic (p),
      routes (p)
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
    tabs.addTab ("Files", kBg, &files, false);
    addAndMakeVisible (tabs);

    setResizable (true, true);
    setResizeLimits (640, 500, 1800, 1600);
    setSize (820, 860);
    timerCallback();
    startTimerHz (4);
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
}

void ChurchSynthEditor::paint (juce::Graphics& g)
{
    g.fillAll (kBg);
    g.setColour (kAccent);
    g.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    g.drawText ("CHURCH SYNTH", 16, 0, getWidth() - 32, kHeader, juce::Justification::centredLeft);
    g.setColour (juce::Colours::grey);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText ("v" JucePlugin_VersionString " - milestone 3", 16, 0, getWidth() - 32, kHeader, juce::Justification::centredRight);
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
