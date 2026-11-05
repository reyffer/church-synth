#pragma once

#include "Widgets.h"
#include "../PresetManager.h"

namespace ui
{
class PresetBrowser final : public juce::Component, private juce::ListBoxModel
{
public:
    explicit PresetBrowser (UiContext& ctx);
    void resized() override;
    void paint (juce::Graphics&) override;
    void refresh();                 // rebuild the filtered list
    std::vector<int> filtered() const { return rows; } // indexes into PresetManager::all()
    void step (int delta);          // prev / next within the current filter
    void saveAs();                  // dialog
    void saveOver();                // overwrite the current user preset (or Save As)
    std::function<void()> onClose;

private:
    UiContext& ctx;
    juce::TextEditor search;
    juce::ListBox list { "presets", this };
    FlatButton close, init, save, saveAsBtn, del, favBtn, rescanBtn, tagsBtn;
    juce::StringArray activeTags;                                  // filter: presets with all of these
    std::vector<std::pair<juce::String, juce::Rectangle<int>>> chips; // tag chips above the list
    juce::Rectangle<int> chipArea;
    int layoutChips (int x, int y, int width, bool apply);        // returns the height used
    void showTagMenu (const PresetInfo& p, juce::Component* target);
    void askText (const juce::String& title, const juce::String& message, const juce::String& initial,
                  std::function<void (const juce::String&)> done);
    int bank = 0;     // 0 all, 1 factory, 2 user, 3 favourites
    int category = -1; // -1 all
    std::vector<int> rows;
    juce::Rectangle<int> filterArea, infoArea;

    int getNumRows() override { return (int) rows.size(); }
    void paintListBoxItem (int row, juce::Graphics&, int w, int h, bool selected) override;
    void listBoxItemClicked (int row, const juce::MouseEvent&) override;
    juce::String getTooltipForRow (int row) override;
    void mouseDown (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;
    juce::Rectangle<int> bankRow (int i) const { return { filterArea.getX(), filterArea.getY() + 28 + i * 24, filterArea.getWidth(), 22 }; }
    juce::Rectangle<int> catRow (int i) const { return { filterArea.getX(), filterArea.getY() + 184 + (i + 1) * 24, filterArea.getWidth(), 22 }; }
};
} // namespace ui
