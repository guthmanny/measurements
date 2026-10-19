#pragma once

#include <optional>
#include <string>

#include <juce_atom_theme/juce_atom_theme.h>
#include <juce_core/juce_core.h>

#include "nudsp/common/pot_params.h"

namespace kbuss
{
class Processor;
}

namespace aef::schematic
{

struct ResolvedComponent
{
    juce::String paramId;
    juce::String unit;
    float value = 0.0f;
    nx_pot_taper_e potTaper = NX_POT_TAPER_COUNT;
};

std::optional<ResolvedComponent> resolveComponent(const kbuss::Processor& processor,
                                                 const juce::String& moduleId,
                                                 const juce::String& overlayKey);

} // namespace aef::schematic
