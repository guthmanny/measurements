#include "PluginProcessor.h"

#include "../JuceLibraryCode/JuceHeader.h"
#include "AefAudioUtils.h"
#include "MiddleProcessorEffectEngine.h"

BluesDriverAudioProcessor::BluesDriverAudioProcessor()
    : paramDrive (parameters, "Drive", "", 0.0f, 1.0f, 0.5f),
      paramTone (parameters, "Tone", "", 0.0f, 1.0f, 0.5f),
      paramLevel (parameters, "Pedal Level", "", 0.0f, 1.0f, 0.5f)
{
    aef::setParameterDefault (parameters.valueTreeState, paramInputGain.paramID, -12.0f);
    aef::setParameterDefault (parameters.valueTreeState, paramGateThreshold.paramID, -80.0f);
}

std::unique_ptr<KbussEffectEngine> BluesDriverAudioProcessor::createEffectEngine()
{
    return std::make_unique<MiddleProcessorEffectEngine> (
        "com.kbuss.nudsp.white_box.blues_driver", "Blues Driver", "blues_driver");
}

void BluesDriverAudioProcessor::updateCustomEffectParameters()
{
    const auto bluesDriverId = getKbussEngine().middleProcessorId();
    if (bluesDriverId == kbuss::kInvalidObjectId)
        return;

    getKbussEngine().setParamNormalized (bluesDriverId, "drive",
        readParameterValue (paramDrive.paramID, paramDrive.defaultValue));
    getKbussEngine().setParamNormalized (bluesDriverId, "tone",
        readParameterValue (paramTone.paramID, paramTone.defaultValue));
    getKbussEngine().setParamNormalized (bluesDriverId, "level",
        readParameterValue (paramLevel.paramID, paramLevel.defaultValue));
}

void BluesDriverAudioProcessor::processBlock (juce::AudioSampleBuffer& buffer, juce::MidiBuffer& midiMessages)
{
    aef::mixBufferToMonoDual (buffer, getTotalNumInputChannels(), buffer.getNumSamples());
    AudioEffectFrameworkProcessor::processBlock (buffer, midiMessages);
    aef::duplicateMonoToStereoOutput (buffer, getTotalNumOutputChannels(), buffer.getNumSamples());
}

AudioProcessorEditor* BluesDriverAudioProcessor::createEditor()
{
    return new AudioEffectFrameworkEditor (*this);
}

const juce::String BluesDriverAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool BluesDriverAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BluesDriverAudioProcessor();
}
