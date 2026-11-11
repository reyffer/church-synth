#include "PresetBrowser.h"

#include "../PresetManager.h"

#include <optional>

namespace ui
{
namespace
{
const char* const kBanks[4] = { "All presets", "Factory", "User", "Favourites" };
const juce::String kToggleChip = "\x01toggle"; // the "Show all tags" / "Show fewer" chip
constexpr int kTopTags = 12;                     // tags shown while collapsed
}

class PresetBrowser::SideList final : public juce::Component
{
public:
    explicit SideList (PresetBrowser& b) : owner (b) {}
    void paint (juce::Graphics& g) override
    {
        for (const auto& r : owner.side)
        {
            if (r.kind == SideRow::Heading)
            {
                drawSectionTitle (g, r.bounds, r.label, col::gold);
                continue;
            }
            const bool sel = owner.sideSelected (r);
            const bool sub = r.kind == SideRow::UserFolder || r.kind == SideRow::FavFolder || r.kind == SideRow::NewFavFolder;
            auto rr = r.bounds.withTrimmedLeft (sub ? 10 + r.depth * 12 : 0);
            g.setColour (sel ? col::accentDim : (sub ? col::panelLo : col::panel));
            g.fillRoundedRectangle (rr.toFloat(), 4.0f);
            g.setColour (r.kind == SideRow::NewFavFolder ? col::textDim : (sel ? juce::Colours::white : col::text));
            g.setFont (font (sub ? 12.0f : 13.0f));
            auto tr = rr.reduced (8, 0);
            if (r.count > 0)
            {
                g.setColour (sel ? juce::Colours::white.withAlpha (0.7f) : col::textFaint);
                g.setFont (font (11.0f));
                g.drawText (juce::String (r.count), tr.removeFromRight (34), juce::Justification::centredRight);
                g.setColour (r.kind == SideRow::NewFavFolder ? col::textDim : (sel ? juce::Colours::white : col::text));
                g.setFont (font (sub ? 12.0f : 13.0f));
            }
            juce::String label = r.label;
            if (r.kind == SideRow::Bank && (r.index == 2 || r.index == 3))
                label = juce::String (owner.bank == r.index ? "- " : "+ ") + label; // expands into folders
            g.drawFittedText (label, tr, juce::Justification::centredLeft, 1, 0.8f);
        }
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        for (const auto& r : owner.side)
            if (r.kind != SideRow::Heading && r.bounds.contains (e.getPosition()))
            {
                const auto copy = r; // the click may rebuild the list
                owner.sideClicked (copy, e);
                return;
            }
    }

private:
    PresetBrowser& owner;
};

PresetBrowser::~PresetBrowser() = default;

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
    sideList = std::make_unique<SideList> (*this);
    sideView.setViewedComponent (sideList.get(), false);
    sideView.setScrollBarsShown (true, false);
    sideView.setScrollBarThickness (6);
    addAndMakeVisible (sideView);
    close.text = "Close";
    init.text = "Init";
    save.text = "Save";
    saveAsBtn.text = "Save As...";
    del.text = "Delete";
    favBtn.text = "Favourite...";
    favBtn.setTooltip ("Make this preset a favourite, or put it in one or more favourite folders");
    notes.setMultiLine (true, true);
    notes.setReadOnly (true);
    notes.setScrollbarsShown (true);
    notes.setCaretVisible (false);
    notes.setFont (font (13.0f));
    notes.setColour (juce::TextEditor::backgroundColourId, col::panelLo);
    notes.setColour (juce::TextEditor::outlineColourId, col::border);
    notes.setColour (juce::TextEditor::focusedOutlineColourId, col::border);
    notes.setColour (juce::TextEditor::textColourId, col::text);
    notes.setIndents (8, 6);
    addAndMakeVisible (notes);
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
    favBtn.onClick = [this] { showFavouriteMenu (ctx.proc.presets().currentInfo(), &favBtn); };
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
        if (favFolder.isNotEmpty() && ! pm.favouriteFolders().contains (favFolder))
            favFolder = {};
        if (userFolder.isNotEmpty() && ! pm.userSubfolders().contains (userFolder))
            userFolder = {};
    }
    const auto q = search.getText().trim().toLowerCase();
    rows.clear();
    for (size_t i = 0; i < all.size(); ++i)
    {
        const auto& p = all[i];
        if (bank == 1 && ! p.factory) continue;
        if (bank == 2 && p.factory) continue;
        if (bank == 2 && userFolder.isNotEmpty())
        {
            const auto f = PresetManager::userSubfolder (p);
            if (f != userFolder && ! f.startsWith (userFolder + "/")) continue;
        }
        if (bank == 3 && ! pm.isFavourite (p)) continue;
        if (bank == 3 && favFolder.isNotEmpty() && ! pm.isInFavouriteFolder (p, favFolder)) continue;
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
    notes.setText (info.notes.isNotEmpty() ? info.notes : juce::String ("No notes for this preset yet."), false);
    favBtn.lit = pm.isFavourite (info);
    favBtn.repaint();
    rebuildSide();
    resized(); // the tag chips may need more or fewer rows
    list.repaint();
    repaint();
}

