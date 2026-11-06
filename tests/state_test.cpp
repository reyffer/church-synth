// Plugin-level test: modulation routes and user-file names survive save/restore
// (MainStage concerts depend on this). Links the plugin's shared code.

#include "plugin/PluginEditor.h"
#include "plugin/PluginProcessor.h"
#include "plugin/PresetManager.h"
#include "dsp/fx/Effects.h"
#include "packs/Packs.h"
#include "plugin/ui/Help.h"

#include <atomic>
#include <thread>

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


// Plays `notes` for 4 s (then 2 s of release) and returns the output peak.
static float playPeak (ChurchSynthProcessor& p, std::initializer_list<int> notes, bool* finite = nullptr, double holdSeconds = 4.0)
{
    juce::AudioBuffer<float> buf (2, 512);
    float peak = 0.0f;
    bool ok = true;
    const int blocks = (int) (48000.0 * (holdSeconds + 2.0) / 512.0);
    for (int blk = 0; blk < blocks; ++blk)
    {
        juce::MidiBuffer midi;
        if (blk == 2)
            for (int n : notes)
                midi.addEvent (juce::MidiMessage::noteOn (1, n, (juce::uint8) 100), 0);
        if (blk == (int) (48000.0 * holdSeconds / 512.0))
            for (int n : notes)
                midi.addEvent (juce::MidiMessage::noteOff (1, n), 0);
        buf.clear();
        p.processBlock (buf, midi);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 512; ++i)
            {
                const float v = buf.getSample (ch, i);
                ok = ok && std::isfinite (v);
                peak = std::max (peak, std::abs (v));
            }
    }
    if (finite != nullptr)
        *finite = ok;
    return peak;
}

