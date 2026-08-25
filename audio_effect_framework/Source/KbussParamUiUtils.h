#pragma once

#include <string_view>

#include "AefJuceIncludes.h"
#include "kbuss/parameter.hpp"

namespace aef::kbuss_param_ui {

/** User knob: top-level id, or the `.control` entry from bind_smooth_control_all. */
[[nodiscard]] inline bool isUserFacingParam(const kbuss::ParameterDescriptor& desc) noexcept
{
    const std::string_view id = desc.id;
    if (id.find('.') == std::string_view::npos)
        return true;
    return id.ends_with(".control");
}

[[nodiscard]] inline bool isInternalParam(const kbuss::ParameterDescriptor& desc) noexcept
{
    return ! isUserFacingParam(desc);
}

[[nodiscard]] inline juce::String paramDisplayLabel(const kbuss::ParameterDescriptor& desc)
{
    if (! desc.label.empty())
        return juce::String(desc.label);
    return juce::String(desc.id);
}

/** Suffix for atom::Slider text box (leading space when unit present). */
[[nodiscard]] inline juce::String paramUnitSuffix(const kbuss::ParameterDescriptor& desc)
{
    if (desc.unit.empty())
        return {};
    return " " + juce::String(desc.unit);
}

[[nodiscard]] inline double paramSliderInterval(const kbuss::ParameterDescriptor& desc) noexcept
{
    if (desc.type == kbuss::ParameterType::Int)
        return 1.0;

    const double span = static_cast<double>(desc.max_domain - desc.min_domain);
    if (span <= 0.0)
        return 0.0;
    if (span <= 1.0)
        return 0.01;
    if (span <= 10.0)
        return 0.1;
    if (span <= 100.0)
        return 1.0;
    return span / 200.0;
}

}  // namespace aef::kbuss_param_ui
