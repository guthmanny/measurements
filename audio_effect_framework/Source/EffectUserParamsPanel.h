#pragma once

#include <functional>
#include <map>
#include <memory>
#include <vector>

#include "DynamicPluginParamMetadata.h"
#include "KbussParamUiUtils.h"
#include "kbuss/processor.hpp"

class AudioEffectFrameworkProcessor;
class AtomLookAndFeel;
class GfxPedalPanel;

/** Main-panel rows for user-facing middle-processor kbuss parameters. */
class EffectUserParamsPanel final : public juce::Component
{
public:
    EffectUserParamsPanel(AudioEffectFrameworkProcessor& processor, AtomLookAndFeel& lookAndFeel);
    ~EffectUserParamsPanel() override;

    void rebuildFromMiddleProcessor();
    [[nodiscard]] int preferredWidth() const noexcept { return preferredWidth_; }
    [[nodiscard]] int preferredHeight() const noexcept { return preferredHeight_; }
    [[nodiscard]] bool usingGraphicPanel() const noexcept { return gfxPanel_ != nullptr; }
    [[nodiscard]] bool usingReadyGraphicPanel() const noexcept;

    void resized() override;
    void syncGraphicState();
    std::function<void()> onGeometryChanged;

private:
    void clearRows();
    void detachGraphic();
    void attachGraphic(GfxPedalPanel& panel);
    void publishGraphicGeometry();
    [[nodiscard]] bool tryLoadGraphicPanel();
    void addParamRow(const kbuss::ParameterDescriptor& desc);
    void addMetaParamRow(const aef::dynamic_plugin_params::Meta& meta, kbuss::Processor& middle);
    void addIndexedParamRow(std::uint32_t index, float initialValue);
    void rebuildIndexedParams(kbuss::Processor& middle);

    AudioEffectFrameworkProcessor& processor_;
    AtomLookAndFeel& atomLookAndFeel_;
    juce::OwnedArray<juce::Component> rowComponents_;
    std::map<juce::String, std::unique_ptr<GfxPedalPanel>> gfxByPack_;
    GfxPedalPanel* gfxPanel_ = nullptr;
    int preferredWidth_ = 0;
    int preferredHeight_ = 0;

    static constexpr int kRowHeight = 50;
    static constexpr int kPanelPadding = 8;
    static constexpr int kMinHeight = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectUserParamsPanel)
};
