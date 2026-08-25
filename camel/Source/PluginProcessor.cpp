#include "PluginProcessor.h"

#include "CamelCatalog.h"
#include "PluginEditor.h"

#include "../JuceLibraryCode/JuceHeader.h"

namespace
{
juce::StringArray camelEffectDisplayNames()
{
    juce::StringArray names;
    for (std::size_t i = 0; i < kCamelEffectCount; ++i)
        names.add(kCamelEffects[i].displayName);
    return names;
}

void addCamelEffectParameter(juce::AudioProcessorValueTreeState& apvts)
{
    const juce::StringArray names = camelEffectDisplayNames();
    const float maxChoice = (float)juce::jmax(1, names.size() - 1);
    juce::NormalisableRange<float> range(0.0f, maxChoice, 1.0f);

    apvts.createAndAddParameter(
        CamelAudioProcessor::kCamelEffectParamId,
        "CAMEL Effect",
        "",
        range,
        0.0f,
        [names](float value) { return names[juce::jlimit(0, names.size() - 1, (int)std::lround(value))]; },
        [names](const juce::String& text) { return names.indexOf(text); });
}
} // namespace

CamelAudioProcessor::CamelAudioProcessor()
{
    addCamelEffectParameter(parameters.valueTreeState);
    parameters.valueTreeState.addParameterListener(kCamelEffectParamId, this);
}

CamelAudioProcessor::~CamelAudioProcessor()
{
    parameters.valueTreeState.removeParameterListener(kCamelEffectParamId, this);
}

std::unique_ptr<KbussEffectEngine> CamelAudioProcessor::createEffectEngine()
{
    auto engine = std::make_unique<CamelEffectEngine>();
    engine->setEffectIndex(currentEffectIndex());
    return engine;
}

int CamelAudioProcessor::currentEffectIndex() const
{
    if (auto* param = parameters.valueTreeState.getParameter(kCamelEffectParamId))
        return juce::jlimit(0, (int)kCamelEffectCount - 1, (int)std::lround(param->convertFrom0to1(param->getValue())));

    if (auto* raw = parameters.valueTreeState.getRawParameterValue(kCamelEffectParamId))
        return juce::jlimit(0, (int)kCamelEffectCount - 1, (int)std::lround(raw->load()));

    return 0;
}

void CamelAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    juce::ignoreUnused(newValue);
    if (parameterID == kCamelEffectParamId)
        queueEffectReprepare(currentEffectIndex());
}

void CamelAudioProcessor::queueEffectReprepare(int effectIndex)
{
    pendingEffectIndex_ = juce::jlimit(0, (int)kCamelEffectCount - 1, effectIndex);
    effectPreparePending_.store(true);
}

void CamelAudioProcessor::reprepareSelectedEffect()
{
    auto* camelEngine = dynamic_cast<CamelEffectEngine*>(&getKbussEngine());
    if (camelEngine == nullptr || lastSampleRate_ <= 0.0 || lastBlockSize_ <= 0)
        return;

    camelEngine->setEffectIndex(pendingEffectIndex_);
    camelEngine->reprepare((float)lastSampleRate_, (std::uint32_t)juce::jmax(1, lastBlockSize_));
    resetMiddleProcessorParamDefaults();
    updateEffectParameters();
    applyEffectTopologyBypassOverrides();
    bumpMiddleProcessorGeneration();
}

void CamelAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    lastSampleRate_ = sampleRate;
    lastBlockSize_ = samplesPerBlock;
    pendingEffectIndex_ = currentEffectIndex();

    AudioEffectFrameworkProcessor::prepareToPlay(sampleRate, samplesPerBlock);
}

void CamelAudioProcessor::processBlock(juce::AudioSampleBuffer& buffer, juce::MidiBuffer& midiMessages)
{
    if (effectPreparePending_.exchange(false))
        reprepareSelectedEffect();

    AudioEffectFrameworkProcessor::processBlock(buffer, midiMessages);
}

AudioProcessorEditor* CamelAudioProcessor::createEditor()
{
    return new CamelAudioProcessorEditor(*this);
}

const juce::String CamelAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool CamelAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CamelAudioProcessor();
}
