#pragma once

#include <memory>

#include "AudioEffectFrameworkProcessor.h"
#include "AudioEffectFrameworkEditor.h"

/** Boss CS-1 Dyna Comp — MuDSP white-box model via kbuss. */
class DynaCompAudioProcessor final : public AudioEffectFrameworkProcessor
{
public:
    DynaCompAudioProcessor();

    AudioProcessorEditor* createEditor() override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;

    void processBlock (juce::AudioSampleBuffer& buffer, juce::MidiBuffer& midiMessages) override;

protected:
    std::unique_ptr<KbussEffectEngine> createEffectEngine() override;
    void updateCustomEffectParameters() override;
    bool bypassNoiseGateOnStartup() const override { return true; }

private:
    PluginParameterLinSlider paramAttack;
    PluginParameterLinSlider paramSensitivity;
    PluginParameterLinSlider paramLevel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DynaCompAudioProcessor)
};
