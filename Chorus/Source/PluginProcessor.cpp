#include "PluginProcessor.h"

#include "PluginEditor.h"

#include <cmath>

#include "../JuceLibraryCode/JuceHeader.h"

namespace
{
constexpr int kModelCount = 3;

juce::StringArray chorusModelDisplayNames()
{
    return { "Chorus", "Stereo Phase90", "Mono Phase90" };
}

void addChorusModelParameter(juce::AudioProcessorValueTreeState& apvts)
{
    const juce::StringArray names = chorusModelDisplayNames();
    const float maxChoice = (float) juce::jmax(1, names.size() - 1);
    juce::NormalisableRange<float> range(0.0f, maxChoice, 1.0f);

    apvts.createAndAddParameter(
        ChorusAudioProcessor::kChorusModelParamId,
        "Effect",
        "",
        range,
        0.0f,
        [names](float value) { return names[juce::jlimit(0, names.size() - 1, (int) std::lround(value))]; },
        [names](const juce::String& text) { return (float) names.indexOf(text); });
}

juce::String paramLeaf(juce::String id)
{
    if (id.endsWithIgnoreCase(".control"))
        return id.dropLastCharacters(8);
    return id;
}
}  // namespace

ChorusAudioProcessor::ChorusAudioProcessor()
    : paramMix(parameters, "Mix", "", 0.0f, 1.0f, 0.5f),
      paramEffectMax(parameters, "Effect Max", "", 0.0f, 1.0f, 1.0f),
      paramRateMin(parameters, "Rate Min", "Hz", queryChorusNuDspLimits().rateMin,
                   queryChorusNuDspLimits().rateMax, queryChorusNuDspLimits().rateMin),
      paramRateMax(parameters, "Rate Max", "Hz", queryChorusNuDspLimits().rateMin,
                   queryChorusNuDspLimits().rateMax, queryChorusNuDspLimits().rateMax)
{
    addChorusModelParameter(parameters.valueTreeState);
    parameters.valueTreeState.addParameterListener(kChorusModelParamId, this);
    parameters.valueTreeState.addParameterListener(paramMix.paramID, this);
    parameters.valueTreeState.addParameterListener(paramEffectMax.paramID, this);
}

ChorusAudioProcessor::~ChorusAudioProcessor()
{
    parameters.valueTreeState.removeParameterListener(kChorusModelParamId, this);
    parameters.valueTreeState.removeParameterListener(paramMix.paramID, this);
    parameters.valueTreeState.removeParameterListener(paramEffectMax.paramID, this);
}

std::unique_ptr<KbussEffectEngine> ChorusAudioProcessor::createEffectEngine()
{
    auto engine = std::make_unique<ChorusEffectEngine>();
    engine->setEffect(static_cast<ChorusEffectEngine::Effect>(currentModelIndex()));
    return engine;
}

int ChorusAudioProcessor::currentModelIndex() const
{
    if (auto* param = parameters.valueTreeState.getParameter(kChorusModelParamId))
        return juce::jlimit(0, kModelCount - 1, (int) std::lround(param->convertFrom0to1(param->getValue())));

    if (auto* raw = parameters.valueTreeState.getRawParameterValue(kChorusModelParamId))
        return juce::jlimit(0, kModelCount - 1, (int) std::lround(raw->load()));

    return 0;
}

ChorusNuDspLimits ChorusAudioProcessor::getChorusNuDspLimits() const noexcept
{
    return queryChorusNuDspLimits();
}

juce::String ChorusAudioProcessor::viewParamId(const juce::String& leaf) const
{
    auto* proc = getKbussEngine().getMiddleProcessor();
    if (proc != nullptr)
    {
        const auto controlId = (leaf + ".control").toStdString();
        if (proc->parameter(controlId) != nullptr)
            return leaf + ".control";
        if (proc->parameter(leaf.toStdString()) != nullptr)
            return leaf;
    }
    return leaf + ".control";
}

float ChorusAudioProcessor::hostParamValue(const juce::String& paramId, float fallback) const
{
    return readParameterValue(paramId, fallback);
}

bool ChorusAudioProcessor::isInsideViewParam(const juce::String& paramId) const
{
    const auto leaf = paramLeaf(paramId);
    if (leaf == "rate" || leaf == "lfo_type")
        return true;
    if (!isPhase90() && (leaf == "delay" || leaf == "amount" || leaf == "coeff_mod"))
        return true;
    return false;
}

bool ChorusAudioProcessor::isSettingsOnlyParam(const juce::String& paramId) const
{
    return paramId == paramMix.paramID || paramId == paramEffectMax.paramID
           || paramId == paramRateMin.paramID || paramId == paramRateMax.paramID;
}

void ChorusAudioProcessor::pushCoeffMod()
{
    if (isPhase90())
        return;
    const float mix = readParameterValue(paramMix.paramID, paramMix.defaultValue);
    const float effectMax = readParameterValue(paramEffectMax.paramID, paramEffectMax.defaultValue);
    setMiddleParamDomain(viewParamId("coeff_mod"), juce::jlimit(0.0f, 1.0f, mix * effectMax));
}

void ChorusAudioProcessor::updateCustomEffectParameters()
{
    pushCoeffMod();
}

void ChorusAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    juce::ignoreUnused(newValue);
    if (parameterID == kChorusModelParamId)
        queueModelReprepare(currentModelIndex());
    else if (parameterID == paramMix.paramID || parameterID == paramEffectMax.paramID)
        pushCoeffMod();
}

void ChorusAudioProcessor::queueModelReprepare(int modelIndex)
{
    pendingModelIndex_ = juce::jlimit(0, kModelCount - 1, modelIndex);
    modelPreparePending_.store(true);
}

void ChorusAudioProcessor::reprepareSelectedModel()
{
    auto* chorusEngine = dynamic_cast<ChorusEffectEngine*>(&getKbussEngine());
    if (chorusEngine == nullptr || lastSampleRate_ <= 0.0 || lastBlockSize_ <= 0)
        return;

    chorusEngine->setEffect(static_cast<ChorusEffectEngine::Effect>(pendingModelIndex_));
    chorusEngine->reprepare((float) lastSampleRate_, (std::uint32_t) juce::jmax(1, lastBlockSize_));
    resetMiddleProcessorParamDefaults();
    updateEffectParameters();
    applyEffectTopologyBypassOverrides();
    bumpMiddleProcessorGeneration();
}

void ChorusAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    lastSampleRate_ = sampleRate;
    lastBlockSize_ = samplesPerBlock;
    pendingModelIndex_ = currentModelIndex();

    AudioEffectFrameworkProcessor::prepareToPlay(sampleRate, samplesPerBlock);
}

void ChorusAudioProcessor::processBlock(juce::AudioSampleBuffer& buffer, juce::MidiBuffer& midiMessages)
{
    if (modelPreparePending_.exchange(false))
        reprepareSelectedModel();

    AudioEffectFrameworkProcessor::processBlock(buffer, midiMessages);
}

AudioProcessorEditor* ChorusAudioProcessor::createEditor()
{
    return new ChorusAudioProcessorEditor(*this);
}

const juce::String ChorusAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ChorusAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ChorusAudioProcessor();
}
