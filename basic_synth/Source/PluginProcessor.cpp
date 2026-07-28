#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    return {
        std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "wave", 1 }, "Wave", 0.0f, 1.0f, 0.0f),
        std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "cutoff", 1 }, "Cutoff", 80.0f, 8000.0f, 1000.0f),
        std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "gain", 1 }, "Gain", 0.0f, 1.0f, 0.25f),
        std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "eg1_attack", 1 }, "EG1 Attack", 0.1f, 5000.0f, 10.0f),
        std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "eg1_release", 1 }, "EG1 Release", 1.0f, 10000.0f, 200.0f),
        std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "eg2_attack", 1 }, "EG2 Attack", 0.1f, 5000.0f, 3.0f),
        std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "eg2_release", 1 }, "EG2 Release", 1.0f, 10000.0f, 180.0f),
        std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "eg3_attack", 1 }, "EG3 Attack", 0.1f, 5000.0f, 5.0f),
        std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "eg3_release", 1 }, "EG3 Release", 1.0f, 10000.0f, 120.0f),
        std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "lfo1_rate", 1 }, "LFO1 Rate", 0.01f, 20.0f, 5.0f),
        std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "lfo2_rate", 1 }, "LFO2 Rate", 0.01f, 20.0f, 0.25f),
    };
}
} // namespace

BasicSynthAudioProcessor::BasicSynthAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "PARAMS", createParameterLayout())
{
    for (auto* id : { "wave", "cutoff", "gain",
                      "eg1_attack", "eg1_release",
                      "eg2_attack", "eg2_release",
                      "eg3_attack", "eg3_release",
                      "lfo1_rate", "lfo2_rate" })
        parameters.addParameterListener (id, this);
}

BasicSynthAudioProcessor::~BasicSynthAudioProcessor()
{
    for (auto* id : { "wave", "cutoff", "gain",
                      "eg1_attack", "eg1_release",
                      "eg2_attack", "eg2_release",
                      "eg3_attack", "eg3_release",
                      "lfo1_rate", "lfo2_rate" })
        parameters.removeParameterListener (id, this);
}

void BasicSynthAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synthEngine_.prepare ((float) sampleRate, (std::uint32_t) samplesPerBlock);
    processBuffer_.setSize (2, samplesPerBlock);
    updateSynthParameters();
}

void BasicSynthAudioProcessor::releaseResources()
{
    synthEngine_.release();
}

void BasicSynthAudioProcessor::parameterChanged (const juce::String& parameterID, float newValue)
{
    juce::ignoreUnused (parameterID, newValue);
    updateSynthParameters();
}

void BasicSynthAudioProcessor::updateSynthParameters()
{
    if (! synthEngine_.isReady())
        return;

    const auto id = synthEngine_.synthId();
    synthEngine_.setParamDomain (id, "wave", *parameters.getRawParameterValue ("wave"));
    synthEngine_.setParamDomain (id, "cutoff", *parameters.getRawParameterValue ("cutoff"));
    synthEngine_.setParamDomain (id, "gain", *parameters.getRawParameterValue ("gain"));
    synthEngine_.setParamDomain (id, "eg1_attack", *parameters.getRawParameterValue ("eg1_attack"));
    synthEngine_.setParamDomain (id, "eg1_release", *parameters.getRawParameterValue ("eg1_release"));
    synthEngine_.setParamDomain (id, "eg2_attack", *parameters.getRawParameterValue ("eg2_attack"));
    synthEngine_.setParamDomain (id, "eg2_release", *parameters.getRawParameterValue ("eg2_release"));
    synthEngine_.setParamDomain (id, "eg3_attack", *parameters.getRawParameterValue ("eg3_attack"));
    synthEngine_.setParamDomain (id, "eg3_release", *parameters.getRawParameterValue ("eg3_release"));
    synthEngine_.setParamDomain (id, "lfo1_rate", *parameters.getRawParameterValue ("lfo1_rate"));
    synthEngine_.setParamDomain (id, "lfo2_rate", *parameters.getRawParameterValue ("lfo2_rate"));
}

void BasicSynthAudioProcessor::handleIncomingMidi (const juce::MidiBuffer& midiMessages)
{
    for (const auto metadata : midiMessages)
    {
        const auto msg = metadata.getMessage();
        if (msg.isNoteOn())
            synthEngine_.sendNoteOn (msg.getNoteNumber(), msg.getFloatVelocity());
        else if (msg.isNoteOff())
            synthEngine_.sendNoteOff (msg.getNoteNumber(), msg.getFloatVelocity());
    }
}

void BasicSynthAudioProcessor::processBlock (juce::AudioSampleBuffer& buffer,
                                             juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    if (numSamples <= 0)
        return;

    keyboardState.processNextMidiBuffer (midiMessages, 0, numSamples, true);
    handleIncomingMidi (midiMessages);
    updateSynthParameters();

    processBuffer_.setSize (2, numSamples, false, false, true);
    processBuffer_.clear();

    if (synthEngine_.isReady())
    {
        float* outs[2] = { processBuffer_.getWritePointer (0),
                           processBuffer_.getWritePointer (1) };
        synthEngine_.process (outs, (std::uint32_t) numSamples);
    }

    const int outCh = juce::jmin (buffer.getNumChannels(), 2);
    for (int ch = 0; ch < outCh; ++ch)
        buffer.copyFrom (ch, 0, processBuffer_, ch, 0, numSamples);

    for (int ch = outCh; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);
}

juce::AudioProcessorEditor* BasicSynthAudioProcessor::createEditor()
{
    return new BasicSynthAudioProcessorEditor (*this);
}

void BasicSynthAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void BasicSynthAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BasicSynthAudioProcessor();
}
