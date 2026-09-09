#include "WhiteBoxEffectEngine.h"

#include "../JuceLibraryCode/JuceHeader.h"

void WhiteBoxEffectEngine::setModel(const juce::String& uid, const juce::String& name) noexcept
{
    if (uid.isNotEmpty())
        uid_ = uid;
    if (name.isNotEmpty())
        name_ = name;
}

void WhiteBoxEffectEngine::reprepare(float sampleRate, std::uint32_t maxBlockSize)
{
    release();
    prepare(sampleRate, maxBlockSize);
}

bool WhiteBoxEffectEngine::installMiddleProcessors(const ProcessorCreateFn& create)
{
    const auto id = create(uid_.toRawUTF8(), name_.toRawUTF8(), "white_box");
    middleProcessorId_ = id;
    return id != kbuss::kInvalidObjectId;
}
