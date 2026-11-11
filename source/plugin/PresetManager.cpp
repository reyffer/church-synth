#include "PresetManager.h"

#include "dsp/fx/Effects.h"

namespace
{
const char* const kFxSlotIds[9] = { "a1", "a2", "a3", "b1", "b2", "b3", "x1", "x2", "x3" };
constexpr int kPresetFormat = 1;
} // namespace

// ===========================================================================
// PresetBuilder
// ===========================================================================
PresetBuilder::PresetBuilder() : params ("ChurchSynth")
{
    for (const auto& d : cs::parameterDefs())
    {
        juce::ValueTree p ("PARAM");
        p.setProperty ("id", d.id, nullptr);
        p.setProperty ("value", d.def, nullptr);
        params.appendChild (p, nullptr);
    }
}

juce::ValueTree PresetBuilder::param (const juce::String& id)
{
    auto p = params.getChildWithProperty ("id", id);
    if (! p.isValid())
        errors.add ("unknown parameter " + id);
    return p;
}

PresetBuilder& PresetBuilder::set (const juce::String& id, float value)
{
    if (auto p = param (id); p.isValid())
    {
        for (const auto& d : cs::parameterDefs())
            if (d.id == id)
            {
                if (d.kind == cs::ParamDef::Choice)
                    value = juce::jlimit (0.0f, (float) (d.choices.size() - 1), std::round (value));
                else
                    value = juce::jlimit (d.min, d.max, value);
                break;
            }
        p.setProperty ("value", value, nullptr);
    }
    return *this;
}

PresetBuilder& PresetBuilder::choice (const juce::String& id, const juce::String& itemName)
{
    for (const auto& d : cs::parameterDefs())
        if (d.id == id)
        {
            int idx = d.choices.indexOf (itemName, true);
            if (idx < 0) // "Category: Name" lists: match the name part
                for (int i = 0; i < d.choices.size(); ++i)
                    if (d.choices[i].fromFirstOccurrenceOf (": ", false, false).equalsIgnoreCase (itemName))
                        idx = i;
            if (idx < 0)
                errors.add ("unknown item '" + itemName + "' for " + id);
            if (idx >= 0)
                set (id, (float) idx);
            break;
        }
    return *this;
}

PresetBuilder& PresetBuilder::route (const juce::String& source, const juce::String& destId, float amount,
                                     const juce::String& sidechain, float scAmount)
{
    if (cs::findModSource (source.toStdString()) < 0 || cs::findModDest (destId.toStdString()) < 0
        || (sidechain.isNotEmpty() && cs::findModSource (sidechain.toStdString()) < 0))
        errors.add ("bad route " + source + " > " + destId + (sidechain.isNotEmpty() ? " (sc " + sidechain + ")" : juce::String()));
    juce::ValueTree e ("ROUTE");
    e.setProperty ("src", source, nullptr);
    e.setProperty ("dest", destId, nullptr);
    e.setProperty ("amt", amount, nullptr);
    e.setProperty ("sc", sidechain, nullptr);
    e.setProperty ("sca", scAmount, nullptr);
    routes.appendChild (e, nullptr);
    return *this;
}

PresetBuilder& PresetBuilder::fx (int slot, int type, std::initializer_list<std::pair<int, float>> knobs)
{
    const juce::String p = juce::String ("fx_") + kFxSlotIds[juce::jlimit (0, 8, slot)] + "_";
    set (p + "type", (float) type);
    set (p + "on", 1.0f);
    const auto& info = cs::fx::fxInfo (type);
    for (int k = 0; k < cs::fx::kFxParams; ++k)
    {
        float v = info.defaults[(size_t) k];
        if (slot >= 6 && info.params[(size_t) k] != nullptr && juce::String (info.params[(size_t) k]) == "Dry/Wet")
            v = 1.0f;
        set (p + "p" + juce::String (k + 1), v);
    }
    for (const auto& kv : knobs)
        set (p + "p" + juce::String (kv.first + 1), kv.second);
    return *this;
}

