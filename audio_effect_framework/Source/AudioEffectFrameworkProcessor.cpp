#include "AefJuceIncludes.h"
/*
  ==============================================================================

    Audio effect template — DSP via kbuss Track + Processors.

  ==============================================================================
*/

#include "AudioEffectFrameworkProcessor.h"

#include "kbuss/processor.hpp"

#include <algorithm>
#include <cmath>

#include "MeterDisplayUtils.h"
#include "PluginParameter.h"

//==============================================================================

std::unique_ptr<KbussEffectEngine> AudioEffectFrameworkProcessor::createEffectEngine()
{
  return std::make_unique<KbussEffectEngine>();
}

AudioEffectFrameworkProcessor::AudioEffectFrameworkProcessor()
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
      paramOversampleQuality(parameters, "Oversample Quality", {"STANDARD", "HIGH", "ULTRA"}, 0),
      paramUpsamplerMode(parameters,
                         "Upsampler Mode",
                         {"Zero-Order Hold", "Zero Insert", "Linear", "Quadratic", "Cubic"},
                         4),
      paramDownsamplerMode(parameters,
                           "Downsampler Mode",
                           {"Box", "Linear", "Quadratic", "Cubic"},
                           3)
{
  parameters.valueTreeState.state = ValueTree(Identifier(getName().removeCharacters("- ")));
}

AudioEffectFrameworkProcessor::~AudioEffectFrameworkProcessor()
{
  spectrumEnabled.store(false);
  spectrumAnalyzer.stopAnalysis();
  if (effectEngine_ != nullptr)
    effectEngine_->release();
}

void AudioEffectFrameworkProcessor::setTunerEnabled(bool shouldEnable) noexcept
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

TunerDetector::Result AudioEffectFrameworkProcessor::getTunerResult() const noexcept
{
  const juce::SpinLock::ScopedLockType lock(tunerLock);
  return tunerResult;
}

void AudioEffectFrameworkProcessor::setTunerPeriodicityThreshold(float threshold) noexcept
{
  const float clamped = juce::jlimit(0.0f, 1.0f, threshold);
  tunerPeriodicityThreshold.store(clamped);
  tunerDetector.setPeriodicityThreshold(clamped);
}

void AudioEffectFrameworkProcessor::setSpectrumEnabled(bool shouldEnable) noexcept
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

void AudioEffectFrameworkProcessor::setSpectrumFftSize(int fftSize)
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

bool AudioEffectFrameworkProcessor::copySpectrumMagnitudesIfNew(uint32_t& lastFrameId, std::vector<float>& dest) const
{
  return spectrumAnalyzer.copyMagnitudesIfNew(lastFrameId, dest);
}

//==============================================================================

float AudioEffectFrameworkProcessor::readParameterValue(const String& paramId, float fallback) const
{
  if (auto* param = parameters.valueTreeState.getParameter(paramId)) return param->convertFrom0to1(param->getValue());

  if (auto* value = parameters.valueTreeState.getRawParameterValue(paramId)) return value->load();

  return fallback;
}

void AudioEffectFrameworkProcessor::ensureEffectEngine()
{
  if (effectEngine_ == nullptr)
    effectEngine_ = createEffectEngine();
}

void AudioEffectFrameworkProcessor::syncParametersFromValueTree()
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
  paramBypass.setCurrentAndTargetValue(readParameterValue(paramBypass.paramID, (float)paramBypass.defaultState));
  paramOversampleQuality.setCurrentAndTargetValue(
      readParameterValue(paramOversampleQuality.paramID, (float)paramOversampleQuality.defaultChoice));
  paramUpsamplerMode.setCurrentAndTargetValue(
      readParameterValue(paramUpsamplerMode.paramID, (float)paramUpsamplerMode.defaultChoice));
  paramDownsamplerMode.setCurrentAndTargetValue(
      readParameterValue(paramDownsamplerMode.paramID, (float)paramDownsamplerMode.defaultChoice));
}

void AudioEffectFrameworkProcessor::ensureScratchBuffers(int numChannels, int numSamples)
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

