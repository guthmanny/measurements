#include "ChorusEffectEngine.h"

#include "../JuceLibraryCode/JuceHeader.h"

void ChorusEffectEngine::setEffect(Effect effect) noexcept
{
    effect_ = effect;
}

void ChorusEffectEngine::reprepare(float sampleRate, std::uint32_t maxBlockSize)
{
    release();
    prepare(sampleRate, maxBlockSize);
}

bool ChorusEffectEngine::installMiddleProcessors(const ProcessorCreateFn& create)
{
    const char* uid = "com.chorus.nudsp.camel.mono_chorus";
    const char* name = "Mono Chorus";
    const char* instance = "mono_chorus";
    if (effect_ == Effect::MonoPhase90)
    {
        uid = "com.chorus.nudsp.camel.mono_phase90";
        name = "Mono Phase90";
        instance = "mono_phase90";
    }
    else if (effect_ == Effect::StereoPhase90)
    {
        uid = "com.chorus.nudsp.camel.phase90";
        name = "Stereo Phase90";
        instance = "phase90";
    }
    const auto id = create(uid, name, instance);
    middleProcessorId_ = id;
    return id != kbuss::kInvalidObjectId;
}
