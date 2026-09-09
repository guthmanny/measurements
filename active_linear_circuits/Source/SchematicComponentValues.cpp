#include "SchematicComponentValues.h"

#include <cmath>

namespace ds1_ac
{

std::optional<double> SchematicComponentValues::parseLabelText(const juce::String& text)
{
    auto trimmed = text.trim();
    if (trimmed.isEmpty())
        return std::nullopt;

    if (trimmed.startsWithChar('+'))
        trimmed = trimmed.substring(1).trim();

    double multiplier = 1.0;
    if (trimmed.endsWithIgnoreCase("pF") || trimmed.endsWithIgnoreCase("p"))
    {
        multiplier = 1.0e-12;
        trimmed = trimmed.dropLastCharacters(trimmed.endsWithIgnoreCase("pF") ? 2 : 1);
    }
    else if (trimmed.endsWithIgnoreCase("nF") || trimmed.endsWithIgnoreCase("n"))
    {
        multiplier = 1.0e-9;
        trimmed = trimmed.dropLastCharacters(trimmed.endsWithIgnoreCase("nF") ? 2 : 1);
    }
    else if (trimmed.endsWithIgnoreCase("uF") || trimmed.endsWithIgnoreCase("u")
             || trimmed.endsWithIgnoreCase("μF") || trimmed.endsWithIgnoreCase("µF"))
    {
        multiplier = 1.0e-6;
        if (trimmed.endsWithIgnoreCase("uF") || trimmed.endsWithIgnoreCase("μF")
            || trimmed.endsWithIgnoreCase("µF"))
            trimmed = trimmed.dropLastCharacters(2);
        else
            trimmed = trimmed.dropLastCharacters(1);
    }
    else if (trimmed.endsWithIgnoreCase("mF"))
    {
        multiplier = 1.0e-3;
        trimmed = trimmed.dropLastCharacters(2);
    }
    else if (trimmed.endsWithIgnoreCase("k") || trimmed.endsWithIgnoreCase("kΩ")
             || trimmed.endsWithIgnoreCase("kOhm"))
    {
        multiplier = 1.0e3;
        if (trimmed.endsWithIgnoreCase("kOhm"))
            trimmed = trimmed.dropLastCharacters(4);
        else if (trimmed.endsWithIgnoreCase("kΩ"))
            trimmed = trimmed.dropLastCharacters(2);
        else
            trimmed = trimmed.dropLastCharacters(1);
    }
    else if (trimmed.endsWithIgnoreCase("M") || trimmed.endsWithIgnoreCase("MΩ"))
    {
        multiplier = 1.0e6;
        trimmed = trimmed.dropLastCharacters(trimmed.endsWithIgnoreCase("MΩ") ? 2 : 1);
    }
    else if (trimmed.endsWithIgnoreCase("Ω") || trimmed.endsWithIgnoreCase("Ohm")
             || trimmed.endsWithIgnoreCase("V") || trimmed.endsWithIgnoreCase("F"))
    {
        if (trimmed.endsWithIgnoreCase("Ohm"))
            trimmed = trimmed.dropLastCharacters(3);
        else
            trimmed = trimmed.dropLastCharacters(1);
    }

    trimmed = trimmed.trim();
    if (! trimmed.containsOnly("0123456789.+-eE"))
        return std::nullopt;

    return trimmed.getDoubleValue() * multiplier;
}

} // namespace ds1_ac