PresetBuilder& PresetBuilder::pattern (const cs::SeqPattern& pat)
{
    seq = ChurchSynthProcessor::patternTreeOf (pat);
    return *this;
}

PresetBuilder& PresetBuilder::macroName (int m, const juce::String& name)
{
    macroNames[juce::jlimit (0, 3, m)] = name;
    return *this;
}

juce::ValueTree PresetBuilder::build (const juce::String& name, const juce::String& category) const
{
    auto t = params.createCopy();
    t.appendChild (routes.createCopy(), nullptr);
    t.appendChild (seq.isValid() ? seq.createCopy() : ChurchSynthProcessor::patternTreeOf (cs::SeqPattern()), nullptr);
    for (int m = 0; m < 4; ++m)
        t.setProperty ("macroName" + juce::String (m + 1), macroNames[m].isEmpty() ? "Macro " + juce::String (m + 1) : macroNames[m], nullptr);
    t.setProperty ("presetName", name, nullptr);
    t.setProperty ("presetCategory", category, nullptr);
    return t;
}

// ===========================================================================
// PresetManager
// ===========================================================================
PresetManager::PresetManager (ChurchSynthProcessor& p) : proc (p)
{
    loadFavourites();
    loadFavouriteFolders();
}

juce::File PresetManager::rootFolder()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("Church Synth");
}

const juce::StringArray& PresetManager::categories()
{
    static const juce::StringArray c { "Pad", "Keys", "Pluck", "Lead", "Bass", "Strings/Brass", "Texture", "Sequence", "FX" };
    return c;
}

juce::String PresetManager::safeFileName (const juce::String& name)
{
    return juce::File::createLegalFileName (name.trim()).substring (0, 80);
}

void PresetManager::rescan()
{
    list.clear();
    const auto& fp = factoryPresets();
    for (size_t i = 0; i < fp.size(); ++i)
    {
        PresetInfo p;
        p.name = fp[i].name;
        p.category = fp[i].category;
        p.author = "Church Synth";
        p.description = fp[i].description;
        p.factory = true;
        p.factoryIndex = (int) i;
        p.tags = factoryPresetTags (p.name);
        p.notes = factoryPresetNotes (p.name);
        list.push_back (p);
    }
    std::vector<PresetInfo> user;
    for (const auto& f : userPresetFolder().findChildFiles (juce::File::findFiles, true, juce::String ("*") + kExtension))
        if (auto xml = juce::XmlDocument::parse (f); xml != nullptr && xml->hasTagName ("ChurchSynthPreset"))
        {
            PresetInfo p;
            p.name = xml->getStringAttribute ("name", f.getFileNameWithoutExtension());
            p.category = xml->getStringAttribute ("category");
            p.author = xml->getStringAttribute ("author");
            p.description = xml->getStringAttribute ("description");
            p.factory = false;
            p.file = f;
            p.tags = parseTags (xml->getStringAttribute ("tags"));
            if (auto* n = xml->getChildByName ("NOTES"))
                p.notes = n->getAllSubText().trim();
            user.push_back (p);
        }
    std::sort (user.begin(), user.end(), [] (const PresetInfo& a, const PresetInfo& b) { return a.name.compareNatural (b.name) < 0; });
    list.insert (list.end(), user.begin(), user.end());
    loadTagEdits(); // re-read: another instance may have changed them
    loadFavourites();
    loadFavouriteFolders();
    for (auto& p : list)
        if (auto it = tagEdits.find (p.key()); it != tagEdits.end())
            p.tags = it->second;
        else
            p.tags = applyTagRules (p.tags);
    scanned = true;
}

const std::vector<PresetInfo>& PresetManager::all()
{
    if (! scanned)
        rescan();
    return list;
}

