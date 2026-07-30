/*
  ==============================================================================

    Chorus / Phase90 plugin — DSP via minibuss Track + Processors.

  ==============================================================================
*/

#include "PluginProcessor.h"

#include <algorithm>
#include <cmath>

#include "PluginEditor.h"
#include "MeterDisplayUtils.h"
#include "PluginParameter.h"

namespace
{
juce::StringArray makeMidiCcChoiceList()
{
  juce::StringArray items;
  items.add ("Off");
  for (int cc = 0; cc <= 127; ++cc)
    items.add ("CC " + juce::String (cc));
  return items;
}
} // namespace

//==============================================================================

ChorusAudioProcessor::ChorusAudioProcessor()
    :
#ifndef JucePlugin_PreferredChannelConfigurations
      AudioProcessor(BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
                         .withInput("Input", AudioChannelSet::stereo(), true)
#endif
                         .withOutput("Output", AudioChannelSet::stereo(), true)
#endif
                         ),
#endif
      parameters(*this),
      paramInputGain(parameters, "Input Gain", "dB", -12.0f, 12.0f, 0.0f),
      paramGateThreshold(parameters, "Gate Threshold", "dB", -80.0f, 0.0f, -40.0f),
      paramGateThreshMin(parameters, "Gate Thresh Min", "dB", -80.0f, 0.0f, -80.0f),
      paramGateThreshMax(parameters, "Gate Thresh Max", "dB", -80.0f, 0.0f, 0.0f),
      paramGateOffAtMin(parameters, "Gate Off At Min", {"Off", "On"}, 0),
      paramGateRatio(parameters, "Gate Ratio", "", 1.0f, 10.0f, 4.0f),
      paramGateAttack(parameters, "Gate Attack", "ms", 5.0f, 100.0f, 5.0f),
      paramGateRelease(parameters, "Gate Release", "ms", 100.0f, 10000.0f, 100.0f),
      paramGateKnee(parameters, "Gate Knee", {"Hard", "Soft"}, 0),
      paramGateKneeWidth(parameters, "Gate Knee Width", "dB", 0.1f, 24.0f, 6.0f),
      paramOutputGain(parameters, "Output Gain", "dB", -100.0f, 0.0f, 0.0f),
      paramBypass(parameters, "Bypass", false),
      paramMeterAttack(parameters, "Meter Attack", "ms", 0.0f, 10.0f, 0.0f),
      paramMeterRelease(parameters, "Meter Release", "s", 0.5f, 10.0f, 1.0f),
      paramMeterDisplayRange(parameters,
                             "Meter Display Range",
                             {"20 dB", "40 dB", "60 dB", "80 dB", "100 dB", "120 dB"},
                             3),
      paramChorusMaxDelayTime(parameters,
                              "Chorus Max Delay Time",
                              "ms",
                              queryChorusNuDspLimits().delayMin,
                              queryChorusNuDspLimits().delayMax,
                              queryChorusNuDspLimits().delayMax),
      paramChorusMinLfoFreq(parameters,
                            "Chorus Min Lfo Freq",
                            "Hz",
                            queryChorusNuDspLimits().rateMin,
                            1.0f,
                            queryChorusNuDspLimits().rateMin),
      paramChorusMaxLfoFreq(parameters,
                            "Chorus Max Lfo Freq",
                            "Hz",
                            queryChorusNuDspLimits().rateMin,
                            queryChorusNuDspLimits().rateMax,
                            queryChorusNuDspLimits().rateMax),
      paramChorusMaxAmount(parameters,
                           "Chorus Max Amount",
                           "ms",
                           queryChorusNuDspLimits().amountMin,
                           queryChorusNuDspLimits().amountMax,
                           queryChorusNuDspLimits().amountMax),
      // Chorus params (domain aligned with MonoChorus: rate/delay/amount/coeff_fb/wet)
      paramChorusRate(parameters,
                      "Chorus Rate",
                      "Hz",
                      queryChorusNuDspLimits().rateMin,
                      queryChorusNuDspLimits().rateMax,
                      queryChorusNuDspLimits().rateDefault),
      paramChorusLfoShape(parameters, "LFO SHAPE", {"Sine", "Triangle"}, 0),
      paramChorusDelay(parameters,
                       "Chorus Delay",
                       "ms",
                       queryChorusNuDspLimits().delayMin,
                       queryChorusNuDspLimits().delayMax,
                       queryChorusNuDspLimits().delayDefault),
      paramChorusAmount(parameters,
                        "Chorus Amount",
                        "ms",
                        queryChorusNuDspLimits().amountMin,
                        queryChorusNuDspLimits().amountMax,
                        queryChorusNuDspLimits().amountDefault),
      paramChorusWet(parameters, "Chorus Wet", "", 0.0f, 1.0f, 0.5f),
      paramChorusFeedback(parameters, "Chorus Feedback", "", -1.0f, 1.0f, 0.15f),
      paramMidiCcChorusRate(parameters, "MIDI CC Chorus Rate", makeMidiCcChoiceList(), 0),
      paramMidiCcChorusDelay(parameters, "MIDI CC Chorus Delay", makeMidiCcChoiceList(), 0),
      paramMidiCcChorusAmount(parameters, "MIDI CC Chorus Amount", makeMidiCcChoiceList(), 0),
      paramMidiCcChorusWet(parameters, "MIDI CC Chorus Wet", makeMidiCcChoiceList(), 0),
      paramMidiCcChorusFeedback(parameters, "MIDI CC Chorus Feedback", makeMidiCcChoiceList(), 0),
      // Phase90 params
      paramPhase90Rate(parameters, "Phase90 Rate", "Hz", 0.1f, 20.0f, 1.0f),
      paramCenter(parameters, "Center", "Hz", 20.0f, 10000.0f, 1000.0f),
      paramPhase90Amount(parameters, "Phase90 Amount", "oct", 0.0f, 4.0f, 1.0f),
      paramPhase90Feedback(parameters, "Phase90 Feedback", "", 0.0f, 0.95f, 0.7f),
      paramMix(parameters, "Mix", "", 0.0f, 1.0f, 0.5f)
{
  parameters.valueTreeState.state = ValueTree(Identifier(getName().removeCharacters("- ")));
  parameters.valueTreeState.addParameterListener(paramChorusLfoShape.paramID, this);
  applyChorusLimits (false);
}

