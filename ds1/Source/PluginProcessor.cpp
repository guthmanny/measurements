#include "PluginProcessor.h"

#include "../JuceLibraryCode/JuceHeader.h"

namespace
{
void setParameterDefault (juce::AudioProcessorValueTreeState& state, const juce::String& paramId, float domainValue)
{
    if (auto* param = state.getParameter (paramId))
        param->setValueNotifyingHost (param->convertTo0to1 (domainValue));
}

void mixBufferToMonoDual (juce::AudioSampleBuffer& buffer, int numChannels, int numSamples)
{
    if (numChannels < 2 || numSamples <= 0)
        return;

    float* left = buffer.getWritePointer (0);
    float* right = buffer.getWritePointer (1);

    for (int i = 0; i < numSamples; ++i)
    {
        const float mono = 0.5f * (left[i] + right[i]);
        left[i] = mono;
        right[i] = mono;
    }
}
} // namespace

Ds1AudioProcessor::Ds1AudioProcessor()
    : paramDs1Gain (parameters, "Distortion", "", 0.0f, 1.0f, 0.5f),
      paramDs1Tone (parameters, "Tone", "", 0.0f, 1.0f, 0.5f),
      paramDs1Level (parameters, "Pedal Level", "", 0.0f, 1.0f, 0.5f)
{
    setParameterDefault (parameters.valueTreeState, paramInputGain.paramID, -12.0f);
    setParameterDefault (parameters.valueTreeState, paramGateThreshold.paramID, -80.0f);
}

std::unique_ptr<MinibussEffectEngine> Ds1AudioProcessor::createEffectEngine()
{
    return std::make_unique<Ds1EffectEngine>();
}

void Ds1AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    AudioEffectFrameworkProcessor::prepareToPlay (sampleRate, samplesPerBlock);

    auto& engine = getMinibussEngine();
    if (engine.gateId() != minibuss::kInvalidObjectId)
        engine.setProcessorBypassed (engine.gateId(), true);
}

void Ds1AudioProcessor::updateCustomEffectParameters()
{
    const auto ds1Id = getMinibussEngine().middleProcessorId();
    if (ds1Id == minibuss::kInvalidObjectId)
        return;

    getMinibussEngine().setParamNormalized (ds1Id, "gain",
        readParameterValue (paramDs1Gain.paramID, paramDs1Gain.defaultValue));
    getMinibussEngine().setParamNormalized (ds1Id, "tone",
        readParameterValue (paramDs1Tone.paramID, paramDs1Tone.defaultValue));
    getMinibussEngine().setParamNormalized (ds1Id, "level",
        readParameterValue (paramDs1Level.paramID, paramDs1Level.defaultValue));
}

void Ds1AudioProcessor::processBlock (juce::AudioSampleBuffer& buffer, juce::MidiBuffer& midiMessages)
{
    mixBufferToMonoDual (buffer, getTotalNumInputChannels(), buffer.getNumSamples());
    AudioEffectFrameworkProcessor::processBlock (buffer, midiMessages);

    if (getTotalNumOutputChannels() >= 2 && buffer.getNumSamples() > 0)
        buffer.copyFrom (1, 0, buffer, 0, 0, buffer.getNumSamples());
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
