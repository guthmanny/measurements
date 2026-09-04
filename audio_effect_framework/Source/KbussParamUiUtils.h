#pragma once

#include <string_view>
#include <unordered_set>

#include "AefJuceIncludes.h"
#include "kbuss/parameter.hpp"

namespace aef::kbuss_param_ui {

[[nodiscard]] inline bool isTopLevelParamId(std::string_view id) noexcept
{
    return id.find('.') == std::string_view::npos;
}

/** Leaf name for `prefix.knob.control` → `knob`; empty if not a `.control` id. */
[[nodiscard]] inline std::string_view controlLeafName(std::string_view id) noexcept
{
    constexpr std::string_view kSuffix = ".control";
    if (id.size() <= kSuffix.size() || ! id.ends_with(kSuffix))
        return {};

    id.remove_suffix(kSuffix.size());
    const auto dot = id.rfind('.');
    return dot == std::string_view::npos ? id : id.substr(dot + 1);
}

template <typename ParamRange>
[[nodiscard]] inline std::unordered_set<std::string> collectTopLevelParamIds(const ParamRange& params)
{
    std::unordered_set<std::string> topLevel;
    for (const auto& desc : params)
    {
        if (isTopLevelParamId(desc.id))
            topLevel.insert(desc.id);
    }
    return topLevel;
}

/** True when a top-level alias covers `*.leaf.control` (`bass` or `tremolo_intensity`). */
[[nodiscard]] inline bool topLevelAliasesControlLeaf(
    std::string_view leaf,
    const std::unordered_set<std::string>& topLevelIds) noexcept
{
    const std::string leafStr(leaf);
    if (topLevelIds.contains(leafStr))
        return true;

    for (const auto& topId : topLevelIds)
    {
        if (topId.size() <= leaf.size())
            continue;

        if (topId.ends_with(leafStr) && topId[topId.size() - leaf.size() - 1] == '_')
            return true;
    }

    return false;
}

/**
 * Main-panel knobs:
 * - top-level short aliases (`bass`, `drive`, …)
 * - nested `*.control` only when no short alias exists for that leaf
 *   (legacy plugins without bind_pot_control aliases)
 */
[[nodiscard]] inline bool isUserFacingParam(const kbuss::ParameterDescriptor& desc,
                                            const std::unordered_set<std::string>& topLevelIds) noexcept
{
    const std::string_view id = desc.id;
    if (isTopLevelParamId(id))
        return true;

    const auto leaf = controlLeafName(id);
    if (leaf.empty())
        return false;

    return ! topLevelAliasesControlLeaf(leaf, topLevelIds);
}

[[nodiscard]] inline bool isInternalParam(const kbuss::ParameterDescriptor& desc,
                                          const std::unordered_set<std::string>& topLevelIds) noexcept
{
    return ! isUserFacingParam(desc, topLevelIds);
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
