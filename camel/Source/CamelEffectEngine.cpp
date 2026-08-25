#include "CamelEffectEngine.h"

#include "../JuceLibraryCode/JuceHeader.h"

void CamelEffectEngine::setEffectIndex(int index) noexcept
{
    effectIndex_ = juce::jlimit(0, (int)kCamelEffectCount - 1, index);
}

void CamelEffectEngine::reprepare(float sampleRate, std::uint32_t maxBlockSize)
{
    release();
    prepare(sampleRate, maxBlockSize);
}

bool CamelEffectEngine::installMiddleProcessors(const ProcessorCreateFn& create)
{
    const auto& entry = kCamelEffects[(std::size_t)effectIndex_];
    const auto id = create(entry.uid, entry.displayName, "camel_fx");
    middleProcessorId_ = id;
    return id != kbuss::kInvalidObjectId;
}
