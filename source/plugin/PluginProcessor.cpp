#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PresetManager.h"

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
    {
        rawParams.push_back (apvts.getRawParameterValue (d.id));
        paramPtrs.push_back (apvts.getParameter (d.id));
    }
    for (int k = 0; k < cs::kNumFuncs; ++k)
    {
        funcShapeParam[k] = paramIndex ("func" + juce::String (k + 1) + "_shape");
        funcEdit[k] = funcShared[k] = funcAudio[k] = cs::functionPreset (0);
    }
    for (auto& c : ccMap)
        c.store (-1);
    for (auto* prm : paramPtrs)
        prm->addListener (this);
    formats.registerBasicFormats();
    presetManager = std::make_unique<PresetManager> (*this);

    refreshContent(); // defaults ready before the first block
    loader = std::make_unique<Loader> (*this);
    loader->startThread (juce::Thread::Priority::low);

    readFxTypes (lastFxType);
    fxHost.rebuild (lastFxType, (float) currentRate);
    startTimerHz (20);
    resetHistory ("Init");
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
    cancelPendingUpdate();
    for (auto* prm : paramPtrs)
        prm->removeListener (this);
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
    for (int k = 0; k < cs::kNumFuncs; ++k)
        if ((int) std::lround (rawParams[(size_t) funcShapeParam[k]]->load (std::memory_order_relaxed)) >= cs::kNumFunctionPresets)
            synth.params.func[(size_t) k].shape = &funcAudio[k];
}

// --- Custom function shapes ----------------------------------------------------
cs::FunctionShape ChurchSynthProcessor::getCustomShape (int f) const
{
    const juce::ScopedLock sl (funcLock);
    return funcEdit[juce::jlimit (0, cs::kNumFuncs - 1, f)];
}

void ChurchSynthProcessor::setCustomShape (int f, const cs::FunctionShape& shape)
{
    const juce::ScopedLock sl (funcLock);
    funcEdit[juce::jlimit (0, cs::kNumFuncs - 1, f)] = shape;
    funcSeq.fetch_add (1, std::memory_order_acq_rel);
    std::atomic_thread_fence (std::memory_order_release);
    for (int k = 0; k < cs::kNumFuncs; ++k)
        funcShared[k] = funcEdit[k];
    std::atomic_thread_fence (std::memory_order_release);
    funcSeq.fetch_add (1, std::memory_order_acq_rel);
    ++editCounter;
}

void ChurchSynthProcessor::pullFuncs() noexcept
{
    const uint32_t s1 = funcSeq.load (std::memory_order_acquire);
    if (s1 == funcSeqSeen || (s1 & 1u) != 0)
        return;
    cs::FunctionShape tmp[cs::kNumFuncs];
    for (int k = 0; k < cs::kNumFuncs; ++k)
        tmp[k] = funcShared[k];
    std::atomic_thread_fence (std::memory_order_acquire);
    if (funcSeq.load (std::memory_order_acquire) != s1)
        return;
    for (int k = 0; k < cs::kNumFuncs; ++k)
        funcAudio[k] = tmp[k];
    funcSeqSeen = s1;
}

// --- Macro names -------------------------------------------------------------------
juce::String ChurchSynthProcessor::getMacroName (int m) const
{
    return macroNames[juce::jlimit (0, 3, m)];
}

void ChurchSynthProcessor::setMacroName (int m, const juce::String& name)
{
    macroNames[juce::jlimit (0, 3, m)] = name.isEmpty() ? "Macro " + juce::String (m + 1) : name.substring (0, 24);
    ++editCounter;
}

// --- MIDI learn ------------------------------------------------------------------------
int ChurchSynthProcessor::paramIndex (const juce::String& id) const
{
    const auto& defs = cs::parameterDefs();
    for (size_t i = 0; i < defs.size(); ++i)
        if (defs[i].id == id)
            return (int) i;
    return -1;
}

void ChurchSynthProcessor::startMidiLearn (const juce::String& paramId)
{
    learnIndex.store (paramIndex (paramId));
}

juce::String ChurchSynthProcessor::learningParam() const
{
    const int i = learnIndex.load();
    return i >= 0 ? cs::parameterDefs()[(size_t) i].id : juce::String();
}

