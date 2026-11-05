// Plugin-level test: modulation routes and user-file names survive save/restore
// (MainStage concerts depend on this). Links the plugin's shared code.

#include "plugin/PluginEditor.h"
#include "plugin/PluginProcessor.h"
#include "plugin/PresetManager.h"
#include "dsp/fx/Effects.h"

#include <cmath>
#include <cstdio>

// Optional: state_test --snapshot <dir> [preset name] writes PNGs of the editor pages (layout check).
static void snapshotEditor (ChurchSynthProcessor& p, const juce::File& dir)
{
    std::unique_ptr<juce::AudioProcessorEditor> base (p.createEditor());
    auto* ed = dynamic_cast<ChurchSynthEditor*> (base.get());
    if (ed == nullptr)
        return;
    ed->setSize (1280, 800);
    auto shot = [&] (const juce::String& name) {
        const auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        juce::FileOutputStream out (dir.getChildFile ("ui_" + name + ".png"));
        out.setPosition (0);
        out.truncate();
        juce::PNGImageFormat().writeImageToStream (img, out);
    };
    ed->showView (0, 1, 0); shot ("synth_engine1_env");
    ed->showView (0, 2, 1); shot ("synth_engine2_lfo");
    ed->showView (0, 3, 2); shot ("synth_utility_func");
    ed->showView (0, 4, 0); shot ("synth_random");
    ed->showView (0, 5, 0); shot ("synth_comb");
    ed->showView (0, 0, 0); shot ("synth_keyboard");
    ed->showView (1, 1); shot ("fx");
    // Param EQ graph and the distortion menus.
    for (auto [id, t] : { std::pair<const char*, int> { "fx_b1_type", cs::fx::FxParamEq }, { "fx_b2_type", cs::fx::FxDistortion }, { "fx_b3_type", cs::fx::FxPhaser } })
        if (auto* prm = p.apvts.getParameter (id))
            prm->setValueNotifyingHost (prm->convertTo0to1 ((float) t));
    p.syncEffects();
    for (auto [id, v] : { std::pair<const char*, float> { "fx_b1_p4", 0.8f }, { "fx_b1_p10", 0.25f }, { "fx_b2_p1", 0.3f } })
        if (auto* prm = p.apvts.getParameter (id))
            prm->setValueNotifyingHost (v);
    ed->showView (1, 1, 1);
    shot ("fx_eq_dist");
    ed->showView (2, 1); shot ("seq");
    ed->showView (3); shot ("browser");
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    int failures = 0;

    if (argc > 1 && juce::String (argv[1]) == "--dump-params")
    {
        for (const auto& d : cs::parameterDefs())
            std::printf ("%s|%s|%d|%g|%g|%g|%s\n", d.id.toRawUTF8(), d.name.toRawUTF8(), (int) d.kind, d.min, d.max, d.def,
                         d.choices.joinIntoString (";").substring (0, 400).toRawUTF8());
        return 0;
    }

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

    // Undo / redo, MIDI learn, user preset round trip and Seq Lock.
    {
        auto pPtr = std::make_unique<ChurchSynthProcessor>();
        auto& p = *pPtr;
        p.prepareToPlay (48000.0, 512);
        auto& pm = p.presets();
        PresetInfo sanctuary;
        for (const auto& info : pm.all())
            if (info.name == "Sanctuary Pad")
                sanctuary = info;
        pm.load (sanctuary);
        auto* cutoff = p.apvts.getParameter ("f1_cutoff");
        const float original = cutoff->getValue();
        cutoff->setValueNotifyingHost (0.9f);
        p.checkpoint ("edit cutoff");
        if (! p.presetModified())
        {
            std::printf ("FAIL: edit not flagged as modified\n");
            ++failures;
        }
        p.undo();
        if (std::abs (cutoff->getValue() - original) > 1.0e-4f)
        {
            std::printf ("FAIL: undo didn't restore the cutoff\n");
            ++failures;
        }
        p.redo();
        if (std::abs (cutoff->getValue() - 0.9f) > 1.0e-4f)
        {
            std::printf ("FAIL: redo didn't re-apply the cutoff\n");
            ++failures;
        }

        // MIDI learn: CC 74 drives the cutoff.
        p.startMidiLearn ("f1_cutoff");
        {
            juce::AudioBuffer<float> buf (2, 512);
            juce::MidiBuffer midi;
            midi.addEvent (juce::MidiMessage::controllerEvent (1, 74, 64), 0);
            p.processBlock (buf, midi);
        }
        if (p.ccForParam ("f1_cutoff") != 74 || std::abs (cutoff->getValue() - 64.0f / 127.0f) > 1.0e-3f)
        {
            std::printf ("FAIL: MIDI learn (cc %d, value %.3f)\n", p.ccForParam ("f1_cutoff"), cutoff->getValue());
            ++failures;
        }
        // Mappings survive the host state but are not part of presets.
        {
            juce::MemoryBlock st;
            p.getStateInformation (st);
            auto qPtr = std::make_unique<ChurchSynthProcessor>();
            qPtr->setStateInformation (st.getData(), (int) st.getSize());
            if (qPtr->ccForParam ("f1_cutoff") != 74)
            {
                std::printf ("FAIL: MIDI mapping not restored with the host state\n");
                ++failures;
            }
        }

        // User preset round trip.
        juce::String err;
        const juce::String testName = "zz Church Synth test preset";
        cutoff->setValueNotifyingHost (0.42f);
        if (! pm.save (testName, "Pad", "test", "round trip", err))
        {
            std::printf ("FAIL: save preset: %s\n", err.toRawUTF8());
            ++failures;
        }
        pm.load (sanctuary);
        PresetInfo saved;
        for (const auto& info : pm.all())
            if (info.name == testName && ! info.factory)
                saved = info;
        if (saved.name != testName || ! pm.load (saved) || std::abs (cutoff->getValue() - 0.42f) > 1.0e-3f || p.presetName() != testName)
        {
            std::printf ("FAIL: user preset round trip\n");
            ++failures;
        }
        pm.remove (saved);

        // Seq Lock keeps the pattern across preset loads.
        auto pat = p.getPattern();
        pat.steps[2].pitch = 9;
        p.setPattern (pat);
        p.apvts.getParameter ("seq_lock")->setValueNotifyingHost (1.0f);
        pm.load (sanctuary);
        if (p.getPattern().steps[2].pitch != 9)
        {
            std::printf ("FAIL: Seq Lock didn't keep the pattern\n");
            ++failures;
        }
        p.apvts.getParameter ("seq_lock")->setValueNotifyingHost (0.0f);
        pm.load (sanctuary);
        if (p.getPattern().steps[2].pitch == 9)
        {
            std::printf ("FAIL: preset pattern not loaded with Lock off\n");
            ++failures;
        }

        // Custom function shape survives the state.
        auto shape = cs::functionPreset (5);
        shape.y[1] = 0.33f;
        p.setCustomShape (1, shape);
        juce::MemoryBlock st;
        p.getStateInformation (st);
        auto qPtr = std::make_unique<ChurchSynthProcessor>();
        qPtr->setStateInformation (st.getData(), (int) st.getSize());
        if (std::abs (qPtr->getCustomShape (1).y[1] - 0.33f) > 1.0e-3f)
        {
            std::printf ("FAIL: custom function shape not restored\n");
            ++failures;
        }
    }

    // --render-presets <dir>: one WAV per factory preset (audition reel).
    if (argc > 2 && juce::String (argv[1]) == "--render-presets")
    {
        auto pPtr = std::make_unique<ChurchSynthProcessor>();
        auto& p = *pPtr;
        const juce::File dir (argv[2]);
        dir.createDirectory();
        int index = 0;
        for (const auto& info : p.presets().all())
        {
            if (! info.factory)
                continue;
            p.presets().load (info);
            p.syncEffects();
            p.prepareToPlay (48000.0, 512);
            const bool melodic = info.category == "Lead" || info.category == "Bass";
            const double seconds = 6.5;
            juce::AudioBuffer<float> out (2, (int) (48000.0 * seconds));
            juce::AudioBuffer<float> buf (2, 512);
            for (int pos = 0; pos + 512 <= out.getNumSamples(); pos += 512)
            {
                juce::MidiBuffer midi;
                const double t = pos / 48000.0;
                auto at = [&] (double when) { return t <= when && when < t + 512 / 48000.0; };
                if (melodic)
                {
                    const int notes[4] = { 60, 64, 67, 72 };
                    for (int k = 0; k < 4; ++k)
                    {
                        const int n = notes[k] - (info.category == "Bass" ? 24 : 0);
                        if (at (0.05 + k * 0.9)) midi.addEvent (juce::MidiMessage::noteOn (1, n, (juce::uint8) 100), 0);
                        if (at (0.05 + k * 0.9 + (k == 3 ? 1.6 : 0.95))) midi.addEvent (juce::MidiMessage::noteOff (1, n), 0);
                    }
                }
                else
                {
                    for (int n : { 48, 60, 64, 67 })
                    {
                        if (at (0.05)) midi.addEvent (juce::MidiMessage::noteOn (1, n, (juce::uint8) 100), 0);
                        if (at (4.5)) midi.addEvent (juce::MidiMessage::noteOff (1, n), 0);
                    }
                }
                buf.clear();
                p.processBlock (buf, midi);
                for (int ch = 0; ch < 2; ++ch)
                    out.copyFrom (ch, pos, buf, ch, 0, 512);
            }
            // fade the last 0.3 s
            out.applyGainRamp (out.getNumSamples() - 14400, 14400, 1.0f, 0.0f);
            const auto f = dir.getChildFile (juce::String (index++).paddedLeft ('0', 2) + " " + info.name + ".wav");
            f.deleteFile();
            if (auto os = std::unique_ptr<juce::OutputStream> (f.createOutputStream()))
            {
                juce::WavAudioFormat wav;
                JUCE_BEGIN_IGNORE_DEPRECATION_WARNINGS
                auto w = std::unique_ptr<juce::AudioFormatWriter> (wav.createWriterFor (os.get(), 48000.0, 2, 24, {}, 0));
                JUCE_END_IGNORE_DEPRECATION_WARNINGS
                if (w != nullptr)
                {
                    os.release();
                    w->writeFromAudioSampleBuffer (out, 0, out.getNumSamples());
                }
            }
        }
        return 0;
    }

    // Every factory preset loads, keeps its routes and plays cleanly (no NaN, no clipping, audible).
    {
        auto pPtr = std::make_unique<ChurchSynthProcessor>();
        auto& p = *pPtr;
        p.prepareToPlay (48000.0, 512);
        const bool levels = argc > 1 && juce::String (argv[1]) == "--level-report";
        const bool report = levels || (argc > 1 && juce::String (argv[1]) == "--preset-report");
        const auto all = p.presets().all();
        for (const auto& info : all)
        {
            if (! info.factory)
                continue;
            const auto tree = p.presets().treeFor (info);
            const int wantRoutes = tree.getChildWithName ("ROUTES").getNumChildren();
            p.presets().load (info);
            p.syncEffects();
            p.prepareToPlay (48000.0, 512); // fresh voices / effects for each preset
            // --level-report: play 12 dB down so the output limiter stays out of the way,
            // and print the true peak and the master volume that would hit the target.
            const float masterVol = p.apvts.getRawParameterValue ("master_vol")->load();
            if (levels)
                p.apvts.getParameter ("master_vol")->setValueNotifyingHost (masterVol * 0.25f);
            if (p.getRoutes().count != wantRoutes)
            {
                std::printf ("FAIL: preset %s lost routes (%d of %d)\n", info.name.toRawUTF8(), p.getRoutes().count, wantRoutes);
                ++failures;
            }
            juce::AudioBuffer<float> buf (2, 512);
            double sum2 = 0.0;
            float peak = 0.0f;
            bool finite = true;
            const int blocks = (int) (48000.0 * 6.0 / 512.0);
            for (int blk = 0; blk < blocks; ++blk)
            {
                juce::MidiBuffer midi;
                if (blk == 2)
                    for (int n : { 48, 60, 64, 67 })
                        midi.addEvent (juce::MidiMessage::noteOn (1, n, (juce::uint8) 100), 0);
                if (blk == (int) (48000.0 * 4.0 / 512.0))
                    for (int n : { 48, 60, 64, 67 })
                        midi.addEvent (juce::MidiMessage::noteOff (1, n), 0);
                buf.clear();
                p.processBlock (buf, midi);
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < 512; ++i)
                    {
                        const float v = buf.getSample (ch, i);
                        finite = finite && std::isfinite (v);
                        peak = std::max (peak, std::abs (v));
                        sum2 += (double) v * v;
                    }
            }
            const double rmsDb = 10.0 * std::log10 (sum2 / (blocks * 512.0 * 2.0) + 1.0e-12);
            const double peakDb = 20.0 * std::log10 (peak + 1.0e-9);
            if (levels)
            {
                const bool mono = p.apvts.getRawParameterValue ("voicing")->load() < 2.0f; // Mono / Mono Legato
                const double truePeak = peakDb + 12.0, target = mono ? -7.0 : -4.0;
                std::printf ("LEVEL|%s|%d|%.4f|%.2f|%.4f\n", info.name.toRawUTF8(), (int) mono, masterVol, truePeak,
                             masterVol * std::pow (10.0, (target - truePeak) / 20.0));
            }
            else if (report)
                std::printf ("%-24s %-14s peak %6.1f dBFS  rms %6.1f dB\n", info.name.toRawUTF8(), info.category.toRawUTF8(), peakDb, rmsDb);
            if (! finite || peak > 1.0f || rmsDb < -50.0)
            {
                std::printf ("FAIL: preset %s plays badly (finite %d, peak %.1f dB, rms %.1f dB)\n", info.name.toRawUTF8(), (int) finite, peakDb, rmsDb);
                ++failures;
            }
        }
        if (report)
            return failures == 0 ? 0 : 1;
    }

    if (argc > 2 && juce::String (argv[1]) == "--snapshot")
    {
        if (argc > 3)
            for (const auto& pi : b.presets().all())
                if (pi.name == juce::String (argv[3]))
                    b.presets().load (pi);
        b.syncEffects();
        snapshotEditor (b, juce::File (argv[2]));
    }

    std::printf (failures == 0 ? "State tests passed.\n" : "State tests FAILED (%d).\n", failures);
    return failures == 0 ? 0 : 1;
}