// --export-pack <dir>: writes every preset pack as .csp files, level-matched like
// the factory set (-4 dBFS peak for a 4-note chord, -7 dBFS for a mono note).
static int exportPacks (const juce::File& dir, const juce::String& only = {})
{
    auto pPtr = std::make_unique<ChurchSynthProcessor>();
    auto& p = *pPtr;
    int failures = 0;
    for (const auto& pack : presetPacks())
    {
        if (only.isNotEmpty() && ! juce::String (pack.folder).containsIgnoreCase (only))
            continue;
        const auto folder = dir.getChildFile (pack.folder);
        folder.deleteRecursively();
        folder.createDirectory();
        for (const auto& fp : pack.presets())
        {
            PresetBuilder b;
            fp.build (b);
            const auto tree = b.build (fp.name, fp.category);
            for (const auto& e : b.errors)
            {
                std::printf ("FAIL: %s: %s\n", fp.name, e.toRawUTF8());
                ++failures;
            }
            const int wantRoutes = tree.getChildWithName ("ROUTES").getNumChildren();
            auto measure = [&] (float vol) {
                p.restoreStateTree (tree, ChurchSynthProcessor::LoadKind::Preset);
                p.apvts.getParameter ("master_vol")->setValueNotifyingHost (vol);
                p.syncEffects();
                p.prepareToPlay (48000.0, 512);
                const bool mono = p.apvts.getRawParameterValue ("voicing")->load() < 2.0f;
                const juce::String cat (fp.category);
                const bool bass = cat == "Bass";
                // Drones, swells and risers take longer to reach full level.
                const double hold = (cat == "Texture" || cat == "FX") ? 10.0 : 4.0;
                bool finite = true;
                // Textures (drones, atmospheres) are played with one held note, like mono sounds.
                const bool single = mono || cat == "Texture";
                const float pk = single ? playPeak (p, { bass ? 36 : (mono ? 60 : 48) }, &finite, hold)
                                        : playPeak (p, { 48, 60, 64, 67 }, &finite, hold);
                if (! finite)
                {
                    std::printf ("FAIL: %s is not finite\n", fp.name);
                    ++failures;
                }
                return std::make_pair (pk, single);
            };
            // Well below the limiter; three runs (random phases, free-running modulators) and keep the loudest.
            auto [quiet, mono] = measure (0.2f);
            for (int run = 0; run < 2; ++run)
                quiet = std::max (quiet, measure (0.2f).first);
            const double truePeakDb = 20.0 * std::log10 (quiet / 0.2 + 1.0e-9);
            const double target = mono ? -7.0 : -4.0;
            const float vol = (float) juce::jlimit (0.05, 1.0, std::pow (10.0, (target - truePeakDb) / 20.0));
            const float finalPeak = measure (std::round (vol * 100.0f) / 100.0f).first;
            if (p.getRoutes().count != wantRoutes)
            {
                std::printf ("FAIL: %s lost routes (%d of %d)\n", fp.name, p.getRoutes().count, wantRoutes);
                ++failures;
            }
            if (finalPeak > 1.0f || finalPeak < 0.05f)
            {
                std::printf ("FAIL: %s level %.1f dBFS\n", fp.name, 20.0 * std::log10 (finalPeak + 1.0e-9));
                ++failures;
            }
            // Write the sound as loaded (fresh state, no MIDI map) with the matched volume.
            p.restoreStateTree (tree, ChurchSynthProcessor::LoadKind::Preset);
            p.apvts.getParameter ("master_vol")->setValueNotifyingHost (std::round (vol * 100.0f) / 100.0f);
            const auto file = folder.getChildFile (PresetManager::safeFileName (fp.name) + PresetManager::kExtension);
            const auto tags = pack.tags (fp.name);
            if (tags.isEmpty())
            {
                std::printf ("FAIL: %s has no tags\n", fp.name);
                ++failures;
            }
            const auto notes = pack.notes (fp.name);
            if (notes.length() < 80)
            {
                std::printf ("FAIL: %s has no notes\n", fp.name);
                ++failures;
            }
            if (! p.presets().writePresetFile (file, fp.name, fp.category, "Church Synth", fp.description, tags, notes))
            {
                std::printf ("FAIL: couldn't write %s\n", file.getFullPathName().toRawUTF8());
                ++failures;
            }
            std::printf ("%-24s %-14s %s  vol %.2f  peak %6.1f dBFS\n", fp.name, fp.category, mono ? "mono" : "poly",
                         std::round (vol * 100.0f) / 100.0f, 20.0 * std::log10 (finalPeak + 1.0e-9));
        }
    }
    // Read back: every file parses and restores with its routes.
    for (const auto& f : dir.findChildFiles (juce::File::findFiles, true, "*.csp"))
    {
        PresetInfo info;
        info.factory = false;
        info.file = f;
        const auto t = p.presets().treeFor (info);
        if (! t.isValid())
        {
            std::printf ("FAIL: %s doesn't read back\n", f.getFileName().toRawUTF8());
            ++failures;
        }
    }
    std::printf (failures == 0 ? "Pack export OK.\n" : "Pack export FAILED (%d).\n", failures);
    return failures == 0 ? 0 : 1;
}

