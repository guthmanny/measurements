#pragma once

#include "AefJuceIncludes.h"
#include "KbussEffectEngine.h"

#include "extensions/white_box/white_box_os_f32.h"

namespace aef::composite_quality
{

inline constexpr const char* kMiddleQualityParamId = "quality";

[[nodiscard]] inline int clampChoice (int choice) noexcept
{
    return juce::jlimit (0, 2, choice);
}

[[nodiscard]] inline nx_composite_quality_e choiceToEnum (int choice) noexcept
{
    return static_cast<nx_composite_quality_e> (clampChoice (choice));
}

[[nodiscard]] inline float choiceToNormalized (int choice) noexcept
{
    return static_cast<float> (clampChoice (choice)) / 2.0f;
}

[[nodiscard]] inline int islandUpFactorForChoice (int choice) noexcept
{
    return nx_white_box_island_up_factor_f32 (choiceToEnum (choice));
}

/** Apply composite processing quality when the middle processor exposes a "quality" param. */
[[nodiscard]] inline bool applyToMiddle (KbussEffectEngine& engine, int choice)
{
    const auto middleId = engine.middleProcessorId();
    if (middleId == kbuss::kInvalidObjectId)
        return false;

    if (engine.paramDescriptor (middleId, kMiddleQualityParamId) == nullptr)
        return false;

    engine.setParamNormalized (middleId, kMiddleQualityParamId, choiceToNormalized (choice));
    return true;
}

}  // namespace aef::composite_quality
