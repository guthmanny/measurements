#include "MoogLadderFrCurveComponent.h"

#include <cmath>

namespace
{
const char* modeName(nx_moog_ladder_mode_t mode) noexcept
{
  switch (mode)
  {
    case NX_MOOG_LADDER_LP2:
      return "LP2";
    case NX_MOOG_LADDER_HP4:
      return "HP4";
    case NX_MOOG_LADDER_HP2:
      return "HP2";
    case NX_MOOG_LADDER_BP4:
      return "BP4";
    case NX_MOOG_LADDER_BP2:
      return "BP2";
    case NX_MOOG_LADDER_NOTCH:
      return "Notch";
    case NX_MOOG_LADDER_LP4:
    default:
      return "LP4";
  }
}

void applyLogFrequencyStyle(atom::CurveControl::Style& style)
{
  style.metrics.gridMode = atom::CurveControl::GridMode::Logarithmic;
  style.metrics.logXConfig.enabled = true;
  style.metrics.logXConfig.minValue = 20.0;
  style.metrics.logXConfig.maxValue = 20000.0;
  style.metrics.numGridDivisions = 5;
  style.metrics.plotBufferX = 8;
  style.metrics.plotBufferY = 8;
  style.metrics.pathStrokeSize = 2.25f;
  style.metrics.showXLabel = true;
  style.metrics.showYLabel = true;
  style.metrics.showTickLabels = true;
  style.metrics.frameMode = atom::CurveControl::FrameMode::All;
}
}  // namespace

MoogLadderFrCurveComponent::MoogLadderFrCurveComponent()
    : curveControl_(atom::CurveControl::Direction::Speedup)
{
  auto style = atom::CurveControl::Style::fromTheme(atom::Theme::getDarkTheme());
  applyLogFrequencyStyle(style);
  curveControl_.setStyle(style);
  curveControl_.setMode(atom::CurveControl::Mode::DisplayOnly);
  curveControl_.setXLabel("Frequency");
  curveControl_.setYLabel("Magnitude");

  freqsHz_.resize((size_t)numPoints);
  magDb_.resize((size_t)numPoints);
  const double ratio = freqMaxHz / freqMinHz;
  for (int i = 0; i < numPoints; ++i)
  {
    const double t = (numPoints <= 1) ? 0.0 : (double)i / (double)(numPoints - 1);
    freqsHz_[(size_t)i] = freqMinHz * std::pow(ratio, t);
  }

  addAndMakeVisible(curveControl_);
  rebuildCurve();
  syncCurveControl();
}

double MoogLadderFrCurveComponent::dspSampleRate() const noexcept
{
  return hostSampleRate_ * (double)oversampleFactor_;
}

void MoogLadderFrCurveComponent::setHostRateAndOversample(double hostSampleRateHz, int oversampleFactor)
{
  hostSampleRate_ = juce::jmax(1000.0, hostSampleRateHz);
  oversampleFactor_ = oversampleFactor >= 8 ? 8 : (oversampleFactor >= 4 ? 4 : 2);
  rebuildCurve();
  syncCurveControl();
}

void MoogLadderFrCurveComponent::setParameters(float cutoffHz, float resonance, float drive, nx_moog_ladder_mode_t mode,
                                               nx_moog_ladder_sat_t saturator, nx_moog_ladder_quality_t quality,
                                               bool adaaEnabled)
{
  cutoffHz_ = juce::jlimit(20.0f, 20000.0f, cutoffHz);
  resonance_ = juce::jlimit(0.0f, 1.0f, resonance);
  drive_ = juce::jmax(0.01f, drive);
  mode_ = mode;
  saturator_ = saturator;
  quality_ = quality;
  adaaEnabled_ = adaaEnabled;

  rebuildCurve();
  syncCurveControl();
}

juce::String MoogLadderFrCurveComponent::formatFrequencyTick(float log10Hz)
{
  const double hz = std::pow(10.0, (double)log10Hz);
  if (hz >= 1000.0) return juce::String(hz / 1000.0, hz >= 10000.0 ? 0 : 1) + " kHz";
  return juce::String(juce::roundToInt(hz)) + " Hz";
}

juce::String MoogLadderFrCurveComponent::formatMagnitudeTick(float magDb)
{
  return juce::String(magDb, 0) + " dB";
}

std::vector<float> MoogLadderFrCurveComponent::buildLogFrequencyTicks()
{
  // Fixed decades: 20, 100, 1k, 10k, 20k (log10 domain).
  return {std::log10(20.0f), 2.0f, 3.0f, 4.0f, std::log10(20000.0f)};
}

std::vector<float> MoogLadderFrCurveComponent::buildMagnitudeTicks()
{
  return {10.0f, 0.0f, -10.0f, -20.0f, -30.0f, -40.0f, -50.0f, -60.0f};
}

void MoogLadderFrCurveComponent::rebuildCurve()
{
  const double sr = dspSampleRate();

  nudsp::MoogLadderF32 probe;
  probe.prepare(sr);
  probe.setCutoff((double)cutoffHz_);
  probe.setResonance((double)resonance_);
  probe.setDrive((double)drive_);
  probe.setMode(mode_);
  probe.setSaturator(saturator_);
  probe.setQuality(quality_);
  probe.setAdaaEnabled(adaaEnabled_);
  probe.tick(1);
  probe.ac(freqsHz_.data(), magDb_.data(), nullptr, freqsHz_.size());

  magnitudeCurve_.clear();
  magnitudeCurve_.reserve(freqsHz_.size());

  for (size_t i = 0; i < freqsHz_.size(); ++i)
  {
    const float f = (float)freqsHz_[i];
    float db = (float)magDb_[i];
    if (!std::isfinite(db)) db = magMinDb;
    magnitudeCurve_.emplace_back(std::log10(juce::jmax(1.0e-6f, f)), db);
  }
}

void MoogLadderFrCurveComponent::syncCurveControl()
{
  curveControl_.setTitle(juce::String("FR after upsampler  |  ") + modeName(mode_) + "  |  fc "
                         + juce::String(cutoffHz_, cutoffHz_ >= 1000.0f ? 0 : 1) + " Hz  |  Res "
                         + juce::String(resonance_, 2) + "  |  OS " + juce::String(oversampleFactor_) + "x");

  const float logMinX = (float)std::log10(freqMinHz);
  const float logMaxX = (float)std::log10(freqMaxHz);

  atom::CurveControl::DataAxis xAxis;
  xAxis.minValue = logMinX;
  xAxis.maxValue = logMaxX;
  xAxis.formatTick = [](float log10Hz) { return formatFrequencyTick(log10Hz); };

  atom::CurveControl::DataAxis yAxis;
  yAxis.minValue = magMinDb;
  yAxis.maxValue = magMaxDb;
  yAxis.formatTick = [](float magDb) { return formatMagnitudeTick(magDb); };

  curveControl_.setDataAxes(xAxis, yAxis);
  curveControl_.setCustomAxisTicks(buildLogFrequencyTicks(), buildMagnitudeTicks());
  curveControl_.setCustomCurve(magnitudeCurve_);
  curveControl_.clearSecondaryCustomCurve();

  atom::CurveControl::ReferenceLine zeroDb;
  zeroDb.yValue = 0.0f;
  zeroDb.enabled = true;
  curveControl_.setReferenceLine(zeroDb);
}

void MoogLadderFrCurveComponent::resized()
{
  curveControl_.setBounds(getLocalBounds());
  syncCurveControl();
}
