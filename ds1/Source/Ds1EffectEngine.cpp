#include "Ds1EffectEngine.h"

bool Ds1EffectEngine::installMiddleProcessors (const ProcessorCreateFn& create)
{
    const auto id = create ("com.minibuss.nudsp.white_box.ds1", "DS-1", "ds1");
    middleProcessorId_ = id;
    return id != minibuss::kInvalidObjectId;
}
