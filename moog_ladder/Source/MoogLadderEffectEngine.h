#pragma once

#include "MinibussEffectEngine.h"

/** Gain → NoiseGate → Upsampler → MoogLadder → Downsampler → Level */
class MoogLadderEffectEngine final : public MinibussEffectEngine
{
 protected:
  bool installMiddleProcessors(const ProcessorCreateFn& create) override;
};
