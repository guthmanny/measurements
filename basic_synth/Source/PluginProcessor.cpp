#include <algorithm>

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>

namespace
{
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    return {
        std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "instrument", 1 }, "Instrument",
            juce::StringArray { "Basic Synth", "KS Flute" }, 0),
        std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "wave", 1 }, "Wave", 0.0f, 1.0f, 0.0f),
        std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "cutoff", 1 }, "Cutoff", 80.0f, 8000.0f, 1000.0f),
        std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "feedback", 1 }, "Feedback", 0.5f, 2.0f, 1.1f),
        std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "base_note", 1 }, "Base Note", 0.0f, 3.99f, 1.5f),
        std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "gain", 1 }, "Gain", 0.0f, 2.0f, 0.25f),
        std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "output_boost", 1 }, "Output Boost", 0.5f, 3.0f, 1.5f),
        std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "noise", 1 }, "Breath Noise", 0.0f, 0.08f, 0.01f),
        std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "exciter_cutoff", 1 }, "Exciter Cutoff", 300.0f, 12000.0f, 2200.0f),
        std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "loop_cutoff", 1 }, "Loop Cutoff", 300.0f, 8000.0f, 2300.0f),
        std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "reverb_mix", 1 }, "Reverb Mix", 0.0f, 1.0f, 0.35f),
        std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "eg1_attack", 1 }, "EG1 Attack", 0.1f, 5000.0f, 3.0f),
        std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "eg1_release", 1 }, "EG1 Release", 1.0f, 10000.0f, 1200.0f),
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
    for (auto* id : { "instrument", "wave", "cutoff", "feedback", "base_note", "gain", "output_boost",
                      "noise", "exciter_cutoff", "loop_cutoff", "reverb_mix",
                      "eg1_attack", "eg1_release",
                      "eg2_attack", "eg2_release",
                      "eg3_attack", "eg3_release",
                      "lfo1_rate", "lfo2_rate" })
        parameters.addParameterListener (id, this);
}

BasicSynthAudioProcessor::~BasicSynthAudioProcessor()
{
    for (auto* id : { "instrument", "wave", "cutoff", "feedback", "base_note", "gain", "output_boost",
                      "noise", "exciter_cutoff", "loop_cutoff", "reverb_mix",
                      "eg1_attack", "eg1_release",
                      "eg2_attack", "eg2_release",
                      "eg3_attack", "eg3_release",
                      "lfo1_rate", "lfo2_rate" })
        parameters.removeParameterListener (id, this);
}

void BasicSynthAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    lastSampleRate_ = sampleRate;
    lastBlockSize_ = samplesPerBlock;
    pendingInstrument_ = currentInstrument();
    instrumentPreparePending_ = false;
    syncEngineToInstrument();
    processBuffer_.setSize (2, samplesPerBlock);
}

void BasicSynthAudioProcessor::releaseResources()
{
    synthEngine_.release();
}

void BasicSynthAudioProcessor::parameterChanged (const juce::String& parameterID, float newValue)
{
    juce::ignoreUnused (newValue);
    if (parameterID == "instrument")
    {
        pendingInstrument_ = currentInstrument();
        instrumentPreparePending_ = true;
        return;
    }
    updateSynthParameters();
}

void BasicSynthAudioProcessor::syncEngineToInstrument()
{
    if (lastSampleRate_ <= 0.0 || lastBlockSize_ <= 0)
        return;

    synthEngine_.prepare ((float) lastSampleRate_,
                          (std::uint32_t) lastBlockSize_,
                          pendingInstrument_);
    keyboardState.reset();
    updateSynthParameters();
}

SynthInstrument BasicSynthAudioProcessor::currentInstrument() const
{
    const int idx = (int) std::lround (*parameters.getRawParameterValue ("instrument"));
    return idx == 1 ? SynthInstrument::KsFlute : SynthInstrument::BasicSynth;
}

void BasicSynthAudioProcessor::updateSynthParameters()
{
    if (! synthEngine_.isReady())
        return;

    const auto id = synthEngine_.synthId();

    if (synthEngine_.instrument() == SynthInstrument::KsFlute)
    {
        synthEngine_.setParamDomain (id, "gain", *parameters.getRawParameterValue ("gain"));
        synthEngine_.setParamDomain (id, "output_boost", *parameters.getRawParameterValue ("output_boost"));
        synthEngine_.setParamDomain (id, "feedback", *parameters.getRawParameterValue ("feedback"));
        synthEngine_.setParamDomain (id, "base_note", *parameters.getRawParameterValue ("base_note"));
        synthEngine_.setParamDomain (id, "noise", *parameters.getRawParameterValue ("noise"));
        synthEngine_.setParamDomain (id, "exciter_cutoff", *parameters.getRawParameterValue ("exciter_cutoff"));
        synthEngine_.setParamDomain (id, "loop_cutoff", *parameters.getRawParameterValue ("loop_cutoff"));
        synthEngine_.setParamDomain (id, "reverb_mix", *parameters.getRawParameterValue ("reverb_mix"));
        synthEngine_.setParamDomain (id, "eg1_attack", *parameters.getRawParameterValue ("eg1_attack"));
        synthEngine_.setParamDomain (id, "eg1_release", *parameters.getRawParameterValue ("eg1_release"));
        synthEngine_.setParamDomain (id, "lfo1_rate", *parameters.getRawParameterValue ("lfo1_rate"));
        synthEngine_.setParamDomain (id, "lfo2_rate", *parameters.getRawParameterValue ("lfo2_rate"));
        return;
    }

    synthEngine_.setParamDomain (id, "gain", *parameters.getRawParameterValue ("gain"));
    synthEngine_.setParamDomain (id, "eg1_attack", *parameters.getRawParameterValue ("eg1_attack"));
    synthEngine_.setParamDomain (id, "eg1_release", *parameters.getRawParameterValue ("eg1_release"));
    synthEngine_.setParamDomain (id, "lfo1_rate", *parameters.getRawParameterValue ("lfo1_rate"));
    synthEngine_.setParamDomain (id, "lfo2_rate", *parameters.getRawParameterValue ("lfo2_rate"));
    synthEngine_.setParamDomain (id, "wave", *parameters.getRawParameterValue ("wave"));
    synthEngine_.setParamDomain (id, "cutoff", *parameters.getRawParameterValue ("cutoff"));
    synthEngine_.setParamDomain (id, "eg2_attack", *parameters.getRawParameterValue ("eg2_attack"));
    synthEngine_.setParamDomain (id, "eg2_release", *parameters.getRawParameterValue ("eg2_release"));
    synthEngine_.setParamDomain (id, "eg3_attack", *parameters.getRawParameterValue ("eg3_attack"));
    synthEngine_.setParamDomain (id, "eg3_release", *parameters.getRawParameterValue ("eg3_release"));
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

    if (instrumentPreparePending_.exchange (false))
        syncEngineToInstrument();

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

    pendingInstrument_ = currentInstrument();
    instrumentPreparePending_ = true;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BasicSynthAudioProcessor();
}