void PresetBrowser::setView (int newBank, const juce::String& folder, bool allTags)
{
    bank = juce::jlimit (0, 3, newBank);
    userFolder = bank == 2 ? folder : juce::String();
    favFolder = bank == 3 ? folder : juce::String();
    showAllTags = allTags;
    refresh();
}

int PresetBrowser::layoutChips (int x, int y, int width, bool apply)
{
    if (apply)
        chips.clear();
    auto& pm = ctx.proc.presets();
    const auto all = pm.allTags();
    if (all.isEmpty())
        return 0;
    // Collapsed: the tags on the most presets (most used first), plus any that are active.
    juce::StringArray tags = all;
    if (! showAllTags && all.size() > kTopTags)
    {
        std::map<juce::String, int> count;
        for (const auto& p : pm.all())
            for (const auto& t : p.tags)
                ++count[t.toLowerCase()];
        std::vector<juce::String> order (all.begin(), all.end());
        std::stable_sort (order.begin(), order.end(), [&count] (const juce::String& a, const juce::String& b) {
            return count[a.toLowerCase()] > count[b.toLowerCase()];
        });
        tags.clear();
        for (int i = 0; i < kTopTags; ++i)
            tags.add (order[(size_t) i]);
        for (const auto& t : activeTags)
            tags.addIfNotAlreadyThere (t, true);
    }
    int cx = x, cy = y;
    const auto f = font (12.0f);
    const auto toggleText = showAllTags ? juce::String ("Show fewer") : "Show all tags (" + juce::String (all.size()) + ")";
    auto place = [&] (const juce::String& t) {
        const int w = (int) std::ceil (juce::GlyphArrangement::getStringWidth (f, t == kToggleChip ? toggleText : t)) + 20;
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
    if (all.size() > kTopTags)
        place (kToggleChip);
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
void PresetBrowser::addTagItems (juce::PopupMenu& m, const PresetInfo& p, int base)
{
    const auto tags = ctx.proc.presets().allTags();
    for (int i = 0; i < tags.size(); ++i)
        m.addItem (base + i + 1, tags[i], true, p.tags.contains (tags[i], true));
    m.addSeparator();
    m.addItem (base + 10000, "New tag...");
    if (! p.tags.isEmpty())
        m.addItem (base + 10001, "Clear tags");
}

void PresetBrowser::handleTagItem (const PresetInfo& p, const juce::StringArray& tags, int r)
{
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
    else if (r >= 1 && r <= tags.size())
    {
        if (const auto& tag = tags[r - 1]; t.contains (tag, true))
            t.removeString (tag, true);
        else
            t.add (tag);
    }
    else
        return;
    ctx.proc.presets().setTags (p, t);
    refresh();
}

void PresetBrowser::showTagMenu (const PresetInfo& p, juce::Component* target)
{
    juce::PopupMenu m;
    m.addSectionHeader ("Tags for " + p.name);
    addTagItems (m, p, 0);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (target),
                     [this, p, tags = ctx.proc.presets().allTags(), safe = juce::Component::SafePointer<PresetBrowser> (this)] (int r) {
                         if (safe != nullptr && r > 0)
                             handleTagItem (p, tags, r);
                     });
}

// Favourite on / off, and membership of each favourite folder.
void PresetBrowser::addFavouriteItems (juce::PopupMenu& m, const PresetInfo& p, int base)
{
    auto& pm = ctx.proc.presets();
    m.addItem (base + 1, "Favourite", true, pm.isFavourite (p));
    const auto folders = pm.favouriteFolders();
    if (! folders.isEmpty())
    {
        m.addSeparator();
        m.addSectionHeader ("Favourite folders");
        for (int i = 0; i < folders.size(); ++i)
            m.addItem (base + 10 + i, folders[i], true, pm.isInFavouriteFolder (p, folders[i]));
    }
    m.addSeparator();
    m.addItem (base + 2, "New folder...");
}

void PresetBrowser::handleFavouriteItem (const PresetInfo& p, const juce::StringArray& folders, int r)
{
    auto& pm = ctx.proc.presets();
    if (r == 1)
        pm.setFavourite (p, ! pm.isFavourite (p));
    else if (r == 2)
    {
        newFavouriteFolder (&p);
        return;
    }
    else if (r >= 10 && r - 10 < folders.size())
        pm.setInFavouriteFolder (p, folders[r - 10], ! pm.isInFavouriteFolder (p, folders[r - 10]));
    else
        return;
    refresh();
}

void PresetBrowser::showFavouriteMenu (const PresetInfo& p, juce::Component* target)
{
    juce::PopupMenu m;
    m.addSectionHeader (p.name);
    addFavouriteItems (m, p, 0);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (target),
                     [this, p, folders = ctx.proc.presets().favouriteFolders(), safe = juce::Component::SafePointer<PresetBrowser> (this)] (int r) {
                         if (safe != nullptr && r > 0)
                             handleFavouriteItem (p, folders, r);
                     });
}

