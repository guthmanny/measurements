#pragma once

#include "Ds1OpampAcMath.h"
#include "SchematicComponentValues.h"

namespace ds1_ac
{

/** Apply schematic overlay component values to a freshly created circuit instance. */
void applySchematicComponentValues(CircuitKind circuit,
                                   void* instance,
                                   const SchematicComponentValues* values);

struct OperatorDeviceModels
{
    nx_opamp_model_e opamp{NX_OPAMP_BA728};
    nx_diode_model_t diode{NX_DIODE_1N4148};
    nx_bjt_npn_model_e bjt{NX_BJT_2N3904};
    bool hasOpamp{false};
    bool hasDiode{false};
    bool hasBjt{false};
};

/** Read default R/C/V/pot values from a freshly created operator instance. */
SchematicComponentValues readOperatorComponentValues(CircuitKind circuit);

/** Read default device models from a freshly created operator instance. */
OperatorDeviceModels readOperatorDeviceModels(CircuitKind circuit);

} // namespace ds1_ac
