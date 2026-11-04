#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
const char* const kSlotParam[] = { "e1_wt_table", "e2_wt_table", "u_n1_type", "u_n2_type" };
const char* const kSlotStateKey[] = { "user_wt1", "user_wt2", "user_noise1", "user_noise2" };
bool isWavetableSlot (int slot) { return slot < 2; }

constexpr size_t kMaxWavetableSamples = (size_t) cs::Wavetable::kMaxFrames * cs::Wavetable::kBaseSize;
constexpr double kMaxNoiseSeconds = 20.0;
} // namespace

// Background thread that resolves wavetable / noise selections, so generating
// or loading content never happens on the audio thread.
class ChurchSynthProcessor::Loader final : public juce::Thread
{
public:
    explicit Loader (ChurchSynthProcessor& p) : juce::Thread ("Church Synth content"), owner (p) {}
    void run() override
    {
        while (! threadShouldExit())
        {
            owner.refreshContent();
            wait (40);
        }
    }

private:
    ChurchSynthProcessor& owner;
};

ChurchSynthProcessor::ChurchSynthProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "ChurchSynth", cs::createParameterLayout())
{
    for (const auto& d : cs::parameterDefs())
        rawParams.push_back (apvts.getRawParameterValue (d.id));
    formats.registerBasicFormats();

    refreshContent(); // defaults ready before the first block
    loader = std::make_unique<Loader> (*this);
    loader->startThread (juce::Thread::Priority::low);

    readFxTypes (lastFxType);
    fxHost.rebuild (lastFxType, (float) currentRate);
    startTimerHz (20);
}

// --- FX ----------------------------------------------------------------------
namespace
{
const char* const kFxSlotIds[9] = { "a1", "a2", "a3", "b1", "b2", "b3", "x1", "x2", "x3" };
juce::String fxParamId (int slot, const char* suffix) { return juce::String ("fx_") + kFxSlotIds[slot] + "_" + suffix; }
} // namespace

void ChurchSynthProcessor::readFxTypes (int* types) const
{
    for (int sl = 0; sl < cs::fx::kNumFxSlots; ++sl)
        types[sl] = (int) std::lround (apvts.getRawParameterValue (fxParamId (sl, "type"))->load());
}

// New effect picked in a slot: load that effect's default settings
// (Aux slots default to 100 % wet, as send effects should be).
void ChurchSynthProcessor::applyFxDefaults (int slot, int type)
{
    const auto& info = cs::fx::fxInfo (type);
    for (int k = 0; k < cs::fx::kFxParams; ++k)
    {
        float v = info.defaults[(size_t) k];
        if (slot >= 6 && info.params[(size_t) k] != nullptr && juce::String (info.params[(size_t) k]) == "Dry/Wet")
            v = 1.0f;
        if (auto* prm = apvts.getParameter (fxParamId (slot, ("p" + juce::String (k + 1)).toRawUTF8())))
            prm->setValueNotifyingHost (prm->convertTo0to1 (v));
    }
}

void ChurchSynthProcessor::syncEffects()
{
    const juce::ScopedLock fxScope (fxLock);
    int types[cs::fx::kNumFxSlots];
    readFxTypes (types);
    for (int sl = 0; sl < cs::fx::kNumFxSlots; ++sl)
        if (types[sl] != lastFxType[sl])
        {
            if (types[sl] != cs::fx::FxNone)
                applyFxDefaults (sl, types[sl]);
            lastFxType[sl] = types[sl];
        }
    fxHost.sync (types, (float) currentRate, synth.getRenderEpoch());
}

void ChurchSynthProcessor::timerCallback()
{
    syncEffects();
}

ChurchSynthProcessor::~ChurchSynthProcessor()
{
    stopTimer();
    loader->stopThread (2000);
}

bool ChurchSynthProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void ChurchSynthProcessor::prepareToPlay (double sampleRate, int)
{
    currentRate = sampleRate;
    synth.setSampleRate ((float) sampleRate);
    {
        const juce::ScopedLock fxScope (fxLock);
        readFxTypes (lastFxType);
        fxHost.rebuild (lastFxType, (float) sampleRate); // audio is stopped here: safe to rebuild
    }
    pullParameters();
}

void ChurchSynthProcessor::pullParameters() noexcept
{
    const auto& defs = cs::parameterDefs();
    for (size_t i = 0; i < defs.size(); ++i)
        defs[i].apply (synth.params, rawParams[i]->load (std::memory_order_relaxed));

    for (size_t e = 0; e < 2; ++e)
    {
        synth.params.engine[e].wt.table = wtPtr[e].load (std::memory_order_acquire);
        synth.params.utility.noise[e].sample = noisePtr[e].load (std::memory_order_acquire);
    }
}

