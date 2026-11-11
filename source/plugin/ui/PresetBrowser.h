#pragma once

#include "Widgets.h"
#include "../PresetManager.h"

namespace ui
{
class PresetBrowser final : public juce::Component, private juce::ListBoxModel
{
public:
    explicit PresetBrowser (UiContext& ctx);
    ~PresetBrowser() override;
    void resized() override;
    void paint (juce::Graphics&) override;
    void refresh();                 // rebuild the filtered list
    std::vector<int> filtered() const { return rows; } // indexes into PresetManager::all()
    void step (int delta);          // prev / next within the current filter
    void saveAs();                  // dialog
    void saveOver();                // overwrite the current user preset (or Save As)
    std::function<void()> onClose;
    // For screenshots / tests: bank 0..3, a User or Favourites folder, tags expanded.
    void setView (int newBank, const juce::String& folder, bool allTags);

private:
    UiContext& ctx;
    juce::TextEditor search, notes;
    juce::Rectangle<int> notesTitle;
    juce::ListBox list { "presets", this };
    FlatButton close, init, save, saveAsBtn, del, favBtn, rescanBtn, tagsBtn;
    juce::StringArray activeTags;                                  // filter: presets with all of these
    std::vector<std::pair<juce::String, juce::Rectangle<int>>> chips; // tag chips above the list
    juce::Rectangle<int> chipArea;
    bool showAllTags = false;                                      // collapsed: only the most-used tags
    int layoutChips (int x, int y, int width, bool apply);        // returns the height used
    void showTagMenu (const PresetInfo& p, juce::Component* target);
    void addTagItems (juce::PopupMenu& m, const PresetInfo& p, int base);       // items base+1.. for tags
    void handleTagItem (const PresetInfo& p, const juce::StringArray& tags, int r); // r relative to base
    void addFavouriteItems (juce::PopupMenu& m, const PresetInfo& p, int base);  // Favourite + folders
    void handleFavouriteItem (const PresetInfo& p, const juce::StringArray& folders, int r);
    void showFavouriteMenu (const PresetInfo& p, juce::Component* target);
    void showPresetMenu (const PresetInfo& p, juce::Component* target);          // right-click on a row
    void newFavouriteFolder (const PresetInfo* addPreset);
    void askText (const juce::String& title, const juce::String& message, const juce::String& initial,
                  std::function<void (const juce::String&)> done);
    int bank = 0;     // 0 all, 1 factory, 2 user, 3 favourites
    juce::String userFolder; // User bank: "" = all user presets, else a subfolder ("Pack 3 - Electric Pianos")
    juce::String favFolder;  // Favourites bank: "" = all favourites, else a folder
    int category = -1; // -1 all
    std::vector<int> rows;
    juce::Rectangle<int> filterArea, infoArea;

    // Left side: banks (User and Favourites expand into their folders) and categories,
    // in a scrolling list.
    struct SideRow
    {
        enum Kind { Heading, Bank, UserFolder, FavFolder, NewFavFolder, Category } kind = Heading;
        juce::String label, value;
        int index = 0, depth = 0, count = 0;
        juce::Rectangle<int> bounds;
    };
    class SideList;
    std::vector<SideRow> side;
    std::unique_ptr<SideList> sideList;
    juce::Viewport sideView;
    void rebuildSide();
    void sideClicked (const SideRow& r, const juce::MouseEvent& e);
    bool sideSelected (const SideRow& r) const;

    int getNumRows() override { return (int) rows.size(); }
    void paintListBoxItem (int row, juce::Graphics&, int w, int h, bool selected) override;
    void listBoxItemClicked (int row, const juce::MouseEvent&) override;
    juce::String getTooltipForRow (int row) override;
    void mouseDown (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;
};
} // namespace ui
