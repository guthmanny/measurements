#pragma once

#include <juce_atom_theme/juce_atom_theme.h>

namespace ds1_ac
{

inline atom::svg::OverlayInfo classifyCircuitSvgId(const std::string& elementId)
{
    return atom::schematic::classifyCellId(elementId);
}

} // namespace ds1_ac