// ---------------------------------------------------------------------------
juce::File ChurchSynthProcessor::userFolder (bool wavetables)
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
        .getChildFile ("Church Synth")
        .getChildFile (wavetables ? "Wavetables" : "Noises");
}

juce::String ChurchSynthProcessor::desiredKey (int slot) const
{
    if (userFile[slot].isNotEmpty())
    {
        const auto f = userFolder (isWavetableSlot (slot)).getChildFile (userFile[slot]);
        if (f.existsAsFile())
            return "user:" + userFile[slot] + "|" + juce::String (f.getLastModificationTime().toMilliseconds());
    }
    const int index = (int) std::lround (apvts.getRawParameterValue (kSlotParam[slot])->load());
    return "factory:" + juce::String (index);
}

void ChurchSynthProcessor::refreshContent()
{
    const juce::ScopedLock sl (contentLock);
    auto& lib = cs::ContentLibrary::get();

    for (int slot = 0; slot < NumContentSlots; ++slot)
    {
        const auto key = desiredKey (slot);
        missing[slot] = userFile[slot].isNotEmpty() && key.startsWith ("factory:");
        if (key == loadedKey[slot])
            continue;

        const bool wt = isWavetableSlot (slot);
        const cs::Wavetable* table = nullptr;
        const cs::NoiseSample* noise = nullptr;

        if (key.startsWith ("user:"))
        {
            const auto stdKey = key.toStdString();
            table = wt ? lib.findUserWavetable (stdKey) : nullptr;
            noise = wt ? nullptr : lib.findUserNoise (stdKey);

            if (table == nullptr && noise == nullptr)
            {
                const auto file = userFolder (wt).getChildFile (userFile[slot]);
                if (std::unique_ptr<juce::AudioFormatReader> reader { formats.createReaderFor (file) })
                {
                    const auto maxLen = wt ? (juce::int64) kMaxWavetableSamples
                                           : (juce::int64) (kMaxNoiseSeconds * reader->sampleRate);
                    const int len = (int) juce::jmin (reader->lengthInSamples, maxLen);
                    juce::AudioBuffer<float> buf ((int) juce::jmax (1u, reader->numChannels), juce::jmax (len, 1));
                    buf.clear();
                    reader->read (&buf, 0, len, 0, true, true);

                    if (wt)
                    {
                        // Mono mix of all channels.
                        std::vector<float> mono ((size_t) len, 0.0f);
                        for (int ch = 0; ch < buf.getNumChannels(); ++ch)
                            for (int i = 0; i < len; ++i)
                                mono[(size_t) i] += buf.getSample (ch, i) / (float) buf.getNumChannels();
                        table = lib.addUserWavetable (stdKey, cs::Wavetable::fromSamples (file.getFileNameWithoutExtension().toStdString(),
                                                                                          mono.data(), mono.size()));
                    }
                    else
                    {
                        const float* r = buf.getNumChannels() > 1 ? buf.getReadPointer (1) : nullptr;
                        noise = lib.addUserNoise (stdKey, cs::NoiseSample::fromSamples (file.getFileNameWithoutExtension().toStdString(),
                                                                                        buf.getReadPointer (0), r, (size_t) len,
                                                                                        (float) reader->sampleRate));
                    }
                }
            }
            if (table == nullptr && noise == nullptr) // unreadable: fall back to the factory choice
            {
                missing[slot] = true;
                const int index = (int) std::lround (apvts.getRawParameterValue (kSlotParam[slot])->load());
                table = wt ? lib.factoryWavetable (index) : nullptr;
                noise = wt ? nullptr : lib.factoryNoise (index);
            }
        }
        else
        {
            const int index = key.fromFirstOccurrenceOf (":", false, false).getIntValue();
            table = wt ? lib.factoryWavetable (index) : nullptr;
            noise = wt ? nullptr : lib.factoryNoise (index);
        }

        if (wt)
            wtPtr[slot].store (table, std::memory_order_release);
        else
            noisePtr[slot - Noise1].store (noise, std::memory_order_release);
        loadedKey[slot] = key;
    }
}

