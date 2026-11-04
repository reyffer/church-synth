// Plugin-level test: modulation routes and user-file names survive save/restore
// (MainStage concerts depend on this). Links the plugin's shared code.

#include "plugin/PluginProcessor.h"

#include <cstdio>

// Optional: state_test --snapshot <dir> writes a PNG of each editor tab (for checking the layout).
static void snapshotEditor (ChurchSynthProcessor& p, const juce::File& dir)
{
    std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
    juce::TabbedComponent* tabs = nullptr;
    for (auto* c : ed->getChildren())
        if (auto* t = dynamic_cast<juce::TabbedComponent*> (c))
            tabs = t;
    if (tabs == nullptr)
        return;
    for (int i = 0; i < tabs->getNumTabs(); ++i)
    {
        tabs->setCurrentTabIndex (i);
        if (auto* tc = dynamic_cast<juce::Timer*> (ed.get()))
            tc->timerCallback(); // let the page refresh once
        const auto img = ed->createComponentSnapshot (ed->getLocalBounds());
        juce::FileOutputStream out (dir.getChildFile ("editor_" + tabs->getTabNames()[i].replace ("/", "-") + ".png"));
        out.setPosition (0);
        out.truncate();
        juce::PNGImageFormat().writeImageToStream (img, out);
    }
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    int failures = 0;

    auto aPtr = std::make_unique<ChurchSynthProcessor>(); // large object: keep it off the stack
    auto& a = *aPtr;
    cs::ModRouting r;
    r.routes[r.count++] = { cs::SrcLfo1, cs::findModDest ("f1_cutoff"), 0.25f, cs::SrcModWheel, 0.75f };
    r.routes[r.count++] = { cs::SrcEnv2, cs::findModDest ("e1_wt_pos"), -0.5f, -1, 1.0f };
    a.setRoutes (r);
    if (auto* p = a.apvts.getParameter ("lfo1_rate"))
        p->setValueNotifyingHost (0.7f);
    // Put a reverb in Aux 1 and a delay in FX A1, then tweak a setting after the defaults load.
    a.apvts.getParameter ("fx_x1_type")->setValueNotifyingHost (a.apvts.getParameter ("fx_x1_type")->convertTo0to1 (4.0f));
    a.apvts.getParameter ("fx_a1_type")->setValueNotifyingHost (a.apvts.getParameter ("fx_a1_type")->convertTo0to1 (1.0f));
    a.syncEffects();
    a.apvts.getParameter ("fx_a1_p3")->setValueNotifyingHost (0.123f);
    a.apvts.getParameter ("fx_aux_send")->setValueNotifyingHost (0.5f);

    // A sequencer pattern with a few edits.
    auto pat = a.getPattern();
    pat.steps[3].pitch = 7;
    pat.steps[5].on = false;
    pat.steps[2].slide = 0.4f;
    pat.tracks[cs::TrackGate].length = 5;
    pat.tracks[cs::TrackPitch].random = 0.25f;
    pat.customWeights[1] = 2.0f;
    a.setPattern (pat);
    a.apvts.getParameter ("seq_on")->setValueNotifyingHost (1.0f);

    juce::MemoryBlock state;
    a.getStateInformation (state);

    auto bPtr = std::make_unique<ChurchSynthProcessor>();
    auto& b = *bPtr;
    b.setStateInformation (state.getData(), (int) state.getSize());
    const auto got = b.getRoutes();

    auto same = [] (const cs::ModRoute& x, const cs::ModRoute& y) {
        return x.source == y.source && x.dest == y.dest && std::abs (x.amount - y.amount) < 1.0e-6f
               && x.sidechain == y.sidechain && std::abs (x.scAmount - y.scAmount) < 1.0e-6f;
    };
    if (got.count != 2 || ! same (got.routes[0], r.routes[0]) || ! same (got.routes[1], r.routes[1]))
    {
        std::printf ("FAIL: routes not restored\n");
        ++failures;
    }
    if (std::abs (b.apvts.getParameter ("lfo1_rate")->getValue() - 0.7f) > 1.0e-4f)
    {
        std::printf ("FAIL: parameter not restored\n");
        ++failures;
    }

    if (std::abs (b.apvts.getParameter ("fx_a1_p3")->getValue() - 0.123f) > 1.0e-4f
        || std::abs (b.apvts.getParameter ("fx_x1_p8")->getValue() - 1.0f) > 1.0e-4f)
    {
        std::printf ("FAIL: FX settings not restored (or aux default not 100%% wet)\n");
        ++failures;
    }

    {
        const auto pb = b.getPattern();
        if (pb.steps[3].pitch != 7 || pb.steps[5].on || std::abs (pb.steps[2].slide - 0.4f) > 1.0e-4f
            || pb.tracks[cs::TrackGate].length != 5 || std::abs (pb.tracks[cs::TrackPitch].random - 0.25f) > 1.0e-4f
            || std::abs (pb.customWeights[1] - 2.0f) > 1.0e-3f || b.apvts.getParameter ("seq_on")->getValue() < 0.5f)
        {
            std::printf ("FAIL: sequencer pattern not restored\n");
            ++failures;
        }
    }

    // Render a block with the restored routes to make sure the hand-off works.
    b.setRateAndBufferSizeDetails (48000.0, 256);
    b.prepareToPlay (48000.0, 256);
    juce::AudioBuffer<float> buf (2, 256);
    juce::MidiBuffer midi;
    midi.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0);
    for (int i = 0; i < 50; ++i)
    {
        b.processBlock (buf, midi);
        midi.clear();
    }
    if (buf.getMagnitude (0, 256) <= 0.0f)
    {
        std::printf ("FAIL: no audio after restore\n");
        ++failures;
    }

    if (failures == 0)
        std::printf ("State tests passed.\n");
    if (argc > 2 && juce::String (argv[1]) == "--snapshot")
        snapshotEditor (b, juce::File (argv[2]));

    return failures == 0 ? 0 : 1;
}