void PresetBrowser::showPresetMenu (const PresetInfo& p, juce::Component* target)
{
    juce::PopupMenu m, tagMenu;
    m.addSectionHeader (p.name);
    addFavouriteItems (m, p, 20000);
    addTagItems (tagMenu, p, 0);
    m.addSeparator();
    m.addSubMenu ("Tags", tagMenu);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (target),
                     [this, p, folders = ctx.proc.presets().favouriteFolders(), tags = ctx.proc.presets().allTags(),
                      safe = juce::Component::SafePointer<PresetBrowser> (this)] (int r) {
                         if (safe == nullptr || r <= 0)
                             return;
                         if (r > 20000)
                             handleFavouriteItem (p, folders, r - 20000);
                         else
                             handleTagItem (p, tags, r);
                     });
}

void PresetBrowser::newFavouriteFolder (const PresetInfo* addPreset)
{
    std::optional<PresetInfo> p;
    if (addPreset != nullptr)
        p = *addPreset;
    askText ("New favourite folder", p ? "Name of the new folder (\"" + p->name + "\" will be put in it):" : juce::String ("Name of the new folder:"), {},
             [this, p] (const juce::String& name) {
                 auto& pm = ctx.proc.presets();
                 if (! pm.addFavouriteFolder (name) && ! pm.favouriteFolders().contains (name, true))
                     return;
                 if (p)
                     for (const auto& f : pm.favouriteFolders())
                         if (f.equalsIgnoreCase (name.trim()))
                             pm.setInFavouriteFolder (*p, f, true);
                 refresh();
             });
}

