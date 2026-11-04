#include "PluginProcessor.h"
#include "PluginEditor.h"

ChurchSynthProcessor::ChurchSynthProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "ChurchSynth", cs::createParameterLayout())
{
    for (const auto& d : cs::parameterDefs())
        rawParams.push_back (apvts.getRawParameterValue (d.id));
}

bool ChurchSynthProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void ChurchSynthProcessor::prepareToPlay (double sampleRate, int)
{
    synth.setSampleRate ((float) sampleRate);
    pullParameters();
}

void ChurchSynthProcessor::pullParameters() noexcept
{
    const auto& defs = cs::parameterDefs();
    for (size_t i = 0; i < defs.size(); ++i)
        defs[i].apply (synth.params, rawParams[i]->load (std::memory_order_relaxed));
}

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
    else if (m.isAllNotesOff() || m.isAllSoundOff())
        synth.allNotesOff();
}

void ChurchSynthProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    pullParameters();

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
    state.setProperty ("stateVersion", 1, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void ChurchSynthProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ChurchSynthProcessor();
}
