#pragma once

#include <juce_atom_theme/juce_atom_theme.h>

#include "kbuss/parameter.hpp"

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

inline void configureKbussChoiceCombo(atom::ComboBox& combo,
                                     const kbuss::ParameterDescriptor& desc,
                                     int selectedIndex)
{
    combo.clear(juce::dontSendNotification);
    for (std::size_t i = 0; i < desc.choices.size(); ++i)
        combo.addItem(desc.choices[i], static_cast<int>(i + 1));

    const int clamped = juce::jlimit(0, static_cast<int>(desc.choices.size()) - 1, selectedIndex);
    combo.setSelectedId(clamped + 1, juce::dontSendNotification);
    combo.setJustificationType(juce::Justification::centredLeft);
}

}  // namespace aef::kbuss_param_ui
