#include "PluginProcessor.h"

#include <algorithm>
#include <string_view>

#include "../JuceLibraryCode/JuceHeader.h"
#include "AefAudioUtils.h"
#include "DynamicMiddleProcessorEffectEngine.h"

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
        addUniqueSearchDir (dirs, dir);
        if (! dir.getParentDirectory().exists())
            break;
        dir = dir.getParentDirectory();
    }

#if defined(DYNA_COMP_PLUGIN_DIR)
    addUniqueSearchDir (dirs, juce::File (juce::String (DYNA_COMP_PLUGIN_DIR)));
#endif

    return dirs;
}

int indexOfUid (const DynamicPluginCatalog& catalog, std::string_view uid)
{
    const auto& entries = catalog.entries();
    for (int i = 0; i < (int) entries.size(); ++i)
    {
        if (entries[(std::size_t) i].uid == uid)
            return i;
    }
    return -1;
}
} // namespace

DynaCompAudioProcessor::DynaCompAudioProcessor()
{
    catalog_.scan (pluginSearchDirs());
    const int uidIndex = indexOfUid (catalog_, kDynamicUid);
    selectedPluginIndex_ = juce::jmax (0, uidIndex);

    if (uidIndex < 0)
        juce::Logger::writeToLog ("DynaComp: DynamicDynaComp.kbplug not found — check plugins/");
    else
        juce::Logger::writeToLog ("DynaComp: using " + juce::String (kDynamicUid));

    aef::setParameterDefault (parameters.valueTreeState, paramInputGain.paramID, -12.0f);
    aef::setParameterDefault (parameters.valueTreeState, paramGateThreshold.paramID, -80.0f);
}

std::unique_ptr<KbussEffectEngine> DynaCompAudioProcessor::createEffectEngine()
{
    return std::make_unique<DynamicMiddleProcessorEffectEngine> (catalog_, selectedPluginIndex_);
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
