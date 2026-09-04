#pragma once

#include <atomic>

#include "AudioEffectFrameworkProcessor.h"
#include "DynamicMiddleProcessorEffectEngine.h"
#include "DynamicPluginCatalog.h"

/** Generic .kbplug host — scan a plugins directory and load the selected effect at runtime. */
class LittleHostProcessor final : public AudioEffectFrameworkProcessor,
                                  private juce::AudioProcessorValueTreeState::Listener
{
public:
    static constexpr const char* kPluginChoiceParamId = "dynamic_plugin";

    LittleHostProcessor();
    ~LittleHostProcessor() override;

    AudioProcessorEditor* createEditor() override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void processBlock (juce::AudioSampleBuffer& buffer, juce::MidiBuffer& midiMessages) override;

    [[nodiscard]] const DynamicPluginCatalog& pluginCatalog() const noexcept { return catalog_; }
    [[nodiscard]] int currentPluginIndex() const;
    void rescanPlugins();

protected:
    std::unique_ptr<KbussEffectEngine> createEffectEngine() override;
    bool bypassNoiseGateOnStartup() const override { return true; }

private:
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    void queuePluginReprepare (int pluginIndex);
    void reprepareSelectedPlugin();

    DynamicPluginCatalog catalog_;
    std::atomic<bool> pluginPreparePending_{false};
    int pendingPluginIndex_ = 0;
    double lastSampleRate_ = 0.0;
    int lastBlockSize_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LittleHostProcessor)
};
