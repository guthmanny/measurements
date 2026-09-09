#pragma once

#include <optional>

#include "AefJuceIncludes.h"

namespace white_box_lab {

/** SI-style schematic labels <-> domain values (Ω / F / V). */
struct ComponentValueCodec
{
    static std::optional<double> parse(const juce::String& text);
    static juce::String format(double value, const juce::String& unit);
};

}  // namespace white_box_lab
