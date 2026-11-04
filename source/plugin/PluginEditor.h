#pragma once

// Temporary editor (until the UI milestone):
//   Sound      - every parameter, grouped
//   Modulation - the routing matrix (source -> destination, amount, sidechain)
//   FX         - bus routing and the nine effect slots
//   Seq/Arp    - sequencer / arpeggiator settings and the step grid
//   Files      - user wavetables / noise samples

#include "PluginProcessor.h"

#include <juce_audio_utils/juce_audio_utils.h>

class ChurchSynthEditor;

// One row of the routing list.
class RouteRow final : public juce::Component
{
public:
    explicit RouteRow (std::function<void()> onChange, std::function<void()> onRemove);
    void set (const cs::ModRoute& r);
    cs::ModRoute get() const;
    void resized() override;

private:
    juce::ComboBox source, dest, sidechain;
    juce::Slider amount, scAmount;
    juce::TextButton remove { "X" };
};

class RoutePanel final : public juce::Component
{
public:
    explicit RoutePanel (ChurchSynthProcessor& p);
    void resized() override;
    void paint (juce::Graphics&) override;
    void reload(); // rebuild rows from the processor

private:
    ChurchSynthProcessor& proc;
    juce::TextButton add { "+ Add route" };
    juce::Label help;
    juce::Viewport viewport;
    juce::Component list;
    std::vector<std::unique_ptr<RouteRow>> rows;

    void push(); // rows -> processor
    void layoutRows();
};

// FX tab: bus controls on top, then the selected slot's effect and its controls.
class FxPanel final : public juce::Component
{
public:
    explicit FxPanel (ChurchSynthProcessor& p);
    void resized() override;
    void paint (juce::Graphics&) override;
    void refresh(); // follow effect-type / sync changes (called from the editor timer)

private:
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAtt = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    ChurchSynthProcessor& proc;
    // Bus section
    juce::ComboBox routing;
    juce::ToggleButton busOn[3], auxPre { "Aux pre A/B" };
    juce::Slider busVol[2], auxSend, auxReturn;
    juce::Label busLabels[4];
    std::unique_ptr<ComboAtt> routingAtt;
    std::unique_ptr<ButtonAtt> busOnAtt[3], auxPreAtt;
    std::unique_ptr<SliderAtt> busVolAtt[2], auxSendAtt, auxReturnAtt;
    // Slot section
    juce::TextButton slotButtons[cs::fx::kNumFxSlots];
    juce::ComboBox type;
    juce::ToggleButton slotOn { "On" }, sync { "Sync" };
    juce::Slider knobs[cs::fx::kFxParams];
    juce::Label knobLabels[cs::fx::kFxParams];
    std::unique_ptr<ComboAtt> typeAtt;
    std::unique_ptr<ButtonAtt> slotOnAtt, syncAtt;
    std::unique_ptr<SliderAtt> knobAtt[cs::fx::kFxParams];
    int selected = 0, shownType = -1, shownSync = -1;

    void select (int slot);
    void updateLabels();
};

// Seq/Arp tab: settings on top, then one lane per track (drag to draw values).
class SeqPanel final : public juce::Component
{
public:
    explicit SeqPanel (ChurchSynthProcessor& p);
    ~SeqPanel() override;
    void resized() override;
    void paint (juce::Graphics&) override;
    void refresh(); // follow the playhead / external pattern changes (editor timer)

    // One lane of 16 cells. Values are read/written through the panel's pattern copy.
    enum LaneKind { LaneOnOff, LaneProb, LanePitch, LaneVelocity, LaneOctave, LaneGate, LaneSlide, LaneLock, NumLanes };
    class Lane final : public juce::Component
    {
    public:
        Lane (SeqPanel& o, LaneKind k) : owner (o), kind (k) {}
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override;

    private:
        SeqPanel& owner;
        LaneKind kind;
        int lastCol = -1;
        bool paintOn = true;
        void edit (const juce::MouseEvent& e, bool first);
    };

private:
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAtt = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    struct Control
    {
        juce::Label label;
        std::unique_ptr<juce::Component> widget;
        std::unique_ptr<SliderAtt> sAtt;
        std::unique_ptr<ButtonAtt> bAtt;
        std::unique_ptr<ComboAtt> cAtt;
    };
    struct TrackHeader
    {
        juce::Label name;
        juce::ToggleButton on;
        juce::Slider length, random;
        juce::ComboBox divider;
        juce::TextButton left { "<" }, right { ">" };
    };

    ChurchSynthProcessor& proc;
    std::vector<std::unique_ptr<Control>> controls;
    juce::TextButton regen { "Regen" }, applyRandom { "Apply random" }, clearRandom { "Clear random" },
        rotLeft { "< Rotate" }, rotRight { "Rotate >" }, clearPattern { "Clear pattern" };
    juce::Label help;
    std::unique_ptr<Lane> lanes[NumLanes];
    juce::Label laneNames[NumLanes];
    TrackHeader headers[cs::NumSeqTracks];

    cs::SeqPattern pat;
    uint32_t seenVersion = 0;
    int playStep = -1;
    uint32_t shownSeed = 0;
    cs::SeqParams seqParams; // host values, refreshed by the timer (for the random preview)
    bool editing = false;

    void addControl (const juce::String& id, const juce::String& name);
    void commit();             // pattern copy -> processor
    void loadFromProcessor();  // processor -> pattern copy and track headers
    void updateHeaders();
    int columnAt (int x, int width) const noexcept { return juce::jlimit (0, cs::kSeqSteps - 1, x * cs::kSeqSteps / juce::jmax (1, width)); }
    friend class Lane;
};

class ChurchSynthEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit ChurchSynthEditor (ChurchSynthProcessor&);
    ~ChurchSynthEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct ContentRow
    {
        juce::Label title, current;
        juce::TextButton load { "Load file..." }, factory { "Use factory" };
    };

    ChurchSynthProcessor& proc;
    juce::LookAndFeel_V4 darkLook;
    juce::TabbedComponent tabs { juce::TabbedButtonBar::TabsAtTop };
    juce::GenericAudioProcessorEditor generic;
    RoutePanel routes;
    FxPanel fxPanel;
    SeqPanel seqPanel;
    juce::Component files;
    ContentRow rows[ChurchSynthProcessor::NumContentSlots];
    std::unique_ptr<juce::FileChooser> chooser;

    void choose (int slot);
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChurchSynthEditor)
};