bool ChurchSynthProcessor::importUserFile (int slot, const juce::File& file, juce::String& error)
{
    const bool wt = isWavetableSlot (slot);
    std::unique_ptr<juce::AudioFormatReader> reader { formats.createReaderFor (file) };
    if (reader == nullptr)
    {
        error = "Couldn't read " + file.getFileName() + ". Use a WAV, AIFF or FLAC file.";
        return false;
    }
    if (wt && reader->lengthInSamples < cs::Wavetable::kBaseSize)
    {
        error = "A wavetable needs at least 2048 samples (one cycle per 2048-sample frame).";
        return false;
    }

    auto folder = userFolder (wt);
    folder.createDirectory();
    auto dest = folder.getChildFile (file.getFileName());
    if (dest != file && ! file.copyFileTo (dest))
    {
        error = "Couldn't copy the file into " + folder.getFullPathName();
        return false;
    }

    {
        const juce::ScopedLock sl (contentLock);
        userFile[slot] = dest.getFileName();
    }
    storeUserNamesInState();
    refreshContent();
    return true;
}

void ChurchSynthProcessor::clearUserFile (int slot)
{
    {
        const juce::ScopedLock sl (contentLock);
        userFile[slot].clear();
    }
    storeUserNamesInState();
    refreshContent();
}

juce::String ChurchSynthProcessor::getSlotDisplayName (int slot) const
{
    const juce::ScopedLock sl (contentLock);
    if (userFile[slot].isNotEmpty())
        return "User: " + userFile[slot] + (missing[slot] ? "  (MISSING - using factory)" : "");
    const int index = (int) std::lround (apvts.getRawParameterValue (kSlotParam[slot])->load());
    auto& lib = cs::ContentLibrary::get();
    const auto e = isWavetableSlot (slot) ? lib.wavetableEntry (index) : lib.noiseEntry (index);
    return juce::String (e.category) + ": " + e.name;
}

void ChurchSynthProcessor::storeUserNamesInState()
{
    const juce::ScopedLock sl (contentLock);
    for (int slot = 0; slot < NumContentSlots; ++slot)
        apvts.state.setProperty (kSlotStateKey[slot], userFile[slot], nullptr);
}

// ---------------------------------------------------------------------------
cs::ModRouting ChurchSynthProcessor::getRoutes() const
{
    const juce::ScopedLock sl (routeLock);
    return routesEdit;
}

void ChurchSynthProcessor::setRoutes (const cs::ModRouting& r)
{
    {
        const juce::ScopedLock sl (routeLock);
        routesEdit = r;
    }
    publishRoutes (r);
}

void ChurchSynthProcessor::publishRoutes (const cs::ModRouting& r)
{
    const juce::ScopedLock sl (routeLock); // one writer at a time
    routeSeq.fetch_add (1, std::memory_order_acq_rel); // odd: writing
    std::atomic_thread_fence (std::memory_order_release);
    routesShared = r;
    std::atomic_thread_fence (std::memory_order_release);
    routeSeq.fetch_add (1, std::memory_order_acq_rel); // even: stable
}

void ChurchSynthProcessor::pullRoutes() noexcept
{
    const uint32_t s1 = routeSeq.load (std::memory_order_acquire);
    if (s1 == routeSeqSeen || (s1 & 1u) != 0)
        return;
    routesAudio = routesShared;
    std::atomic_thread_fence (std::memory_order_acquire);
    if (routeSeq.load (std::memory_order_acquire) != s1)
        return; // a write raced us: try again next block
    synth.setRouting (routesAudio);
    routeSeqSeen = s1;
}

juce::ValueTree ChurchSynthProcessor::routesToTree (const cs::ModRouting& r)
{
    juce::ValueTree t ("ROUTES");
    const auto& dests = cs::modDestinations();
    for (int i = 0; i < r.count; ++i)
    {
        const auto& rt = r.routes[i];
        if (rt.source < 0 || rt.dest < 0 || rt.dest >= (int) dests.size())
            continue;
        juce::ValueTree e ("ROUTE");
        e.setProperty ("src", cs::modSourceName (rt.source), nullptr);
        e.setProperty ("dest", dests[(size_t) rt.dest].id, nullptr);
        e.setProperty ("amt", rt.amount, nullptr);
        e.setProperty ("sc", rt.sidechain >= 0 ? juce::String (cs::modSourceName (rt.sidechain)) : juce::String(), nullptr);
        e.setProperty ("sca", rt.scAmount, nullptr);
        t.appendChild (e, nullptr);
    }
    return t;
}

