#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "KbussParamUiUtils.h"

class AudioEffectFrameworkProcessor;
class AtomLookAndFeel;

/** Main-panel rows for user-facing middle-processor kbuss parameters. */
class EffectUserParamsPanel final : public juce::Component
{
public:
    EffectUserParamsPanel(AudioEffectFrameworkProcessor& processor, AtomLookAndFeel& lookAndFeel);

    void rebuildFromMiddleProcessor();
    [[nodiscard]] int preferredHeight() const noexcept { return preferredHeight_; }

    void resized() override;

private:
    void clearRows();
    void addParamRow(const kbuss::ParameterDescriptor& desc);

    AudioEffectFrameworkProcessor& processor_;
    AtomLookAndFeel& atomLookAndFeel_;
    juce::OwnedArray<juce::Component> rowComponents_;
    int preferredHeight_ = 0;

    static constexpr int kRowHeight = 50;
    static constexpr int kPanelPadding = 8;
    static constexpr int kMinHeight = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectUserParamsPanel)
};