juce::ValueTree PresetManager::treeFor (const PresetInfo& p)
{
    if (p.factory)
    {
        const auto& fp = factoryPresets();
        if (p.factoryIndex < 0 || p.factoryIndex >= (int) fp.size())
            return {};
        PresetBuilder b;
        fp[(size_t) p.factoryIndex].build (b);
        return b.build (p.name, p.category);
    }
    if (auto xml = juce::XmlDocument::parse (p.file); xml != nullptr && xml->hasTagName ("ChurchSynthPreset"))
        if (auto* inner = xml->getChildByName ("ChurchSynth"))
        {
            auto t = juce::ValueTree::fromXml (*inner);
            t.setProperty ("presetName", p.name, nullptr);
            t.setProperty ("presetCategory", p.category, nullptr);
            return t;
        }
    return {};
}

bool PresetManager::load (const PresetInfo& p)
{
    const auto t = treeFor (p);
    if (! t.isValid())
        return false;
    proc.restoreStateTree (t, ChurchSynthProcessor::LoadKind::Preset);
    proc.setPresetInfo (p.name, p.category, false);
    proc.checkpoint ("Preset: " + p.name);
    proc.setPresetInfo (p.name, p.category, false);
    return true;
}

void PresetManager::loadInit()
{
    PresetBuilder b;
    proc.restoreStateTree (b.build ("Init", {}), ChurchSynthProcessor::LoadKind::Preset);
    proc.setPresetInfo ("Init", {}, false);
    proc.checkpoint ("Init preset");
    proc.setPresetInfo ("Init", {}, false);
}

juce::ValueTree PresetManager::currentAsPresetTree (const juce::String& name, const juce::String& category,
                                                     const juce::String& author, const juce::String& description)
{
    auto t = proc.createStateTree();
    t.removeChild (t.getChildWithName ("MIDIMAP"), nullptr);
    t.removeProperty ("presetModified", nullptr);
    t.setProperty ("presetName", name, nullptr);
    t.setProperty ("presetCategory", category, nullptr);
    juce::ignoreUnused (author, description);
    return t;
}

bool PresetManager::save (const juce::String& name, const juce::String& category, const juce::String& author,
                          const juce::String& description, juce::String& error)
{
    if (name.trim().isEmpty())
    {
        error = "Give the preset a name.";
        return false;
    }
    auto folder = userPresetFolder();
    if (! folder.createDirectory())
    {
        error = "Couldn't create " + folder.getFullPathName();
        return false;
    }
    const auto cur = currentInfo();
    // Re-saving the loaded user preset (it may live in a pack subfolder): overwrite it in place.
    const bool inPlace = ! cur.factory && cur.file.existsAsFile() && cur.name == name.trim();
    const auto file = inPlace ? cur.file : folder.getChildFile (safeFileName (name) + kExtension);
    if (! writePresetFile (file, name, category, author, description, cur.tags))
    {
        error = "Couldn't write " + file.getFullPathName();
        return false;
    }
    proc.setPresetInfo (name.trim(), category, false);
    rescan();
    return true;
}

bool PresetManager::writePresetFile (const juce::File& file, const juce::String& name, const juce::String& category,
                                     const juce::String& author, const juce::String& description,
                                     const juce::StringArray& tags, const juce::String& notes)
{
    juce::XmlElement root ("ChurchSynthPreset");
    root.setAttribute ("format", kPresetFormat);
    root.setAttribute ("name", name.trim());
    root.setAttribute ("category", category);
    root.setAttribute ("author", author);
    root.setAttribute ("description", description);
    root.setAttribute ("version", JucePlugin_VersionString);
    if (! tags.isEmpty())
        root.setAttribute ("tags", tags.joinIntoString (", "));
    if (notes.isNotEmpty())
        root.createNewChildElement ("NOTES")->addTextElement (notes);
    if (auto inner = currentAsPresetTree (name.trim(), category, author, description).createXml())
        root.addChildElement (inner.release());
    return root.writeTo (file);
}

bool PresetManager::remove (const PresetInfo& p)
{
    if (p.factory || ! p.file.existsAsFile())
        return false;
    const bool ok = p.file.deleteFile();
    setFavourite (p, false);
    if (tagEdits.erase (p.key()) > 0)
        saveTagEdits();
    rescan();
    return ok;
}