ChorusAudioProcessor::~ChorusAudioProcessor()
{
  parameters.valueTreeState.removeParameterListener(paramChorusLfoShape.paramID, this);
  spectrumEnabled.store(false);
  spectrumAnalyzer.stopAnalysis();
  minibussEngine.release();
}

void ChorusAudioProcessor::setEffectModel(EffectModel model) noexcept
{
  currentModel.store(model);
  minibussEngine.setEffectModel(model == kPhase90 ? MinibussChorusEngine::EffectModel::Phase90
                                                  : MinibussChorusEngine::EffectModel::Chorus);
}

void ChorusAudioProcessor::setTunerEnabled(bool shouldEnable) noexcept
{
  tunerEnabled.store(shouldEnable);

  if (shouldEnable)
  {
    tunerDetector.reset();
    tunerInputDbFs.store(-100.0f);
    tunerInputDbFsRight.store(-100.0f);
    const juce::SpinLock::ScopedLockType lock(tunerLock);
    tunerResult = {};
  }
}

TunerDetector::Result ChorusAudioProcessor::getTunerResult() const noexcept
{
  const juce::SpinLock::ScopedLockType lock(tunerLock);
  return tunerResult;
}

void ChorusAudioProcessor::setTunerPeriodicityThreshold(float threshold) noexcept
{
  const float clamped = juce::jlimit(0.0f, 1.0f, threshold);
  tunerPeriodicityThreshold.store(clamped);
  tunerDetector.setPeriodicityThreshold(clamped);
}

void ChorusAudioProcessor::setSpectrumEnabled(bool shouldEnable) noexcept
{
  if (shouldEnable)
  {
    spectrumAnalyzer.ensureReady();
    spectrumAnalyzer.reset();
    spectrumFftSize.store(spectrumAnalyzer.getFftSize());
    spectrumAnalyzer.startAnalysis();
    spectrumEnabled.store(true);
  }
  else
  {
    spectrumEnabled.store(false);
    spectrumAnalyzer.stopAnalysis();
  }
}

void ChorusAudioProcessor::setSpectrumFftSize(int fftSize)
{
  const bool wasEnabled = spectrumEnabled.exchange(false);
  spectrumAnalyzer.stopAnalysis();

  spectrumAnalyzer.setFftSize(fftSize);
  spectrumAnalyzer.ensureReady();
  spectrumAnalyzer.reset();
  spectrumFftSize.store(spectrumAnalyzer.getFftSize());

  if (wasEnabled)
  {
    spectrumAnalyzer.startAnalysis();
    spectrumEnabled.store(true);
  }
}

bool ChorusAudioProcessor::copySpectrumMagnitudesIfNew(uint32_t& lastFrameId, std::vector<float>& dest) const
{
  return spectrumAnalyzer.copyMagnitudesIfNew(lastFrameId, dest);
}

//==============================================================================

float ChorusAudioProcessor::readParameterValue(const String& paramId, float fallback) const
{
  if (auto* param = parameters.valueTreeState.getParameter(paramId)) return param->convertFrom0to1(param->getValue());

  if (auto* value = parameters.valueTreeState.getRawParameterValue(paramId)) return value->load();

  return fallback;
}

void ChorusAudioProcessor::syncChorusLfoShapeToEngine()
{
  if (! minibussEngine.isReady())
    return;

  float normalized = (float) paramChorusLfoShape.defaultChoice;
  if (auto* param = parameters.valueTreeState.getParameter (paramChorusLfoShape.paramID))
    normalized = param->getValue() >= 0.5f ? 1.0f : 0.0f;

  minibussEngine.setParamNormalized (minibussEngine.chorusId(), "lfo_shape", normalized);
}

void ChorusAudioProcessor::parameterChanged (const String& parameterID, float /*newValue*/)
{
  if (parameterID == paramChorusLfoShape.paramID)
    syncChorusLfoShapeToEngine();
}

ChorusNuDspLimits ChorusAudioProcessor::getChorusNuDspLimits() const noexcept
{
  return queryChorusNuDspLimits();
}

float ChorusAudioProcessor::getChorusMaxDelayTime() const noexcept
{
  const auto limits = getChorusNuDspLimits();
  return jlimit (limits.delayMin,
                 limits.delayMax,
                 readParameterValue (paramChorusMaxDelayTime.paramID, limits.delayMax));
}