void AudioEffectFrameworkProcessor::updateEffectParameters()
{
  ensureEffectEngine();

  const bool userBypass = readParameterValue(paramBypass.paramID, (float)paramBypass.defaultState) >= 0.5f;
  getMinibussEngine().setBypass(userBypass);

  getMinibussEngine().setParamDomain(getMinibussEngine().gainId(), "gain",
                                readParameterValue(paramInputGain.paramID, paramInputGain.defaultValue));

  float threshMinDb = readParameterValue(paramGateThreshMin.paramID, paramGateThreshMin.defaultValue);
  float threshMaxDb = readParameterValue(paramGateThreshMax.paramID, paramGateThreshMax.defaultValue);
  if (threshMinDb > threshMaxDb)
    std::swap(threshMinDb, threshMaxDb);
  getMinibussEngine().setParamDomain(getMinibussEngine().gateId(), "thresh_min", threshMinDb);
  getMinibussEngine().setParamDomain(getMinibussEngine().gateId(), "thresh_max", threshMaxDb);

  const float thresholdDb =
      jlimit(threshMinDb, threshMaxDb,
             readParameterValue(paramGateThreshold.paramID, paramGateThreshold.defaultValue));
  getMinibussEngine().setParamDomain(getMinibussEngine().gateId(), "threshold", thresholdDb);
  getMinibussEngine().setParamDomain(
      getMinibussEngine().gateId(), "off_at_min",
      readParameterValue(paramGateOffAtMin.paramID, (float)paramGateOffAtMin.defaultChoice) >= 0.5f ? 1.f : 0.f);
  getMinibussEngine().setParamDomain(getMinibussEngine().gateId(), "ratio",
                                readParameterValue(paramGateRatio.paramID, paramGateRatio.defaultValue));
  getMinibussEngine().setParamDomain(getMinibussEngine().gateId(), "attack",
                                readParameterValue(paramGateAttack.paramID, paramGateAttack.defaultValue));
  getMinibussEngine().setParamDomain(getMinibussEngine().gateId(), "release",
                                readParameterValue(paramGateRelease.paramID, paramGateRelease.defaultValue));
  getMinibussEngine().setParamDomain(
      getMinibussEngine().gateId(), "knee_mode",
      readParameterValue(paramGateKnee.paramID, (float)paramGateKnee.defaultChoice) >= 0.5f ? 1.f : 0.f);
  getMinibussEngine().setParamDomain(getMinibussEngine().gateId(), "knee_width",
                                readParameterValue(paramGateKneeWidth.paramID, paramGateKneeWidth.defaultValue));

  getMinibussEngine().setParamDomain(getMinibussEngine().levelId(), "level",
                                readParameterValue(paramOutputGain.paramID, paramOutputGain.defaultValue));

  const int qualityChoice = juce::roundToInt(
      readParameterValue(paramOversampleQuality.paramID, (float)paramOversampleQuality.defaultChoice));
  const int osFactor = qualityChoice >= 2 ? 8 : (qualityChoice >= 1 ? 4 : 2);
  const int upMode = juce::roundToInt(
      readParameterValue(paramUpsamplerMode.paramID, (float)paramUpsamplerMode.defaultChoice));
  const int downMode = juce::roundToInt(
      readParameterValue(paramDownsamplerMode.paramID, (float)paramDownsamplerMode.defaultChoice));
  getMinibussEngine().setOversampling(osFactor, upMode, downMode);

  updateCustomEffectParameters();
}

void AudioEffectFrameworkProcessor::mixToMonoBuffer(const AudioSampleBuffer& buffer, int numChannels, int numSamples)
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

void AudioEffectFrameworkProcessor::ensureTunerSampleRate()
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

void AudioEffectFrameworkProcessor::setInputRmsMeteringEnabled (bool shouldEnable) noexcept
{
  inputRmsMeteringEnabled.store (shouldEnable);
  if (! shouldEnable)
  {
    inputRmsMono.store (0.0f);
    inputRmsBlockCounter = 0;
  }
}