int PresetManager::indexOfCurrent()
{
    const auto& l = all();
    for (int i = (int) l.size() - 1; i >= 0; --i) // user presets win over factory ones with the same name
        if (l[(size_t) i].name == proc.presetName() && l[(size_t) i].category == proc.presetCategory())
            return i;
    return -1;
}

PresetInfo PresetManager::currentInfo()
{
    const int i = indexOfCurrent();
    if (i >= 0)
        return all()[(size_t) i];
    PresetInfo p;
    p.name = proc.presetName();
    p.category = proc.presetCategory();
    return p;
}

void PresetManager::loadFavourites()
{
    favourites.clear();
    juce::StringArray lines;
    lines.addLines (rootFolder().getChildFile ("Favourites.txt").loadFileAsString());
    for (const auto& l : lines)
        if (l.trim().isNotEmpty())
            favourites.insert (l.trim());
}

void PresetManager::saveFavourites() const
{
    rootFolder().createDirectory();
    juce::String s;
    for (const auto& f : favourites)
        s << f << "\n";
    rootFolder().getChildFile ("Favourites.txt").replaceWithText (s);
}

// --- Tags ------------------------------------------------------------------------------
juce::StringArray PresetManager::parseTags (const juce::String& commaList)
{
    juce::StringArray t;
    t.addTokens (commaList, ",", "\"");
    t.trim();
    t.removeEmptyStrings();
    t.removeDuplicates (true);
    return t;
}

juce::StringArray PresetManager::allTags()
{
    juce::StringArray t;
    for (const auto& p : all())
        for (const auto& tag : p.tags)
            if (! t.contains (tag, true))
                t.add (tag);
    t.sortNatural();
    return t;
}

juce::StringArray PresetManager::applyTagRules (const juce::StringArray& tags) const
{
    juce::StringArray out;
    for (auto t : tags)
    {
        for (int hop = 0; hop < 16; ++hop) // follow rename chains (A -> B -> C)
        {
            const auto it = tagRenames.find (t.toLowerCase());
            if (it == tagRenames.end())
                break;
            t = it->second;
        }
        if (! tagDeletes.contains (t, true))
            out.addIfNotAlreadyThere (t, true);
    }
    return out;
}

void PresetManager::setTags (const PresetInfo& p, const juce::StringArray& tags)
{
    all();
    for (const auto& t : tags) // using a tag again un-deletes it for future imports
        tagDeletes.removeString (t, true);
    tagEdits[p.key()] = tags;
    for (auto& q : list)
        if (q.key() == p.key())
            q.tags = tags;
    saveTagEdits();
}

void PresetManager::renameTag (const juce::String& from, const juce::String& to)
{
    const auto newName = to.trim();
    if (newName.isEmpty() || from == newName)
        return;
    all();
    // Remember the rule (and point older rules at the new name, without loops).
    tagRenames.erase (newName.toLowerCase());
    for (auto& [src, dst] : tagRenames)
        if (dst.equalsIgnoreCase (from))
            dst = newName;
    if (! from.equalsIgnoreCase (newName))
        tagRenames[from.toLowerCase()] = newName;
    tagDeletes.removeString (newName, true);
    for (auto& p : list)
        if (const int i = p.tags.indexOf (from, true); i >= 0)
        {
            auto t = p.tags;
            if (t.contains (newName, true))
                t.remove (i);
            else
                t.set (i, newName);
            p.tags = t;
            tagEdits[p.key()] = t;
        }
    saveTagEdits();
}

void PresetManager::deleteTag (const juce::String& tag)
{
    all();
    tagDeletes.addIfNotAlreadyThere (tag, true);
    for (auto& p : list)
        if (p.tags.contains (tag, true))
        {
            p.tags.removeString (tag, true);
            tagEdits[p.key()] = p.tags;
        }
    saveTagEdits();
}

