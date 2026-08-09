#pragma once

#include <vector>

#include "../JuceLibraryCode/JuceHeader.h"
#include <juce_atom_theme/juce_atom_theme.h>
#include "nudsp/filters/moog_ladder.hpp"

/** Post-upsampler Moog Ladder magnitude response (Atom CurveControl DisplayOnly).
 *
 *  AC is evaluated at hostRate × oversampleFactor. Plot axes are fixed:
 *  X = 20 Hz … 20 kHz, Y = +10 … -60 dB. */
class MoogLadderFrCurveComponent final : public juce::Component
{
 public:
  MoogLadderFrCurveComponent();

  /** Host device rate and oversample factor; AC uses hostRate × factor. */
  void setHostRateAndOversample(double hostSampleRateHz, int oversampleFactor);

  void setParameters(float cutoffHz, float resonance, float drive, nx_moog_ladder_mode_t mode,
                     nx_moog_ladder_sat_t saturator, nx_moog_ladder_quality_t quality, bool adaaEnabled);

  void resized() override;

 private:
  void rebuildCurve();
  void syncCurveControl();

  [[nodiscard]] double dspSampleRate() const noexcept;

  static juce::String formatFrequencyTick(float log10Hz);
  static juce::String formatMagnitudeTick(float magDb);
  static std::vector<float> buildLogFrequencyTicks();
  static std::vector<float> buildMagnitudeTicks();

  atom::CurveControl curveControl_;

  double hostSampleRate_ = 48000.0;
  int oversampleFactor_ = 2;
  float cutoffHz_ = 1000.0f;
  float resonance_ = 0.1f;
  float drive_ = 1.0f;
  nx_moog_ladder_mode_t mode_ = NX_MOOG_LADDER_LP4;
  nx_moog_ladder_sat_t saturator_ = NX_MOOG_LADDER_SAT_ALGEBRAIC;
  nx_moog_ladder_quality_t quality_ = NX_MOOG_LADDER_QUALITY_RELINEARIZED;
  bool adaaEnabled_ = true;

  std::vector<double> freqsHz_;
  std::vector<double> magDb_;
  std::vector<std::pair<float, float>> magnitudeCurve_;

  static constexpr int numPoints = 256;
  static constexpr double freqMinHz = 20.0;
  static constexpr double freqMaxHz = 20000.0;
  static constexpr float magMinDb = -60.0f;
  static constexpr float magMaxDb = 10.0f;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MoogLadderFrCurveComponent)
};