float ChorusAudioProcessor::getChorusMinLfoFreq() const noexcept
{
  const auto limits = getChorusNuDspLimits();
  return jlimit (limits.rateMin,
                 1.0f,
                 readParameterValue (paramChorusMinLfoFreq.paramID, limits.rateMin));
}

float ChorusAudioProcessor::getChorusMaxLfoFreq() const noexcept
{
  const auto limits = getChorusNuDspLimits();
  return jlimit (limits.rateMin,
                 limits.rateMax,
                 readParameterValue (paramChorusMaxLfoFreq.paramID, limits.rateMax));
}

float ChorusAudioProcessor::getChorusMaxAmount() const noexcept
{
  const auto limits = getChorusNuDspLimits();
  return jlimit (limits.amountMin,
                 limits.amountMax,
                 readParameterValue (paramChorusMaxAmount.paramID, limits.amountMax));
}

void ChorusAudioProcessor::setParameterDomainValue (const String& paramId, float domainValue)
{
  if (auto* param = parameters.valueTreeState.getParameter (paramId))
    param->setValueNotifyingHost (param->convertTo0to1 (domainValue));
}

void ChorusAudioProcessor::applyChorusLimits (const bool resetChorusDefaults)
{
  const auto limits = getChorusNuDspLimits();
  const float maxDelay = getChorusMaxDelayTime();
  const float minRate = getChorusMinLfoFreq();
  const float maxRate = getChorusMaxLfoFreq();
  const float maxAmount = getChorusMaxAmount();
  const float effectiveMinRate = juce::jmin (minRate, maxRate);
  const float effectiveMaxRate = juce::jmax (minRate, maxRate);

  if (resetChorusDefaults)
  {
    const float rateDefault = jlimit (effectiveMinRate, effectiveMaxRate, limits.rateDefault);
    const float delayDefault = jlimit (limits.delayMin, maxDelay, limits.delayDefault);
    const float amountDefault = jlimit (limits.amountMin, maxAmount, limits.amountDefault);
    setParameterDomainValue (paramChorusRate.paramID, rateDefault);
    setParameterDomainValue (paramChorusDelay.paramID, delayDefault);
    setParameterDomainValue (paramChorusAmount.paramID, amountDefault);
  }
  else
  {
    const float currentRate = readParameterValue (paramChorusRate.paramID, limits.rateDefault);
    const float currentDelay = readParameterValue (paramChorusDelay.paramID, limits.delayDefault);
    const float currentAmount = readParameterValue (paramChorusAmount.paramID, limits.amountDefault);
    setParameterDomainValue (paramChorusRate.paramID, jlimit (effectiveMinRate, effectiveMaxRate, currentRate));
    setParameterDomainValue (paramChorusDelay.paramID, jlimit (limits.delayMin, maxDelay, currentDelay));
    setParameterDomainValue (paramChorusAmount.paramID, jlimit (limits.amountMin, maxAmount, currentAmount));
  }

  syncParametersFromValueTree();
  sendChangeMessage();
}

void ChorusAudioProcessor::syncParametersFromValueTree()
{
  paramInputGain.setCurrentAndTargetValue(readParameterValue(paramInputGain.paramID, paramInputGain.defaultValue));
  paramGateThreshold.setCurrentAndTargetValue(
      readParameterValue(paramGateThreshold.paramID, paramGateThreshold.defaultValue));
  paramGateThreshMin.setCurrentAndTargetValue(
      readParameterValue(paramGateThreshMin.paramID, paramGateThreshMin.defaultValue));
  paramGateThreshMax.setCurrentAndTargetValue(
      readParameterValue(paramGateThreshMax.paramID, paramGateThreshMax.defaultValue));
  paramGateOffAtMin.setCurrentAndTargetValue(
      readParameterValue(paramGateOffAtMin.paramID, (float)paramGateOffAtMin.defaultChoice));
  paramGateRatio.setCurrentAndTargetValue(readParameterValue(paramGateRatio.paramID, paramGateRatio.defaultValue));
  paramGateAttack.setCurrentAndTargetValue(readParameterValue(paramGateAttack.paramID, paramGateAttack.defaultValue));
  paramGateRelease.setCurrentAndTargetValue(
      readParameterValue(paramGateRelease.paramID, paramGateRelease.defaultValue));
  paramGateKnee.setCurrentAndTargetValue(readParameterValue(paramGateKnee.paramID, (float)paramGateKnee.defaultChoice));
  paramGateKneeWidth.setCurrentAndTargetValue(
      readParameterValue(paramGateKneeWidth.paramID, paramGateKneeWidth.defaultValue));
  paramOutputGain.setCurrentAndTargetValue(readParameterValue(paramOutputGain.paramID, paramOutputGain.defaultValue));
  // Chorus
  paramChorusRate.setCurrentAndTargetValue(readParameterValue(paramChorusRate.paramID, paramChorusRate.defaultValue));
  paramChorusDelay.setCurrentAndTargetValue(readParameterValue(paramChorusDelay.paramID, paramChorusDelay.defaultValue));
  paramChorusAmount.setCurrentAndTargetValue(
      readParameterValue(paramChorusAmount.paramID, paramChorusAmount.defaultValue));
  paramChorusWet.setCurrentAndTargetValue(readParameterValue(paramChorusWet.paramID, paramChorusWet.defaultValue));
  paramChorusFeedback.setCurrentAndTargetValue(
      readParameterValue(paramChorusFeedback.paramID, paramChorusFeedback.defaultValue));
  paramChorusLfoShape.setCurrentAndTargetValue(
      readParameterValue(paramChorusLfoShape.paramID, (float)paramChorusLfoShape.defaultChoice));
  // Phase90
  paramPhase90Rate.setCurrentAndTargetValue(
      readParameterValue(paramPhase90Rate.paramID, paramPhase90Rate.defaultValue));
  paramCenter.setCurrentAndTargetValue(readParameterValue(paramCenter.paramID, paramCenter.defaultValue));
  paramPhase90Amount.setCurrentAndTargetValue(
      readParameterValue(paramPhase90Amount.paramID, paramPhase90Amount.defaultValue));
  paramPhase90Feedback.setCurrentAndTargetValue(
      readParameterValue(paramPhase90Feedback.paramID, paramPhase90Feedback.defaultValue));
  paramMix.setCurrentAndTargetValue(readParameterValue(paramMix.paramID, paramMix.defaultValue));
  paramBypass.setCurrentAndTargetValue(readParameterValue(paramBypass.paramID, (float)paramBypass.defaultState));
}

