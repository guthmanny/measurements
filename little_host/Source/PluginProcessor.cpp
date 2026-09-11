#include "PluginProcessor.h"

#include <algorithm>

#include "PluginEditor.h"

#include "../JuceLibraryCode/JuceHeader.h"
#include "AefAudioUtils.h"
#include "DynamicMiddleProcessorEffectEngine.h"
#include "MudspAssetResolver.h"

namespace
{
void addUniqueSearchDir (std::vector<std::string>& dirs, const juce::File& dir)
{
    if (! dir.isDirectory())
        return;

    const auto path = dir.getFullPathName().toStdString();
    if (std::find (dirs.begin(), dirs.end(), path) == dirs.end())
        dirs.push_back (path);
}

std::vector<std::string> pluginSearchDirs()
{
    std::vector<std::string> dirs;

    const auto exe = juce::File::getSpecialLocation (juce::File::currentExecutableFile);
    auto dir = exe.getParentDirectory();
    for (int depth = 0; depth < 5; ++depth)
    {
        addUniqueSearchDir (dirs, dir.getChildFile ("plugins"));
        // Also accept .kbplug bundles placed next to the executable.
        addUniqueSearchDir (dirs, dir);
        if (! dir.getParentDirectory().exists())
            break;
        dir = dir.getParentDirectory();
    }

#if defined(LITTLE_HOST_PLUGIN_DIR)
    // Compile-time build dir — only used when no runtime plugins/ folder was found.
    if (dirs.empty())
        addUniqueSearchDir (dirs, juce::File (juce::String (LITTLE_HOST_PLUGIN_DIR)));
#endif

    return dirs;
}

void addPluginChoiceParameter (juce::AudioProcessorValueTreeState& apvts,
                               const juce::StringArray& pluginNames)
{
    apvts.createAndAddParameter (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { LittleHostProcessor::kPluginChoiceParamId, 1 },
        "Plugin",
        pluginNames,
        0));
}
} // namespace

LittleHostProcessor::LittleHostProcessor()
{
    const bool catalogLoaded = white_box_lab::MudspAssetResolver::get().load();
    if (! catalogLoaded)
        juce::Logger::writeToLog ("LittleHost: MuDSP catalog not found — schematic UI disabled");

    catalog_.scan (pluginSearchDirs());
    addPluginChoiceParameter (parameters.valueTreeState, catalog_.displayNames());
    parameters.valueTreeState.addParameterListener (kPluginChoiceParamId, this);

    aef::setParameterDefault (parameters.valueTreeState, paramInputGain.paramID, -12.0f);
    aef::setParameterDefault (parameters.valueTreeState, paramGateThreshold.paramID, -80.0f);
}

LittleHostProcessor::~LittleHostProcessor()
{
    parameters.valueTreeState.removeParameterListener (kPluginChoiceParamId, this);
}

void LittleHostProcessor::rescanPlugins()
{
    catalog_.scan (pluginSearchDirs());
}

std::unique_ptr<KbussEffectEngine> LittleHostProcessor::createEffectEngine()
{
    return std::make_unique<DynamicMiddleProcessorEffectEngine> (catalog_, currentPluginIndex());
}

int LittleHostProcessor::currentPluginIndex() const
{
    if (catalog_.empty())
        return 0;

    if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (
            parameters.valueTreeState.getParameter (kPluginChoiceParamId)))
    {
        return juce::jlimit (0, (int) catalog_.size() - 1, choice->getIndex());
    }

    if (auto* param = parameters.valueTreeState.getParameter (kPluginChoiceParamId))
        return juce::jlimit (0, (int) catalog_.size() - 1,
                            (int) std::lround (param->convertFrom0to1 (param->getValue())));

    return 0;
}

juce::String LittleHostProcessor::currentCompositeKey() const
{
    const auto* entry = catalog_.entry ((std::size_t) currentPluginIndex());
    if (entry == nullptr || entry->composite_key.empty())
        return {};
    return entry->composite_key;
}

void LittleHostProcessor::parameterChanged (const juce::String& parameterID, float newValue)
{
    juce::ignoreUnused (newValue);
    if (parameterID == kPluginChoiceParamId)
        queuePluginReprepare (currentPluginIndex());
}

void LittleHostProcessor::queuePluginReprepare (int pluginIndex)
{
    pendingPluginIndex_ = catalog_.empty() ? 0
                                          : juce::jlimit (0, (int) catalog_.size() - 1, pluginIndex);
    pluginPreparePending_.store (true);
}

void LittleHostProcessor::reprepareSelectedPlugin()
{
    auto* dynamicEngine = dynamic_cast<DynamicMiddleProcessorEffectEngine*> (&getKbussEngine());
    if (dynamicEngine == nullptr || lastSampleRate_ <= 0.0 || lastBlockSize_ <= 0)
        return;

    dynamicEngine->setSelectedPluginIndex (pendingPluginIndex_);
    dynamicEngine->reprepare ((float) lastSampleRate_, (std::uint32_t) juce::jmax (1, lastBlockSize_));
    resetMiddleProcessorParamDefaults();
    updateEffectParameters();
    applyEffectTopologyBypassOverrides();
    bumpMiddleProcessorGeneration();
}

void LittleHostProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    lastSampleRate_ = sampleRate;
    lastBlockSize_ = samplesPerBlock;
    pendingPluginIndex_ = currentPluginIndex();

    AudioEffectFrameworkProcessor::prepareToPlay (sampleRate, samplesPerBlock);

    if (! getKbussEngine().isReady())
        juce::Logger::writeToLog ("LittleHost: kbuss engine failed to prepare — check plugins/ directory");
}

void LittleHostProcessor::processBlock (juce::AudioSampleBuffer& buffer, juce::MidiBuffer& midiMessages)
{
    if (pluginPreparePending_.exchange (false))
        reprepareSelectedPlugin();

    AudioEffectFrameworkProcessor::processBlock (buffer, midiMessages);
}

AudioProcessorEditor* LittleHostProcessor::createEditor()
{
    return new LittleHostProcessorEditor (*this);
}

const juce::String LittleHostProcessor::getName() const
{
    return JucePlugin_Name;
}

bool LittleHostProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LittleHostProcessor();
}
