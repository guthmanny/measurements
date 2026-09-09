#pragma once

#include <map>
#include <optional>

#include <juce_core/juce_core.h>

namespace ds1_ac
{

struct SchematicComponentValues
{
    std::map<juce::String, double> values;

    bool empty() const noexcept { return values.empty(); }

    bool operator==(const SchematicComponentValues& other) const noexcept
    {
        return values == other.values;
    }

    bool operator!=(const SchematicComponentValues& other) const noexcept
    {
        return ! (*this == other);
    }

    /** Parse overlay label text (e.g. "47k", "100n", "+9") into SI units. */
    static std::optional<double> parseLabelText(const juce::String& text);
};

} // namespace ds1_ac
