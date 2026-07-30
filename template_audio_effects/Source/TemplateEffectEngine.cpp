#include "TemplateEffectEngine.h"

bool TemplateEffectEngine::installMiddleProcessors (const ProcessorCreateFn& create)
{
    // Placeholder: swap UID/name/instance for your effect (see ds1/Ds1EffectEngine.cpp).
    const auto id = create ("com.minibuss.simple_gain", "Effect", "effect");
    middleProcessorId_ = id;
    return id != minibuss::kInvalidObjectId;
}
