#pragma once

#include <memory>

#include "AudioEffectFrameworkProcessor.h"
#include "AudioEffectFrameworkEditor.h"

/** Fender Vibro Champ amp — MuDSP white-box model via kbuss.
 *  Pedal knobs come from the middle processor (EffectUserParamsPanel). */
class VibroChampAudioProcessor final : public AudioEffectFrameworkProcessor
{
public:
    VibroChampAudioProcessor();

    AudioProcessorEditor* createEditor() override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;

    void processBlock (juce::AudioSampleBuffer& buffer, juce::MidiBuffer& midiMessages) override;

protected:
    std::unique_ptr<KbussEffectEngine> createEffectEngine() override;
    bool bypassNoiseGateOnStartup() const override { return true; }

    /** Amp Newton is too heavy for default 2× OS — stay at 1× regardless of QUALITY. */
    int oversampleFactorForQuality (int /*qualityChoice*/) const override { return 1; }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VibroChampAudioProcessor)
};
