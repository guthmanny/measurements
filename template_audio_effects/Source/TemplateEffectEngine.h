#pragma once

#include "MinibussEffectEngine.h"

/** Gain → NoiseGate → Upsampler → [middle] → Downsampler → Level

    Replace SimpleGain with your minibuss plugin UID in installMiddleProcessors().
*/
class TemplateEffectEngine final : public MinibussEffectEngine
{
protected:
    bool installMiddleProcessors (const ProcessorCreateFn& create) override;
};