cs::ModRouting ChurchSynthProcessor::routesFromTree (const juce::ValueTree& t)
{
    cs::ModRouting r;
    for (const auto& e : t)
    {
        if (r.count >= cs::kMaxRoutes)
            break;
        cs::ModRoute rt;
        rt.source = cs::findModSource (e["src"].toString().toStdString());
        rt.dest = cs::findModDest (e["dest"].toString().toStdString());
        rt.amount = juce::jlimit (-1.0f, 1.0f, (float) e["amt"]);
        const auto sc = e["sc"].toString();
        rt.sidechain = sc.isEmpty() ? -1 : cs::findModSource (sc.toStdString());
        rt.scAmount = e.hasProperty ("sca") ? juce::jlimit (0.0f, 1.0f, (float) e["sca"]) : 1.0f;
        if (rt.source >= 0 && rt.dest >= 0) // unknown sources/destinations are dropped, not crashed on
            r.routes[r.count++] = rt;
    }
    return r;
}

// ---------------------------------------------------------------------------
void ChurchSynthProcessor::handleMidi (const juce::MidiMessage& m) noexcept
{
    if (m.isNoteOn())
        synth.noteOn (m.getNoteNumber(), m.getFloatVelocity());
    else if (m.isNoteOff())
        synth.noteOff (m.getNoteNumber());
    else if (m.isPitchWheel())
        synth.setPitchBend ((float) (m.getPitchWheelValue() - 8192) / 8192.0f);
    else if (m.isSustainPedalOn())
        synth.setSustain (true);
    else if (m.isSustainPedalOff())
        synth.setSustain (false);
    else if (m.isControllerOfType (1))
        synth.setModWheel ((float) m.getControllerValue() / 127.0f);
    else if (m.isControllerOfType (11))
        synth.setExpression ((float) m.getControllerValue() / 127.0f);
    else if (m.isChannelPressure())
        synth.setChannelPressure ((float) m.getChannelPressureValue() / 127.0f);
    else if (m.isAftertouch())
        synth.setPolyPressure (m.getNoteNumber(), (float) m.getAfterTouchValue() / 127.0f);
    else if (m.isAllNotesOff() || m.isAllSoundOff())
        synth.allNotesOff();
}

void ChurchSynthProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    pullParameters();
    pullRoutes();

    if (auto* ph = getPlayHead())
        if (const auto pos = ph->getPosition())
        {
            cs::Transport t;
            t.bpm = pos->getBpm().orFallback (120.0);
            t.ppq = pos->getPpqPosition().orFallback (0.0);
            t.playing = pos->getIsPlaying() && pos->getPpqPosition().hasValue();
            synth.setTransport (t);
        }

    const int numSamples = buffer.getNumSamples();
    float* left = buffer.getWritePointer (0);
    float* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
    float scratch[2048];

    // Sample-accurate MIDI: render up to each event, then apply it.
    int pos = 0;
    auto renderTo = [&] (int end) {
        while (pos < end)
        {
            const int n = std::min (end - pos, 2048);
            synth.render (left + pos, right != nullptr ? right + pos : scratch, n);
            pos += n;
        }
    };

    for (const auto meta : midi)
    {
        renderTo (juce::jlimit (0, numSamples, meta.samplePosition));
        handleMidi (meta.getMessage());
    }
    renderTo (numSamples);

    for (int ch = 2; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);
}

juce::AudioProcessorEditor* ChurchSynthProcessor::createEditor()
{
    return new ChurchSynthEditor (*this);
}

void ChurchSynthProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("stateVersion", 3, nullptr);
    {
        const juce::ScopedLock sl (routeLock);
        state.removeChild (state.getChildWithName ("ROUTES"), nullptr);
        state.appendChild (routesToTree (routesEdit), nullptr);
    }
    {
        const juce::ScopedLock sl (contentLock);
        for (int slot = 0; slot < NumContentSlots; ++slot)
            state.setProperty (kSlotStateKey[slot], userFile[slot], nullptr);
    }
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void ChurchSynthProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            const auto tree = juce::ValueTree::fromXml (*xml);
            {
                const juce::ScopedLock sl (contentLock);
                for (int slot = 0; slot < NumContentSlots; ++slot)
                    userFile[slot] = tree.getProperty (kSlotStateKey[slot]).toString();
            }
            const auto routes = routesFromTree (tree.getChildWithName ("ROUTES"));
            apvts.replaceState (tree);
            setRoutes (routes);
            refreshContent();
            // Loaded effect types keep their saved settings (no defaults applied).
            const juce::ScopedLock fxScope (fxLock);
            readFxTypes (lastFxType);
            fxHost.sync (lastFxType, (float) currentRate, synth.getRenderEpoch()); // resolve now so an offline bounce starts with the right sound
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ChurchSynthProcessor();
}
