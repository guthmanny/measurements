#include "EffectHeaderComponent.h"

#include "AefJuceIncludes.h"
#include "MeterDisplayUtils.h"

namespace
{
void setupRotarySlider(atom::Slider& slider, const juce::String& label, double min, double max, double value)
{
  slider.setRange(min, max, (max - min) > 100.0 ? 0.1 : 0.01);
  slider.setValue(value, juce::dontSendNotification);
  slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
  slider.setValueLabelPos(atom::Slider::ValueLabelPos::Below);
  slider.setCustomText(label);
  slider.setRequiredWidthMode(atom::Slider::RequiredWidthMode::Content);
  slider.setValueLabelGap(0);
}
}  // namespace

EffectHeaderComponent::EffectHeaderComponent()
{
  meter_display::configurePeakMeter(meterLeft, 1);
  meter_display::configurePeakMeter(meterRight, 2);
  applyMeterSettings (80);

  ::setupRotarySlider (sliderInput, "INPUT", -12.0, 12.0, 0.0);
  ::setupRotarySlider (sliderGate, "GATE", -80.0, 0.0, -40.0);
  ::setupRotarySlider (sliderOutput, "OUTPUT", -100.0, 0.0, 0.0);

  btnSettings.setTooltip("Settings");
  btnTuner.setTooltip("Tuner");
  btnSpectrum.setTooltip("Spectrum");

  atom::TapCircle::TapConfig tapConfig;
  tapConfig.initialBpm = 30.0;
  tapConfig.minTapBpm = 1.0;
  tapConfig.maxTapBpm = 1200.0;
  tapTempo.setTapConfig(tapConfig);

  // ---- Register child controls with the generic HeaderBar layout skeleton ----
  const int h = 80;  // reference height used to derive knob/meter sizes
  const int knobSize = (int)(h * 0.75f);
  const int margin = h / 6;
  const int meterH = (int)(h * 0.85f);
  const int meterW = 14;
  const int cogSize = juce::jmin(knobSize, 28);
  const int tapSize = juce::jmin(knobSize, 48);
  const auto tapStyle = tapTempo.getResolvedStyle();
  const auto tapFont = AtomLookAndFeel::getUIFont(tapStyle.textFontHeight, juce::Font::plain);
  const int tapLabelWidth = juce::roundToInt(tapFont.getStringWidthFloat("120 BPM")) + 8;
  const int tapWidth = juce::jmax(tapSize, tapLabelWidth);

  const int inW = sliderInput.getRequiredWidth(knobSize);
  const int inH = sliderInput.getRequiredHeight(knobSize);
  const int gateW = sliderGate.getRequiredWidth(knobSize);
  const int gateH = sliderGate.getRequiredHeight(knobSize);
  const int outW = sliderOutput.getRequiredWidth(knobSize);
  const int outH = sliderOutput.getRequiredHeight(knobSize);

  using atom::core::FlexRowItem;

  addItem(&meterLeft, {meterW, meterH, 6, 10});
  addItem(&sliderInput, {inW, inH, 6, 6});
  addItem(&sliderGate, {gateW, gateH, 6, 6});
  addItem(&btnSettings, {cogSize, cogSize, 6, 6});
  addItem(&btnTuner, {cogSize, cogSize, 6, 6});
  addItem(&btnSpectrum, {cogSize, cogSize, 6, 6});
  addItem(&tapTempo, {tapWidth, tapSize, 6, 6});

  FlexRowItem spacer;
  spacer.flexGrow = 1.0f;
  addItem(nullptr, spacer);

  addItem(&sliderOutput, {outW, outH, 6, 6});
  addItem(&meterRight, {meterW * 2, meterH, 10, 6});

  // Reserve horizontal padding so content does not touch the edges.
  setContentPadding(margin * 2);
}

EffectHeaderComponent::~EffectHeaderComponent() = default;

void EffectHeaderComponent::applyMeterSettings(int displayRangeDbSpanIn)
{
  const int displayRangeDbSpan = juce::jmax(1, displayRangeDbSpanIn);
  for (auto* meter : {&meterLeft, &meterRight}) meter_display::applyMeterValueRange(*meter, displayRangeDbSpan);
}

void EffectHeaderComponent::setMeterDisplayLevels(float monoNorm, float leftNorm, float rightNorm)
{
  meterLeft.setLevels({juce::jlimit(0.0f, 1.0f, monoNorm)});
  meterRight.setLevels({juce::jlimit(0.0f, 1.0f, leftNorm), juce::jlimit(0.0f, 1.0f, rightNorm)});
}