void ChorusAudioProcessor::ensureScratchBuffers(int numChannels, int numSamples)
{
  const int channels = jmax(1, numChannels);
  const int samples = jmax(1, numSamples);

  if (dryBuffer.getNumChannels() < channels || dryBuffer.getNumSamples() < samples)
    dryBuffer.setSize(channels, samples, false, false, true);

  if (monoBuffer.getNumChannels() < 1 || monoBuffer.getNumSamples() < samples)
    monoBuffer.setSize(1, samples, false, false, true);

  if (processBuffer.getNumChannels() < channels || processBuffer.getNumSamples() < samples)
    processBuffer.setSize(channels, samples, false, false, true);
}

void ChorusAudioProcessor::updateEffectParameters()
{
  const bool userBypass = readParameterValue(paramBypass.paramID, (float)paramBypass.defaultState) >= 0.5f;
  minibussEngine.setBypass(userBypass);

  minibussEngine.setParamDomain(minibussEngine.gainId(), "gain",
                                readParameterValue(paramInputGain.paramID, paramInputGain.defaultValue));

  float threshMinDb = readParameterValue(paramGateThreshMin.paramID, paramGateThreshMin.defaultValue);
  float threshMaxDb = readParameterValue(paramGateThreshMax.paramID, paramGateThreshMax.defaultValue);
  if (threshMinDb > threshMaxDb)
    std::swap(threshMinDb, threshMaxDb);
  minibussEngine.setParamDomain(minibussEngine.gateId(), "thresh_min", threshMinDb);
  minibussEngine.setParamDomain(minibussEngine.gateId(), "thresh_max", threshMaxDb);

  const float thresholdDb =
      jlimit(threshMinDb, threshMaxDb,
             readParameterValue(paramGateThreshold.paramID, paramGateThreshold.defaultValue));
  minibussEngine.setParamDomain(minibussEngine.gateId(), "threshold", thresholdDb);
  minibussEngine.setParamDomain(
      minibussEngine.gateId(), "off_at_min",
      readParameterValue(paramGateOffAtMin.paramID, (float)paramGateOffAtMin.defaultChoice) >= 0.5f ? 1.f : 0.f);
  minibussEngine.setParamDomain(minibussEngine.gateId(), "ratio",
                                readParameterValue(paramGateRatio.paramID, paramGateRatio.defaultValue));
  minibussEngine.setParamDomain(minibussEngine.gateId(), "attack",
                                readParameterValue(paramGateAttack.paramID, paramGateAttack.defaultValue));
  minibussEngine.setParamDomain(minibussEngine.gateId(), "release",
                                readParameterValue(paramGateRelease.paramID, paramGateRelease.defaultValue));
  minibussEngine.setParamDomain(
      minibussEngine.gateId(), "knee_mode",
      readParameterValue(paramGateKnee.paramID, (float)paramGateKnee.defaultChoice) >= 0.5f ? 1.f : 0.f);
  minibussEngine.setParamDomain(minibussEngine.gateId(), "knee_width",
                                readParameterValue(paramGateKneeWidth.paramID, paramGateKneeWidth.defaultValue));

  minibussEngine.setParamDomain(minibussEngine.levelId(), "level",
                                readParameterValue(paramOutputGain.paramID, paramOutputGain.defaultValue));

  const auto chorusLimits = getChorusNuDspLimits();
  const float chorusRate = jlimit (getChorusMinLfoFreq(),
                                   getChorusMaxLfoFreq(),
                                   readParameterValue (paramChorusRate.paramID, chorusLimits.rateDefault));
  const float chorusDelay = jlimit (chorusLimits.delayMin,
                                    getChorusMaxDelayTime(),
                                    readParameterValue (paramChorusDelay.paramID, chorusLimits.delayDefault));
  const float chorusAmount = jlimit (chorusLimits.amountMin,
                                     getChorusMaxAmount(),
                                     readParameterValue (paramChorusAmount.paramID, chorusLimits.amountDefault));

  minibussEngine.setParamDomain(minibussEngine.chorusId(), "rate", chorusRate);
  minibussEngine.setParamDomain(minibussEngine.chorusId(), "delay", chorusDelay);
  minibussEngine.setParamDomain(minibussEngine.chorusId(), "amount", chorusAmount);
  minibussEngine.setParamDomain(minibussEngine.chorusId(), "coeff_fb",
                                readParameterValue(paramChorusFeedback.paramID, paramChorusFeedback.defaultValue));
  minibussEngine.setParamDomain(minibussEngine.chorusId(), "wet",
                                readParameterValue(paramChorusWet.paramID, paramChorusWet.defaultValue));
  syncChorusLfoShapeToEngine();

  minibussEngine.setParamDomain(minibussEngine.phase90Id(), "rate",
                                readParameterValue(paramPhase90Rate.paramID, paramPhase90Rate.defaultValue));
  minibussEngine.setParamDomain(minibussEngine.phase90Id(), "center",
                                readParameterValue(paramCenter.paramID, paramCenter.defaultValue));
  minibussEngine.setParamDomain(minibussEngine.phase90Id(), "amount",
                                readParameterValue(paramPhase90Amount.paramID, paramPhase90Amount.defaultValue));
  minibussEngine.setParamDomain(minibussEngine.phase90Id(), "feedback",
                                readParameterValue(paramPhase90Feedback.paramID, paramPhase90Feedback.defaultValue));
  minibussEngine.setParamDomain(minibussEngine.phase90Id(), "mix",
                                readParameterValue(paramMix.paramID, paramMix.defaultValue));

  minibussEngine.setEffectModel(currentModel.load() == kPhase90 ? MinibussChorusEngine::EffectModel::Phase90
                                                                : MinibussChorusEngine::EffectModel::Chorus);
}

