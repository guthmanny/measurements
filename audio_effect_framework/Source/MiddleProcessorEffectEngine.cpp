#include "MiddleProcessorEffectEngine.h"

MiddleProcessorEffectEngine::MiddleProcessorEffectEngine (const char* pluginUid,
                                                          const char* pluginName,
                                                          const char* instanceName)
    : pluginUid_ (pluginUid),
      pluginName_ (pluginName),
      instanceName_ (instanceName)
{
}

bool MiddleProcessorEffectEngine::installMiddleProcessors (const ProcessorCreateFn& create)
{
    const auto id = create (pluginUid_, pluginName_, instanceName_);
    middleProcessorId_ = id;
    return id != kbuss::kInvalidObjectId;
}