void AudioEffectFrameworkProcessor::updateInputRms (const AudioSampleBuffer& buffer, int numChannels, int numSamples)
{
  if (! inputRmsMeteringEnabled.load (std::memory_order_relaxed))
    return;

  if (numChannels <= 0 || numSamples <= 0)
  {
    inputRmsMono.store (0.0f, std::memory_order_relaxed);
    return;
  }

  if ((++inputRmsBlockCounter & 15) != 0)
    return;

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

void AudioEffectFrameworkProcessor::updateInputMeter (const AudioSampleBuffer& buffer, int numChannels,
                                                      int numSamples)
{
  if (numChannels <= 0 || numSamples <= 0)
  {
    meterInputMono.store (0.0f);
    return;
  }

  const float inputGain =
      Decibels::decibelsToGain (readParameterValue (paramInputGain.paramID, paramInputGain.defaultValue));

  float left = buffer.getMagnitude (0, 0, numSamples) * inputGain;
  float right = left;
  if (numChannels > 1)
    right = buffer.getMagnitude (1, 0, numSamples) * inputGain;

  meterInputMono.store (numChannels > 1 ? 0.5f * (left + right) : left);
}

bool AudioEffectFrameworkProcessor::calibrateInputFromReference (float referenceVoltage, float rms)
{
  const float volts = juce::jmax (0.0f, referenceVoltage);
  calibrationRefVoltage.store (volts);

  constexpr float kMinRms = 1.0e-6f;
  if (rms < kMinRms)
    return false;

  calibrationKi.store (volts / rms);
  return true;
}

bool AudioEffectFrameworkProcessor::calibrateOutputFromMeasured (float measuredOutputVoltage)
{
  const float measured = juce::jmax (0.0f, measuredOutputVoltage);
  calibrationMeasuredOutputVoltage.store (measured);

  constexpr float kMinVolts = 1.0e-6f;
  if (measured < kMinVolts)
    return false;

  const float refV = calibrationRefVoltage.load();
  if (refV < kMinVolts)
    return false;

  // Ko compensates measured external output toward the shared Reference V.
  calibrationKo.store (refV / measured);
  return true;
}

void AudioEffectFrameworkProcessor::setCalibrationKi (float ki) noexcept
{
  calibrationKi.store (juce::jmax (0.0f, ki));
}

void AudioEffectFrameworkProcessor::setCalibrationKo (float ko) noexcept
{
  calibrationKo.store (juce::jmax (0.0f, ko));
}

void AudioEffectFrameworkProcessor::setCalibrationReferenceVoltage (float volts) noexcept
{
  calibrationRefVoltage.store (juce::jmax (0.0f, volts));
}

void AudioEffectFrameworkProcessor::setCalibrationMeasuredOutputVoltage (float volts) noexcept
{
  calibrationMeasuredOutputVoltage.store (juce::jmax (0.0f, volts));
}

void AudioEffectFrameworkProcessor::resetInputCalibration() noexcept
{
  calibrationKi.store (1.0f);
}

void AudioEffectFrameworkProcessor::resetOutputCalibration() noexcept
{
  calibrationKo.store (1.0f);
}

void AudioEffectFrameworkProcessor::resetCalibration() noexcept
{
  resetInputCalibration();
  resetOutputCalibration();
}

namespace
{
constexpr const char* kCalibrationFileXmlTag = "Calibration";

void applyCalibrationAttributes (AudioEffectFrameworkProcessor& processor, const juce::XmlElement& xml)
{
  if (xml.hasAttribute ("calibrationKi"))
    processor.setCalibrationKi ((float) xml.getDoubleAttribute ("calibrationKi", 1.0));
  else if (xml.hasAttribute ("calibrationK"))
    processor.setCalibrationKi ((float) xml.getDoubleAttribute ("calibrationK", 1.0));

  if (xml.hasAttribute ("calibrationKo"))
    processor.setCalibrationKo ((float) xml.getDoubleAttribute ("calibrationKo", 1.0));

  if (xml.hasAttribute ("calibrationRefVoltage"))
    processor.setCalibrationReferenceVoltage ((float) xml.getDoubleAttribute ("calibrationRefVoltage", 1.0));

  if (xml.hasAttribute ("calibrationMeasuredOutputVoltage"))
    processor.setCalibrationMeasuredOutputVoltage (
        (float) xml.getDoubleAttribute ("calibrationMeasuredOutputVoltage", 1.0));
}
} // namespace

bool AudioEffectFrameworkProcessor::saveCalibrationToFile (const juce::File& file) const
{
  juce::XmlElement xml (kCalibrationFileXmlTag);
  xml.setAttribute ("calibrationKi", (double) calibrationKi.load());
  xml.setAttribute ("calibrationKo", (double) calibrationKo.load());
  xml.setAttribute ("calibrationRefVoltage", (double) calibrationRefVoltage.load());
  xml.setAttribute ("calibrationMeasuredOutputVoltage",
                    (double) calibrationMeasuredOutputVoltage.load());
  return xml.writeTo (file, juce::XmlElement::TextFormat().singleLine());
}

bool AudioEffectFrameworkProcessor::loadCalibrationFromFile (const juce::File& file)
{
  const auto parsed = juce::parseXML (file);
  if (parsed == nullptr || ! parsed->hasTagName (kCalibrationFileXmlTag))
    return false;

  applyCalibrationAttributes (*this, *parsed);
  return true;
}

void AudioEffectFrameworkProcessor::applyInputCalibration (AudioSampleBuffer& buffer,
                                                           int numChannels,
                                                           int numSamples) const noexcept
{
  if (numChannels <= 0 || numSamples <= 0)
    return;

  const float ki = calibrationKi.load (std::memory_order_relaxed);
  if (! std::isfinite (ki) || ki <= 0.0f || std::abs (ki - 1.0f) <= 1.0e-6f)
    return;

  for (int ch = 0; ch < numChannels; ++ch)
    buffer.applyGain (ch, 0, numSamples, ki);
}

void AudioEffectFrameworkProcessor::applyOutputCalibration (AudioSampleBuffer& buffer,
                                                            int numChannels,
                                                            int numSamples) const noexcept
{
  if (numChannels <= 0 || numSamples <= 0)
    return;

  const float ko = calibrationKo.load (std::memory_order_relaxed);
  if (! std::isfinite (ko) || ko <= 0.0f || std::abs (ko - 1.0f) <= 1.0e-6f)
    return;

  for (int ch = 0; ch < numChannels; ++ch)
    buffer.applyGain (ch, 0, numSamples, ko);
}

void AudioEffectFrameworkProcessor::pushTunerMono(const AudioSampleBuffer& buffer, int numChannels, int numSamples)
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

void AudioEffectFrameworkProcessor::pushSpectrumMono(const AudioSampleBuffer& buffer, int numChannels, int numSamples)
{
  if (!spectrumEnabled.load() || numChannels <= 0 || numSamples <= 0)
    return;

  mixToMonoBuffer(buffer, numChannels, numSamples);
  spectrumAnalyzer.pushSamples(monoBuffer.getReadPointer(0), numSamples);
}

void AudioEffectFrameworkProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
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

  ensureEffectEngine();
  getMinibussEngine().prepare((float)sampleRate, (std::uint32_t)jmax(1, samplesPerBlock));
  updateEffectParameters();

  if (bypassNoiseGateOnStartup())
  {
    auto& engine = getMinibussEngine();
    if (engine.gateId() != kbuss::kInvalidObjectId)
      engine.setProcessorBypassed (engine.gateId(), true);
  }

  applyEffectTopologyBypassOverrides();
}

