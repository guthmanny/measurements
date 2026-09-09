#include "ComponentValueCodec.h"

#include <cmath>

namespace white_box_lab {

std::optional<double> ComponentValueCodec::parse(const juce::String& text)
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

juce::String ComponentValueCodec::format(double value, const juce::String& unit)
{
    const auto absValue = std::abs(value);
    const auto unitLower = unit.toLowerCase();

    auto withPrefix = [value](double scaled, const char* suffix)
    {
        if (std::abs(scaled - std::round(scaled)) < 1.0e-6)
            return juce::String(juce::roundToInt(scaled)) + suffix;
        return juce::String(scaled, 3) + suffix;
    };

    if (unitLower.contains("ohm") || unit.contains("Ω"))
    {
        if (absValue >= 1.0e6)
            return withPrefix(value / 1.0e6, "M");
        if (absValue >= 1.0e3)
            return withPrefix(value / 1.0e3, "k");
        return withPrefix(value, "");
    }

    if (unitLower.startsWith("f") || unitLower.contains("farad"))
    {
        if (absValue >= 1.0e-6)
            return withPrefix(value / 1.0e-6, "u");
        if (absValue >= 1.0e-9)
            return withPrefix(value / 1.0e-9, "n");
        if (absValue >= 1.0e-12)
            return withPrefix(value / 1.0e-12, "p");
        return juce::String(value, 6);
    }

    if (unitLower.startsWith("v"))
        return (value >= 0.0 ? "+" : "") + juce::String(value, 1);

    return juce::String(value, 4);
}

}  // namespace white_box_lab
