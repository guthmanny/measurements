#include "PluginProcessor.h"

#include "../JuceLibraryCode/JuceHeader.h"
#include "AefAudioUtils.h"
#include "MiddleProcessorEffectEngine.h"

Ds1AudioProcessor::Ds1AudioProcessor()
    : paramDs1Gain (parameters, "Distortion", "", 0.0f, 1.0f, 0.5f),
      paramDs1Tone (parameters, "Tone", "", 0.0f, 1.0f, 0.5f),
      paramDs1Level (parameters, "Pedal Level", "", 0.0f, 1.0f, 0.5f)
{
    aef::setParameterDefault (parameters.valueTreeState, paramInputGain.paramID, -12.0f);
    aef::setParameterDefault (parameters.valueTreeState, paramGateThreshold.paramID, -80.0f);
}

std::unique_ptr<KbussEffectEngine> Ds1AudioProcessor::createEffectEngine()
{
    return std::make_unique<MiddleProcessorEffectEngine> (
        "com.kbuss.nudsp.white_box.ds1", "DS-1", "ds1");
}

void Ds1AudioProcessor::updateCustomEffectParameters()
{
    const auto ds1Id = getKbussEngine().middleProcessorId();
    if (ds1Id == kbuss::kInvalidObjectId)
        return;

    getKbussEngine().setParamNormalized (ds1Id, "gain",
        readParameterValue (paramDs1Gain.paramID, paramDs1Gain.defaultValue));
    getKbussEngine().setParamNormalized (ds1Id, "tone",
        readParameterValue (paramDs1Tone.paramID, paramDs1Tone.defaultValue));
    getKbussEngine().setParamNormalized (ds1Id, "level",
        readParameterValue (paramDs1Level.paramID, paramDs1Level.defaultValue));
}

void Ds1AudioProcessor::processBlock (juce::AudioSampleBuffer& buffer, juce::MidiBuffer& midiMessages)
{
    aef::mixBufferToMonoDual (buffer, getTotalNumInputChannels(), buffer.getNumSamples());
    AudioEffectFrameworkProcessor::processBlock (buffer, midiMessages);
    aef::duplicateMonoToStereoOutput (buffer, getTotalNumOutputChannels(), buffer.getNumSamples());
}

AudioProcessorEditor* Ds1AudioProcessor::createEditor()
{
    return new AudioEffectFrameworkEditor (*this);
}

const juce::String Ds1AudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool Ds1AudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Ds1AudioProcessor();
}