void AudioEffectFrameworkProcessor::releaseResources()
{
  if (!spectrumEnabled.load()) spectrumAnalyzer.stopAnalysis();
}

void AudioEffectFrameworkProcessor::processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
  ScopedNoDenormals noDenormals;
  juce::ignoreUnused (midiMessages);

  const int numInputChannels = getTotalNumInputChannels();
  const int numOutputChannels = getTotalNumOutputChannels();
  const int numSamples = buffer.getNumSamples();

  if (numSamples == 0) return;

  const int bufCh = jmax(1, jmax(numInputChannels, numOutputChannels));
  ensureScratchBuffers(bufCh, numSamples);

  updateInputRms (buffer, numInputChannels, numSamples);
  pushTunerMono(buffer, numInputChannels, numSamples);

  updateEffectParameters();
  updateInputMeter (buffer, numInputChannels, numSamples);

  constexpr int engCh = 2;
  for (int ch = 0; ch < engCh; ++ch)
  {
    const int src = jmin(ch, jmax(0, numInputChannels - 1));
    if (numInputChannels > 0)
      processBuffer.copyFrom(ch, 0, buffer, src, 0, numSamples);
    else
      processBuffer.clear(ch, 0, numSamples);
  }

  // Pre-FX input calibration (Ki); input meter above uses raw ADC x Input Gain only.
  applyInputCalibration (processBuffer, engCh, numSamples);

  for (int ch = 0; ch < engCh; ++ch)
    dryBuffer.clear(ch, 0, numSamples);

  const float* inPtrs[2] = { processBuffer.getReadPointer(0), processBuffer.getReadPointer(1) };
  float* outPtrs[2] = { dryBuffer.getWritePointer(0), dryBuffer.getWritePointer(1) };

  if (getMinibussEngine().isReady() && ! tunerEnabled.load())
    getMinibussEngine().process (inPtrs, outPtrs, (std::uint32_t) numSamples);

  // FX output is in dryBuffer. Host buffer is filled from that, then Ko is applied last.
  const int procChannels = jmin(numOutputChannels, engCh);
  for (int ch = 0; ch < procChannels; ++ch)
    buffer.copyFrom(ch, 0, dryBuffer, ch, 0, numSamples);

  for (int ch = procChannels; ch < numOutputChannels; ++ch)
    buffer.clear(ch, 0, numSamples);

  // Post-FX output calibration only (after Gain→Gate→OS→Effect→Level).
  applyOutputCalibration (buffer, numOutputChannels, numSamples);

  pushSpectrumMono(buffer, procChannels, numSamples);

  float leftPeak = 0.0f, rightPeak = 0.0f;
  if (numOutputChannels > 0) leftPeak = buffer.getMagnitude(0, 0, numSamples);
  if (numOutputChannels > 1) rightPeak = buffer.getMagnitude(1, 0, numSamples);

  meterLeft.store(leftPeak);
  meterRight.store(rightPeak);
}

