#include "PresetBrowser.h"

#include "../PresetManager.h"

namespace ui
{
namespace
{
const char* const kBanks[4] = { "All presets", "Factory", "User", "Favourites" };
}

PresetBrowser::PresetBrowser (UiContext& c) : ctx (c)
{
    search.setTextToShowWhenEmpty ("Search presets", col::textFaint);
    search.setFont (font (14.0f));
    search.onTextChange = [this] { refresh(); };
    search.onEscapeKey = [this] { if (onClose) onClose(); };
    addAndMakeVisible (search);
    list.setRowHeight (26);
    list.setColour (juce::ListBox::backgroundColourId, col::panelLo);
    addAndMakeVisible (list);
    close.text = "Close";
    init.text = "Init";
    save.text = "Save";
    saveAsBtn.text = "Save As...";
    del.text = "Delete";
    favBtn.text = "Favourite";
    rescanBtn.text = "Rescan";
    rescanBtn.setTooltip ("Re-read the Presets folder (after copying preset files into it)");
    rescanBtn.onClick = [this] {
        ctx.proc.presets().rescan();
        refresh();
    };
    tagsBtn.text = "Edit Tags...";
    tagsBtn.onClick = [this] { showTagMenu (ctx.proc.presets().currentInfo(), &tagsBtn); };
    close.onClick = [this] { if (onClose) onClose(); };
    init.onClick = [this] { ctx.proc.presets().loadInit(); refresh(); };
    save.onClick = [this] { saveOver(); };
    saveAsBtn.onClick = [this] { saveAs(); };
    del.onClick = [this] {
        auto& pm = ctx.proc.presets();
        const auto info = pm.currentInfo();
        if (info.factory || ! info.file.existsAsFile())
            return;
        juce::AlertWindow::showOkCancelBox (juce::MessageBoxIconType::QuestionIcon, "Delete preset",
                                            "Delete \"" + info.name + "\"? This can't be undone.", "Delete", "Cancel", nullptr,
                                            juce::ModalCallbackFunction::create ([this, info] (int r) {
                                                if (r == 1)
                                                {
                                                    ctx.proc.presets().remove (info);
                                                    refresh();
                                                }
                                            }));
    };
    favBtn.onClick = [this] {
        auto& pm = ctx.proc.presets();
        const auto info = pm.currentInfo();
        pm.setFavourite (info, ! pm.isFavourite (info));
        refresh();
    };
    for (auto* b : { &close, &init, &save, &saveAsBtn, &del, &favBtn, &rescanBtn, &tagsBtn })
        addAndMakeVisible (b);
    setWantsKeyboardFocus (true);
    refresh();
}

void PresetBrowser::refresh()
{
    auto& pm = ctx.proc.presets();
    const auto& all = pm.all();
    {
        const auto tags = pm.allTags();
        for (int i = activeTags.size(); --i >= 0;) // drop filters for tags that no longer exist
            if (! tags.contains (activeTags[i], true))
                activeTags.remove (i);
    }
    const auto q = search.getText().trim().toLowerCase();
    rows.clear();
    for (size_t i = 0; i < all.size(); ++i)
    {
        const auto& p = all[i];
        if (bank == 1 && ! p.factory) continue;
        if (bank == 2 && p.factory) continue;
        if (bank == 3 && ! pm.isFavourite (p)) continue;
        if (category >= 0 && p.category != PresetManager::categories()[category]) continue;
        bool hasTags = true;
        for (const auto& t : activeTags)
            hasTags = hasTags && p.tags.contains (t, true);
        if (! hasTags) continue;
        if (q.isNotEmpty() && ! (p.name.toLowerCase().contains (q) || p.category.toLowerCase().contains (q)
                                 || p.description.toLowerCase().contains (q) || p.tags.joinIntoString (" ").toLowerCase().contains (q)))
            continue;
        rows.push_back ((int) i);
    }
    list.updateContent();
    const int cur = pm.indexOfCurrent();
    for (size_t r = 0; r < rows.size(); ++r)
        if (rows[r] == cur)
        {
            list.selectRow ((int) r, true, true);
            list.scrollToEnsureRowIsOnscreen ((int) r);
        }
    const auto info = pm.currentInfo();
    del.setEnabled (! info.factory && info.file.existsAsFile());
    favBtn.lit = pm.isFavourite (info);
    favBtn.repaint();
    resized(); // the tag chips may need more or fewer rows
    list.repaint();
    repaint();
}

int PresetBrowser::layoutChips (int x, int y, int width, bool apply)
{
    if (apply)
        chips.clear();
    const auto tags = ctx.proc.presets().allTags();
    if (tags.isEmpty())
        return 0;
    int cx = x, cy = y;
    const auto f = font (12.0f);
    auto place = [&] (const juce::String& t) {
        const int w = (int) std::ceil (juce::GlyphArrangement::getStringWidth (f, t)) + 20;
        if (cx + w > x + width && cx > x)
        {
            cx = x;
            cy += 26;
        }
        if (apply)
            chips.push_back ({ t, { cx, cy, w, 22 } });
        cx += w + 6;
    };
    place ("All tags");
    for (const auto& t : tags)
        place (t);
    return cy + 22 - y;
}

void PresetBrowser::askText (const juce::String& title, const juce::String& message, const juce::String& initial,
                             std::function<void (const juce::String&)> done)
{
    auto* w = new juce::AlertWindow (title, message, juce::MessageBoxIconType::NoIcon);
    w->addTextEditor ("text", initial);
    w->addButton ("OK", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    w->enterModalState (true, juce::ModalCallbackFunction::create ([w, done = std::move (done)] (int r) {
        const auto text = w->getTextEditorContents ("text").trim();
        if (r == 1 && text.isNotEmpty())
            done (text);
    }), true);
}

// Tick / untick tags on one preset, or make a new one.
void PresetBrowser::showTagMenu (const PresetInfo& p, juce::Component* target)
{
    auto& pm = ctx.proc.presets();
    const auto tags = pm.allTags();
    juce::PopupMenu m;
    m.addSectionHeader ("Tags for " + p.name);
    for (int i = 0; i < tags.size(); ++i)
        m.addItem (i + 1, tags[i], true, p.tags.contains (tags[i], true));
    m.addSeparator();
    m.addItem (10000, "New tag...");
    if (! p.tags.isEmpty())
        m.addItem (10001, "Clear tags");
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (target),
                     [this, p, tags, safe = juce::Component::SafePointer<PresetBrowser> (this)] (int r) {
                         if (safe == nullptr || r <= 0)
                             return;
                         auto& mgr = ctx.proc.presets();
                         auto t = p.tags;
                         if (r == 10000)
                         {
                             askText ("New tag", "Add a tag to \"" + p.name + "\":", {}, [this, p] (const juce::String& name) {
                                 auto t2 = p.tags;
                                 t2.addIfNotAlreadyThere (name, true);
                                 ctx.proc.presets().setTags (p, t2);
                                 refresh();
                             });
                             return;
                         }
                         if (r == 10001)
                             t.clear();
                         else if (const auto& tag = tags[r - 1]; t.contains (tag, true))
                             t.removeString (tag, true);
                         else
                             t.add (tag);
                         mgr.setTags (p, t);
                         refresh();
                     });
}

juce::String PresetBrowser::getTooltipForRow (int row)
{
    if (row < 0 || row >= (int) rows.size())
        return {};
    const auto& p = ctx.proc.presets().all()[(size_t) rows[(size_t) row]];
    return p.description + (p.tags.isEmpty() ? juce::String() : "\nTags: " + p.tags.joinIntoString (", "));
}

void PresetBrowser::step (int delta)
{
    auto& pm = ctx.proc.presets();
    auto r = rows;
    if (r.empty())
        return;
    const int cur = pm.indexOfCurrent();
    int pos = -1;
    for (size_t i = 0; i < r.size(); ++i)
        if (r[i] == cur)
            pos = (int) i;
    pos = pos < 0 ? 0 : (pos + delta + (int) r.size()) % (int) r.size();
    pm.load (pm.all()[(size_t) r[(size_t) pos]]);
    refresh();
}

void PresetBrowser::saveOver()
{
    auto& pm = ctx.proc.presets();
    const auto info = pm.currentInfo();
    if (info.factory || ! info.file.existsAsFile())
    {
        saveAs();
        return;
    }
    juce::String err;
    if (! pm.save (info.name, info.category, info.author, info.description, err))
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Church Synth", err);
    refresh();
}

void PresetBrowser::saveAs()
{
    auto& pm = ctx.proc.presets();
    const auto info = pm.currentInfo();
    auto* w = new juce::AlertWindow ("Save preset", "Your presets are saved in Documents/Church Synth/Presets.", juce::MessageBoxIconType::NoIcon);
    w->addTextEditor ("name", info.name == "Init" ? juce::String ("My Sound") : info.name, "Name");
    w->addComboBox ("category", PresetManager::categories(), "Category");
    if (auto* cb = w->getComboBoxComponent ("category"))
        cb->setSelectedItemIndex (juce::jmax (0, PresetManager::categories().indexOf (info.category)));
    w->addTextEditor ("author", info.author.isEmpty() || info.factory ? juce::String() : info.author, "Author");
    w->addTextEditor ("desc", info.factory ? juce::String() : info.description, "Description");
    w->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    w->enterModalState (true, juce::ModalCallbackFunction::create ([this, w] (int result) {
        if (result != 1)
            return;
        auto& pmgr = ctx.proc.presets();
        const auto name = w->getTextEditorContents ("name").trim();
        const auto cat = w->getComboBoxComponent ("category")->getText();
        const auto existing = PresetManager::userPresetFolder().getChildFile (juce::File::createLegalFileName (name) + PresetManager::kExtension);
        auto doSave = [this, name, cat, author = w->getTextEditorContents ("author"), desc = w->getTextEditorContents ("desc")] {
            juce::String err;
            if (! ctx.proc.presets().save (name, cat, author, desc, err))
                juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Church Synth", err);
            refresh();
        };
        juce::ignoreUnused (pmgr);
        if (existing.existsAsFile())
            juce::AlertWindow::showOkCancelBox (juce::MessageBoxIconType::QuestionIcon, "Replace preset?",
                                                "A user preset called \"" + name + "\" exists. Replace it?", "Replace", "Cancel", nullptr,
                                                juce::ModalCallbackFunction::create ([doSave] (int r) { if (r == 1) doSave(); }));
        else
            doSave();
    }), true);
}

void PresetBrowser::resized()
{
    auto r = getLocalBounds().reduced (12);
    filterArea = r.removeFromLeft (200);
    r.removeFromLeft (12);
    infoArea = r.removeFromRight (280);
    r.removeFromRight (12);
    auto top = r.removeFromTop (32);
    rescanBtn.setBounds (top.removeFromRight (90));
    top.removeFromRight (8);
    search.setBounds (top);
    r.removeFromTop (8);
    const int ch = layoutChips (r.getX(), r.getY(), r.getWidth(), true);
    chipArea = r.removeFromTop (ch);
    if (ch > 0)
        r.removeFromTop (8);
    list.setBounds (r.withTrimmedBottom (18));
    tagsBtn.setBounds (infoArea.getX(), infoArea.getY() + 186, infoArea.getWidth(), 26);
    auto b = infoArea.withTrimmedTop (226);
    auto row = [&b] { auto x = b.removeFromTop (30); b.removeFromTop (6); return x; };
    favBtn.setBounds (row());
    save.setBounds (row());
    saveAsBtn.setBounds (row());
    del.setBounds (row());
    init.setBounds (row());
    close.setBounds (infoArea.removeFromBottom (30));
}

void PresetBrowser::paint (juce::Graphics& g)
{
    g.fillAll (col::bg);
    drawPanel (g, filterArea.toFloat().expanded (6.0f));
    drawPanel (g, infoArea.toFloat().expanded (6.0f));
    drawSectionTitle (g, { filterArea.getX(), filterArea.getY(), filterArea.getWidth(), 20 }, "Banks", col::gold);
    for (int i = 0; i < 4; ++i)
    {
        const auto rr = bankRow (i);
        g.setColour (i == bank ? col::accentDim : col::panel);
        g.fillRoundedRectangle (rr.toFloat(), 4.0f);
        g.setColour (i == bank ? juce::Colours::white : col::text);
        g.setFont (font (13.0f));
        g.drawText (kBanks[i], rr.reduced (8, 0), juce::Justification::centredLeft);
    }
    drawSectionTitle (g, { filterArea.getX(), filterArea.getY() + 160, filterArea.getWidth(), 20 }, "Categories", col::gold);
    const auto& cats = PresetManager::categories();
    for (int i = -1; i < cats.size(); ++i)
    {
        const auto rr = catRow (i);
        g.setColour (i == category ? col::accentDim : col::panel);
        g.fillRoundedRectangle (rr.toFloat(), 4.0f);
        g.setColour (i == category ? juce::Colours::white : col::text);
        g.setFont (font (13.0f));
        g.drawText (i < 0 ? juce::String ("All") : cats[i], rr.reduced (8, 0), juce::Justification::centredLeft);
    }
    // Current preset info
    const auto info = ctx.proc.presets().currentInfo();
    auto ir = infoArea;
    g.setColour (col::text);
    g.setFont (font (18.0f, true));
    g.drawFittedText (info.name, ir.removeFromTop (30), juce::Justification::centredLeft, 1);
    g.setColour (col::accent);
    g.setFont (font (12.0f));
    g.drawText ((info.category.isEmpty() ? juce::String ("-") : info.category) + (info.factory ? "   Factory" : "   User"),
                ir.removeFromTop (20), juce::Justification::centredLeft);
    g.setColour (col::textDim);
    if (info.author.isNotEmpty())
        g.drawText ("by " + info.author, ir.removeFromTop (20), juce::Justification::centredLeft);
    g.setColour (col::text);
    g.setFont (font (12.5f));
    g.drawFittedText (info.description, ir.removeFromTop (90).reduced (0, 6), juce::Justification::topLeft, 5);
    g.setColour (col::gold);
    g.setFont (font (12.0f));
    g.drawFittedText (info.tags.isEmpty() ? juce::String ("No tags") : "Tags: " + info.tags.joinIntoString (", "),
                      ir.removeFromTop (34), juce::Justification::topLeft, 2);
    // Tag chips
    for (const auto& [tag, rr] : chips)
    {
        const bool all = tag == "All tags";
        const bool on = all ? activeTags.isEmpty() : activeTags.contains (tag, true);
        g.setColour (on ? col::accentDim : col::panel);
        g.fillRoundedRectangle (rr.toFloat(), 11.0f);
        g.setColour (on ? col::accent : col::border);
        g.drawRoundedRectangle (rr.toFloat().reduced (0.5f), 11.0f, 1.0f);
        g.setColour (on ? juce::Colours::white : (all ? col::textDim : col::text));
        g.setFont (font (12.0f));
        g.drawText (tag, rr, juce::Justification::centred);
    }
    g.setColour (col::textFaint);
    g.setFont (font (11.0f));
    g.drawText (juce::String ((int) rows.size()) + " presets", list.getBounds().translated (0, 0).removeFromBottom (0).withHeight (16).translated (0, 2),
                juce::Justification::centredRight);
}

void PresetBrowser::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected)
{
    if (row < 0 || row >= (int) rows.size())
        return;
    auto& pm = ctx.proc.presets();
    const auto& p = pm.all()[(size_t) rows[(size_t) row]];
    g.setColour (selected ? col::accentDim : (row % 2 ? col::panelLo : col::panel.withAlpha (0.6f)));
    g.fillRect (0, 0, w, h);
    if (pm.isFavourite (p))
    {
        g.setColour (col::gold);
        g.fillEllipse (10.0f, (float) h * 0.5f - 3.0f, 6.0f, 6.0f);
    }
    g.setColour (selected ? juce::Colours::white : col::text);
    g.setFont (font (13.5f));
    g.drawText (p.name, 26, 0, (int) (w * 0.42f) - 30, h, juce::Justification::centredLeft);
    g.setColour (selected ? juce::Colours::white : col::textDim);
    g.setFont (font (12.0f));
    g.drawText (p.category, (int) (w * 0.42f), 0, (int) (w * 0.16f), h, juce::Justification::centredLeft);
    g.setColour (selected ? juce::Colours::white.withAlpha (0.8f) : col::gold.withAlpha (0.75f));
    g.setFont (font (11.5f));
    g.drawText (p.tags.joinIntoString (", "), (int) (w * 0.58f), 0, (int) (w * 0.30f), h, juce::Justification::centredLeft);
    g.setColour (selected ? juce::Colours::white : col::textDim);
    g.setFont (font (12.0f));
    g.drawText (p.factory ? "Factory" : "User", (int) (w * 0.89f), 0, (int) (w * 0.11f) - 4, h, juce::Justification::centredLeft);
}

