#pragma once

#include "KbussEffectEngine.h"

/** kbuss chain with a configurable middle plugin (UID / name / instance). */
class MiddleProcessorEffectEngine final : public KbussEffectEngine
{
public:
    MiddleProcessorEffectEngine (const char* pluginUid, const char* pluginName, const char* instanceName);

protected:
    bool installMiddleProcessors (const ProcessorCreateFn& create) override;

private:
    const char* pluginUid_;
    const char* pluginName_;
    const char* instanceName_;
};