int ChurchSynthProcessor::ccForParam (const juce::String& paramId) const
{
    const int idx = paramIndex (paramId);
    for (int cc = 0; cc < 128; ++cc)
        if (idx >= 0 && ccMap[cc].load() == idx)
            return cc;
    return -1;
}

void ChurchSynthProcessor::clearMidiMapping (const juce::String& paramId)
{
    const int idx = paramIndex (paramId);
    for (auto& c : ccMap)
        if (c.load() == idx)
            c.store (-1);
}

// --- Undo history ------------------------------------------------------------------------
void ChurchSynthProcessor::resetHistory (const juce::String& label)
{
    history.clear();
    history.push_back ({ label, createStateTree(), juce::Time::currentTimeMillis() });
    historyIndex = 0;
    ++editCounter;
}

void ChurchSynthProcessor::checkpoint (const juce::String& label)
{
    if (restoring.load())
        return;
    const auto now = juce::Time::currentTimeMillis();
    // Drop the redo branch, then merge quick repeats of the same edit (e.g. drawing a lane).
    history.resize ((size_t) (historyIndex + 1));
    if (historyIndex > 0 && history.back().label == label && now - history.back().time < 1500)
        history.back() = { label, createStateTree(), now };
    else
        history.push_back ({ label, createStateTree(), now });
    if (history.size() > 100)
        history.erase (history.begin());
    historyIndex = (int) history.size() - 1;
    modified = true;
    ++editCounter;
}

void ChurchSynthProcessor::jumpToHistory (int index)
{
    if (index < 0 || index >= (int) history.size() || index == historyIndex)
        return;
    historyIndex = index;
    restoreStateTree (history[(size_t) index].state, LoadKind::Undo);
    ++editCounter;
}

juce::StringArray ChurchSynthProcessor::historyLabels() const
{
    juce::StringArray a;
    for (const auto& h : history)
        a.add (h.label);
    return a;
}

void ChurchSynthProcessor::parameterGestureChanged (int index, bool starting)
{
    // Only edits made in our editor (message thread) become undo steps.
    if (starting || restoring.load() || ! juce::MessageManager::existsAndIsCurrentThread())
        return;
    if (index >= 0 && index < (int) paramPtrs.size())
        pendingGestureLabel = paramPtrs[(size_t) index]->getName (60);
    triggerAsyncUpdate();
}

void ChurchSynthProcessor::handleAsyncUpdate()
{
    if (pendingGestureLabel.isNotEmpty())
        checkpoint (pendingGestureLabel);
    pendingGestureLabel.clear();
}

void ChurchSynthProcessor::setPresetInfo (const juce::String& name, const juce::String& category, bool isModified)
{
    currentPreset = name;
    currentCategory = category;
    modified = isModified;
    ++editCounter;
}

// --- FX slot copy / swap ------------------------------------------------------------------
void ChurchSynthProcessor::copyFxSlot (int from, int to)
{
    if (from == to || from < 0 || to < 0 || from >= cs::fx::kNumFxSlots || to >= cs::fx::kNumFxSlots)
        return;
    const juce::ScopedLock fxScope (fxLock);
    auto copyParam = [this, from, to] (const char* suffix) {
        auto* src = apvts.getParameter (fxParamId (from, suffix));
        auto* dst = apvts.getParameter (fxParamId (to, suffix));
        dst->setValueNotifyingHost (src->getValue());
    };
    copyParam ("type");
    copyParam ("on");
    copyParam ("sync");
    for (int k = 0; k < cs::fx::kFxParams; ++k)
        copyParam (("p" + juce::String (k + 1)).toRawUTF8());
    readFxTypes (lastFxType); // the new type keeps the copied settings (no defaults)
    fxHost.sync (lastFxType, (float) currentRate, synth.getRenderEpoch());
}