void ChorusAudioProcessor::mixToMonoBuffer(const AudioSampleBuffer& buffer, int numChannels, int numSamples)
{
  if (numChannels >= 2)
  {
    const float* left = buffer.getReadPointer(0);
    const float* right = buffer.getReadPointer(1);
    for (int i = 0; i < numSamples; ++i)
      monoBuffer.setSample(0, i, 0.5f * (left[i] + right[i]));
  }
  else
  {
    monoBuffer.copyFrom(0, 0, buffer, 0, 0, numSamples);
  }
}

void ChorusAudioProcessor::ensureTunerSampleRate()
{
  const double sr = getSampleRate() > 0.0 ? getSampleRate() : currentSampleRate;
  if (sr <= 0.0)
    return;

  if (std::abs(sr - tunerPreparedRate) <= 0.5)
    return;

  tunerDetector.prepare(sr, 80.0f, 1200.0f);
  tunerPreparedRate = sr;
  currentSampleRate = sr;
  tunerDetector.setPeriodicityThreshold(tunerPeriodicityThreshold.load());
}

void ChorusAudioProcessor::setInputRmsMeteringEnabled (bool shouldEnable) noexcept
{
  inputRmsMeteringEnabled.store (shouldEnable);
  if (! shouldEnable)
  {
    inputRmsMono.store (0.0f);
    inputRmsBlockCounter = 0;
  }
}

void ChorusAudioProcessor::updateInputRms (const AudioSampleBuffer& buffer, int numChannels, int numSamples)
{
  // Keep the audio callback cheap: only measure while the calibration page is open.
  if (! inputRmsMeteringEnabled.load (std::memory_order_relaxed))
    return;

  if (numChannels <= 0 || numSamples <= 0)
  {
    inputRmsMono.store (0.0f, std::memory_order_relaxed);
    return;
  }

  // Decimate: ~once every 16 blocks is enough for a 350 ms UI smoother.
  if ((++inputRmsBlockCounter & 15) != 0)
    return;

  // Light subsampled mean-square (avoids a full-buffer getRMSLevel sweep).
  auto meanSquare = [numSamples] (const float* data) noexcept -> float
  {
    if (data == nullptr || numSamples <= 0)
      return 0.0f;

    const int step = juce::jmax (1, numSamples / 64);
    float sum = 0.0f;
    int count = 0;
    for (int i = 0; i < numSamples; i += step)
    {
      const float s = data[i];
      sum += s * s;
      ++count;
    }
    return count > 0 ? sum / (float) count : 0.0f;
  };

  float ms = meanSquare (buffer.getReadPointer (0));
  if (numChannels > 1)
    ms = 0.5f * (ms + meanSquare (buffer.getReadPointer (1)));

  inputRmsMono.store (std::sqrt (juce::jmax (0.0f, ms)), std::memory_order_relaxed);
}

bool ChorusAudioProcessor::calibrateInputFromReference (float referenceVoltage, float rms)
{
  const float volts = juce::jmax (0.0f, referenceVoltage);
  calibrationRefVoltage.store (volts);

  constexpr float kMinRms = 1.0e-6f;
  if (rms < kMinRms)
    return false;

  calibrationK.store (volts / rms);
  return true;
}

void ChorusAudioProcessor::setCalibrationK (float k) noexcept
{
  calibrationK.store (juce::jmax (0.0f, k));
}

void ChorusAudioProcessor::setCalibrationReferenceVoltage (float volts) noexcept
{
  calibrationRefVoltage.store (juce::jmax (0.0f, volts));
}

void ChorusAudioProcessor::resetCalibration() noexcept
{
  calibrationK.store (1.0f);
}

