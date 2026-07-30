#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "AudioEffectFrameworkProcessor.h"
#include "AudioEffectFrameworkEditor.h"

/** Minimal example plugin — add effect-specific parameters and override updateCustomEffectParameters(). */
class TemplateAudioProcessor : public AudioEffectFrameworkProcessor
{
public:
    TemplateAudioProcessor() = default;

    AudioProcessorEditor* createEditor() override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TemplateAudioProcessor)
};
