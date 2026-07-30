#pragma once

#include <memory>

#include "AudioEffectFrameworkProcessor.h"
#include "AudioEffectFrameworkEditor.h"
#include "Ds1EffectEngine.h"

/** Boss DS-1 distortion — NuDSP white-box model via minibuss. */
class Ds1AudioProcessor final : public AudioEffectFrameworkProcessor
{
public:
    Ds1AudioProcessor();

    AudioProcessorEditor* createEditor() override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void processBlock (juce::AudioSampleBuffer& buffer, juce::MidiBuffer& midiMessages) override;

protected:
    std::unique_ptr<MinibussEffectEngine> createEffectEngine() override;
    void updateCustomEffectParameters() override;

private:
    PluginParameterLinSlider paramDs1Gain;
    PluginParameterLinSlider paramDs1Tone;
    PluginParameterLinSlider paramDs1Level;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Ds1AudioProcessor)
};
