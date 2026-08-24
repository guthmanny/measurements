#include "PluginProcessor.h"

#include "../JuceLibraryCode/JuceHeader.h"
#include "AefAudioUtils.h"
#include "MiddleProcessorEffectEngine.h"

VibroChampAudioProcessor::VibroChampAudioProcessor()
    : paramBass (parameters, "Bass", "", 0.0f, 1.0f, 0.5f),
      paramMid (parameters, "Mid", "", 0.0f, 1.0f, 0.5f),
      paramTreble (parameters, "Treble", "", 0.0f, 1.0f, 0.5f),
      paramVolume (parameters, "Volume", "", 0.0f, 1.0f, 0.5f)
{
    aef::setParameterDefault (parameters.valueTreeState, paramInputGain.paramID, -12.0f);
    aef::setParameterDefault (parameters.valueTreeState, paramGateThreshold.paramID, -80.0f);
}

std::unique_ptr<KbussEffectEngine> VibroChampAudioProcessor::createEffectEngine()
{
    return std::make_unique<MiddleProcessorEffectEngine> (
        "com.kbuss.nudsp.white_box.vibro_champ", "Vibro Champ", "vibro_champ");
}

void VibroChampAudioProcessor::updateCustomEffectParameters()
{
    const auto vibroChampId = getKbussEngine().middleProcessorId();
    if (vibroChampId == kbuss::kInvalidObjectId)
        return;

    getKbussEngine().setParamNormalized (vibroChampId, "bass",
        readParameterValue (paramBass.paramID, paramBass.defaultValue));
    getKbussEngine().setParamNormalized (vibroChampId, "mid",
        readParameterValue (paramMid.paramID, paramMid.defaultValue));
    getKbussEngine().setParamNormalized (vibroChampId, "treble",
        readParameterValue (paramTreble.paramID, paramTreble.defaultValue));
    getKbussEngine().setParamNormalized (vibroChampId, "volume",
        readParameterValue (paramVolume.paramID, paramVolume.defaultValue));
}

void VibroChampAudioProcessor::processBlock (juce::AudioSampleBuffer& buffer, juce::MidiBuffer& midiMessages)
{
    aef::mixBufferToMonoDual (buffer, getTotalNumInputChannels(), buffer.getNumSamples());
    AudioEffectFrameworkProcessor::processBlock (buffer, midiMessages);
    aef::duplicateMonoToStereoOutput (buffer, getTotalNumOutputChannels(), buffer.getNumSamples());
}

AudioProcessorEditor* VibroChampAudioProcessor::createEditor()
{
    return new AudioEffectFrameworkEditor (*this);
}

const juce::String VibroChampAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool VibroChampAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VibroChampAudioProcessor();
}