void PresetManager::loadTagEdits()
{
    tagEdits.clear();
    tagRenames.clear();
    tagDeletes.clear();
    if (auto xml = juce::XmlDocument::parse (rootFolder().getChildFile ("Preset Tags.xml")))
    {
        for (auto* e : xml->getChildWithTagNameIterator ("PRESET"))
            tagEdits[e->getStringAttribute ("key")] = parseTags (e->getStringAttribute ("tags"));
        for (auto* e : xml->getChildWithTagNameIterator ("RENAME"))
            tagRenames[e->getStringAttribute ("from").toLowerCase()] = e->getStringAttribute ("to");
        for (auto* e : xml->getChildWithTagNameIterator ("DELETED"))
            tagDeletes.addIfNotAlreadyThere (e->getStringAttribute ("tag"), true);
    }
}

void PresetManager::saveTagEdits() const
{
    rootFolder().createDirectory();
    juce::XmlElement root ("PRESET_TAGS");
    for (const auto& [key, tags] : tagEdits)
    {
        auto* e = root.createNewChildElement ("PRESET");
        e->setAttribute ("key", key);
        e->setAttribute ("tags", tags.joinIntoString (", "));
    }
    for (const auto& [from, to] : tagRenames)
    {
        auto* e = root.createNewChildElement ("RENAME");
        e->setAttribute ("from", from);
        e->setAttribute ("to", to);
    }
    for (const auto& t : tagDeletes)
        root.createNewChildElement ("DELETED")->setAttribute ("tag", t);
    root.writeTo (rootFolder().getChildFile ("Preset Tags.xml"));
}

void PresetManager::setFavourite (const PresetInfo& p, bool fav)
{
    if (fav)
        favourites.insert (p.key());
    else
    {
        favourites.erase (p.key());
        bool changed = false;
        for (auto& [name, keys] : favFolders)
            changed = keys.erase (p.key()) > 0 || changed;
        if (changed)
            saveFavouriteFolders();
    }
    saveFavourites();
}

// --- Favourite folders ------------------------------------------------------------------
void PresetManager::loadFavouriteFolders()
{
    favFolders.clear();
    if (auto xml = juce::XmlDocument::parse (rootFolder().getChildFile ("Favourite Folders.xml")))
        for (auto* f : xml->getChildWithTagNameIterator ("FOLDER"))
        {
            const auto name = f->getStringAttribute ("name").trim();
            if (name.isEmpty())
                continue;
            auto& keys = favFolders[name];
            for (auto* pr : f->getChildWithTagNameIterator ("PRESET"))
                if (const auto k = pr->getStringAttribute ("key"); k.isNotEmpty())
                    keys.insert (k);
        }
}

void PresetManager::saveFavouriteFolders() const
{
    rootFolder().createDirectory();
    juce::XmlElement root ("FAVOURITE_FOLDERS");
    for (const auto& [name, keys] : favFolders)
    {
        auto* f = root.createNewChildElement ("FOLDER");
        f->setAttribute ("name", name);
        for (const auto& k : keys)
            f->createNewChildElement ("PRESET")->setAttribute ("key", k);
    }
    root.writeTo (rootFolder().getChildFile ("Favourite Folders.xml"));
}

juce::StringArray PresetManager::favouriteFolders() const
{
    juce::StringArray r;
    for (const auto& [name, keys] : favFolders)
        r.add (name);
    r.sortNatural();
    return r;
}

bool PresetManager::isInFavouriteFolder (const PresetInfo& p, const juce::String& folder) const
{
    const auto it = favFolders.find (folder);
    return it != favFolders.end() && it->second.count (p.key()) > 0;
}

void PresetManager::setInFavouriteFolder (const PresetInfo& p, const juce::String& folder, bool in)
{
    auto it = favFolders.find (folder);
    if (it == favFolders.end())
        return;
    if (in)
    {
        it->second.insert (p.key());
        if (favourites.insert (p.key()).second)
            saveFavourites();
    }
    else
        it->second.erase (p.key());
    saveFavouriteFolders();
}

bool PresetManager::addFavouriteFolder (const juce::String& name)
{
    const auto n = name.trim();
    for (const auto& [existing, keys] : favFolders)
        if (existing.equalsIgnoreCase (n))
            return false;
    if (n.isEmpty())
        return false;
    favFolders[n];
    saveFavouriteFolders();
    return true;
}

