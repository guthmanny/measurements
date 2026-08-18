#include "PluginProcessor.h"

#include "../JuceLibraryCode/JuceHeader.h"
#include "AefAudioUtils.h"
#include "MiddleProcessorEffectEngine.h"

FuzzFaceAudioProcessor::FuzzFaceAudioProcessor()
    : paramFuzz (parameters, "Fuzz", "", 0.0f, 1.0f, 0.5f),
      paramVolume (parameters, "Volume", "", 0.0f, 1.0f, 0.5f)
{
    aef::setParameterDefault (parameters.valueTreeState, paramInputGain.paramID, -12.0f);
    aef::setParameterDefault (parameters.valueTreeState, paramGateThreshold.paramID, -80.0f);
}

std::unique_ptr<KbussEffectEngine> FuzzFaceAudioProcessor::createEffectEngine()
{
    return std::make_unique<MiddleProcessorEffectEngine> (
        "com.kbuss.nudsp.white_box.fuzz_face", "Fuzz Face", "fuzz_face");
}

void FuzzFaceAudioProcessor::updateCustomEffectParameters()
{
    const auto fuzzFaceId = getKbussEngine().middleProcessorId();
    if (fuzzFaceId == kbuss::kInvalidObjectId)
        return;

    getKbussEngine().setParamNormalized (fuzzFaceId, "fuzz",
        readParameterValue (paramFuzz.paramID, paramFuzz.defaultValue));
    getKbussEngine().setParamNormalized (fuzzFaceId, "volume",
        readParameterValue (paramVolume.paramID, paramVolume.defaultValue));
}

void FuzzFaceAudioProcessor::processBlock (juce::AudioSampleBuffer& buffer, juce::MidiBuffer& midiMessages)
{
    aef::mixBufferToMonoDual (buffer, getTotalNumInputChannels(), buffer.getNumSamples());
    AudioEffectFrameworkProcessor::processBlock (buffer, midiMessages);
    aef::duplicateMonoToStereoOutput (buffer, getTotalNumOutputChannels(), buffer.getNumSamples());
}

AudioProcessorEditor* FuzzFaceAudioProcessor::createEditor()
{
    return new AudioEffectFrameworkEditor (*this);
}

const juce::String FuzzFaceAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool FuzzFaceAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FuzzFaceAudioProcessor();
}
