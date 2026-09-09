#pragma once

#include <vector>

#include "CircuitBindingResolver.h"
#include "kbuss/processor.hpp"

namespace white_box_lab {

class SvgCircuitSync
{
public:
    void loadModule(kbuss::Processor* middle,
                    const juce::String& topologyModuleId,
                    const juce::File& svgFile,
                    const juce::File& cgJsonFile);

    void pullFromProcessor(kbuss::Processor* middle);
    bool pushElement(kbuss::Processor* middle, const juce::String& paramId, double domainValue);

    [[nodiscard]] const std::vector<CircuitBinding>& bindings() const { return bindings_; }
    [[nodiscard]] juce::File svgFile() const { return svgFile_; }
    [[nodiscard]] juce::String topologyId() const { return topologyId_; }

private:
    juce::File svgFile_;
    juce::String topologyId_;
    std::vector<CircuitBinding> bindings_;
};

}  // namespace white_box_lab
