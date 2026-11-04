#pragma once

#include "SynthPage.h"

namespace ui
{
class FxPage final : public juce::Component, public Refreshable
{
public:
    explicit FxPage (UiContext& ctx);
    ~FxPage() override;
    void resized() override;
    void paint (juce::Graphics&) override;
    void refreshView() override;

    void selectBus (int bus);
    int selectedBus() const noexcept { return bus; }

private:
    class SlotButton;
    class SlotEditor;
    UiContext& ctx;
    int bus = 0;
    std::unique_ptr<ControlPanel> left;
    std::vector<std::unique_ptr<SlotButton>> slotButtons;
    std::unique_ptr<Toggle> busPower[3];
    std::unique_ptr<SlotEditor> editors[3];
    friend class SlotButton;
};
} // namespace ui
