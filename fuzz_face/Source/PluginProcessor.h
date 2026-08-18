#pragma once

#include <memory>

#include "AudioEffectFrameworkProcessor.h"
#include "AudioEffectFrameworkEditor.h"

/** Dallas Arbiter Fuzz Face — MuDSP white-box model via kbuss. */
class FuzzFaceAudioProcessor final : public AudioEffectFrameworkProcessor
{
public:
    FuzzFaceAudioProcessor();

    AudioProcessorEditor* createEditor() override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;

    void processBlock (juce::AudioSampleBuffer& buffer, juce::MidiBuffer& midiMessages) override;

protected:
    std::unique_ptr<KbussEffectEngine> createEffectEngine() override;
    void updateCustomEffectParameters() override;
    bool bypassNoiseGateOnStartup() const override { return true; }

private:
    PluginParameterLinSlider paramFuzz;
    PluginParameterLinSlider paramVolume;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FuzzFaceAudioProcessor)
};
