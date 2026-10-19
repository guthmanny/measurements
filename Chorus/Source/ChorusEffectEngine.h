#pragma once

#include "KbussEffectEngine.h"

/** kbuss chain with a swappable Chorus / Phase90 middle slot. */
class ChorusEffectEngine final : public KbussEffectEngine
{
public:
    enum class Effect
    {
        Chorus = 0,
        StereoPhase90 = 1,
        MonoPhase90 = 2,
        Phase90 = StereoPhase90
    };

    ChorusEffectEngine() = default;

    void setEffect(Effect effect) noexcept;
    [[nodiscard]] Effect effect() const noexcept { return effect_; }

    void reprepare(float sampleRate, std::uint32_t maxBlockSize);

protected:
    bool installMiddleProcessors(const ProcessorCreateFn& create) override;

private:
    Effect effect_ = Effect::Chorus;
};
