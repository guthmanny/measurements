#pragma once

#include "AudioEffectFrameworkEditor.h"
#include "MoogLadderFrCurveComponent.h"
#include "PluginProcessor.h"

class MoogLadderEditor final : public AudioEffectFrameworkEditor
{
 public:
  explicit MoogLadderEditor(MoogLadderAudioProcessor& processor);

 protected:
  int getBodyComponentBaseHeight(const juce::Component* component) const noexcept override;
  void onEditorTimerTick() override;

 private:
  void refreshFrequencyResponse();

  MoogLadderFrCurveComponent frCurve_;

  float lastCutoff_ = -1.0f;
  float lastResonance_ = -1.0f;
  float lastDrive_ = -1.0f;
  int lastModeChoice_ = -1;
  int lastSatChoice_ = -1;
  int lastQualityChoice_ = -1;
  int lastAdaaChoice_ = -1;
  int lastOsFactor_ = -1;
  double lastSampleRate_ = -1.0;

  static constexpr int frCurveBaseHeight = 200;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MoogLadderEditor)
};
