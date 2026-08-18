#pragma once

#include <memory>

#include "AudioEffectFrameworkProcessor.h"

/** Moog Ladder filter — MuDSP SSMEL model via kbuss.
 *
 *  Parameters:
 *    Cutoff       [20, 20000] Hz
 *    Resonance    [0, 1]
 *    Drive        [0.1, 10]
 *    Mode         {LP4, LP2, HP4, HP2, BP4, BP2, Notch}
 *    Saturator    {Algebraic, Tanh}
 *    Quality      {Static, Relinearized, Outer2}
 *    ADAA         {Off, On}
 */
class MoogLadderAudioProcessor final : public AudioEffectFrameworkProcessor
{
 public:
  MoogLadderAudioProcessor();

  AudioProcessorEditor* createEditor() override;

  const juce::String getName() const override;
  bool acceptsMidi() const override;

  void processBlock(juce::AudioSampleBuffer& buffer, juce::MidiBuffer& midiMessages) override;

 protected:
  std::unique_ptr<KbussEffectEngine> createEffectEngine() override;
  void updateCustomEffectParameters() override;
  bool bypassNoiseGateOnStartup() const override { return true; }

 private:
  PluginParameterLogSlider paramCutoff;
  PluginParameterLinSlider paramResonance;
  PluginParameterLogSlider paramDrive;
  PluginParameterComboBox paramMode;
  PluginParameterComboBox paramSaturator;
  PluginParameterComboBox paramQuality;
  PluginParameterComboBox paramAdaa;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MoogLadderAudioProcessor)
};