// --render-pack <exported dir> <wav dir>: one WAV per exported pack preset, with a
// phrase that suits it (chords for keys/pads, a melody for bells and leads, a held
// note for bass sequences). For the audition reel.
static int renderPack (const juce::File& packDir, const juce::File& wavDir, const juce::String& only = {})
{
    auto pPtr = std::make_unique<ChurchSynthProcessor>();
    auto& p = *pPtr;
    wavDir.createDirectory();
    int index = 0;
    for (const auto& pack : presetPacks())
        for (const auto& fp : pack.presets())
        {
            if (only.isNotEmpty() && ! juce::String (pack.folder).containsIgnoreCase (only))
                continue;
            PresetInfo info;
            info.factory = false;
            info.name = fp.name;
            info.category = fp.category;
            info.file = packDir.getChildFile (pack.folder).getChildFile (PresetManager::safeFileName (fp.name) + PresetManager::kExtension);
            if (! p.presets().load (info))
            {
                std::printf ("FAIL: can't load %s\n", info.file.getFullPathName().toRawUTF8());
                return 1;
            }
            p.syncEffects();
            p.prepareToPlay (48000.0, 512);
            const juce::String cat (fp.category);
            const bool seq = p.apvts.getRawParameterValue ("seq_on")->load() >= 0.5f;
            const bool mono = p.apvts.getRawParameterValue ("voicing")->load() < 2.0f;
            struct Ev { double on, off; int note; int vel; };
            std::vector<Ev> evs;
            auto chord = [&] (double on, double off, std::initializer_list<int> notes, int vel) {
                for (int n : notes) evs.push_back ({ on, off, n, vel });
            };
            double len = 8.0;
            if (seq && cat == "Bass")
            {
                evs.push_back ({ 0.05, 3.0, 36, 100 });
                evs.push_back ({ 3.0, 4.5, 33, 100 });
                evs.push_back ({ 4.5, 6.0, 29, 100 });
                len = 7.0;
            }
            else if (seq)
            {
                chord (0.05, 3.0, { 48, 55, 60, 64 }, 100);
                chord (3.0, 6.0, { 45, 52, 57, 60 }, 100);
                len = 8.0;
            }
            else if (cat == "Texture" || cat == "FX")
            {
                // Held long enough for drones to settle and swells/risers to finish.
                if (cat == "Texture")
                    chord (0.05, 10.0, { juce::String (fp.name).contains ("Sub") ? 36 : 48 }, 100);
                else
                    chord (0.05, 9.0, { 48, 55, 60, 64 }, 100);
                len = 13.0;
            }
            else if (mono)
            {
                const int base = cat == "Bass" ? 36 : 60;
                const int mel[8] = { 0, 4, 7, 9, 7, 4, 2, 0 };
                for (int k = 0; k < 8; ++k)
                    evs.push_back ({ 0.05 + k * 0.4, 0.05 + k * 0.4 + (k == 7 ? 1.4 : 0.45), base + mel[k], 90 + (k % 2) * 20 });
                len = 6.0;
            }
            else if (cat == "Pluck")
            {
                const int mel[6] = { 76, 74, 72, 67, 72, 79 };
                for (int k = 0; k < 6; ++k)
                    evs.push_back ({ 0.05 + k * 0.45, 0.4 + k * 0.45, mel[k], 80 + (k % 3) * 15 });
                chord (2.9, 4.4, { 60, 64, 67, 72 }, 100);
                len = 8.0;
            }
            else
            {
                const bool slow = cat == "Pad" || cat == "Strings/Brass";
                const double d = slow ? 2.4 : 1.4;
                chord (0.05, 0.05 + d, { 48, 60, 64, 67 }, 100);
                chord (0.05 + d, 0.05 + 2 * d, { 45, 57, 60, 64 }, 85);
                chord (0.05 + 2 * d, 0.05 + 3 * d, { 41, 57, 60, 65 }, 110);
                len = 3 * d + (slow ? 3.0 : 2.5);
            }
            juce::AudioBuffer<float> out (2, (int) (48000.0 * len) / 512 * 512);
            juce::AudioBuffer<float> buf (2, 512);
            for (int pos = 0; pos + 512 <= out.getNumSamples(); pos += 512)
            {
                juce::MidiBuffer midi;
                const double t0 = pos / 48000.0, t1 = (pos + 512) / 48000.0;
                for (const auto& e : evs)
                {
                    if (e.off >= t0 && e.off < t1)
                        midi.addEvent (juce::MidiMessage::noteOff (1, e.note), juce::jlimit (0, 511, (int) ((e.off - t0) * 48000.0)));
                    if (e.on >= t0 && e.on < t1)
                        midi.addEvent (juce::MidiMessage::noteOn (1, e.note, (juce::uint8) e.vel), juce::jlimit (0, 511, (int) ((e.on - t0) * 48000.0)));
                }
                buf.clear();
                p.processBlock (buf, midi);
                for (int ch = 0; ch < 2; ++ch)
                    out.copyFrom (ch, pos, buf, ch, 0, 512);
            }
            out.applyGainRamp (out.getNumSamples() - 14400, 14400, 1.0f, 0.0f);
            const auto f = wavDir.getChildFile (juce::String (++index).paddedLeft ('0', 2) + " " + fp.name + ".wav");
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

// --stress-threads [seconds]: like pluginval's "Parameter thread safety" test, but
// longer and meaner: the audio thread plays notes while other threads set random
// parameters (including FX types), read and write the state, and load presets.
// Run it in a ThreadSanitizer build to find data races.
static int stressThreads (double seconds, int mask = 7)
{
    auto pPtr = std::make_unique<ChurchSynthProcessor>();
    auto& p = *pPtr;
    p.prepareToPlay (48000.0, 256);
    std::atomic<bool> stop { false };
    auto& params = p.getParameters();
    std::thread setter ([&] {
        juce::Random rng (1);
        while (! stop.load() && (mask & 1) != 0)
            for (int k = 0; k < 64 && ! stop.load(); ++k)
                params[rng.nextInt (params.size())]->setValue (rng.nextFloat());
    });
    std::thread setterNotify ([&] {
        juce::Random rng (2);
        while (! stop.load() && (mask & 2) != 0)
        {
            auto* prm = params[rng.nextInt (params.size())];
            prm->beginChangeGesture();
            prm->setValueNotifyingHost (rng.nextFloat());
            prm->endChangeGesture();
        }
    });
    std::thread stateThread ([&] {
        juce::Random rng (3);
        while (! stop.load() && (mask & 4) != 0)
        {
            juce::MemoryBlock mb;
            p.getStateInformation (mb);
            if (rng.nextInt (4) == 0 && (mask & 8) == 0)
                p.setStateInformation (mb.getData(), (int) mb.getSize());
        }
    });
    juce::AudioBuffer<float> buf (2, 256);
    juce::Random rng (4);
    const auto end = juce::Time::getMillisecondCounterHiRes() + seconds * 1000.0;
    int blocks = 0;
    bool finite = true;
    while (juce::Time::getMillisecondCounterHiRes() < end)
    {
        juce::MidiBuffer midi;
        if (rng.nextInt (8) == 0)
            midi.addEvent (juce::MidiMessage::noteOn (1, 36 + rng.nextInt (48), (juce::uint8) 100), rng.nextInt (256));
        if (rng.nextInt (8) == 0)
            midi.addEvent (juce::MidiMessage::noteOff (1, 36 + rng.nextInt (48)), rng.nextInt (256));
        buf.clear();
        p.processBlock (buf, midi);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 256; ++i)
                if (finite && ! std::isfinite (buf.getSample (ch, i)))
                {
                    finite = false;
                    std::printf ("first non-finite sample at block %d\n", blocks);
                }
        if ((++blocks & 31) == 0)
            p.syncEffects(); // what the plugin timer does on the message thread
    }
    stop.store (true);
    setter.join();
    setterNotify.join();
    stateThread.join();
    std::printf ("stress: %d blocks, output %s\n", blocks, finite ? "finite" : "NOT FINITE");
    return finite ? 0 : 1;
}

// --fuzz [trials]: random values for every parameter, play notes, check the output
// stays finite. On a failure, prints which parameters cause it (resetting each one
// alone to see if the problem goes away).
static int fuzzParams (int trials)
{
    auto pPtr = std::make_unique<ChurchSynthProcessor>();
    auto& p = *pPtr;
    auto& params = p.getParameters();
    auto run = [&] (const std::vector<float>& values) {
        for (int k = 0; k < params.size(); ++k)
            params[k]->setValue (values[(size_t) k]);
        p.syncEffects();
        p.prepareToPlay (48000.0, 256);
        juce::AudioBuffer<float> buf (2, 256);
        for (int blk = 0; blk < 200; ++blk)
        {
            juce::MidiBuffer midi;
            if (blk == 1)
                for (int n : { 36, 60, 64, 67, 84 })
                    midi.addEvent (juce::MidiMessage::noteOn (1, n, (juce::uint8) 110), 0);
            if (blk == 120)
                for (int n : { 36, 60, 64, 67, 84 })
                    midi.addEvent (juce::MidiMessage::noteOff (1, n), 0);
            buf.clear();
            p.processBlock (buf, midi);
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 256; ++i)
                    if (! std::isfinite (buf.getSample (ch, i)) || std::abs (buf.getSample (ch, i)) > 1.0f)
                        return false;
        }
        return true;
    };
    int failures = 0;
    for (int t = 0; t < trials; ++t)
    {
        juce::Random rng (1000 + t);
        std::vector<float> values ((size_t) params.size()), defaults ((size_t) params.size());
        for (int k = 0; k < params.size(); ++k)
        {
            values[(size_t) k] = rng.nextFloat();
            defaults[(size_t) k] = params[k]->getDefaultValue();
        }
        if (run (values))
            continue;
        ++failures;
        std::printf ("FAIL: trial %d is not finite/over full scale. Culprits:", t);
        for (int k = 0; k < params.size(); ++k)
        {
            auto v2 = values;
            v2[(size_t) k] = defaults[(size_t) k];
            if (run (v2))
                std::printf (" %s=%.3f", params[k]->getName (40).toRawUTF8(), values[(size_t) k]);
        }
        std::printf ("\n");
        if (failures >= 5)
            break;
    }
    std::printf ("fuzz: %d failing of %d trials\n", failures, trials);
    return failures == 0 ? 0 : 1;
}

