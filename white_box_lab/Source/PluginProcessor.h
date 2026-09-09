#pragma once

#include <atomic>

#include "AudioEffectFrameworkProcessor.h"
#include "WhiteBoxEffectEngine.h"

class WhiteBoxLabProcessor final : public AudioEffectFrameworkProcessor,
                                   private juce::AudioProcessorValueTreeState::Listener
{
public:
    static constexpr const char* kModelParamId = "white_box_model";

    WhiteBoxLabProcessor();
    ~WhiteBoxLabProcessor() override;

    AudioProcessorEditor* createEditor() override;
    const juce::String getName() const override;
    bool acceptsMidi() const override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void processBlock(juce::AudioSampleBuffer& buffer, juce::MidiBuffer& midiMessages) override;

    [[nodiscard]] int currentModelIndex() const;
    [[nodiscard]] juce::String currentCompositeKey() const;
    [[nodiscard]] juce::StringArray modelDisplayNames() const;

protected:
    std::unique_ptr<KbussEffectEngine> createEffectEngine() override;
    bool bypassNoiseGateOnStartup() const override { return true; }

private:
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void queueModelReprepare(int modelIndex);
    void reprepareSelectedModel();
    void addModelParameter();

    std::atomic<bool> modelPreparePending_{false};
    int pendingModelIndex_ = 0;
    double lastSampleRate_ = 0.0;
    int lastBlockSize_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WhiteBoxLabProcessor)
};