void PresetBrowser::listBoxItemClicked (int row, const juce::MouseEvent& e)
{
    if (row < 0 || row >= (int) rows.size())
        return;
    auto& pm = ctx.proc.presets();
    if (e.mods.isPopupMenu())
    {
        showTagMenu (pm.all()[(size_t) rows[(size_t) row]], &list);
        return;
    }
    pm.load (pm.all()[(size_t) rows[(size_t) row]]);
    refresh();
}

void PresetBrowser::mouseDown (const juce::MouseEvent& e)
{
    for (const auto& [tag, rr] : chips)
        if (rr.contains (e.getPosition()))
        {
            if (tag == "All tags")
            {
                activeTags.clear();
                refresh();
                return;
            }
            if (e.mods.isPopupMenu())
            {
                juce::PopupMenu m;
                m.addSectionHeader ("Tag: " + tag);
                m.addItem (1, "Rename tag...");
                m.addItem (2, "Delete tag...");
                m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (rr)),
                                 [this, tag, safe = juce::Component::SafePointer<PresetBrowser> (this)] (int r) {
                                     if (safe == nullptr)
                                         return;
                                     if (r == 1)
                                         askText ("Rename tag", "New name for \"" + tag + "\" (on every preset):", tag,
                                                  [this, tag] (const juce::String& to) {
                                                      ctx.proc.presets().renameTag (tag, to);
                                                      if (activeTags.contains (tag, true))
                                                      {
                                                          activeTags.removeString (tag, true);
                                                          activeTags.addIfNotAlreadyThere (to, true);
                                                      }
                                                      refresh();
                                                  });
                                     else if (r == 2)
                                         juce::AlertWindow::showOkCancelBox (juce::MessageBoxIconType::QuestionIcon, "Delete tag",
                                                                             "Remove the tag \"" + tag + "\" from every preset?", "Delete", "Cancel", nullptr,
                                                                             juce::ModalCallbackFunction::create ([this, tag] (int ok) {
                                                                                 if (ok == 1)
                                                                                 {
                                                                                     ctx.proc.presets().deleteTag (tag);
                                                                                     refresh();
                                                                                 }
                                                                             }));
                                 });
                return;
            }
            if (activeTags.contains (tag, true))
                activeTags.removeString (tag, true);
            else
                activeTags.add (tag);
            refresh();
            return;
        }
    for (int i = 0; i < 4; ++i)
        if (bankRow (i).contains (e.getPosition()))
        {
            bank = i;
            refresh();
        }
    for (int i = -1; i < PresetManager::categories().size(); ++i)
        if (catRow (i).contains (e.getPosition()))
        {
            category = i;
            refresh();
        }
}

bool PresetBrowser::keyPressed (const juce::KeyPress& k)
{
    if (k == juce::KeyPress::escapeKey)
    {
        if (onClose)
            onClose();
        return true;
    }
    if (k == juce::KeyPress::upKey || k == juce::KeyPress::downKey)
    {
        step (k == juce::KeyPress::upKey ? -1 : 1);
        return true;
    }
    return false;
}
} // namespace ui
