#pragma once

#include "SynthPage.h"

#include "dsp/SeqArp.h"

namespace ui
{
class SeqPage final : public juce::Component, public Refreshable
{
public:
    explicit SeqPage (UiContext& ctx);
    ~SeqPage() override;
    void resized() override;
    void paint (juce::Graphics&) override;
    void refreshView() override;
    void tick(); // playhead (editor timer)

    enum LaneKind { LaneOnOff, LaneProb, LanePitch, LaneVelocity, LaneOctave, LaneGate, LaneSlide, LaneLock, NumLanes };

private:
    class Lane;
    class TrackHeader;
    class LeftColumn;
    UiContext& ctx;
    std::unique_ptr<LeftColumn> left;
    std::unique_ptr<Lane> lanes[NumLanes];
    std::unique_ptr<TrackHeader> headers[cs::NumSeqTracks];

    cs::SeqPattern pat;
    cs::SeqParams seqParams;
    uint32_t seenVersion = 0, shownSeed = 0;
    int playStep = -1;
    bool editing = false;

    void commit (const juce::String& label);
    void load();
    juce::Rectangle<int> laneRow (int lane) const;
    friend class Lane;
    friend class TrackHeader;
    friend class LeftColumn;
};
} // namespace ui
