#pragma once

#include <memory>

#include "AudioEffectFrameworkProcessor.h"
#include "AudioEffectFrameworkEditor.h"
#include "DynamicPluginCatalog.h"

/** Boss CS-1 Dyna Comp — kbuss dynamic white-box (.kbplug) via MuDSP ota_comp.
 *  Pedal knobs come from the middle processor (EffectUserParamsPanel). */
class DynaCompAudioProcessor final : public AudioEffectFrameworkProcessor
{
public:
    static constexpr const char* kDynamicUid = "com.kbuss.dynamic.dyna_comp";

    DynaCompAudioProcessor();

    AudioProcessorEditor* createEditor() override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;

    void processBlock (juce::AudioSampleBuffer& buffer, juce::MidiBuffer& midiMessages) override;

protected:
    std::unique_ptr<KbussEffectEngine> createEffectEngine() override;
    bool bypassNoiseGateOnStartup() const override { return true; }

private:
    DynamicPluginCatalog catalog_;
    int selectedPluginIndex_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DynaCompAudioProcessor)
};
