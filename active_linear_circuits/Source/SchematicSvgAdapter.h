#pragma once

#include <mudsp/schematic/svg_cell_id.hpp>

#include "core/SvgDocument.h"

namespace ds1_ac
{

inline atom::svg::OverlayInfo toAtomOverlay(const mudsp::schematic::CellOverlayInfo& info)
{
    switch (info.mode)
    {
    case mudsp::schematic::CellOverlayMode::Editable:
        return {info.key, atom::svg::OverlayKind::Editable};
    case mudsp::schematic::CellOverlayMode::ReadOnly:
        return {info.key, atom::svg::OverlayKind::Clickable};
    case mudsp::schematic::CellOverlayMode::Selector:
        return {info.key, atom::svg::OverlayKind::Selector};
    default:
        return {};
    }
}

inline atom::svg::OverlayInfo classifyCircuitSvgId(const std::string& elementId)
{
    return toAtomOverlay(mudsp::schematic::classifySvgCellId(elementId));
}

} // namespace ds1_ac
