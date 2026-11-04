#include "PluginEditor.h"
#include "PresetManager.h"

namespace
{

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

// ===========================================================================
// Editor: Pigments-style layout at 1280 x 800, scaled to the window.
// ===========================================================================
namespace
{
constexpr int kW = 1280, kH = 800;
int openEditors = 0; // message thread only

juce::File settingsFile() { return PresetManager::rootFolder().getChildFile ("Settings.xml"); }

float loadScale()
{
    if (auto xml = juce::XmlDocument::parse (settingsFile()))
        return (float) juce::jlimit (0.6, 2.0, xml->getDoubleAttribute ("uiScale", 1.0));
    return 1.0f;
}

void saveScale (float s)
{
    PresetManager::rootFolder().createDirectory();
    juce::XmlElement x ("ChurchSynthSettings");
    if (auto old = juce::XmlDocument::parse (settingsFile()))
        x = *old;
    x.setAttribute ("uiScale", s);
    x.writeTo (settingsFile());
}
} // namespace

class ChurchSynthEditor::Root final : public juce::Component
{
public:
    explicit Root (ui::UiContext& c)
        : ctx (c), top (c), synth (c), fx (c), seq (c), strip (c), tabs (c), footer (c), browser (c)
    {
        for (auto* comp : std::initializer_list<juce::Component*> { &top, &synth, &fx, &seq, &strip, &tabs, &footer })
            addAndMakeVisible (comp);
        addChildComponent (browser);
        fx.setVisible (false);
        seq.setVisible (false);
        top.onPage = [this] (ui::Page p) { showPage (p); };
        top.onBrowser = [this] { showBrowser (! browser.isVisible()); };
        top.onPrev = [this] { browser.step (-1); };
        top.onNext = [this] { browser.step (1); };
        top.onSave = [this] { browser.saveOver(); };
        browser.onClose = [this] { showBrowser (false); };
        strip.onSourceClicked = [this] (int s) { tabs.showTabForSource (s); };
        footer.onRoutes = [this] { showRoutes(); };
        setSize (kW, kH);
    }

    void resized() override
    {
        top.setBounds (0, 0, kW, 44);
        for (auto* page : std::initializer_list<juce::Component*> { &synth, &fx, &seq })
            page->setBounds (2, 48, kW - 4, 340);
        strip.setBounds (2, 392, kW - 4, 104);
        tabs.setBounds (2, 500, kW - 4, 270);
        footer.setBounds (0, kH - 28, kW, 28);
        browser.setBounds (0, 44, kW, kH - 44 - 28);
    }

    void paint (juce::Graphics& g) override { g.fillAll (ui::col::bg); }

    void showPage (ui::Page p)
    {
        top.page = p;
        synth.setVisible (p == ui::Page::Synth);
        fx.setVisible (p == ui::Page::Fx);
        seq.setVisible (p == ui::Page::Seq);
        showBrowser (false);
        refreshAll();
    }

    void showBrowser (bool show)
    {
        browser.setVisible (show);
        top.browserOpen = show;
        if (show)
        {
            browser.refresh();
            browser.toFront (true);
            browser.grabKeyboardFocus();
        }
        top.refresh();
    }

    void showRoutes()
    {
        auto panel = std::make_unique<RoutePanel> (ctx.proc);
        panel->setSize (760, 420);
        juce::CallOutBox::launchAsynchronously (std::move (panel), footer.getScreenBounds().withWidth (200).translated (500, 0), nullptr);
    }

    void refreshAll()
    {
        ctx.refreshRoutes();
        if (synth.isVisible()) synth.refreshView();
        if (fx.isVisible()) fx.refreshView();
        if (seq.isVisible()) seq.refreshView();
        tabs.refreshView();
        top.refresh();
        footer.refresh();
        if (browser.isVisible())
            browser.repaint();
    }

    void tick (bool slow)
    {
        top.tickMeter();
        strip.tick();
        tabs.tick();
        if (seq.isVisible())
            seq.tick();
        if (slow)
            footer.refresh();
    }

