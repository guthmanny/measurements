#pragma once

#include "MinibussEffectEngine.h"

/** Gain → NoiseGate → DS-1 → Level */
class Ds1EffectEngine final : public MinibussEffectEngine
{
protected:
    bool installMiddleProcessors (const ProcessorCreateFn& create) override;
};
