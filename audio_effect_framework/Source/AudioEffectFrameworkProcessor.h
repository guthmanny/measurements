/*
  ==============================================================================

    Audio effect template — DSP via minibuss Track + Processors.

  ==============================================================================
*/

#pragma once

#include <atomic>
#include <memory>
#include <vector>

#include "AefJuceIncludes.h"
#include "MinibussEffectEngine.h"
#include "TunerDetector.h"
#include "SpectrumAnalyzer.h"
#include "PluginParameter.h"

//==============================================================================

class AudioEffectFrameworkProcessor : public AudioProcessor
{
 public:
  //==============================================================================

  AudioEffectFrameworkProcessor();
  ~AudioEffectFrameworkProcessor() override;

  //==============================================================================

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;
  using AudioProcessor::processBlock;
  void processBlock(AudioSampleBuffer&, MidiBuffer&) override;

  //==============================================================================

  void getStateInformation(MemoryBlock& destData) override;
  void setStateInformation(const void* data, int sizeInBytes) override;

#ifndef JucePlugin_PreferredChannelConfigurations
  bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

  //==============================================================================

  AudioProcessorEditor* createEditor() override = 0;
  bool hasEditor() const override;

  // Plugin identity — override in subclass (uses JucePlugin_* from generated header)
  const String getName() const override;

  bool acceptsMidi() const override;
  bool producesMidi() const override;
  bool isMidiEffect() const override;
  double getTailLengthSeconds() const override;

  int getNumPrograms() override;
  int getCurrentProgram() override;
  void setCurrentProgram(int index) override;
  const String getProgramName(int index) override;
  void changeProgramName(int index, const String& newName) override;

  //==============================================================================

  PluginParametersManager parameters;

  // Shared infrastructure parameters
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

  // Settings → Peak Display
  PluginParameterLinSlider paramMeterAttack;
  PluginParameterLinSlider paramMeterRelease;
  PluginParameterComboBox paramMeterDisplayRange;

  // Footer QUALITY + Settings → Modeling (oversampling modes)
  PluginParameterComboBox paramOversampleQuality;
  PluginParameterComboBox paramUpsamplerMode;
  PluginParameterComboBox paramDownsamplerMode;

  float getMeterLevelMono() const noexcept { return meterInputMono.load(); }
  float getMeterLevelLeft() const noexcept { return meterLeft.load(); }
  float getMeterLevelRight() const noexcept { return meterRight.load(); }

  void updateMeterDisplay (float attackMs, float releaseSec, int displayRangeDbSpan, float dtSec);
  float getMeterEnvelopeMono() const noexcept { return meterDisplayEnvelopeInputMono; }
  float getMeterEnvelopeLeft() const noexcept { return meterDisplayEnvelopeLeft; }
  float getMeterEnvelopeRight() const noexcept { return meterDisplayEnvelopeRight; }
  float getMeterDisplayMonoNormalized() const noexcept { return meterDisplayInputMonoNorm; }
  float getMeterDisplayLeftNormalized() const noexcept { return meterDisplayLeftNorm; }
  float getMeterDisplayRightNormalized() const noexcept { return meterDisplayRightNorm; }

  float getInputRms() const noexcept { return inputRmsMono.load(); }
  float getCalibrationK() const noexcept { return calibrationK.load(); }
  float getCalibrationReferenceVoltage() const noexcept { return calibrationRefVoltage.load(); }
  float getMappedInputVoltage() const noexcept { return getInputRms() * getCalibrationK(); }
  void setInputRmsMeteringEnabled (bool shouldEnable) noexcept;
  bool isInputRmsMeteringEnabled() const noexcept { return inputRmsMeteringEnabled.load(); }
  bool calibrateInputFromReference (float referenceVoltage, float rms);
  void setCalibrationK (float k) noexcept;
  void setCalibrationReferenceVoltage (float volts) noexcept;
  void resetCalibration() noexcept;

  void setTunerEnabled(bool shouldEnable) noexcept;
  bool isTunerEnabled() const noexcept { return tunerEnabled.load(); }
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
  bool copySpectrumMagnitudesIfNew (uint32_t& lastFrameId, std::vector<float>& dest) const;
  int getSpectrumFftSize() const noexcept { return spectrumFftSize.load(); }
  double getSpectrumSampleRate() const noexcept { return currentSampleRate; }

  /** Override in plugin subclass to push effect-specific minibuss parameters. */
  virtual void updateCustomEffectParameters() {}

 protected:
  /** Override to supply a plugin-specific minibuss engine (e.g. with a middle processor). */
  virtual std::unique_ptr<MinibussEffectEngine> createEffectEngine();

  void ensureEffectEngine();

  MinibussEffectEngine& getMinibussEngine() noexcept { return *effectEngine_; }
  [[nodiscard]] const MinibussEffectEngine& getMinibussEngine() const noexcept { return *effectEngine_; }

  float readParameterValue(const String& paramId, float fallback) const;

 private:
  //==============================================================================

  void syncParametersFromValueTree();
  void updateEffectParameters();
  void ensureScratchBuffers(int numChannels, int numSamples);
  void mixToMonoBuffer(const AudioSampleBuffer& buffer, int numChannels, int numSamples);
  void pushTunerMono(const AudioSampleBuffer& buffer, int numChannels, int numSamples);
  void pushSpectrumMono(const AudioSampleBuffer& buffer, int numChannels, int numSamples);
  void updateInputRms (const AudioSampleBuffer& buffer, int numChannels, int numSamples);
  void processTuningOutput(int numSamples, int numInputChannels);
  void ensureTunerSampleRate();

  std::unique_ptr<MinibussEffectEngine> effectEngine_;
  AudioSampleBuffer dryBuffer;
  AudioSampleBuffer monoBuffer;
  AudioSampleBuffer processBuffer;
  double currentSampleRate = 44100.0;
  double tunerPreparedRate = 0.0;

  std::atomic<float> meterInputMono{0.0f};
  std::atomic<float> meterLeft{0.0f};
  std::atomic<float> meterRight{0.0f};

  std::atomic<float> inputRmsMono{0.0f};
  std::atomic<float> calibrationK{1.0f};
  std::atomic<float> calibrationRefVoltage{1.0f};
  std::atomic<bool> inputRmsMeteringEnabled{false};
  int inputRmsBlockCounter = 0;

  float meterDisplayEnvelopeInputMono = 0.0f;
  float meterDisplayEnvelopeLeft = 0.0f;
  float meterDisplayEnvelopeRight = 0.0f;
  float meterDisplayInputMonoNorm = 0.0f;
  float meterDisplayLeftNorm = 0.0f;
  float meterDisplayRightNorm = 0.0f;

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

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEffectFrameworkProcessor)
};
