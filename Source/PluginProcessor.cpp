#include "PluginProcessor.h"
#include "PluginEditor.h"

ChazVelvetProcessor::ChazVelvetProcessor()
    : AudioProcessor (BusesProperties()
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", chaz::createParameterLayout())
{
    currentProgram = 0;
}

void ChazVelvetProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);
    engine.setParams (chaz::readVelvetParams (apvts));
}

bool ChazVelvetProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono();
}

void ChazVelvetProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    engine.setParams (chaz::readVelvetParams (apvts));

    for (const auto metadata : midi)
    {
        const auto msg = metadata.getMessage();
        if (msg.isNoteOn())
            engine.noteOn (msg.getNoteNumber(), msg.getFloatVelocity());
        else if (msg.isNoteOff())
            engine.noteOff (msg.getNoteNumber());
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            engine.allNotesOff();
    }

    const int n = buffer.getNumSamples();
    float* L = buffer.getWritePointer (0);
    float* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : L;
    engine.process (L, R, n);
}

juce::AudioProcessorEditor* ChazVelvetProcessor::createEditor()
{
    return new ChazVelvetEditor (*this);
}

int ChazVelvetProcessor::getProgramCount() const
{
    return (int) chaz::getFactoryPresets().size();
}

int ChazVelvetProcessor::getNumPrograms()
{
    return juce::jmax (1, getProgramCount());
}

const juce::String ChazVelvetProcessor::getProgramName (int index)
{
    const auto& ps = chaz::getFactoryPresets();
    if (index >= 0 && index < (int) ps.size())
        return ps[(size_t) index].name;
    return {};
}

void ChazVelvetProcessor::applyPreset (int index)
{
    const auto& ps = chaz::getFactoryPresets();
    if (index < 0 || index >= (int) ps.size()) return;
    currentProgram = index;
    for (const auto& kv : ps[(size_t) index].values)
        if (auto* p = apvts.getParameter (kv.first))
            p->setValueNotifyingHost (p->convertTo0to1 (kv.second));
    engine.setParams (chaz::readVelvetParams (apvts));
    updateHostDisplay (ChangeDetails().withProgramChanged (true));
}

void ChazVelvetProcessor::setCurrentProgram (int index)
{
    applyPreset (index);
}

void ChazVelvetProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    state.setProperty ("version", JucePlugin_VersionString, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void ChazVelvetProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            currentProgram = juce::jlimit (0, getNumPrograms() - 1, (int) tree.getProperty ("program", 0));
            apvts.replaceState (tree);
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ChazVelvetProcessor();
}
