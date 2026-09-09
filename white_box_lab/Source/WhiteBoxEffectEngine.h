#pragma once

#include "AefJuceIncludes.h"
#include "KbussEffectEngine.h"

/** kbuss chain with a swappable white_box middle processor. */
class WhiteBoxEffectEngine final : public KbussEffectEngine
{
public:
    void setModel(const juce::String& uid, const juce::String& name) noexcept;
    [[nodiscard]] juce::String modelUid() const { return uid_; }
    [[nodiscard]] juce::String modelName() const { return name_; }

    void reprepare(float sampleRate, std::uint32_t maxBlockSize);

protected:
    bool installMiddleProcessors(const ProcessorCreateFn& create) override;

private:
    juce::String uid_ { "com.kbuss.nudsp.white_box.ds1" };
    juce::String name_ { "DS-1" };
};
