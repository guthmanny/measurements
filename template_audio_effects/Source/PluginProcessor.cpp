#include "PluginProcessor.h"

#include "../JuceLibraryCode/JuceHeader.h"
#include "MiddleProcessorEffectEngine.h"

TemplateAudioProcessor::TemplateAudioProcessor()
    : paramEffectGain (parameters, "Effect Gain", "dB", -12.0f, 12.0f, 6.0f)
{
}

std::unique_ptr<KbussEffectEngine> TemplateAudioProcessor::createEffectEngine()
{
    // Placeholder: swap UID/name/instance for your effect (see ds1/PluginProcessor.cpp).
    // White-box effects use com.kbuss.nudsp.white_box.* UIDs registered in kbuss.
    return std::make_unique<MiddleProcessorEffectEngine> (
        "com.kbuss.simple_gain", "Effect", "effect");
}

void TemplateAudioProcessor::updateCustomEffectParameters()
{
    const auto effectId = getKbussEngine().middleProcessorId();
    if (effectId == kbuss::kInvalidObjectId)
        return;

    getKbussEngine().setParamDomain (effectId, "gain",
        readParameterValue (paramEffectGain.paramID, paramEffectGain.defaultValue));
}

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
