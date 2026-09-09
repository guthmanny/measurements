#pragma once

#include <optional>
#include <vector>

#include "AefJuceIncludes.h"
#include "SvgCircuitParser.h"

namespace white_box_lab {

struct CircuitBinding
{
    CircuitElement element;
    juce::String paramId;
};

class CircuitBindingResolver
{
public:
    /** Map parsed SVG elements onto kbuss param ids using topology prefix. */
    static std::vector<CircuitBinding> bind(const juce::String& topologyPrefix,
                                            const std::vector<CircuitElement>& elements,
                                            const juce::File& cgJsonFile);
};

}  // namespace white_box_lab
