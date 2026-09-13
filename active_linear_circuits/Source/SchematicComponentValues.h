#pragma once

#include <map>
#include <optional>

#include <juce_core/juce_core.h>

#include "nudsp/common/pot_params.h"

namespace ds1_ac
{

struct SchematicComponentValues
{
    std::map<juce::String, double> values;
    std::map<juce::String, nx_pot_taper_e> potTapers;

    bool empty() const noexcept { return values.empty(); }

    bool operator==(const SchematicComponentValues& other) const noexcept
    {
        return values == other.values && potTapers == other.potTapers;
    }

    bool operator!=(const SchematicComponentValues& other) const noexcept
    {
        return ! (*this == other);
    }

    /** Parse overlay label text (e.g. "47k", "20kG", "100n", "+9") into SI units. */
    static std::optional<double> parseLabelText(const juce::String& text);

    /** IEC pot suffix from a schematic label, if present (B/C/G/A…). */
    static std::optional<nx_pot_taper_e> parsePotTaperSuffix(const juce::String& text);

    /** Format an operator-model value for schematic overlay text. */
    static juce::String formatLabelText(const juce::String& key, double value);
    static juce::String formatLabelText(const juce::String& key, double value, nx_pot_taper_e taper);
};

} // namespace ds1_ac
