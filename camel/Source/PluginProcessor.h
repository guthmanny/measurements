#pragma once

#include <atomic>

#include "AudioEffectFrameworkProcessor.h"
#include "CamelEffectEngine.h"

/** CAMEL product-effect demo — pick any registered kbuss CAMEL plugin from a combo box. */
class CamelAudioProcessor final : public AudioEffectFrameworkProcessor,
                                  private juce::AudioProcessorValueTreeState::Listener
{
public:
    static constexpr const char* kCamelEffectParamId = "camel_effect";

    CamelAudioProcessor();
    ~CamelAudioProcessor() override;

    AudioProcessorEditor* createEditor() override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void processBlock(juce::AudioSampleBuffer& buffer, juce::MidiBuffer& midiMessages) override;

    [[nodiscard]] int currentEffectIndex() const;

protected:
    std::unique_ptr<KbussEffectEngine> createEffectEngine() override;
    bool bypassNoiseGateOnStartup() const override { return true; }

private:
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    void queueEffectReprepare(int effectIndex);
    void reprepareSelectedEffect();

    std::atomic<bool> effectPreparePending_{false};
    int pendingEffectIndex_ = 0;
    double lastSampleRate_ = 0.0;
    int lastBlockSize_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CamelAudioProcessor)
};
