#include "PluginProcessor.h"

#include "../JuceLibraryCode/JuceHeader.h"
#include "AefAudioUtils.h"
#include "MiddleProcessorEffectEngine.h"

RangemasterAudioProcessor::RangemasterAudioProcessor()
    : paramVolume (parameters, "Volume", "", 0.0f, 1.0f, 0.5f)
{
    aef::setParameterDefault (parameters.valueTreeState, paramInputGain.paramID, -12.0f);
    aef::setParameterDefault (parameters.valueTreeState, paramGateThreshold.paramID, -80.0f);
}

std::unique_ptr<KbussEffectEngine> RangemasterAudioProcessor::createEffectEngine()
{
    return std::make_unique<MiddleProcessorEffectEngine> (
        "com.kbuss.nudsp.white_box.rangemaster", "Rangemaster", "rangemaster");
}

void RangemasterAudioProcessor::updateCustomEffectParameters()
{
    const auto rangemasterId = getKbussEngine().middleProcessorId();
    if (rangemasterId == kbuss::kInvalidObjectId)
        return;

    getKbussEngine().setParamNormalized (rangemasterId, "volume",
        readParameterValue (paramVolume.paramID, paramVolume.defaultValue));
}

void RangemasterAudioProcessor::processBlock (juce::AudioSampleBuffer& buffer, juce::MidiBuffer& midiMessages)
{
    aef::mixBufferToMonoDual (buffer, getTotalNumInputChannels(), buffer.getNumSamples());
    AudioEffectFrameworkProcessor::processBlock (buffer, midiMessages);
    aef::duplicateMonoToStereoOutput (buffer, getTotalNumOutputChannels(), buffer.getNumSamples());
}

AudioProcessorEditor* RangemasterAudioProcessor::createEditor()
{
    return new AudioEffectFrameworkEditor (*this);
}

const juce::String RangemasterAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool RangemasterAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RangemasterAudioProcessor();
}