void AudioEffectFrameworkProcessor::updateMeterDisplay (float attackMs, float releaseSec, int displayRangeDbSpan, float dtSec)
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

//==============================================================================

//==============================================================================

juce::Array<EffectTopologyModule> AudioEffectFrameworkProcessor::getEffectTopologyModules() const
{
  juce::Array<EffectTopologyModule> modules;

  if (effectEngine_ == nullptr || ! effectEngine_->isReady())
    return modules;

  if (auto* proc = effectEngine_->getMiddleProcessor())
  {
    for (const auto& entry : proc->get_module_topology())
    {
      EffectTopologyModule mod;
      mod.id = juce::String (entry.id);
      mod.displayName = juce::String (entry.label);
      mod.bypassed = entry.bypassed;
      modules.add (std::move (mod));
    }
  }

  return modules;
}

juce::String AudioEffectFrameworkProcessor::getEffectTopologyTitle() const
{
  if (effectEngine_ == nullptr || ! effectEngine_->isReady())
    return {};

  if (auto* proc = effectEngine_->getMiddleProcessor())
    return juce::String (proc->description().name) + " internal signal chain";

  return {};
}

bool AudioEffectFrameworkProcessor::setEffectTopologyModuleBypassed (const juce::String& moduleId, bool bypassed)
{
  if (effectEngine_ == nullptr || ! effectEngine_->isReady())
    return false;

  if (auto* proc = effectEngine_->getMiddleProcessor())
  {
    if (proc->set_module_bypass (moduleId.toStdString(), bypassed) == kbuss::Status::Ok)
    {
      effectTopologyBypassOverrides_.set (moduleId, bypassed);
      return true;
    }
  }

  return false;
}

void AudioEffectFrameworkProcessor::applyEffectTopologyBypassOverrides()
{
  if (effectTopologyBypassOverrides_.size() == 0)
    return;

  if (effectEngine_ == nullptr || ! effectEngine_->isReady())
    return;

  if (auto* proc = effectEngine_->getMiddleProcessor())
  {
    for (auto it = effectTopologyBypassOverrides_.begin(); it != effectTopologyBypassOverrides_.end(); ++it)
      proc->set_module_bypass (it.getKey().toStdString(), it.getValue());
  }
}

