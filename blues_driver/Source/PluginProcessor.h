#pragma once

#include <memory>

#include "AudioEffectFrameworkProcessor.h"
#include "AudioEffectFrameworkEditor.h"

/** Boss BD-2 Blues Driver — MuDSP white-box model via kbuss. */
class BluesDriverAudioProcessor final : public AudioEffectFrameworkProcessor
{
public:
    BluesDriverAudioProcessor();

    AudioProcessorEditor* createEditor() override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;

    void processBlock (juce::AudioSampleBuffer& buffer, juce::MidiBuffer& midiMessages) override;

protected:
    std::unique_ptr<KbussEffectEngine> createEffectEngine() override;
    void updateCustomEffectParameters() override;
    bool bypassNoiseGateOnStartup() const override { return true; }

private:
    PluginParameterLinSlider paramDrive;
    PluginParameterLinSlider paramTone;
    PluginParameterLinSlider paramLevel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BluesDriverAudioProcessor)
};