void ChorusAudioProcessor::pushTunerMono(const AudioSampleBuffer& buffer, int numChannels, int numSamples)
{
  if (!tunerEnabled.load())
    return;

  auto clearTunerResult = [this]
  {
    tunerDetector.invalidate();
    tunerInputDbFs.store(-100.0f);
    tunerInputDbFsRight.store(-100.0f);
    const juce::SpinLock::ScopedLockType lock(tunerLock);
    tunerResult = {};
  };

  if (numChannels <= 0 || numSamples <= 0)
  {
    clearTunerResult();
    return;
  }

  ensureTunerSampleRate();

  constexpr float kMinTunerRms = 1.0e-5f;
  const float rmsLeft = buffer.getRMSLevel(0, 0, numSamples);
  const float rmsRight = numChannels > 1 ? buffer.getRMSLevel(1, 0, numSamples) : 0.0f;
  tunerInputDbFs.store(Decibels::gainToDecibels(rmsLeft, -100.0f));
  tunerInputDbFsRight.store(numChannels > 1 ? Decibels::gainToDecibels(rmsRight, -100.0f) : -100.0f);

  const bool leftActive = rmsLeft >= kMinTunerRms;
  const bool rightActive = numChannels > 1 && rmsRight >= kMinTunerRms;
  if (!leftActive && !rightActive)
  {
    clearTunerResult();
    return;
  }

  const float* leftSamples = leftActive ? buffer.getReadPointer(0) : nullptr;
  const float* rightSamples =
      rightActive && numChannels > 1 ? buffer.getReadPointer(1) : nullptr;
  tunerDetector.process(leftSamples, rightSamples, numChannels, numSamples);

  const juce::SpinLock::ScopedLockType lock(tunerLock);
  tunerResult = tunerDetector.getResult();
}

void ChorusAudioProcessor::pushSpectrumMono(const AudioSampleBuffer& buffer, int numChannels, int numSamples)
{
  if (!spectrumEnabled.load() || numChannels <= 0 || numSamples <= 0)
    return;

  mixToMonoBuffer(buffer, numChannels, numSamples);
  spectrumAnalyzer.pushSamples(monoBuffer.getReadPointer(0), numSamples);
}

void ChorusAudioProcessor::processTuningOutput(const int numSamples, const int numInputChannels)
{
  juce::ignoreUnused (numSamples);

  // Output stays silent (dryBuffer already cleared) to break monitor/feedback loops.
  // Tuner reads raw host input; use interface direct monitoring to hear the instrument.
  const float inputGain =
      Decibels::decibelsToGain(readParameterValue(paramInputGain.paramID, paramInputGain.defaultValue));
  const float postGainLeft = processBuffer.getMagnitude(0, 0, numSamples) * inputGain;
  const float postGainRight =
      numInputChannels > 1 && processBuffer.getNumChannels() > 1
          ? processBuffer.getMagnitude(1, 0, numSamples) * inputGain
          : postGainLeft;
  const float postGainMono =
      numInputChannels > 1 ? 0.5f * (postGainLeft + postGainRight) : postGainLeft;
  meterInputMono.store(postGainMono);
}

void ChorusAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
  currentSampleRate = sampleRate;

  const double smoothTime = 1e-3;
  paramInputGain.reset(sampleRate, smoothTime);
  paramGateThreshold.reset(sampleRate, smoothTime);
  paramGateThreshMin.reset(sampleRate, smoothTime);
  paramGateThreshMax.reset(sampleRate, smoothTime);
  paramGateOffAtMin.reset(sampleRate, smoothTime);
  paramGateRatio.reset(sampleRate, smoothTime);
  paramGateAttack.reset(sampleRate, smoothTime);
  paramGateRelease.reset(sampleRate, smoothTime);
  paramGateKnee.reset(sampleRate, smoothTime);
  paramGateKneeWidth.reset(sampleRate, smoothTime);
  paramOutputGain.reset(sampleRate, smoothTime);
  paramChorusRate.reset(sampleRate, smoothTime);
  paramChorusDelay.reset(sampleRate, smoothTime);
  paramChorusAmount.reset(sampleRate, smoothTime);
  paramChorusWet.reset(sampleRate, smoothTime);
  paramChorusFeedback.reset(sampleRate, smoothTime);
  paramChorusLfoShape.reset(sampleRate, smoothTime);
  paramPhase90Rate.reset(sampleRate, smoothTime);
  paramCenter.reset(sampleRate, smoothTime);
  paramPhase90Amount.reset(sampleRate, smoothTime);
  paramPhase90Feedback.reset(sampleRate, smoothTime);
  paramMix.reset(sampleRate, smoothTime);
  paramBypass.reset(sampleRate, smoothTime);

  syncParametersFromValueTree();

  tunerDetector.prepare(sampleRate, 80.0f, 1200.0f);
  tunerPreparedRate = sampleRate;
  tunerDetector.setPeriodicityThreshold(tunerPeriodicityThreshold.load());
  {
    const juce::SpinLock::ScopedLockType lock(tunerLock);
    tunerResult = {};
  }

  spectrumAnalyzer.setSampleRate(sampleRate);
  if (spectrumEnabled.load())
  {
    spectrumAnalyzer.ensureReady();
    spectrumAnalyzer.reset();
    spectrumAnalyzer.startAnalysis();
  }

  const int numChannels = jmax(1, jmax(getTotalNumInputChannels(), getTotalNumOutputChannels()));
  ensureScratchBuffers(numChannels, samplesPerBlock);

  minibussEngine.prepare((float)sampleRate, (std::uint32_t)jmax(1, samplesPerBlock));
  updateEffectParameters();
}

void ChorusAudioProcessor::releaseResources()
{
  if (!spectrumEnabled.load()) spectrumAnalyzer.stopAnalysis();
  // Keep scratch buffers allocated: JACK/PipeWire may restart the device while
  // the audio callback is still draining.
}

