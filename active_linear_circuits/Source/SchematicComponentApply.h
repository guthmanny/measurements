#pragma once

#include "Ds1OpampAcMath.h"
#include "SchematicComponentValues.h"

namespace ds1_ac
{

/** Apply schematic overlay component values to a freshly created circuit instance. */
void applySchematicComponentValues(CircuitKind circuit,
                                   void* instance,
                                   const SchematicComponentValues* values);

} // namespace ds1_ac
