#include "CircuitBindingResolver.h"

namespace white_box_lab {
namespace {

juce::String prefixed(const juce::String& prefix, const juce::String& field)
{
    return prefix.isEmpty() ? field : prefix + "." + field;
}

std::vector<juce::String> potFieldsFromCg(const juce::var& config)
{
    std::vector<juce::String> pots;
    if (auto* obj = config.getDynamicObject())
    {
        for (const auto& prop : obj->getProperties())
        {
            if (auto* child = prop.value.getDynamicObject())
                if (child->hasProperty("control") || child->hasProperty("rt"))
                    pots.push_back(prop.name.toString());
        }
    }
    return pots;
}

bool configHasField(const juce::var& config, const juce::String& field)
{
    if (auto* obj = config.getDynamicObject())
        return obj->hasProperty(field);
    return false;
}

}  // namespace

std::vector<CircuitBinding> CircuitBindingResolver::bind(const juce::String& topologyPrefix,
                                                         const std::vector<CircuitElement>& elements,
                                                         const juce::File& cgJsonFile)
{
    juce::var config;
    if (cgJsonFile.existsAsFile())
    {
        const auto parsed = juce::JSON::parse(cgJsonFile);
        if (auto* root = parsed.getDynamicObject())
            config = root->getProperty("config");
    }

    const auto pots = potFieldsFromCg(config);
    std::vector<CircuitBinding> bindings;

    for (const auto& element : elements)
    {
        CircuitBinding binding;
        binding.element = element;

        switch (element.kind)
        {
            case CircuitElementKind::Control:
                binding.paramId = prefixed(topologyPrefix, element.cellId.toLowerCase() + ".control");
                if (! pots.empty() && ! configHasField(config, element.cellId.toLowerCase()))
                {
                    const auto leaf = element.cellId.toLowerCase();
                    for (const auto& pot : pots)
                        if (pot.equalsIgnoreCase(leaf))
                            binding.paramId = prefixed(topologyPrefix, pot + ".control");
                }
                break;
            case CircuitElementKind::PowerRail:
                binding.paramId = prefixed(topologyPrefix, element.cellId.toLowerCase());
                break;
            case CircuitElementKind::Pot:
                if (! pots.empty())
                    binding.paramId = prefixed(topologyPrefix, pots.front() + ".rt");
                else
                    binding.paramId = prefixed(topologyPrefix, element.cellId + ".rt");
                break;
            case CircuitElementKind::Resistor:
            case CircuitElementKind::Capacitor:
                binding.paramId = prefixed(topologyPrefix, element.cellId);
                break;
            case CircuitElementKind::Other:
                continue;
        }

        bindings.push_back(std::move(binding));
    }

    return bindings;
}

}  // namespace white_box_lab