void ChorusAudioProcessor::processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
  ScopedNoDenormals noDenormals;

  const int numInputChannels = getTotalNumInputChannels();
  const int numOutputChannels = getTotalNumOutputChannels();
  const int numSamples = buffer.getNumSamples();

  if (numSamples == 0) return;

  const int bufCh = jmax(1, jmax(numInputChannels, numOutputChannels));
  ensureScratchBuffers(bufCh, numSamples);

  // Capture host input for tuner / calibration before anything else touches the block.
  updateInputRms (buffer, numInputChannels, numSamples);
  pushTunerMono(buffer, numInputChannels, numSamples);

  handleIncomingMidi (midiMessages);

  updateEffectParameters();

  // Separate in/out buffers — do not process in-place through minibuss.
  constexpr int engCh = 2;
  for (int ch = 0; ch < engCh; ++ch)
  {
    const int src = jmin(ch, jmax(0, numInputChannels - 1));
    if (numInputChannels > 0)
      processBuffer.copyFrom(ch, 0, buffer, src, 0, numSamples);
    else
      processBuffer.clear(ch, 0, numSamples);
  }

  // Keep dryBuffer as a distinct output destination from processBuffer inputs.
  for (int ch = 0; ch < engCh; ++ch)
    dryBuffer.clear(ch, 0, numSamples);

  const float* inPtrs[2] = { processBuffer.getReadPointer(0), processBuffer.getReadPointer(1) };
  float* outPtrs[2] = { dryBuffer.getWritePointer(0), dryBuffer.getWritePointer(1) };

  if (minibussEngine.isReady())
  {
    if (tunerEnabled.load())
      processTuningOutput(numSamples, numInputChannels);
    else
    {
      minibussEngine.process(inPtrs, outPtrs, (std::uint32_t) numSamples);

      float postGainLeft = 0.0f, postGainRight = 0.0f;
      minibussEngine.readPostGainPeaks(postGainLeft, postGainRight);
      const float postGainMono =
          numInputChannels > 1 ? 0.5f * (postGainLeft + postGainRight) : postGainLeft;
      meterInputMono.store(postGainMono);
    }
  }
  else
  {
    meterInputMono.store(0.0f);
  }

  const int procChannels = jmin(numOutputChannels, engCh);
  for (int ch = 0; ch < procChannels; ++ch)
    buffer.copyFrom(ch, 0, dryBuffer, ch, 0, numSamples);

  for (int ch = procChannels; ch < numOutputChannels; ++ch)
    buffer.clear(ch, 0, numSamples);

  pushSpectrumMono(dryBuffer, procChannels, numSamples);

  float monoPeak = 0.0f, leftPeak = 0.0f, rightPeak = 0.0f;
  if (numOutputChannels > 0) leftPeak = buffer.getMagnitude(0, 0, numSamples);
  if (numOutputChannels > 1) rightPeak = buffer.getMagnitude(1, 0, numSamples);
  monoPeak = numOutputChannels > 1 ? 0.5f * (leftPeak + rightPeak) : leftPeak;

  meterLeft.store(leftPeak);
  meterRight.store(rightPeak);
  juce::ignoreUnused(monoPeak);
}

void ChorusAudioProcessor::updateMeterDisplay (float attackMs, float releaseSec, int displayRangeDbSpan, float dtSec)
{
  attackMs = juce::jlimit (0.0f, 10.0f, attackMs);
  releaseSec = juce::jlimit (0.5f, 10.0f, releaseSec);
  displayRangeDbSpan = juce::jmax (1, displayRangeDbSpan);
  dtSec = juce::jmax (1.0e-6f, dtSec);

  const float inputMono = meterInputMono.load();
  const float left = meterLeft.load();
  const float right = meterRight.load();
  const float dbMin = -(float) displayRangeDbSpan;

  meterDisplayEnvelopeInputMono =
      meter_display::stepEnvelope (meterDisplayEnvelopeInputMono, inputMono, attackMs, releaseSec, dtSec);
  meterDisplayEnvelopeLeft =
      meter_display::stepEnvelope (meterDisplayEnvelopeLeft, left, attackMs, releaseSec, dtSec);
  meterDisplayEnvelopeRight =
      meter_display::stepEnvelope (meterDisplayEnvelopeRight, right, attackMs, releaseSec, dtSec);

  meterDisplayInputMonoNorm = meter_display::linearToDbNormalized (meterDisplayEnvelopeInputMono, dbMin);
  meterDisplayLeftNorm = meter_display::linearToDbNormalized (meterDisplayEnvelopeLeft, dbMin);
  meterDisplayRightNorm = meter_display::linearToDbNormalized (meterDisplayEnvelopeRight, dbMin);
}

int ChorusAudioProcessor::midiCcChoiceToNumber (float choiceValue) noexcept
{
  const int choice = juce::roundToInt (choiceValue);
  if (choice <= 0)
    return -1;
  return juce::jlimit (0, 127, choice - 1);
}

void ChorusAudioProcessor::applyMidiCcToParameter (PluginParameterLinSlider& target,
                                                   float controllerNormalized)
{
  const float domain = target.minValue
                       + juce::jlimit (0.0f, 1.0f, controllerNormalized)
                             * (target.maxValue - target.minValue);
  setParameterDomainValue (target.paramID, domain);
}

