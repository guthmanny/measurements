#pragma once

#include <memory>

#include "AudioEffectFrameworkProcessor.h"
#include "AudioEffectFrameworkEditor.h"

/** Dallas Rangemaster treble booster — MuDSP white-box model via kbuss. */
class RangemasterAudioProcessor final : public AudioEffectFrameworkProcessor
{
public:
    RangemasterAudioProcessor();

    AudioProcessorEditor* createEditor() override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;

    void processBlock (juce::AudioSampleBuffer& buffer, juce::MidiBuffer& midiMessages) override;

protected:
    std::unique_ptr<KbussEffectEngine> createEffectEngine() override;
    void updateCustomEffectParameters() override;
    bool bypassNoiseGateOnStartup() const override { return true; }

private:
    PluginParameterLinSlider paramVolume;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RangemasterAudioProcessor)
};