// --fuzz-dynamic [blocks]: single-threaded; before each audio block a few random
// parameters change (like knobs being turned while playing). Stops at the first
// non-finite sample and prints the most recent changes.
static int fuzzDynamic (int maxBlocks, int seed)
{
    auto pPtr = std::make_unique<ChurchSynthProcessor>();
    auto& p = *pPtr;
    auto& params = p.getParameters();
    p.prepareToPlay (48000.0, 256);
    juce::Random rng (seed);
    juce::AudioBuffer<float> buf (2, 256);
    std::vector<std::pair<int, float>> recent;
    for (int blk = 0; blk < maxBlocks; ++blk)
    {
        for (int c = 0; c < 3; ++c)
        {
            const int k = rng.nextInt (params.size());
            const float v = rng.nextFloat();
            params[k]->setValue (v);
            recent.push_back ({ k, v });
            if (recent.size() > 40)
                recent.erase (recent.begin());
        }
        if ((blk & 31) == 0)
            p.syncEffects();
        juce::MidiBuffer midi;
        if (rng.nextInt (8) == 0)
            midi.addEvent (juce::MidiMessage::noteOn (1, 36 + rng.nextInt (48), (juce::uint8) 100), rng.nextInt (256));
        if (rng.nextInt (8) == 0)
            midi.addEvent (juce::MidiMessage::noteOff (1, 36 + rng.nextInt (48)), rng.nextInt (256));
        buf.clear();
        p.processBlock (buf, midi);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 256; ++i)
                if (! std::isfinite (buf.getSample (ch, i)))
                {
                    std::printf ("non-finite at block %d. Recent changes (oldest first):\n", blk);
                    for (auto [k, v] : recent)
                        std::printf ("  %s = %.3f (%s)\n", params[k]->getName (40).toRawUTF8(), v,
                                     params[k]->getText (v, 30).toRawUTF8());
                    return 1;
                }
    }
    std::printf ("fuzz-dynamic: %d blocks finite\n", maxBlocks);
    return 0;
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    int failures = 0;

    if (argc > 1 && juce::String (argv[1]) == "--fuzz-dynamic")
        return fuzzDynamic (argc > 2 ? juce::String (argv[2]).getIntValue() : 200000, argc > 3 ? juce::String (argv[3]).getIntValue() : 1);
    if (argc > 1 && juce::String (argv[1]) == "--fuzz")
        return fuzzParams (argc > 2 ? juce::String (argv[2]).getIntValue() : 50);
    if (argc > 1 && juce::String (argv[1]) == "--stress-threads")
        return stressThreads (argc > 2 ? juce::String (argv[2]).getDoubleValue() : 10.0, argc > 3 ? juce::String (argv[3]).getIntValue() : 7);
    if (argc > 3 && juce::String (argv[1]) == "--render-pack")
        return renderPack (juce::File (argv[2]), juce::File (argv[3]), argc > 4 ? juce::String (argv[4]) : juce::String());
    if (argc > 2 && juce::String (argv[1]) == "--export-pack")
        return exportPacks (juce::File (argv[2]), argc > 3 ? juce::String (argv[3]) : juce::String());

    // Every factory and pack preset uses only known parameters, menu items and routes.
    {
        auto check = [&] (const std::vector<FactoryPreset>& list) {
            for (const auto& fp : list)
            {
                PresetBuilder b;
                fp.build (b);
                for (const auto& e : b.errors)
                {
                    std::printf ("FAIL: preset %s: %s\n", fp.name, e.toRawUTF8());
                    ++failures;
                }
            }
        };
        check (factoryPresets());
        for (const auto& fp : factoryPresets())
            if (factoryPresetNotes (fp.name).length() < 80 || factoryPresetTags (fp.name).isEmpty())
            {
                std::printf ("FAIL: factory preset %s has no notes or tags\n", fp.name);
                ++failures;
            }
        for (const auto& pack : presetPacks())
        {
            check (pack.presets());
            for (const auto& fp : pack.presets())
                if (pack.notes (fp.name).length() < 80 || pack.tags (fp.name).isEmpty())
                {
                    std::printf ("FAIL: pack preset %s has no notes or tags\n", fp.name);
                    ++failures;
                }
        }
    }

    // Every parameter has a help-bar explanation.
    {
        auto pPtr = std::make_unique<ChurchSynthProcessor>();
        ui::UiContext hctx (*pPtr);
        int missing = 0;
        for (const auto& d : cs::parameterDefs())
            if (! ui::paramHelp (hctx, d.id).contains (": "))
            {
                if (++missing <= 10)
                    std::printf ("FAIL: no help text for %s\n", d.id.toRawUTF8());
            }
        failures += missing;
    }

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
        // Tags: the saved preset carries the tags it was made from; edits, renames and
        // deletes persist (Preset Tags.xml) and survive a rescan.
        {
            const auto tagFile = PresetManager::rootFolder().getChildFile ("Preset Tags.xml");
            const bool hadTagFile = tagFile.existsAsFile();
            const auto tagBackup = tagFile.loadFileAsString();
            auto find = [&pm] (const juce::String& n) {
                for (const auto& info : pm.all())
                    if (info.name == n)
                        return info;
                return PresetInfo {};
            };
            const auto sanctuaryInfo = find ("Sanctuary Pad");
            if (! sanctuaryInfo.tags.contains ("Pad") || ! pm.allTags().contains ("Organ"))
            {
                std::printf ("FAIL: factory tags missing\n");
                ++failures;
            }
            pm.setTags (saved, { "zz Test Tag", "Pad" });
            pm.rescan();
            if (! find (testName).tags.contains ("zz Test Tag"))
            {
                std::printf ("FAIL: tag edit not kept after rescan\n");
                ++failures;
            }
            pm.renameTag ("zz Test Tag", "zz Renamed Tag");
            pm.rescan();
            if (! find (testName).tags.contains ("zz Renamed Tag") || pm.allTags().contains ("zz Test Tag"))
            {
                std::printf ("FAIL: tag rename\n");
                ++failures;
            }
            pm.deleteTag ("zz Renamed Tag");
            pm.rescan();
            if (pm.allTags().contains ("zz Renamed Tag") || ! find (testName).tags.contains ("Pad"))
            {
                std::printf ("FAIL: tag delete\n");
                ++failures;
            }
            // Renames and deletes are remembered for presets that arrive later.
            {
                const auto incoming = PresetManager::userPresetFolder().getChildFile ("zz incoming test.csp");
                pm.renameTag ("zz Rule A", "zz Rule B");            // no preset has it yet: just the rule
                pm.deleteTag ("zz Rule Gone");
                pm.writePresetFile (incoming, "zz incoming test", "Pad", "test", "", { "zz Rule A", "zz Rule Gone", "Pad" });
                pm.rescan();
                const auto in = find ("zz incoming test");
                if (! in.tags.contains ("zz Rule B") || in.tags.contains ("zz Rule A") || in.tags.contains ("zz Rule Gone") || ! in.tags.contains ("Pad"))
                {
                    std::printf ("FAIL: tag rules not applied to an incoming preset (%s)\n", in.tags.joinIntoString (", ").toRawUTF8());
                    ++failures;
                }
                pm.renameTag ("zz Rule B", "zz Rule C"); // chains: A -> C as well
                pm.rescan();
                if (! find ("zz incoming test").tags.contains ("zz Rule C"))
                {
                    std::printf ("FAIL: chained tag rename\n");
                    ++failures;
                }
                pm.remove (find ("zz incoming test"));
            }
            // A preset saved from a tagged one keeps its tags in the file.
            pm.load (sanctuaryInfo);
            const juce::String copyName = "zz Church Synth tag copy";
            pm.save (copyName, "Pad", "test", "", err);
            const auto copy = find (copyName);
            if (! copy.tags.contains ("Worship") || ! copy.file.loadFileAsString().contains ("tags=\""))
            {
                std::printf ("FAIL: saved preset lost its tags\n");
                ++failures;
            }
            pm.remove (copy);
            if (hadTagFile)
                tagFile.replaceWithText (tagBackup);
            else
                tagFile.deleteFile();
            pm.rescan();
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
