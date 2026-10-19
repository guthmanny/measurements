#pragma once

#include <atomic>

#include "AudioEffectFrameworkProcessor.h"
#include "ChorusEffectEngine.h"
#include "ChorusNuDspDefaults.h"

/** Chorus / Stereo Phase90 / Mono Phase90 — MuDSP CAMEL models via kbuss + DelayModView. */
class ChorusAudioProcessor final : public AudioEffectFrameworkProcessor,
                                   private juce::AudioProcessorValueTreeState::Listener
{
public:
    static constexpr const char* kChorusModelParamId = "chorus_model";

    ChorusAudioProcessor();
    ~ChorusAudioProcessor() override;

    AudioProcessorEditor* createEditor() override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void processBlock(juce::AudioSampleBuffer& buffer, juce::MidiBuffer& midiMessages) override;

    [[nodiscard]] int currentModelIndex() const;
    [[nodiscard]] bool isPhase90() const { return currentModelIndex() != 0; }
    [[nodiscard]] ChorusNuDspLimits getChorusNuDspLimits() const noexcept;
    [[nodiscard]] juce::String viewParamId(const juce::String& leaf) const;
    [[nodiscard]] float hostParamValue(const juce::String& paramId, float fallback) const;
    bool isInsideViewParam(const juce::String& paramId) const override;
    bool isSettingsOnlyParam(const juce::String& paramId) const override;

    PluginParameterLinSlider paramMix;
    PluginParameterLinSlider paramEffectMax;
    PluginParameterLinSlider paramRateMin;
    PluginParameterLinSlider paramRateMax;

protected:
    std::unique_ptr<KbussEffectEngine> createEffectEngine() override;
    bool bypassNoiseGateOnStartup() const override { return true; }
    void updateCustomEffectParameters() override;

private:
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    void queueModelReprepare(int modelIndex);
    void reprepareSelectedModel();
    void pushCoeffMod();

    std::atomic<bool> modelPreparePending_{false};
    int pendingModelIndex_ = 0;
    double lastSampleRate_ = 0.0;
    int lastBlockSize_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChorusAudioProcessor)
};