bool PresetManager::renameFavouriteFolder (const juce::String& from, const juce::String& to)
{
    const auto n = to.trim();
    auto it = favFolders.find (from);
    if (it == favFolders.end() || n.isEmpty())
        return false;
    for (const auto& [existing, keys] : favFolders)
        if (existing.equalsIgnoreCase (n) && existing != from)
            return false;
    auto keys = std::move (it->second);
    favFolders.erase (it);
    favFolders[n] = std::move (keys);
    saveFavouriteFolders();
    return true;
}

void PresetManager::deleteFavouriteFolder (const juce::String& name)
{
    if (favFolders.erase (name) > 0)
        saveFavouriteFolders();
}

// --- User subfolders --------------------------------------------------------------------
juce::String PresetManager::userSubfolder (const PresetInfo& p)
{
    if (p.factory || p.file == juce::File())
        return {};
    const auto dir = p.file.getParentDirectory();
    if (dir == userPresetFolder() || ! dir.isAChildOf (userPresetFolder()))
        return {};
    return dir.getRelativePathFrom (userPresetFolder()).replaceCharacter ('\\', '/');
}

juce::StringArray PresetManager::userSubfolders()
{
    juce::StringArray r;
    for (const auto& p : all())
        for (auto f = userSubfolder (p); f.isNotEmpty(); f = f.upToLastOccurrenceOf ("/", false, false))
        {
            r.addIfNotAlreadyThere (f);
            if (! f.containsChar ('/'))
                break;
        }
    r.sortNatural();
    return r;
}

// --- Patterns ------------------------------------------------------------------------
std::vector<PresetManager::PatternInfo> PresetManager::patterns (int source)
{
    std::vector<PatternInfo> r;
    if (source == 0)
    {
        const auto& fp = factoryPatterns();
        for (size_t i = 0; i < fp.size(); ++i)
            r.push_back ({ fp[i].name, 0, (int) i, {} });
    }
    else if (source == 1)
    {
        const auto& l = all();
        for (size_t i = 0; i < l.size(); ++i)
            if (l[i].category == "Sequence" || ! l[i].factory)
                r.push_back ({ l[i].name, 1, (int) i, {} });
    }
    else
    {
        for (const auto& f : patternFolder().findChildFiles (juce::File::findFiles, false, "*.cspat"))
            r.push_back ({ f.getFileNameWithoutExtension(), 2, -1, f });
        std::sort (r.begin(), r.end(), [] (const PatternInfo& a, const PatternInfo& b) { return a.name.compareNatural (b.name) < 0; });
    }
    return r;
}

bool PresetManager::loadPattern (const PatternInfo& p)
{
    cs::SeqPattern pat;
    std::vector<std::pair<juce::String, float>> values;
    if (p.source == 0)
    {
        const auto& fp = factoryPatterns();
        if (p.index < 0 || p.index >= (int) fp.size())
            return false;
        fp[(size_t) p.index].build (pat, values);
    }
    else
    {
        juce::ValueTree t;
        if (p.source == 1)
        {
            const auto& l = all();
            if (p.index < 0 || p.index >= (int) l.size())
                return false;
            t = treeFor (l[(size_t) p.index]);
        }
        else if (auto xml = juce::XmlDocument::parse (p.file))
            t = juce::ValueTree::fromXml (*xml);
        if (! t.isValid())
            return false;
        pat = ChurchSynthProcessor::patternOf (t.getChildWithName ("SEQ"));
        for (const auto& c : t)
            if (c.hasType ("PARAM") && c["id"].toString().startsWith ("seq_") && c["id"].toString() != "seq_on"
                && c["id"].toString() != "seq_lock")
                values.push_back ({ c["id"].toString(), (float) c["value"] });
    }
    proc.setPattern (pat);
    for (const auto& kv : values)
        if (auto* prm = proc.apvts.getParameter (kv.first))
            prm->setValueNotifyingHost (prm->convertTo0to1 (kv.second));
    proc.checkpoint ("Pattern: " + p.name);
    return true;
}