void AudioEffectFrameworkProcessor::captureEffectTopologyBypassForState (juce::XmlElement& xml) const
{
  const auto modules = getEffectTopologyModules();
  if (modules.isEmpty())
    return;

  auto* topo = xml.createNewChildElement ("effectTopologyBypass");
  for (const auto& mod : modules)
  {
    if (mod.bypassed)
      topo->setAttribute (mod.id, 1);
  }
}

void AudioEffectFrameworkProcessor::getStateInformation(MemoryBlock& destData)
{
  std::unique_ptr<XmlElement> xml(parameters.valueTreeState.state.createXml());
  if (xml != nullptr)
  {
    xml->setAttribute ("calibrationKi", (double) calibrationKi.load());
    xml->setAttribute ("calibrationKo", (double) calibrationKo.load());
    xml->setAttribute ("calibrationRefVoltage", (double) calibrationRefVoltage.load());
    xml->setAttribute ("calibrationMeasuredOutputVoltage",
                       (double) calibrationMeasuredOutputVoltage.load());
    // Legacy alias for older sessions.
    xml->setAttribute ("calibrationK", (double) calibrationKi.load());
    captureEffectTopologyBypassForState (*xml);
  }
  copyXmlToBinary(*xml, destData);
}

void AudioEffectFrameworkProcessor::setStateInformation(const void* data, int sizeInBytes)
{
  std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
  if (xmlState != nullptr)
    if (xmlState->hasTagName(parameters.valueTreeState.state.getType()))
    {
      if (xmlState->hasAttribute ("calibrationKi"))
        calibrationKi.store ((float) xmlState->getDoubleAttribute ("calibrationKi", 1.0));
      else if (xmlState->hasAttribute ("calibrationK"))
        calibrationKi.store ((float) xmlState->getDoubleAttribute ("calibrationK", 1.0));

      if (xmlState->hasAttribute ("calibrationKo"))
        calibrationKo.store ((float) xmlState->getDoubleAttribute ("calibrationKo", 1.0));

      if (xmlState->hasAttribute ("calibrationRefVoltage"))
        calibrationRefVoltage.store ((float) xmlState->getDoubleAttribute ("calibrationRefVoltage", 1.0));

      if (xmlState->hasAttribute ("calibrationMeasuredOutputVoltage"))
        calibrationMeasuredOutputVoltage.store (
            (float) xmlState->getDoubleAttribute ("calibrationMeasuredOutputVoltage", 1.0));

      effectTopologyBypassOverrides_.clear();
      if (auto* topo = xmlState->getChildByName ("effectTopologyBypass"))
      {
        for (int i = 0; i < topo->getNumAttributes(); ++i)
        {
          const auto name = topo->getAttributeName (i);
          if (topo->getIntAttribute (name, 0) != 0)
            effectTopologyBypassOverrides_.set (name, true);
        }
      }

      parameters.valueTreeState.state = ValueTree::fromXml(*xmlState);
      applyEffectTopologyBypassOverrides();
    }
}

//==============================================================================

bool AudioEffectFrameworkProcessor::hasEditor() const { return true; }

#ifndef JucePlugin_PreferredChannelConfigurations
bool AudioEffectFrameworkProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
  if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono() &&
      layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
    return false;

  if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
    return false;

  return true;
}
#endif

//==============================================================================

const String AudioEffectFrameworkProcessor::getName() const { return "Audio Effect"; }

bool AudioEffectFrameworkProcessor::acceptsMidi() const { return false; }

bool AudioEffectFrameworkProcessor::producesMidi() const { return false; }

bool AudioEffectFrameworkProcessor::isMidiEffect() const { return false; }

double AudioEffectFrameworkProcessor::getTailLengthSeconds() const { return 0.0; }

int AudioEffectFrameworkProcessor::getNumPrograms() { return 1; }

int AudioEffectFrameworkProcessor::getCurrentProgram() { return 0; }

void AudioEffectFrameworkProcessor::setCurrentProgram(int index) { ignoreUnused(index); }

const String AudioEffectFrameworkProcessor::getProgramName(int index)
{
  ignoreUnused(index);
  return {};
}

void AudioEffectFrameworkProcessor::changeProgramName(int index, const String& newName) { ignoreUnused(index, newName); }
