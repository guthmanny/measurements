#pragma once

#include <memory>

#include "AudioEffectFrameworkProcessor.h"
#include "AudioEffectFrameworkEditor.h"

/** Minimal example plugin — add parameters and wire them in updateCustomEffectParameters(). */
class TemplateAudioProcessor final : public AudioEffectFrameworkProcessor
{
public:
    TemplateAudioProcessor();

    AudioProcessorEditor* createEditor() override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;

protected:
    std::unique_ptr<MinibussEffectEngine> createEffectEngine() override;
    void updateCustomEffectParameters() override;

private:
    PluginParameterLinSlider paramEffectGain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TemplateAudioProcessor)
};
