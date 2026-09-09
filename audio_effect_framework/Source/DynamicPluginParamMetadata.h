#pragma once

#include <cstdint>
#include <vector>

#include "AefJuceIncludes.h"

namespace aef::dynamic_plugin_params
{

struct Meta
{
    juce::String id;
    juce::String label;
    std::uint32_t index = 0;
    float minDomain = 0.f;
    float maxDomain = 1.f;
    float defaultDomain = 0.5f;

    [[nodiscard]] float domainToNormalized (float domain) const noexcept;
    [[nodiscard]] juce::String displayLabel() const;
};

/** Load user knob metadata from a .kbplug bundle (plugin.json, then kb_plugin_parameters). */
[[nodiscard]] std::vector<Meta> loadFromBundle (const juce::File& bundlePath,
                                                const juce::String& pluginUid = {});

}  // namespace aef::dynamic_plugin_params
