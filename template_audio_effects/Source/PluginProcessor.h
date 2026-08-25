#pragma once

#include <memory>

#include "AudioEffectFrameworkProcessor.h"
#include "AudioEffectFrameworkEditor.h"

/** Minimal example plugin — middle-processor user params appear on the main panel automatically. */
class TemplateAudioProcessor final : public AudioEffectFrameworkProcessor
{
public:
    TemplateAudioProcessor();

    AudioProcessorEditor* createEditor() override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;

protected:
    std::unique_ptr<KbussEffectEngine> createEffectEngine() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TemplateAudioProcessor)
};
