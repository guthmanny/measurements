#include "PluginProcessor.h"

#include "AefAudioUtils.h"
#include "MudspAssetResolver.h"
#include "PluginEditor.h"

#include "../JuceLibraryCode/JuceHeader.h"

namespace {

const white_box_lab::CompositeEntry* entryAt(int index)
{
    const auto& list = white_box_lab::MudspAssetResolver::get().composites();
    if (! juce::isPositiveAndBelow(index, (int) list.size()))
        return nullptr;
    return &list[(size_t) index];
}

}  // namespace

WhiteBoxLabProcessor::WhiteBoxLabProcessor()
{
    const bool catalogLoaded = white_box_lab::MudspAssetResolver::get().load();
    juce::ignoreUnused(catalogLoaded);
    addModelParameter();
    parameters.valueTreeState.addParameterListener(kModelParamId, this);
    aef::setParameterDefault(parameters.valueTreeState, paramInputGain.paramID, -12.0f);
    aef::setParameterDefault(parameters.valueTreeState, paramGateThreshold.paramID, -80.0f);
}

WhiteBoxLabProcessor::~WhiteBoxLabProcessor()
{
    parameters.valueTreeState.removeParameterListener(kModelParamId, this);
}

void WhiteBoxLabProcessor::addModelParameter()
{
    const auto names = modelDisplayNames();
    const float maxChoice = (float) juce::jmax(1, names.size() - 1);
    juce::NormalisableRange<float> range(0.0f, maxChoice, 1.0f);

    int defaultIndex = 0;
    for (int i = 0; i < names.size(); ++i)
        if (names[i].contains("DS-1"))
            defaultIndex = i;

    parameters.valueTreeState.createAndAddParameter(
        kModelParamId, "White Box Model", "", range, (float) defaultIndex,
        [names](float value)
        {
            return names[juce::jlimit(0, names.size() - 1, (int) std::lround(value))];
        },
        [names](const juce::String& text) { return (float) names.indexOf(text); });
}

juce::StringArray WhiteBoxLabProcessor::modelDisplayNames() const
{
    juce::StringArray names;
    for (const auto& entry : white_box_lab::MudspAssetResolver::get().composites())
    {
        auto label = entry.name;
        if (entry.kbussUid.isEmpty())
            label += " (no kbuss)";
        names.add(label);
    }
    if (names.isEmpty())
        names.add("DS-1");
    return names;
}

int WhiteBoxLabProcessor::currentModelIndex() const
{
    const int count = juce::jmax(1, (int) white_box_lab::MudspAssetResolver::get().composites().size());
    if (auto* param = parameters.valueTreeState.getParameter(kModelParamId))
        return juce::jlimit(0, count - 1, (int) std::lround(param->convertFrom0to1(param->getValue())));
    return 0;
}

juce::String WhiteBoxLabProcessor::currentCompositeKey() const
{
    if (const auto* entry = entryAt(currentModelIndex()))
        return entry->key;
    return "ds1";
}

std::unique_ptr<KbussEffectEngine> WhiteBoxLabProcessor::createEffectEngine()
{
    auto engine = std::make_unique<WhiteBoxEffectEngine>();
    if (const auto* entry = entryAt(currentModelIndex()))
    {
        const auto uid = entry->kbussUid.isNotEmpty() ? entry->kbussUid
                                                      : juce::String("com.kbuss.nudsp.white_box.ds1");
        engine->setModel(uid, entry->name);
    }
    return engine;
}

void WhiteBoxLabProcessor::parameterChanged(const juce::String& parameterID, float)
{
    if (parameterID == kModelParamId)
        queueModelReprepare(currentModelIndex());
}

void WhiteBoxLabProcessor::queueModelReprepare(int modelIndex)
{
    pendingModelIndex_ = modelIndex;
    modelPreparePending_.store(true);
}

void WhiteBoxLabProcessor::reprepareSelectedModel()
{
    auto* engine = dynamic_cast<WhiteBoxEffectEngine*>(&getKbussEngine());
    if (engine == nullptr || lastSampleRate_ <= 0.0 || lastBlockSize_ <= 0)
        return;

    if (const auto* entry = entryAt(pendingModelIndex_))
    {
        const auto uid = entry->kbussUid.isNotEmpty() ? entry->kbussUid
                                                      : juce::String("com.kbuss.nudsp.white_box.ds1");
        engine->setModel(uid, entry->name);
    }

    engine->reprepare((float) lastSampleRate_, (std::uint32_t) juce::jmax(1, lastBlockSize_));
    resetMiddleProcessorParamDefaults();
    updateEffectParameters();
    applyEffectTopologyBypassOverrides();
    bumpMiddleProcessorGeneration();
}

void WhiteBoxLabProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    lastSampleRate_ = sampleRate;
    lastBlockSize_ = samplesPerBlock;
    pendingModelIndex_ = currentModelIndex();
    AudioEffectFrameworkProcessor::prepareToPlay(sampleRate, samplesPerBlock);
}

void WhiteBoxLabProcessor::processBlock(juce::AudioSampleBuffer& buffer, juce::MidiBuffer& midiMessages)
{
    if (modelPreparePending_.exchange(false))
        reprepareSelectedModel();

    aef::mixBufferToMonoDual(buffer, getTotalNumInputChannels(), buffer.getNumSamples());
    AudioEffectFrameworkProcessor::processBlock(buffer, midiMessages);
    aef::duplicateMonoToStereoOutput(buffer, getTotalNumOutputChannels(), buffer.getNumSamples());
}

AudioProcessorEditor* WhiteBoxLabProcessor::createEditor()
{
    return new WhiteBoxLabEditor(*this);
}

const juce::String WhiteBoxLabProcessor::getName() const
{
    return JucePlugin_Name;
}

bool WhiteBoxLabProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WhiteBoxLabProcessor();
}
