#pragma once

#include <vector>

#include "AefJuceIncludes.h"

namespace white_box_lab {

enum class CircuitElementKind
{
    Resistor,
    Capacitor,
    Pot,
    Control,
    PowerRail,
    Other
};

struct CircuitElement
{
    CircuitElementKind kind = CircuitElementKind::Other;
    juce::String cellId;
    juce::String displayValue;
    juce::Rectangle<float> bounds;
};

class SvgCircuitParser
{
public:
    [[nodiscard]] static std::vector<CircuitElement> parseFile(const juce::File& svgFile);
};

}  // namespace white_box_lab