// --- Left side list ----------------------------------------------------------------------
void PresetBrowser::rebuildSide()
{
    auto& pm = ctx.proc.presets();
    side.clear();
    const int w = juce::jmax (100, filterArea.getWidth() - 8);
    int y = 0;
    auto mk = [] (SideRow::Kind k, const juce::String& label, const juce::String& value = {}, int index = 0) {
        SideRow r;
        r.kind = k;
        r.label = label;
        r.value = value;
        r.index = index;
        return r;
    };
    auto add = [&] (SideRow r, int h) {
        r.bounds = { 0, y, w, h };
        y += h + 2;
        side.push_back (r);
    };
    add (mk (SideRow::Heading, "Banks"), 22);
    y += 4;
    for (int i = 0; i < 4; ++i)
    {
        add (mk (SideRow::Bank, kBanks[i], {}, i), 22);
        if (i == 2 && bank == 2)
        {
            std::map<juce::String, int> counts;
            for (const auto& p : pm.all())
                for (auto f = PresetManager::userSubfolder (p); f.isNotEmpty();)
                {
                    ++counts[f];
                    if (! f.containsChar ('/'))
                        break;
                    f = f.upToLastOccurrenceOf ("/", false, false);
                }
            for (const auto& f : pm.userSubfolders())
            {
                auto r = mk (SideRow::UserFolder, f.fromLastOccurrenceOf ("/", false, false), f);
                r.depth = f.retainCharacters ("/").length();
                r.count = counts[f];
                add (r, 20);
            }
        }
        if (i == 3 && bank == 3)
        {
            for (const auto& f : pm.favouriteFolders())
            {
                auto r = mk (SideRow::FavFolder, f, f);
                for (const auto& p : pm.all())
                    r.count += pm.isInFavouriteFolder (p, f) ? 1 : 0;
                add (r, 20);
            }
            add (mk (SideRow::NewFavFolder, "+ New folder..."), 20);
        }
    }
    y += 10;
    add (mk (SideRow::Heading, "Categories"), 22);
    y += 4;
    const auto& cats = PresetManager::categories();
    for (int i = -1; i < cats.size(); ++i)
        add (mk (SideRow::Category, i < 0 ? juce::String ("All") : cats[i], {}, i), 22);
    sideList->setSize (w, y);
    sideList->repaint();
}

bool PresetBrowser::sideSelected (const SideRow& r) const
{
    switch (r.kind)
    {
        case SideRow::Bank: return r.index == bank && (bank == 2 ? userFolder.isEmpty() : bank == 3 ? favFolder.isEmpty() : true);
        case SideRow::UserFolder: return bank == 2 && userFolder == r.value;
        case SideRow::FavFolder: return bank == 3 && favFolder == r.value;
        case SideRow::Category: return r.index == category;
        case SideRow::Heading:
        case SideRow::NewFavFolder: return false;
    }
    return false;
}