void ChurchSynthProcessor::swapFxSlots (int a, int b)
{
    if (a == b || a < 0 || b < 0 || a >= cs::fx::kNumFxSlots || b >= cs::fx::kNumFxSlots)
        return;
    const juce::ScopedLock fxScope (fxLock);
    auto swapParam = [this, a, b] (const char* suffix) {
        auto* pa = apvts.getParameter (fxParamId (a, suffix));
        auto* pb = apvts.getParameter (fxParamId (b, suffix));
        const float va = pa->getValue(), vb = pb->getValue();
        pa->setValueNotifyingHost (vb);
        pb->setValueNotifyingHost (va);
    };
    swapParam ("type");
    swapParam ("on");
    swapParam ("sync");
    for (int k = 0; k < cs::fx::kFxParams; ++k)
        swapParam (("p" + juce::String (k + 1)).toRawUTF8());
    readFxTypes (lastFxType);
    fxHost.sync (lastFxType, (float) currentRate, synth.getRenderEpoch());
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

// --- Seq pattern --------------------------------------------------------------
cs::SeqPattern ChurchSynthProcessor::getPattern() const
{
    const juce::ScopedLock sl (patternLock);
    return patternEdit;
}

void ChurchSynthProcessor::setPattern (const cs::SeqPattern& p)
{
    const juce::ScopedLock sl (patternLock);
    patternEdit = p;
    patternSeq.fetch_add (1, std::memory_order_acq_rel); // odd: writing
    std::atomic_thread_fence (std::memory_order_release);
    patternShared = p;
    std::atomic_thread_fence (std::memory_order_release);
    patternSeq.fetch_add (1, std::memory_order_acq_rel); // even: stable
}

void ChurchSynthProcessor::pullPattern() noexcept
{
    const uint32_t s1 = patternSeq.load (std::memory_order_acquire);
    if (s1 == patternSeqSeen || (s1 & 1u) != 0)
        return;
    patternAudio = patternShared;
    std::atomic_thread_fence (std::memory_order_acquire);
    if (patternSeq.load (std::memory_order_acquire) != s1)
        return;
    synth.setSeqPattern (patternAudio);
    patternSeqSeen = s1;
}

cs::SeqParams ChurchSynthProcessor::currentSeqParams() const
{
    cs::SynthParams sp;
    for (size_t i = 0; i < rawParams.size(); ++i)
    {
        const auto& def = cs::parameterDefs()[i];
        if (def.id.startsWith ("seq_"))
            def.apply (sp, rawParams[i]->load());
    }
    return sp.seq;
}

// Pattern state: one STEP per column, one TRACK per track (attributes, so new
// fields can be added later with defaults).
juce::ValueTree ChurchSynthProcessor::patternToTree (const cs::SeqPattern& p)
{
    juce::ValueTree t ("SEQ");
    t.setProperty ("seed", (int) p.seed, nullptr);
    juce::String w;
    for (float x : p.customWeights)
        w << juce::String (x, 3) << " ";
    t.setProperty ("weights", w.trim(), nullptr);
    for (const auto& st : p.steps)
    {
        juce::ValueTree e ("STEP");
        e.setProperty ("on", st.on, nullptr);
        e.setProperty ("prob", st.prob, nullptr);
        e.setProperty ("pitch", st.pitch, nullptr);
        e.setProperty ("vel", st.velocity, nullptr);
        e.setProperty ("oct", st.octave, nullptr);
        e.setProperty ("gate", st.gate, nullptr);
        e.setProperty ("slide", st.slide, nullptr);
        e.setProperty ("lock", (int) st.lockSeed, nullptr);
        t.appendChild (e, nullptr);
    }
    for (int i = 0; i < cs::NumSeqTracks; ++i)
    {
        const auto& tr = p.tracks[(size_t) i];
        juce::ValueTree e ("TRACK");
        e.setProperty ("name", cs::kSeqTrackNames[i], nullptr);
        e.setProperty ("on", tr.enabled, nullptr);
        e.setProperty ("len", tr.length, nullptr);
        e.setProperty ("div", tr.divider, nullptr);
        e.setProperty ("rnd", tr.random, nullptr);
        t.appendChild (e, nullptr);
    }
    return t;
}

cs::SeqPattern ChurchSynthProcessor::patternFromTree (const juce::ValueTree& t)
{
    cs::SeqPattern p;
    if (! t.isValid())
        return p;
    p.seed = (uint32_t) (int) t.getProperty ("seed", 1);
    if (p.seed == 0)
        p.seed = 1;
    juce::StringArray w;
    w.addTokens (t.getProperty ("weights").toString(), " ", {});
    for (int i = 0; i < 12 && i < w.size(); ++i)
        p.customWeights[(size_t) i] = juce::jlimit (0.0f, 10.0f, w[i].getFloatValue());
    int si = 0, ti = 0;
    for (const auto& e : t)
    {
        if (e.hasType ("STEP") && si < cs::kSeqSteps)
        {
            auto& st = p.steps[(size_t) si++];
            st.on = e.getProperty ("on", true);
            st.prob = juce::jlimit (0.0f, 1.0f, (float) e.getProperty ("prob", 1.0f));
            st.pitch = juce::jlimit (0, 12, (int) e.getProperty ("pitch", 0));
            st.velocity = juce::jlimit (0.0f, 1.0f, (float) e.getProperty ("vel", 0.8f));
            st.octave = juce::jlimit (-2, 2, (int) e.getProperty ("oct", 0));
            st.gate = juce::jlimit (0.05f, 4.0f, (float) e.getProperty ("gate", 0.5f));
            st.slide = juce::jlimit (0.0f, 1.0f, (float) e.getProperty ("slide", 0.0f));
            st.lockSeed = (uint32_t) (int) e.getProperty ("lock", 0);
        }
        else if (e.hasType ("TRACK") && ti < cs::NumSeqTracks)
        {
            auto& tr = p.tracks[(size_t) ti++];
            tr.enabled = e.getProperty ("on", true);
            tr.length = juce::jlimit (1, cs::kSeqSteps, (int) e.getProperty ("len", cs::kSeqSteps));
            tr.divider = juce::jlimit (1, 16, (int) e.getProperty ("div", 1));
            tr.random = juce::jlimit (0.0f, 1.0f, (float) e.getProperty ("rnd", 0.0f));
        }
    }
    return p;
}

// ---------------------------------------------------------------------------
void ChurchSynthProcessor::handleMidi (const juce::MidiMessage& m) noexcept
{
    if (m.isController() && m.getControllerNumber() != 64 && m.getControllerNumber() < 120)
    {
        const int cc = m.getControllerNumber();
        const int learn = learnIndex.load (std::memory_order_relaxed);
        if (learn >= 0)
        {
            for (auto& c : ccMap) // one CC per parameter
                if (c.load (std::memory_order_relaxed) == learn)
                    c.store (-1, std::memory_order_relaxed);
            ccMap[cc].store (learn, std::memory_order_relaxed);
            learnIndex.store (-1, std::memory_order_relaxed);
        }
        const int target = ccMap[cc].load (std::memory_order_relaxed);
        if (target >= 0 && target < (int) paramPtrs.size())
            paramPtrs[(size_t) target]->setValueNotifyingHost ((float) m.getControllerValue() / 127.0f);
    }

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
    const auto t0 = juce::Time::getHighResolutionTicks();
    pullFuncs();
    pullParameters();
    pullRoutes();
    pullPattern();

    cs::Transport t;
    t.hostTempo = false; // no tempo from the host (standalone): the synth uses its own
    t.playing = false;
    if (auto* ph = getPlayHead())
        if (const auto pos = ph->getPosition())
        {
            t.hostTempo = pos->getBpm().hasValue();
            t.bpm = pos->getBpm().orFallback (120.0);
            t.ppq = pos->getPpqPosition().orFallback (0.0);
            t.playing = pos->getIsPlaying() && pos->getPpqPosition().hasValue();
        }
    synth.setTransport (t);

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

    {
        float pk = 0.0f;
        for (int ch = 0; ch < juce::jmin (2, buffer.getNumChannels()); ++ch)
            pk = juce::jmax (pk, buffer.getMagnitude (ch, 0, numSamples));
        if (pk > outPeak.load (std::memory_order_relaxed))
            outPeak.store (pk, std::memory_order_relaxed);
    }

    // CPU: time spent / time available, smoothed.
    const double spent = juce::Time::highResolutionTicksToSeconds (juce::Time::getHighResolutionTicks() - t0);
    const double avail = numSamples / juce::jmax (1.0, currentRate);
    const float load = (float) (spent / juce::jmax (1.0e-6, avail));
    cpuLoad.store (cpuLoad.load (std::memory_order_relaxed) * 0.9f + load * 0.1f, std::memory_order_relaxed);
}

juce::AudioProcessorEditor* ChurchSynthProcessor::createEditor()
{
    return new ChurchSynthEditor (*this);
}

juce::ValueTree ChurchSynthProcessor::createStateTree()
{
    auto state = apvts.copyState();
    state.setProperty ("stateVersion", 4, nullptr);
    {
        const juce::ScopedLock sl (routeLock);
        state.removeChild (state.getChildWithName ("ROUTES"), nullptr);
        state.appendChild (routesToTree (routesEdit), nullptr);
    }
    {
        // Save the seed that is playing now (Auto Regen may have moved it on).
        auto pat = getPattern();
        pat.seed = synth.getSeq().uiSeed.load (std::memory_order_relaxed);
        state.removeChild (state.getChildWithName ("SEQ"), nullptr);
        state.appendChild (patternToTree (pat), nullptr);
    }
    {
        const juce::ScopedLock sl (funcLock);
        state.removeChild (state.getChildWithName ("FUNCS"), nullptr);
        juce::ValueTree funcs ("FUNCS");
        for (int k = 0; k < cs::kNumFuncs; ++k)
        {
            const auto& f = funcEdit[k];
            juce::ValueTree e ("FUNC");
            juce::String pts;
            for (int i = 0; i < f.count; ++i)
                pts << juce::String (f.x[i], 5) << "," << juce::String (f.y[i], 5) << "," << juce::String (f.curve[i], 4) << " ";
            e.setProperty ("points", pts.trim(), nullptr);
            e.setProperty ("sustain", f.sustain, nullptr);
            e.setProperty ("loop", f.loopStart, nullptr);
            funcs.appendChild (e, nullptr);
        }
        state.appendChild (funcs, nullptr);
    }
    for (int m = 0; m < 4; ++m)
        state.setProperty ("macroName" + juce::String (m + 1), macroNames[m], nullptr);
    {
        state.removeChild (state.getChildWithName ("MIDIMAP"), nullptr);
        juce::ValueTree mm ("MIDIMAP");
        for (int cc = 0; cc < 128; ++cc)
            if (const int idx = ccMap[cc].load(); idx >= 0)
            {
                juce::ValueTree e ("CC");
                e.setProperty ("cc", cc, nullptr);
                e.setProperty ("param", cs::parameterDefs()[(size_t) idx].id, nullptr);
                mm.appendChild (e, nullptr);
            }
        state.appendChild (mm, nullptr);
    }
    {
        const juce::ScopedLock sl (contentLock);
        for (int slot = 0; slot < NumContentSlots; ++slot)
            state.setProperty (kSlotStateKey[slot], userFile[slot], nullptr);
    }
    state.setProperty ("presetName", currentPreset, nullptr);
    state.setProperty ("presetCategory", currentCategory, nullptr);
    state.setProperty ("presetModified", modified, nullptr);
    return state;
}

void ChurchSynthProcessor::restoreStateTree (const juce::ValueTree& source, LoadKind kind)
{
    if (! source.isValid())
        return;
    struct RestoreGuard
    {
        std::atomic<bool>& flag;
        const bool previous;
        explicit RestoreGuard (std::atomic<bool>& f) : flag (f), previous (f.exchange (true)) {}
        ~RestoreGuard() { flag.store (previous); }
    } guard (restoring);
    auto tree = source.createCopy();
    tree.removeProperty ("presetFormat", nullptr);

    // Seq Lock: a preset load keeps the current pattern and sequencer settings.
    const bool keepSeq = kind == LoadKind::Preset && apvts.getRawParameterValue ("seq_lock")->load() >= 0.5f;
    if (keepSeq)
    {
        for (const auto& d : cs::parameterDefs())
            if (d.id.startsWith ("seq_"))
            {
                auto child = tree.getChildWithProperty ("id", d.id);
                if (! child.isValid())
                {
                    child = juce::ValueTree ("PARAM");
                    child.setProperty ("id", d.id, nullptr);
                    tree.appendChild (child, nullptr);
                }
                child.setProperty ("value", apvts.getRawParameterValue (d.id)->load(), nullptr);
            }
        tree.removeChild (tree.getChildWithName ("SEQ"), nullptr);
        tree.appendChild (patternToTree (getPattern()), nullptr);
    }

    {
        const juce::ScopedLock sl (contentLock);
        for (int slot = 0; slot < NumContentSlots; ++slot)
            userFile[slot] = tree.getProperty (kSlotStateKey[slot]).toString();
    }
    const auto routes = routesFromTree (tree.getChildWithName ("ROUTES"));
    const auto pattern = patternFromTree (tree.getChildWithName ("SEQ"));
    // Functions: drawn shapes (fall back to the first preset shape).
    {
        const auto funcs = tree.getChildWithName ("FUNCS");
        const juce::ScopedLock sl (funcLock);
        for (int k = 0; k < cs::kNumFuncs; ++k)
        {
            cs::FunctionShape f = cs::functionPreset (0);
            const auto e = funcs.isValid() && k < funcs.getNumChildren() ? funcs.getChild (k) : juce::ValueTree();
            if (e.isValid())
            {
                juce::StringArray pts;
                pts.addTokens (e["points"].toString(), " ", {});
                int n = 0;
                for (const auto& ptxt : pts)
                {
                    juce::StringArray v;
                    v.addTokens (ptxt, ",", {});
                    if (v.size() < 2 || n >= cs::FunctionShape::kMaxPoints)
                        continue;
                    f.x[n] = juce::jlimit (0.0f, 1.0f, v[0].getFloatValue());
                    f.y[n] = juce::jlimit (-1.0f, 1.0f, v[1].getFloatValue());
                    f.curve[n] = v.size() > 2 ? juce::jlimit (-1.0f, 1.0f, v[2].getFloatValue()) : 0.0f;
                    ++n;
                }
                if (n >= 2)
                {
                    f.count = n;
                    f.sustain = juce::jlimit (-1, n - 1, (int) e.getProperty ("sustain", -1));
                    f.loopStart = juce::jlimit (0, n - 1, (int) e.getProperty ("loop", 0));
                }
            }
            funcEdit[k] = f;
        }
    }
    setCustomShape (0, getCustomShape (0)); // publish all three
    for (int m = 0; m < 4; ++m)
        macroNames[m] = tree.getProperty ("macroName" + juce::String (m + 1), "Macro " + juce::String (m + 1)).toString();

    if (kind != LoadKind::Preset)
    {
        for (auto& c : ccMap)
            c.store (-1);
        for (const auto& e : tree.getChildWithName ("MIDIMAP"))
        {
            const int cc = e["cc"];
            const int idx = paramIndex (e["param"].toString());
            if (cc >= 0 && cc < 128 && idx >= 0)
                ccMap[cc].store (idx);
        }
    }
    else
    {
        // Presets never carry MIDI mappings: keep the current ones.
        tree.removeChild (tree.getChildWithName ("MIDIMAP"), nullptr);
        tree.appendChild (createStateTree().getChildWithName ("MIDIMAP").createCopy(), nullptr);
    }

    if (kind != LoadKind::Undo || tree.hasProperty ("presetName"))
    {
        currentPreset = tree.getProperty ("presetName", currentPreset).toString();
        currentCategory = tree.getProperty ("presetCategory", currentCategory).toString();
        modified = kind == LoadKind::Preset ? false : (bool) tree.getProperty ("presetModified", false);
    }

    apvts.replaceState (tree);
    setRoutes (routes);
    setPattern (pattern);
    refreshContent();
    // Loaded effect types keep their saved settings (no defaults applied).
    {
        const juce::ScopedLock fxScope (fxLock);
        readFxTypes (lastFxType);
        fxHost.sync (lastFxType, (float) currentRate, synth.getRenderEpoch()); // resolve now so an offline bounce starts with the right sound
    }
    ++editCounter;
}

void ChurchSynthProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = createStateTree().createXml())
        copyXmlToBinary (*xml, destData);
}

void ChurchSynthProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            restoreStateTree (juce::ValueTree::fromXml (*xml), LoadKind::HostState);
            resetHistory ("Load");
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ChurchSynthProcessor();
}
