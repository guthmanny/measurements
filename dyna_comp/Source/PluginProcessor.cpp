#include "PluginProcessor.h"

#include "../JuceLibraryCode/JuceHeader.h"
#include "AefAudioUtils.h"
#include "MiddleProcessorEffectEngine.h"

DynaCompAudioProcessor::DynaCompAudioProcessor()
    : paramAttack (parameters, "Attack", "", 0.0f, 1.0f, 0.5f),
      paramSensitivity (parameters, "Sensitivity", "", 0.0f, 1.0f, 0.5f),
      paramLevel (parameters, "Pedal Level", "", 0.0f, 1.0f, 0.5f)
{
    aef::setParameterDefault (parameters.valueTreeState, paramInputGain.paramID, -12.0f);
    aef::setParameterDefault (parameters.valueTreeState, paramGateThreshold.paramID, -80.0f);
}

std::unique_ptr<KbussEffectEngine> DynaCompAudioProcessor::createEffectEngine()
{
    return std::make_unique<MiddleProcessorEffectEngine> (
        "com.kbuss.nudsp.white_box.dyna_comp", "Dyna Comp", "dyna_comp");
}

void DynaCompAudioProcessor::updateCustomEffectParameters()
{
    const auto dynaCompId = getKbussEngine().middleProcessorId();
    if (dynaCompId == kbuss::kInvalidObjectId)
        return;

    getKbussEngine().setParamNormalized (dynaCompId, "attack",
        readParameterValue (paramAttack.paramID, paramAttack.defaultValue));
    getKbussEngine().setParamNormalized (dynaCompId, "sensitivity",
        readParameterValue (paramSensitivity.paramID, paramSensitivity.defaultValue));
    getKbussEngine().setParamNormalized (dynaCompId, "level",
        readParameterValue (paramLevel.paramID, paramLevel.defaultValue));
}

void DynaCompAudioProcessor::processBlock (juce::AudioSampleBuffer& buffer, juce::MidiBuffer& midiMessages)
{
    aef::mixBufferToMonoDual (buffer, getTotalNumInputChannels(), buffer.getNumSamples());
    AudioEffectFrameworkProcessor::processBlock (buffer, midiMessages);
    aef::duplicateMonoToStereoOutput (buffer, getTotalNumOutputChannels(), buffer.getNumSamples());
}

AudioProcessorEditor* DynaCompAudioProcessor::createEditor()
{
    return new AudioEffectFrameworkEditor (*this);
}

const juce::String DynaCompAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool DynaCompAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DynaCompAudioProcessor();
}