void PresetBrowser::sideClicked (const SideRow& r, const juce::MouseEvent& e)
{
    auto& pm = ctx.proc.presets();
    switch (r.kind)
    {
        case SideRow::Bank:
            if (e.mods.isPopupMenu() && r.index == 3)
            {
                juce::PopupMenu m;
                m.addItem (1, "New favourite folder...");
                m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (sideList->localAreaToGlobal (r.bounds)),
                                 [this, safe = juce::Component::SafePointer<PresetBrowser> (this)] (int res) {
                                     if (safe != nullptr && res == 1)
                                         newFavouriteFolder (nullptr);
                                 });
                return;
            }
            bank = r.index;
            userFolder = {};
            favFolder = {};
            break;
        case SideRow::UserFolder:
            userFolder = r.value;
            break;
        case SideRow::FavFolder:
            if (e.mods.isPopupMenu())
            {
                juce::PopupMenu m;
                m.addSectionHeader ("Folder: " + r.value);
                m.addItem (1, "Rename folder...");
                m.addItem (2, "Delete folder...");
                m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (sideList->localAreaToGlobal (r.bounds)),
                                 [this, name = r.value, safe = juce::Component::SafePointer<PresetBrowser> (this)] (int res) {
                                     if (safe == nullptr)
                                         return;
                                     if (res == 1)
                                         askText ("Rename folder", "New name for \"" + name + "\":", name, [this, name] (const juce::String& to) {
                                             if (ctx.proc.presets().renameFavouriteFolder (name, to) && favFolder == name)
                                                 favFolder = to.trim();
                                             refresh();
                                         });
                                     else if (res == 2)
                                         juce::AlertWindow::showOkCancelBox (juce::MessageBoxIconType::QuestionIcon, "Delete folder",
                                                                             "Delete the favourite folder \"" + name + "\"? Its presets stay favourites.", "Delete", "Cancel", nullptr,
                                                                             juce::ModalCallbackFunction::create ([this, name] (int ok) {
                                                                                 if (ok == 1)
                                                                                 {
                                                                                     ctx.proc.presets().deleteFavouriteFolder (name);
                                                                                     refresh();
                                                                                 }
                                                                             }));
                                 });
                return;
            }
            favFolder = r.value;
            break;
        case SideRow::NewFavFolder:
            newFavouriteFolder (nullptr);
            return;
        case SideRow::Category:
            category = r.index;
            break;
        case SideRow::Heading:
            return;
    }
    juce::ignoreUnused (pm);
    refresh();
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
    sideView.setBounds (filterArea);
    if (sideList != nullptr && sideList->getWidth() != juce::jmax (100, filterArea.getWidth() - 8))
        rebuildSide();
    tagsBtn.setBounds (infoArea.getX(), infoArea.getY() + 186, infoArea.getWidth(), 26);
    auto b = infoArea.withTrimmedTop (222);
    auto row = [&b] (FlatButton& x, FlatButton& y) {
        auto rr = b.removeFromTop (28);
        x.setBounds (rr.removeFromLeft (rr.getWidth() / 2 - 3));
        rr.removeFromLeft (6);
        y.setBounds (rr);
        b.removeFromTop (6);
    };
    row (favBtn, save);
    row (saveAsBtn, del);
    init.setBounds (b.removeFromTop (28));
    b.removeFromTop (12);
    close.setBounds (b.removeFromBottom (30));
    b.removeFromBottom (10);
    notesTitle = b.removeFromTop (18);
    notes.setBounds (b);
}

void PresetBrowser::paint (juce::Graphics& g)
{
    g.fillAll (col::bg);
    drawPanel (g, filterArea.toFloat().expanded (6.0f));
    drawPanel (g, infoArea.toFloat().expanded (6.0f));
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
    g.setColour (col::textDim);
    g.setFont (font (11.0f, true));
    g.drawText ("HOW IT'S BUILT", notesTitle, juce::Justification::centredLeft);
    // Tag chips
    for (const auto& [tag, rr] : chips)
    {
        const bool all = tag == "All tags";
        if (tag == kToggleChip)
        {
            g.setColour (col::panelLo);
            g.fillRoundedRectangle (rr.toFloat(), 11.0f);
            g.setColour (col::gold.withAlpha (0.6f));
            g.drawRoundedRectangle (rr.toFloat().reduced (0.5f), 11.0f, 1.0f);
            g.setColour (col::gold);
            g.setFont (font (12.0f));
            g.drawText (showAllTags ? juce::String ("Show fewer") : "Show all tags (" + juce::String (ctx.proc.presets().allTags().size()) + ")",
                        rr, juce::Justification::centred);
            continue;
        }
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
        showPresetMenu (pm.all()[(size_t) rows[(size_t) row]], &list);
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
            if (tag == kToggleChip)
            {
                showAllTags = ! showAllTags;
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
                                         askText ("Rename tag", "New name for \"" + tag + "\" (on every preset, and on presets you add later):", tag,
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
                                                                             "Remove the tag \"" + tag + "\" from every preset (and from presets you add later)?", "Delete", "Cancel", nullptr,
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
