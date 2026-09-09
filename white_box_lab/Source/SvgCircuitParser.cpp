#include "SvgCircuitParser.h"

#include <functional>

namespace white_box_lab {
namespace {

CircuitElementKind kindFromCell(const juce::String& prefix, const juce::String& id)
{
    if (prefix == "CONTROL")
        return CircuitElementKind::Control;
    if (prefix == "POWER")
        return CircuitElementKind::PowerRail;
    if (prefix == "COMPONENT")
    {
        if (id.startsWithIgnoreCase("P"))
            return CircuitElementKind::Pot;
        if (id.startsWithIgnoreCase("C"))
            return CircuitElementKind::Capacitor;
        if (id.startsWithIgnoreCase("R") || id.startsWithIgnoreCase("L"))
            return CircuitElementKind::Resistor;
    }
    return CircuitElementKind::Other;
}

juce::String firstText(const juce::XmlElement* element)
{
    if (element == nullptr)
        return {};
    if (element->hasTagName("text") || element->hasTagName("tspan"))
    {
        const auto text = element->getAllSubText().trim();
        if (text.isNotEmpty())
            return text;
    }
    for (auto* child : element->getChildIterator())
    {
        const auto text = firstText(child);
        if (text.isNotEmpty())
            return text;
    }
    return {};
}

juce::Rectangle<float> boundsFromGroup(const juce::XmlElement* element)
{
    juce::Rectangle<float> bounds;
    bool any = false;
    std::function<void(const juce::XmlElement*)> walk = [&](const juce::XmlElement* node)
    {
        if (node == nullptr)
            return;
        if (node->hasTagName("rect"))
        {
            const juce::Rectangle<float> r(
                (float) node->getDoubleAttribute("x"),
                (float) node->getDoubleAttribute("y"),
                (float) node->getDoubleAttribute("width"),
                (float) node->getDoubleAttribute("height"));
            bounds = any ? bounds.getUnion(r) : r;
            any = true;
        }
        for (auto* child : node->getChildIterator())
            walk(child);
    };
    walk(element);
    return bounds;
}

void collect(const juce::XmlElement* element, std::vector<CircuitElement>& out)
{
    if (element == nullptr)
        return;

    const auto cellId = element->getStringAttribute("data-cell-id");
    const auto colon = cellId.indexOfChar(':');
    if (colon > 0)
    {
        const auto prefix = cellId.substring(0, colon);
        const auto id = cellId.substring(colon + 1);
        if (prefix == "COMPONENT" || prefix == "CONTROL" || prefix == "POWER")
        {
            CircuitElement item;
            item.kind = kindFromCell(prefix, id);
            item.cellId = id;
            item.displayValue = firstText(element);
            item.bounds = boundsFromGroup(element);
            out.push_back(std::move(item));
        }
    }

    for (auto* child : element->getChildIterator())
        collect(child, out);
}

}  // namespace

std::vector<CircuitElement> SvgCircuitParser::parseFile(const juce::File& svgFile)
{
    std::vector<CircuitElement> elements;
    if (! svgFile.existsAsFile())
        return elements;

    auto xml = juce::parseXML(svgFile);
    if (xml == nullptr)
        return elements;

    collect(xml.get(), elements);
    return elements;
}

}  // namespace white_box_lab
