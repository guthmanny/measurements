#include "PluginProcessor.h"

#include "../JuceLibraryCode/JuceHeader.h"
#include "AefAudioUtils.h"
#include "MiddleProcessorEffectEngine.h"

GuvnorAudioProcessor::GuvnorAudioProcessor()
    : paramGuvnorGain(parameters, "Gain", "", 0.0f, 1.0f, 0.5f),
      paramGuvnorBass(parameters, "Bass", "", 0.0f, 1.0f, 0.5f),
      paramGuvnorMid(parameters, "Mid", "", 0.0f, 1.0f, 0.5f),
      paramGuvnorTreble(parameters, "Treble", "", 0.0f, 1.0f, 0.5f),
      paramGuvnorLevel(parameters, "Pedal Level", "", 0.0f, 1.0f, 0.99f)
{
    aef::setParameterDefault(parameters.valueTreeState, paramInputGain.paramID, -12.0f);
    aef::setParameterDefault(parameters.valueTreeState, paramGateThreshold.paramID, -80.0f);
}

std::unique_ptr<KbussEffectEngine> GuvnorAudioProcessor::createEffectEngine()
{
    return std::make_unique<MiddleProcessorEffectEngine>(
        "com.kbuss.nudsp.white_box.guvnor", "Guv'nor", "guvnor");
}

void GuvnorAudioProcessor::updateCustomEffectParameters()
{
    const auto guvnorId = getKbussEngine().middleProcessorId();
    if (guvnorId == kbuss::kInvalidObjectId)
        return;

    getKbussEngine().setParamNormalized(guvnorId, "gain",
                                           readParameterValue(paramGuvnorGain.paramID, paramGuvnorGain.defaultValue));
    getKbussEngine().setParamNormalized(guvnorId, "bass",
                                           readParameterValue(paramGuvnorBass.paramID, paramGuvnorBass.defaultValue));
    getKbussEngine().setParamNormalized(guvnorId, "mid",
                                           readParameterValue(paramGuvnorMid.paramID, paramGuvnorMid.defaultValue));
    getKbussEngine().setParamNormalized(guvnorId, "treble",
                                           readParameterValue(paramGuvnorTreble.paramID, paramGuvnorTreble.defaultValue));
    getKbussEngine().setParamNormalized(guvnorId, "level",
                                           readParameterValue(paramGuvnorLevel.paramID, paramGuvnorLevel.defaultValue));
}

void GuvnorAudioProcessor::processBlock(juce::AudioSampleBuffer &buffer, juce::MidiBuffer &midiMessages)
{
    aef::mixBufferToMonoDual(buffer, getTotalNumInputChannels(), buffer.getNumSamples());
    AudioEffectFrameworkProcessor::processBlock(buffer, midiMessages);
    aef::duplicateMonoToStereoOutput(buffer, getTotalNumOutputChannels(), buffer.getNumSamples());
}

AudioProcessorEditor *GuvnorAudioProcessor::createEditor()
{
    return new AudioEffectFrameworkEditor(*this);
}

const juce::String GuvnorAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool GuvnorAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new GuvnorAudioProcessor();
}
