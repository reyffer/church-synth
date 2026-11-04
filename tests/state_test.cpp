// Plugin-level test: modulation routes and user-file names survive save/restore
// (MainStage concerts depend on this). Links the plugin's shared code.

#include "plugin/PluginProcessor.h"

#include <cstdio>

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    int failures = 0;

    ChurchSynthProcessor a;
    cs::ModRouting r;
    r.routes[r.count++] = { cs::SrcLfo1, cs::findModDest ("f1_cutoff"), 0.25f, cs::SrcModWheel, 0.75f };
    r.routes[r.count++] = { cs::SrcEnv2, cs::findModDest ("e1_wt_pos"), -0.5f, -1, 1.0f };
    a.setRoutes (r);
    if (auto* p = a.apvts.getParameter ("lfo1_rate"))
        p->setValueNotifyingHost (0.7f);

    juce::MemoryBlock state;
    a.getStateInformation (state);

    ChurchSynthProcessor b;
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
    return failures == 0 ? 0 : 1;
}
