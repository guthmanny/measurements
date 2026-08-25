#pragma once

#include <juce_atom_theme/juce_atom_theme.h>

namespace aef::kbuss_param_ui {

/** Shared horizontal slider styling (value column width matches settings panels). */
inline void configureKbussParamSlider(atom::Slider& slider,
                                      AtomLookAndFeel& lookAndFeel,
                                      double minDomain,
                                      double maxDomain,
                                      double interval,
                                      const juce::String& unitSuffix)
{
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setRange(minDomain, maxDomain, interval);
    slider.setTextValueSuffix(unitSuffix);
    slider.setSliderSnapsToMousePosition(false);
    slider.setValueLabelPos(atom::Slider::ValueLabelPos::Right);

    atom::SliderStyleOverride styleOverride;
    styleOverride.metrics.linearHorizontalValueLabelReserveDlu = 72.0f;
    lookAndFeel.setSliderStyleOverride(slider, styleOverride);
}

}  // namespace aef::kbuss_param_ui
