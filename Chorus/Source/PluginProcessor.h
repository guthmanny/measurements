/*
  ==============================================================================

    Chorus / Phase90 plugin — DSP via minibuss Track + Processors.

  ==============================================================================
*/

#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>

#include "../JuceLibraryCode/JuceHeader.h"
#include "MinibussChorusEngine.h"
#include "ChorusNuDspDefaults.h"
#include "TunerDetector.h"
#include "SpectrumAnalyzer.h"
#include "PluginParameter.h"

//==============================================================================

class ChorusAudioProcessor : public AudioProcessor,
                             public ChangeBroadcaster,
                             private juce::AudioProcessorValueTreeState::Listener
{
 public:
  //==============================================================================

  ChorusAudioProcessor();
  ~ChorusAudioProcessor() override;

  //==============================================================================

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;
  using AudioProcessor::processBlock;
  void processBlock(AudioSampleBuffer&, MidiBuffer&) override;

  //==============================================================================

  void getStateInformation(MemoryBlock& destData) override;
  void setStateInformation(const void* data, int sizeInBytes) override;

  //==============================================================================

  AudioProcessorEditor* createEditor() override;
  bool hasEditor() const override;

  //==============================================================================

#ifndef JucePlugin_PreferredChannelConfigurations
  bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

  //==============================================================================

  const String getName() const override;

  bool acceptsMidi() const override;
  bool producesMidi() const override;
  bool isMidiEffect() const override;
  double getTailLengthSeconds() const override;

  //==============================================================================

  int getNumPrograms() override;
  int getCurrentProgram() override;
  void setCurrentProgram(int index) override;
  const String getProgramName(int index) override;
  void changeProgramName(int index, const String& newName) override;

  //==============================================================================

  enum EffectModel { kChorus = 0, kPhase90 = 1 };

  PluginParametersManager parameters;

  // Shared parameters (both effect models)
  PluginParameterLinSlider paramInputGain;
  PluginParameterLinSlider paramGateThreshold;
  PluginParameterLinSlider paramGateThreshMin;
  PluginParameterLinSlider paramGateThreshMax;
  PluginParameterComboBox paramGateOffAtMin;
  PluginParameterLinSlider paramGateRatio;
  PluginParameterLinSlider paramGateAttack;
  PluginParameterLinSlider paramGateRelease;
  PluginParameterComboBox paramGateKnee;
  PluginParameterLinSlider paramGateKneeWidth;
  PluginParameterLinSlider paramOutputGain;
  PluginParameterToggle paramBypass;

  // Settings → Peak Display：header meter ballistics
  PluginParameterLinSlider paramMeterAttack;
  PluginParameterLinSlider paramMeterRelease;
  PluginParameterComboBox paramMeterDisplayRange;

  // Settings → Chorus: engine limits (Rate / Delay / Amount ranges)
  PluginParameterLinSlider paramChorusMaxDelayTime;
  PluginParameterLinSlider paramChorusMinLfoFreq;
  PluginParameterLinSlider paramChorusMaxLfoFreq;
  PluginParameterLinSlider paramChorusMaxAmount;

  // Chorus model parameters (MonoChorus reflection ids)
  PluginParameterLinSlider paramChorusRate;
  PluginParameterComboBox paramChorusLfoShape;
  PluginParameterLinSlider paramChorusDelay;
  PluginParameterLinSlider paramChorusAmount;
  PluginParameterLinSlider paramChorusWet;
  PluginParameterLinSlider paramChorusFeedback;

  // Settings → MIDI CC: CC assignment for Chorus params (0=Off, 1..128 = CC0..CC127)
  PluginParameterComboBox paramMidiCcChorusRate;
  PluginParameterComboBox paramMidiCcChorusDelay;
  PluginParameterComboBox paramMidiCcChorusAmount;
  PluginParameterComboBox paramMidiCcChorusWet;
  PluginParameterComboBox paramMidiCcChorusFeedback;

  // Phase90 model parameters
  PluginParameterLogSlider paramPhase90Rate;
  PluginParameterLogSlider paramCenter;
  PluginParameterLinSlider paramPhase90Amount;
  PluginParameterLinSlider paramPhase90Feedback;
  PluginParameterLinSlider paramMix;

  float getMeterLevelMono() const noexcept { return meterInputMono.load(); }
  float getMeterLevelLeft() const noexcept { return meterLeft.load(); }
  float getMeterLevelRight() const noexcept { return meterRight.load(); }

  /** Single shared meter envelope + dB display (header + Peak Display). */
  void updateMeterDisplay (float attackMs, float releaseSec, int displayRangeDbSpan, float dtSec);
  float getMeterEnvelopeMono() const noexcept { return meterDisplayEnvelopeInputMono; }
  float getMeterEnvelopeLeft() const noexcept { return meterDisplayEnvelopeLeft; }
  float getMeterEnvelopeRight() const noexcept { return meterDisplayEnvelopeRight; }
  float getMeterDisplayMonoNormalized() const noexcept { return meterDisplayInputMonoNorm; }
  float getMeterDisplayLeftNormalized() const noexcept { return meterDisplayLeftNorm; }
  float getMeterDisplayRightNormalized() const noexcept { return meterDisplayRightNorm; }

  EffectModel getEffectModel() const noexcept { return currentModel.load(); }
  void setEffectModel(EffectModel model) noexcept;

  [[nodiscard]] ChorusNuDspLimits getChorusNuDspLimits() const noexcept;
  float getChorusMaxDelayTime() const noexcept;
  float getChorusMinLfoFreq() const noexcept;
  float getChorusMaxLfoFreq() const noexcept;
  float getChorusMaxAmount() const noexcept;
  /** Update Rate/Delay/Amount ranges; optionally reset them to NuDSP defaults. */
  void applyChorusLimits (bool resetChorusDefaults);

  void setTunerEnabled(bool shouldEnable) noexcept;
  bool isTunerEnabled() const noexcept { return tunerEnabled.load(); }
  /** True while tuner is open: FX bypassed on output to avoid monitor feedback. */
  bool isTuningModeActive() const noexcept { return tunerEnabled.load(); }
  float getTunerInputDbFs() const noexcept { return tunerInputDbFs.load(); }
  float getTunerInputDbFsRight() const noexcept { return tunerInputDbFsRight.load(); }
  double getTunerSampleRate() const noexcept { return tunerDetector.getSampleRate(); }
  TunerDetector::Result getTunerResult() const noexcept;

  void setTunerPeriodicityThreshold(float threshold) noexcept;
  float getTunerPeriodicityThreshold() const noexcept { return tunerPeriodicityThreshold.load(); }

  void setSpectrumEnabled(bool shouldEnable) noexcept;
  bool isSpectrumEnabled() const noexcept { return spectrumEnabled.load(); }
  void setSpectrumFftSize(int fftSize);
  /** Copies new spectrum data into @p dest if frame advanced since @p lastFrameId. */
  bool copySpectrumMagnitudesIfNew (uint32_t& lastFrameId, std::vector<float>& dest) const;
  int getSpectrumFftSize() const noexcept { return spectrumFftSize.load(); }
  double getSpectrumSampleRate() const noexcept { return currentSampleRate; }

 private:
  //==============================================================================

  static constexpr int maxChannels = 2;

  float readParameterValue(const String& paramId, float fallback) const;
  void syncParametersFromValueTree();
  void updateEffectParameters();
  void setParameterDomainValue (const String& paramId, float domainValue);
  void ensureScratchBuffers(int numChannels, int numSamples);
  void mixToMonoBuffer(const AudioSampleBuffer& buffer, int numChannels, int numSamples);
  void pushTunerMono(const AudioSampleBuffer& buffer, int numChannels, int numSamples);
  void pushSpectrumMono(const AudioSampleBuffer& buffer, int numChannels, int numSamples);
  void processTuningOutput(int numSamples, int numInputChannels);
  void ensureTunerSampleRate();
  void handleIncomingMidi (const MidiBuffer& midiMessages);
  void parameterChanged (const String& parameterID, float newValue) override;
  void applyMidiCcToParameter (PluginParameterLinSlider& target, float controllerNormalized);
  void applyMidiCcToChorusParameter (const String& paramId,
                                     std::string_view minibussParamId,
                                     float minDomain,
                                     float maxDomain,
                                     float controllerNormalized);
  void syncChorusLfoShapeToEngine();

  /** Choice index 0 = Off; 1..128 = CC 0..127. Returns -1 if Off. */
  static int midiCcChoiceToNumber (float choiceValue) noexcept;

  MinibussChorusEngine minibussEngine;
  AudioSampleBuffer dryBuffer;
  AudioSampleBuffer monoBuffer;
  AudioSampleBuffer processBuffer;
  double currentSampleRate = 44100.0;
  double tunerPreparedRate = 0.0;

  std::atomic<float> meterInputMono{0.0f};
  std::atomic<float> meterLeft{0.0f};
  std::atomic<float> meterRight{0.0f};

  float meterDisplayEnvelopeInputMono = 0.0f;
  float meterDisplayEnvelopeLeft = 0.0f;
  float meterDisplayEnvelopeRight = 0.0f;
  float meterDisplayInputMonoNorm = 0.0f;
  float meterDisplayLeftNorm = 0.0f;
  float meterDisplayRightNorm = 0.0f;

  std::atomic<EffectModel> currentModel{kChorus};

  std::atomic<bool> tunerEnabled{false};
  std::atomic<float> tunerPeriodicityThreshold{0.7f};
  std::atomic<float> tunerInputDbFs{-100.0f};
  std::atomic<float> tunerInputDbFsRight{-100.0f};
  TunerDetector tunerDetector;
  mutable juce::SpinLock tunerLock;
  TunerDetector::Result tunerResult{};

  std::atomic<bool> spectrumEnabled{false};
  std::atomic<int> spectrumFftSize{1 << SpectrumAnalyzer::defaultFftOrder};
  SpectrumAnalyzer spectrumAnalyzer;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChorusAudioProcessor)
};
