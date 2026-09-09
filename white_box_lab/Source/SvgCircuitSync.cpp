#include "SvgCircuitSync.h"

#include "ComponentValueCodec.h"
#include "SvgCircuitParser.h"

namespace white_box_lab {

void SvgCircuitSync::loadModule(kbuss::Processor* middle,
                                const juce::String& topologyModuleId,
                                const juce::File& svgFile,
                                const juce::File& cgJsonFile)
{
    topologyId_ = topologyModuleId;
    svgFile_ = svgFile;
    bindings_ = CircuitBindingResolver::bind(topologyModuleId, SvgCircuitParser::parseFile(svgFile),
                                             cgJsonFile);
    pullFromProcessor(middle);
}

void SvgCircuitSync::pullFromProcessor(kbuss::Processor* middle)
{
    if (middle == nullptr)
        return;

    for (auto& binding : bindings_)
    {
        const auto* desc = middle->parameter(binding.paramId.toStdString());
        if (desc == nullptr)
            continue;

        float domain = desc->default_domain;
        if (middle->get_parameter_domain(desc->index, domain) != kbuss::Status::Ok)
            continue;

        binding.element.displayValue = ComponentValueCodec::format(
            domain, juce::String(desc->unit));
    }
}

bool SvgCircuitSync::pushElement(kbuss::Processor* middle, const juce::String& paramId,
                                 double domainValue)
{
    if (middle == nullptr)
        return false;

    const auto* desc = middle->parameter(paramId.toStdString());
    if (desc == nullptr)
        return false;

    return middle->set_parameter_domain(desc->index, static_cast<float>(domainValue))
        == kbuss::Status::Ok;
}

}  // namespace white_box_lab
