#pragma once

#include <memory>

#include "AudioEffectFrameworkProcessor.h"
#include "AudioEffectFrameworkEditor.h"

/** Tech 21 SansAmp Classic — MuDSP white-box model via kbuss.
 *  Pedal knobs come from the middle processor (EffectUserParamsPanel). */
class SansampClassicAudioProcessor final : public AudioEffectFrameworkProcessor
{
public:
    SansampClassicAudioProcessor();

    AudioProcessorEditor* createEditor() override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;

    void processBlock (juce::AudioSampleBuffer& buffer, juce::MidiBuffer& midiMessages) override;

protected:
    std::unique_ptr<KbussEffectEngine> createEffectEngine() override;
    bool bypassNoiseGateOnStartup() const override { return true; }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SansampClassicAudioProcessor)
};
