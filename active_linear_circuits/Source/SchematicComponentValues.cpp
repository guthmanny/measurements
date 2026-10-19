#include "SchematicComponentValues.h"

#include <juce_atom_theme/juce_atom_theme.h>

namespace ds1_ac
{

std::optional<nx_pot_taper_e> SchematicComponentValues::parsePotTaperSuffix(const juce::String& text)
{
    if (const auto taper = atom::schematic::ComponentLabel::parsePotTaperSuffix(text))
        return static_cast<nx_pot_taper_e>(*taper);
    return std::nullopt;
}

std::optional<double> SchematicComponentValues::parseLabelText(const juce::String& text)
{
    return atom::schematic::ComponentLabel::parse(text);
}

juce::String SchematicComponentValues::formatLabelText(const juce::String& key, double value)
{
    return formatLabelText(key, value, NX_POT_TAPER_COUNT);
}

juce::String SchematicComponentValues::formatLabelText(const juce::String& key, double value, nx_pot_taper_e taper)
{
    const int taperCode = taper < NX_POT_TAPER_COUNT ? static_cast<int>(taper)
                                                     : atom::schematic::kPotTaperNone;
    return atom::schematic::ComponentLabel::format(key, {}, value, taperCode);
}

} // namespace ds1_ac
