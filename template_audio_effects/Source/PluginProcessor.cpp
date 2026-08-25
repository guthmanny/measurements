#include "PluginProcessor.h"

#include "../JuceLibraryCode/JuceHeader.h"
#include "MiddleProcessorEffectEngine.h"

TemplateAudioProcessor::TemplateAudioProcessor() = default;

std::unique_ptr<KbussEffectEngine> TemplateAudioProcessor::createEffectEngine()
{
    // Placeholder: swap UID/name/instance for your effect (see ds1/PluginProcessor.cpp).
    return std::make_unique<MiddleProcessorEffectEngine>(
        "com.kbuss.simple_gain", "Effect", "effect");
}

AudioProcessorEditor* TemplateAudioProcessor::createEditor()
{
    return new AudioEffectFrameworkEditor(*this);
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
