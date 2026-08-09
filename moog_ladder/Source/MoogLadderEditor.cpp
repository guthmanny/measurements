#include "MoogLadderEditor.h"

namespace
{
nx_moog_ladder_mode_t modeFromChoice(int choice) noexcept
{
  switch (juce::jlimit(0, 6, choice))
  {
    case 1:
      return NX_MOOG_LADDER_LP2;
    case 2:
      return NX_MOOG_LADDER_HP4;
    case 3:
      return NX_MOOG_LADDER_HP2;
    case 4:
      return NX_MOOG_LADDER_BP4;
    case 5:
      return NX_MOOG_LADDER_BP2;
    case 6:
      return NX_MOOG_LADDER_NOTCH;
    case 0:
    default:
      return NX_MOOG_LADDER_LP4;
  }
}

nx_moog_ladder_sat_t saturatorFromChoice(int choice) noexcept
{
  return choice >= 1 ? NX_MOOG_LADDER_SAT_TANH : NX_MOOG_LADDER_SAT_ALGEBRAIC;
}

nx_moog_ladder_quality_t qualityFromChoice(int choice) noexcept
{
  switch (juce::jlimit(0, 2, choice))
  {
    case 0:
      return NX_MOOG_LADDER_QUALITY_STATIC;
    case 2:
      return NX_MOOG_LADDER_QUALITY_OUTER2;
    case 1:
    default:
      return NX_MOOG_LADDER_QUALITY_RELINEARIZED;
  }
}
}  // namespace

MoogLadderEditor::MoogLadderEditor(MoogLadderAudioProcessor& p)
    : AudioEffectFrameworkEditor(p, true)
{
  bodyContent.addAndMakeVisible(frCurve_);
  bodyComponents.add(&frCurve_);
  bodyContentHeight += frCurveBaseHeight + bodyPadding;

  completeBodyConstruction();
  refreshFrequencyResponse();
}

int MoogLadderEditor::getBodyComponentBaseHeight(const juce::Component* component) const noexcept
{
  if (component == &frCurve_) return frCurveBaseHeight;
  return AudioEffectFrameworkEditor::getBodyComponentBaseHeight(component);
}

void MoogLadderEditor::onEditorTimerTick() { refreshFrequencyResponse(); }

void MoogLadderEditor::refreshFrequencyResponse()
{
  auto read = [this](const juce::String& id, float fallback) -> float
  {
    if (auto* param = processor.parameters.valueTreeState.getParameter(id))
      return param->convertFrom0to1(param->getValue());
    return fallback;
  };

  const float cutoff = read("cutoff", 1000.0f);
  const float resonance = read("resonance", 0.1f);
  const float drive = read("drive", 1.0f);
  const int modeChoice = juce::roundToInt(read("mode", 0.0f));
  const int satChoice = juce::roundToInt(read("saturator", 0.0f));
  const int qualityChoice = juce::roundToInt(read("quality", 1.0f));
  const int adaaChoice = juce::roundToInt(read("adaa", 1.0f));
  const int osQualityChoice = juce::roundToInt(
      read(processor.paramOversampleQuality.paramID, (float)processor.paramOversampleQuality.defaultChoice));
  const int osFactor = osQualityChoice >= 2 ? 8 : (osQualityChoice >= 1 ? 4 : 2);

  double hostSr = processor.getSampleRate();
  if (!(hostSr > 0.0)) hostSr = 48000.0;

  // Avoid reallocating the AC probe every UI tick when nothing changed.
  const bool changed = std::abs(cutoff - lastCutoff_) > 1.0e-4f || std::abs(resonance - lastResonance_) > 1.0e-5f
                       || std::abs(drive - lastDrive_) > 1.0e-4f || modeChoice != lastModeChoice_
                       || satChoice != lastSatChoice_ || qualityChoice != lastQualityChoice_
                       || adaaChoice != lastAdaaChoice_ || osFactor != lastOsFactor_
                       || std::abs(hostSr - lastSampleRate_) > 0.5;

  if (!changed) return;

  lastCutoff_ = cutoff;
  lastResonance_ = resonance;
  lastDrive_ = drive;
  lastModeChoice_ = modeChoice;
  lastSatChoice_ = satChoice;
  lastQualityChoice_ = qualityChoice;
  lastAdaaChoice_ = adaaChoice;
  lastOsFactor_ = osFactor;
  lastSampleRate_ = hostSr;

  // FR is evaluated after the upsampler (internal rate), matching the live graph.
  frCurve_.setHostRateAndOversample(hostSr, osFactor);
  frCurve_.setParameters(cutoff, resonance, drive, modeFromChoice(modeChoice), saturatorFromChoice(satChoice),
                         qualityFromChoice(qualityChoice), adaaChoice >= 1);
}