bool PresetManager::savePattern (const juce::String& name, juce::String& error)
{
    if (name.trim().isEmpty())
    {
        error = "Give the pattern a name.";
        return false;
    }
    patternFolder().createDirectory();
    auto state = proc.createStateTree();
    juce::ValueTree t ("ChurchSynthPattern");
    t.appendChild (state.getChildWithName ("SEQ").createCopy(), nullptr);
    for (const auto& c : state)
        if (c.hasType ("PARAM") && c["id"].toString().startsWith ("seq_"))
            t.appendChild (c.createCopy(), nullptr);
    const auto file = patternFolder().getChildFile (safeFileName (name) + ".cspat");
    if (auto xml = t.createXml(); xml == nullptr || ! xml->writeTo (file))
    {
        error = "Couldn't write " + file.getFullPathName();
        return false;
    }
    return true;
}

bool PresetManager::deletePattern (const PatternInfo& p)
{
    return p.source == 2 && p.file.deleteFile();
}

// --- FX presets ------------------------------------------------------------------------
std::vector<PresetManager::FxPresetInfo> PresetManager::fxPresets (int type)
{
    std::vector<FxPresetInfo> r;
    const auto& info = cs::fx::fxInfo (type);
    for (const auto& f : factoryFxPresets())
        if (f.type == type)
        {
            FxPresetInfo p;
            p.name = f.name;
            for (int k = 0; k < cs::fx::kFxParams; ++k)
                p.values[(size_t) k] = info.defaults[(size_t) k];
            for (const auto& kv : f.knobs)
                p.values[(size_t) kv.first] = kv.second;
            r.push_back (p);
        }
    const auto folder = fxPresetFolder().getChildFile (info.name);
    for (const auto& f : folder.findChildFiles (juce::File::findFiles, false, "*.csfx"))
    {
        FxPresetInfo p;
        p.name = f.getFileNameWithoutExtension();
        p.factory = false;
        p.file = f;
        juce::StringArray v;
        v.addTokens (f.loadFileAsString().trim(), " ", {});
        for (int k = 0; k < cs::fx::kFxParams && k < v.size(); ++k)
            p.values[(size_t) k] = juce::jlimit (0.0f, 1.0f, v[k].getFloatValue());
        r.push_back (p);
    }
    return r;
}

void PresetManager::applyFxPreset (int slot, const FxPresetInfo& p)
{
    const juce::String base = juce::String ("fx_") + kFxSlotIds[juce::jlimit (0, 8, slot)] + "_p";
    for (int k = 0; k < cs::fx::kFxParams; ++k)
        if (auto* prm = proc.apvts.getParameter (base + juce::String (k + 1)))
            prm->setValueNotifyingHost (p.values[(size_t) k]);
    proc.checkpoint ("FX preset: " + p.name);
}

bool PresetManager::saveFxPreset (int slot, const juce::String& name, juce::String& error)
{
    const juce::String base = juce::String ("fx_") + kFxSlotIds[juce::jlimit (0, 8, slot)] + "_";
    const int type = (int) std::lround (proc.apvts.getRawParameterValue (base + "type")->load());
    if (type == cs::fx::FxNone || name.trim().isEmpty())
    {
        error = "Pick an effect and give the preset a name.";
        return false;
    }
    const auto folder = fxPresetFolder().getChildFile (cs::fx::fxInfo (type).name);
    folder.createDirectory();
    juce::String s;
    for (int k = 0; k < cs::fx::kFxParams; ++k)
        s << juce::String (proc.apvts.getRawParameterValue (base + "p" + juce::String (k + 1))->load(), 4) << " ";
    const auto file = folder.getChildFile (safeFileName (name) + ".csfx");
    if (! file.replaceWithText (s.trim()))
    {
        error = "Couldn't write " + file.getFullPathName();
        return false;
    }
    return true;
}

bool PresetManager::deleteFxPreset (const FxPresetInfo& p)
{
    return ! p.factory && p.file.deleteFile();
}