void ChorusAudioProcessor::applyMidiCcToChorusParameter (const String& paramId,
                                                         std::string_view minibussParamId,
                                                         float minDomain,
                                                         float maxDomain,
                                                         float controllerNormalized)
{
  const float domain = minibussEngine.mapControlToDomain (minibussEngine.chorusId(),
                                                          minibussParamId,
                                                          controllerNormalized,
                                                          minDomain,
                                                          maxDomain);
  setParameterDomainValue (paramId, domain);
}

void ChorusAudioProcessor::handleIncomingMidi (const MidiBuffer& midiMessages)
{
  if (midiMessages.isEmpty())
    return;

  const int rateCc = midiCcChoiceToNumber (
      readParameterValue (paramMidiCcChorusRate.paramID, (float) paramMidiCcChorusRate.defaultChoice));
  const int delayCc = midiCcChoiceToNumber (
      readParameterValue (paramMidiCcChorusDelay.paramID, (float) paramMidiCcChorusDelay.defaultChoice));
  const int amountCc = midiCcChoiceToNumber (
      readParameterValue (paramMidiCcChorusAmount.paramID, (float) paramMidiCcChorusAmount.defaultChoice));
  const int wetCc = midiCcChoiceToNumber (
      readParameterValue (paramMidiCcChorusWet.paramID, (float) paramMidiCcChorusWet.defaultChoice));
  const int feedbackCc = midiCcChoiceToNumber (
      readParameterValue (paramMidiCcChorusFeedback.paramID, (float) paramMidiCcChorusFeedback.defaultChoice));

  const auto chorusLimits = getChorusNuDspLimits();
  const float minRate = getChorusMinLfoFreq();
  const float maxRate = getChorusMaxLfoFreq();
  const float maxDelay = getChorusMaxDelayTime();
  const float maxAmount = getChorusMaxAmount();

  for (const auto metadata : midiMessages)
  {
    const auto msg = metadata.getMessage();
    if (! msg.isController())
      continue;

    const int cc = msg.getControllerNumber();
    const float norm = (float) msg.getControllerValue() / 127.0f;

    if (cc == rateCc)
      applyMidiCcToChorusParameter (paramChorusRate.paramID,
                                    "rate",
                                    minRate,
                                    maxRate,
                                    norm);
    if (cc == delayCc)
      applyMidiCcToChorusParameter (paramChorusDelay.paramID,
                                    "delay",
                                    chorusLimits.delayMin,
                                    maxDelay,
                                    norm);
    if (cc == amountCc)
      applyMidiCcToChorusParameter (paramChorusAmount.paramID,
                                    "amount",
                                    chorusLimits.amountMin,
                                    maxAmount,
                                    norm);
    if (cc == wetCc)
      applyMidiCcToParameter (paramChorusWet, norm);
    if (cc == feedbackCc)
      applyMidiCcToParameter (paramChorusFeedback, norm);
  }
}

//==============================================================================

void ChorusAudioProcessor::getStateInformation(MemoryBlock& destData)
{
  std::unique_ptr<XmlElement> xml(parameters.valueTreeState.state.createXml());
  if (xml != nullptr)
  {
    xml->setAttribute ("calibrationK", (double) calibrationK.load());
    xml->setAttribute ("calibrationRefVoltage", (double) calibrationRefVoltage.load());
  }
  copyXmlToBinary(*xml, destData);
}

void ChorusAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
  std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
  if (xmlState != nullptr)
    if (xmlState->hasTagName(parameters.valueTreeState.state.getType()))
    {
      if (xmlState->hasAttribute ("calibrationK"))
        calibrationK.store ((float) xmlState->getDoubleAttribute ("calibrationK", 1.0));
      if (xmlState->hasAttribute ("calibrationRefVoltage"))
        calibrationRefVoltage.store ((float) xmlState->getDoubleAttribute ("calibrationRefVoltage", 1.0));

      parameters.valueTreeState.state = ValueTree::fromXml(*xmlState);
    }

  applyChorusLimits (false);
}

//==============================================================================

bool ChorusAudioProcessor::hasEditor() const { return true; }

AudioProcessorEditor* ChorusAudioProcessor::createEditor() { return new ChorusAudioProcessorEditor(*this); }

//==============================================================================

#ifndef JucePlugin_PreferredChannelConfigurations
bool ChorusAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
  ignoreUnused(layouts);
  return true;
#else
  if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono() &&
      layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
    return false;

#if !JucePlugin_IsSynth
  if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet()) return false;
#endif

  return true;
#endif
}
#endif

//==============================================================================

const String ChorusAudioProcessor::getName() const { return JucePlugin_Name; }

bool ChorusAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
  return true;
#else
  return false;
#endif
}

bool ChorusAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
  return true;
#else
  return false;
#endif
}

bool ChorusAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
  return true;
#else
  return false;
#endif
}

double ChorusAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int ChorusAudioProcessor::getNumPrograms() { return 1; }

int ChorusAudioProcessor::getCurrentProgram() { return 0; }

void ChorusAudioProcessor::setCurrentProgram(int index) { ignoreUnused(index); }

const String ChorusAudioProcessor::getProgramName(int index)
{
  ignoreUnused(index);
  return {};
}

void ChorusAudioProcessor::changeProgramName(int index, const String& newName) { ignoreUnused(index, newName); }

//==============================================================================

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ChorusAudioProcessor(); }

//==============================================================================
