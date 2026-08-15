#pragma once

#include <memory>

#include "AudioEffectFrameworkProcessor.h"
#include "AudioEffectFrameworkEditor.h"

/** Boss Guv'nor distortion — NuDSP white-box model via minibuss. */
class GuvnorAudioProcessor final : public AudioEffectFrameworkProcessor
{
public:
    GuvnorAudioProcessor();

    AudioProcessorEditor* createEditor() override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;

    void processBlock (juce::AudioSampleBuffer& buffer, juce::MidiBuffer& midiMessages) override;

protected:
    std::unique_ptr<MinibussEffectEngine> createEffectEngine() override;
    void updateCustomEffectParameters() override;
    bool bypassNoiseGateOnStartup() const override { return true; }

private:
    PluginParameterLinSlider paramGuvnorGain;
    PluginParameterLinSlider paramGuvnorBass;
    PluginParameterLinSlider paramGuvnorMid;
    PluginParameterLinSlider paramGuvnorTreble;
    PluginParameterLinSlider paramGuvnorLevel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GuvnorAudioProcessor)
};
