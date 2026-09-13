#include "SchematicComponentValues.h"

#include <cmath>
#include <cstring>

namespace ds1_ac
{

namespace
{
struct TaperSuffix
{
    const char* text;
    nx_pot_taper_e taper;
};

constexpr TaperSuffix kTaperSuffixes[] = {
    {"A15", NX_POT_TAPER_A15},
    {"A30", NX_POT_TAPER_A30},
    {"A45", NX_POT_TAPER_A45},
    {"C30", NX_POT_TAPER_C},
    {"4B", NX_POT_TAPER_4B},
    {"3B", NX_POT_TAPER_3B},
    {"A", NX_POT_TAPER_A30},
    {"B", NX_POT_TAPER_LINEAR},
    {"C", NX_POT_TAPER_C},
    {"G", NX_POT_TAPER_G},
};

juce::String stripPotTaperSuffix(juce::String text)
{
    for (const auto& suffix : kTaperSuffixes)
    {
        if (text.endsWithIgnoreCase(suffix.text))
            return text.dropLastCharacters(int(std::strlen(suffix.text)));
    }
    return text;
}

const char* potTaperSchematicSuffix(nx_pot_taper_e taper) noexcept
{
    switch (taper)
    {
    case NX_POT_TAPER_LINEAR:
        return "B";
    case NX_POT_TAPER_A15:
        return "A15";
    case NX_POT_TAPER_A30:
        return "A";
    case NX_POT_TAPER_A45:
        return "A45";
    case NX_POT_TAPER_4B:
        return "G";
    case NX_POT_TAPER_C:
        return "C";
    case NX_POT_TAPER_3B:
        return "3B";
    default:
        return "";
    }
}
} // namespace

std::optional<nx_pot_taper_e> SchematicComponentValues::parsePotTaperSuffix(const juce::String& text)
{
    auto trimmed = text.trim();
    if (trimmed.startsWithChar('+'))
        trimmed = trimmed.substring(1).trim();

    for (const auto& suffix : kTaperSuffixes)
    {
        if (trimmed.endsWithIgnoreCase(suffix.text))
            return suffix.taper;
    }
    return std::nullopt;
}

std::optional<double> SchematicComponentValues::parseLabelText(const juce::String& text)
{
    auto trimmed = text.trim();
    if (trimmed.isEmpty())
        return std::nullopt;

    if (trimmed.startsWithChar('+'))
        trimmed = trimmed.substring(1).trim();

    trimmed = stripPotTaperSuffix(trimmed).trim();

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

namespace
{
juce::String formatScaled(double value, const char* suffix, double scale)
{
    if (! std::isfinite(value) || scale <= 0.0)
        return {};

    const double scaled = value / scale;
    const int decimals = scaled >= 100.0 ? 0 : (scaled >= 10.0 ? 1 : 2);
    auto text = juce::String(scaled, decimals);
    if (text.containsChar('.'))
    {
        while (text.endsWithChar('0'))
            text = text.dropLastCharacters(1);
        if (text.endsWithChar('.'))
            text = text.dropLastCharacters(1);
    }

    return text + suffix;
}

bool isCapacitorKey(const juce::String& key) noexcept
{
    return key.length() >= 2 && key.startsWithIgnoreCase("C")
           && key.substring(1).containsOnly("0123456789");
}

bool isResistorOrPotKey(const juce::String& key) noexcept
{
    if (key.length() < 2)
        return key.equalsIgnoreCase("GAIN") || key.equalsIgnoreCase("TONE")
               || key.equalsIgnoreCase("LEVEL") || key.equalsIgnoreCase("DRIVE")
               || key.equalsIgnoreCase("BASS") || key.equalsIgnoreCase("TREBLE")
               || key.equalsIgnoreCase("DISTORTION") || key.equalsIgnoreCase("RT")
               || key.equalsIgnoreCase("RV");

    const auto head = key.substring(0, 1);
    return (head.equalsIgnoreCase("R") || head.equalsIgnoreCase("P"))
           && key.substring(1).containsOnly("0123456789");
}

bool isVoltageKey(const juce::String& key) noexcept
{
    return key.equalsIgnoreCase("VA") || key.equalsIgnoreCase("VCC")
           || key.equalsIgnoreCase("VBIAS") || key.equalsIgnoreCase("VCE")
           || key.equalsIgnoreCase("VCE_SAT");
}

bool isPotKey(const juce::String& key) noexcept
{
    if (key.equalsIgnoreCase("GAIN") || key.equalsIgnoreCase("TONE")
        || key.equalsIgnoreCase("LEVEL") || key.equalsIgnoreCase("DRIVE")
        || key.equalsIgnoreCase("BASS") || key.equalsIgnoreCase("TREBLE")
        || key.equalsIgnoreCase("DISTORTION") || key.equalsIgnoreCase("RT")
        || key.equalsIgnoreCase("RV"))
        return true;

    return key.length() >= 2 && key.startsWithIgnoreCase("P")
           && key.substring(1).containsOnly("0123456789");
}
} // namespace

juce::String SchematicComponentValues::formatLabelText(const juce::String& key, double value)
{
    return formatLabelText(key, value, NX_POT_TAPER_COUNT);
}

juce::String SchematicComponentValues::formatLabelText(const juce::String& key, double value, nx_pot_taper_e taper)
{
    if (! std::isfinite(value))
        return {};

    if (isCapacitorKey(key))
    {
        const double absValue = std::abs(value);
        if (absValue >= 1.0e-3)
            return formatScaled(value, "m", 1.0e-3);
        if (absValue >= 1.0e-6)
            return formatScaled(value, "u", 1.0e-6);
        if (absValue >= 1.0e-9)
            return formatScaled(value, "n", 1.0e-9);
        return formatScaled(value, "p", 1.0e-12);
    }

    if (isResistorOrPotKey(key))
    {
        const double absValue = std::abs(value);
        juce::String text;
        if (absValue >= 1.0e6)
            text = formatScaled(value, "M", 1.0e6);
        else if (absValue >= 1.0e3)
            text = formatScaled(value, "k", 1.0e3);
        else
            text = formatScaled(value, "", 1.0);

        if (isPotKey(key) && taper < NX_POT_TAPER_COUNT)
            text += potTaperSchematicSuffix(taper);
        return text;
    }

    if (isVoltageKey(key))
    {
        const auto text = formatScaled(std::abs(value), "", 1.0);
        if (value >= 0.0)
            return "+" + text;
        return "-" + text;
    }

    return juce::String(value, 3);
}

} // namespace ds1_ac
