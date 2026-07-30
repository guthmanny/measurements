#include "PluginProcessor.h"

#include "../JuceLibraryCode/JuceHeader.h"

AudioProcessorEditor* TemplateAudioProcessor::createEditor()
{
    return new AudioEffectFrameworkEditor (*this);
}

const juce::String TemplateAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool TemplateAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TemplateAudioProcessor();
}