    ui::UiContext& ctx;
    ui::TopBar top;
    ui::SynthPage synth;
    ui::FxPage fx;
    ui::SeqPage seq;
    ui::ModStrip strip;
    ui::ModTabs tabs;
    ui::Footer footer;
    ui::PresetBrowser browser;
};

ChurchSynthEditor::ChurchSynthEditor (ChurchSynthProcessor& p) : AudioProcessorEditor (p), proc (p), ctx (p)
{
    ++openEditors;
    juce::LookAndFeel::setDefaultLookAndFeel (&lnf.get()); // menus and dialogs too
    setLookAndFeel (&lnf.get());
    root = std::make_unique<Root> (ctx);
    addAndMakeVisible (*root);
    root->top.onMenu = [this] { showMenu(); };
    tooltips = std::make_unique<juce::TooltipWindow> (this, 700);

    const float scale = loadScale();
    setResizable (true, true);
    if (auto* c = getConstrainer())
    {
        c->setFixedAspectRatio ((double) kW / kH);
        c->setSizeLimits (kW * 6 / 10, kH * 6 / 10, kW * 2, kH * 2);
    }
    setSize (juce::roundToInt (kW * scale), juce::roundToInt (kH * scale));
    proc.setEditorOpen (true);
    setWantsKeyboardFocus (true);
    lastEdit = proc.getEditCounter();
    root->refreshAll();
    startTimerHz (30);
}

ChurchSynthEditor::~ChurchSynthEditor()
{
    stopTimer();
    proc.setEditorOpen (false);
    proc.cancelMidiLearn();
    tooltips.reset();
    root.reset();
    setLookAndFeel (nullptr);
    if (--openEditors == 0) // last editor: stop using our look before it goes away
        juce::LookAndFeel::setDefaultLookAndFeel (nullptr);
}

void ChurchSynthEditor::paint (juce::Graphics& g)
{
    g.fillAll (ui::col::bg);
}

void ChurchSynthEditor::resized()
{
    const float scale = (float) getWidth() / (float) kW;
    root->setTransform (juce::AffineTransform::scale (scale));
    root->setBounds (0, 0, kW, kH);
    saveTimer = 30; // save the size once resizing settles
}

void ChurchSynthEditor::timerCallback()
{
    ++ticks;
    const auto edit = proc.getEditCounter();
    // Sound changed (parameter edit, preset, undo...): refresh the visible views at ~15 Hz.
    if (edit != lastEdit || ticks % 2 == 0)
    {
        if (edit != lastEdit || ticks % 6 == 0)
            root->refreshAll();
        lastEdit = edit;
    }
    root->tick (ticks % 6 == 0);
    if (saveTimer > 0 && --saveTimer == 0)
        saveScale ((float) getWidth() / (float) kW);
}

bool ChurchSynthEditor::keyPressed (const juce::KeyPress& k)
{
    const auto cmd = juce::ModifierKeys::commandModifier;
    if (k == juce::KeyPress ('z', cmd, 0))
    {
        proc.undo();
        return true;
    }
    if (k == juce::KeyPress ('z', cmd | juce::ModifierKeys::shiftModifier, 0) || k == juce::KeyPress ('y', cmd, 0))
    {
        proc.redo();
        return true;
    }
    if (k == juce::KeyPress::escapeKey && ctx.selectedSource >= 0)
    {
        ctx.selectSource (ctx.selectedSource); // toggles off
        return true;
    }
    return false;
}

void ChurchSynthEditor::showView (int view, int modTab, int engineTab)
{
    if (view == 3)
        root->showBrowser (true);
    else
        root->showPage (view == 1 ? ui::Page::Fx : view == 2 ? ui::Page::Seq : ui::Page::Synth);
    if (modTab >= 0)
        root->tabs.showTab (modTab);
    if (engineTab >= 0)
        root->synth.showEngineTab (engineTab);
    root->refreshAll();
    for (int i = 0; i < 8; ++i)
        root->tick (true);
}

void ChurchSynthEditor::showMenu()
{
    juce::PopupMenu m, size;
    m.addItem (1, "Init preset");
    m.addItem (2, "Save preset");
    m.addItem (3, "Save preset as...");
    m.addItem (4, "Preset browser");
    m.addSeparator();
    m.addItem (5, "Mod routes list...");
    m.addItem (6, "Open the Church Synth folder");
    for (int pct : { 70, 85, 100, 115, 130, 150 })
        size.addItem (100 + pct, juce::String (pct) + " %", true, std::abs ((float) getWidth() / kW * 100.0f - (float) pct) < 2.0f);
    m.addSubMenu ("Window size", size);
    m.addSeparator();
    m.addItem (7, "About Church Synth");
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&root->top), [this] (int r) {
        if (r == 1) proc.presets().loadInit();
        else if (r == 2) root->browser.saveOver();
        else if (r == 3) root->browser.saveAs();
        else if (r == 4) root->showBrowser (true);
        else if (r == 5) root->showRoutes();
        else if (r == 6)
        {
            PresetManager::userPresetFolder().createDirectory();
            PresetManager::rootFolder().startAsProcess();
        }
        else if (r == 7)
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::NoIcon, "Church Synth",
                                                    "Church Synth " JucePlugin_VersionString "\nA lean hybrid synth for live worship.\n\nFont: Inter (SIL Open Font License).");
        else if (r > 100)
            setSize (juce::roundToInt (kW * (r - 100) / 100.0f), juce::roundToInt (kH * (r - 100) / 100.0f));
    });
}
