#include "EffectUserParamsPanel.h"

#include "AudioEffectFrameworkProcessor.h"
#include "DynamicPluginParamMetadata.h"
#include "GfxPedalPanel.h"
#include "KbussParamSliderUtils.h"

namespace
{
constexpr int kLabelColumnWidth = 140;
constexpr int kLabelGap = 8;

class ParamRow final : public juce::Component
{
public:
    ParamRow(const juce::String& labelText, std::unique_ptr<juce::Component> controlIn)
        : label("paramLabel", labelText), control(std::move(controlIn))
    {
        jassert(control != nullptr);
        label.setJustificationType(juce::Justification::centredLeft);
        label.setFont(AtomLookAndFeel::getUIFont(AtomLookAndFeel::getSystemUIFontHeight(), juce::Font::plain));
        addAndMakeVisible(label);
        addAndMakeVisible(*control);
    }

    void resized() override
    {
        auto area = getLocalBounds();
        label.setBounds(area.removeFromLeft(kLabelColumnWidth));
        area.removeFromLeft(kLabelGap);
        control->setBounds(area);
    }

private:
    atom::Label label;
    std::unique_ptr<juce::Component> control;
};
}  // namespace

EffectUserParamsPanel::EffectUserParamsPanel(AudioEffectFrameworkProcessor& processor,
                                             AtomLookAndFeel& lookAndFeel)
    : processor_(processor), atomLookAndFeel_(lookAndFeel)
{
    setOpaque(false);
}

EffectUserParamsPanel::~EffectUserParamsPanel()
{
    detachGraphic();
    gfxByPack_.clear();
}

bool EffectUserParamsPanel::usingReadyGraphicPanel() const noexcept
{
    return gfxPanel_ != nullptr && gfxPanel_->isFaceReady();
}

void EffectUserParamsPanel::detachGraphic()
{
    if (gfxPanel_ == nullptr)
        return;
    removeChildComponent(gfxPanel_);
    gfxPanel_ = nullptr;
}

void EffectUserParamsPanel::publishGraphicGeometry()
{
    if (gfxPanel_ == nullptr || ! gfxPanel_->isFaceReady())
        return;

    preferredWidth_ = gfxPanel_->preferredWidth();
    preferredHeight_ = gfxPanel_->preferredHeight();
    setVisible(preferredHeight_ > 0);
    resized();
    if (onGeometryChanged)
        onGeometryChanged();
    gfxPanel_->setVisible(true);
}

void EffectUserParamsPanel::attachGraphic(GfxPedalPanel& panel)
{
    gfxPanel_ = &panel;
    panel.onPreferredSizeChanged = [this]
    {
        publishGraphicGeometry();
    };
    panel.refreshFromProcessor();
    addChildComponent(panel);
    panel.setVisible(false);
    // Wait for fitToArtwork() so the window never opens at the raw canvas
    // size and then snaps back. Cached packs are ready immediately.
    if (panel.isFaceReady())
        publishGraphicGeometry();
}

void EffectUserParamsPanel::clearRows()
{
    detachGraphic();
    rowComponents_.clear();
    removeAllChildren();
    preferredWidth_ = 0;
    preferredHeight_ = kMinHeight;
}

bool EffectUserParamsPanel::tryLoadGraphicPanel()
{
    const auto packDir = resolveGfxPackDirectory(processor_);
    if (! packDir.isDirectory())
        return false;

    const auto key = packDir.getFullPathName();
    if (auto it = gfxByPack_.find(key); it != gfxByPack_.end() && it->second != nullptr)
    {
        // Re-read panel.json so layout tweaks apply on switch-back.
        if (! it->second->load(packDir))
            return false;
        attachGraphic(*it->second);
        return true;
    }

    auto panel = std::make_unique<GfxPedalPanel>(processor_, atomLookAndFeel_);
    if (! panel->load(packDir))
        return false;

    auto* raw = panel.get();
    gfxByPack_[key] = std::move(panel);
    attachGraphic(*raw);
    return true;
}

void EffectUserParamsPanel::addParamRow(const kbuss::ParameterDescriptor& desc)
{
    const juce::String paramId(desc.id);
    const juce::String labelText = aef::kbuss_param_ui::paramDisplayLabel(desc);
    const float initial = processor_.getMiddleParamDomain(paramId, desc.default_domain);

    std::unique_ptr<juce::Component> control;

    if (desc.type == kbuss::ParameterType::Bool)
    {
        auto toggle = std::make_unique<atom::ToggleButton>(paramId, juce::String());
        toggle->setToggleState(initial >= 0.5f, juce::dontSendNotification);
        toggle->setFontHeight(static_cast<float>(AtomLookAndFeel::getSystemUIFontHeight()));
        toggle->onClick = [this, paramId, raw = toggle.get()]() {
            processor_.setMiddleParamDomain(paramId, raw->getToggleState() ? 1.0f : 0.0f);
        };
        control = std::move(toggle);
    }
    else if (aef::kbuss_param_ui::hasChoiceLabels(desc))
    {
        auto combo = std::make_unique<atom::ComboBox>(paramId);
        aef::kbuss_param_ui::configureKbussChoiceCombo(
            *combo, desc, juce::roundToInt(initial));
        combo->onChange = [this, paramId, raw = combo.get()]() {
            processor_.setMiddleParamDomain(paramId, static_cast<float>(raw->getSelectedId() - 1));
        };
        control = std::move(combo);
    }
    else
    {
        auto slider = std::make_unique<atom::Slider>();
        aef::kbuss_param_ui::configureKbussParamSlider(
            *slider,
            atomLookAndFeel_,
            desc.min_domain,
            desc.max_domain,
            aef::kbuss_param_ui::paramSliderInterval(desc),
            aef::kbuss_param_ui::paramUnitSuffix(desc));
        slider->setValue(initial, juce::dontSendNotification);
        slider->onValueChange = [this, paramId, raw = slider.get()]() {
            processor_.setMiddleParamDomain(paramId, static_cast<float>(raw->getValue()));
        };
        control = std::move(slider);
    }

    auto row = std::make_unique<ParamRow>(labelText, std::move(control));
    addAndMakeVisible(row.get());
    rowComponents_.add(row.release());
}

void EffectUserParamsPanel::addMetaParamRow(const aef::dynamic_plugin_params::Meta& meta,
                                            kbuss::Processor& middle)
{
    const juce::String paramId = meta.id;
    const juce::String labelText = meta.displayLabel();
    float initial = processor_.getMiddleParamDomain(paramId, meta.defaultDomain);

    float normalized = meta.domainToNormalized(initial);
    if (middle.get_parameter(meta.index, normalized) == kbuss::Status::Ok)
        initial = meta.minDomain + normalized * (meta.maxDomain - meta.minDomain);

    std::unique_ptr<juce::Component> control;
    if (aef::kbuss_param_ui::isSwitchParam(paramId.toStdString(), meta.minDomain, meta.maxDomain))
    {
        auto toggle = std::make_unique<atom::ToggleButton>(paramId, juce::String());
        toggle->setToggleState(initial >= 0.5f, juce::dontSendNotification);
        toggle->setFontHeight(static_cast<float>(AtomLookAndFeel::getSystemUIFontHeight()));
        toggle->onClick = [this, paramId, raw = toggle.get()]() {
            processor_.setMiddleParamDomain(paramId, raw->getToggleState() ? 1.0f : 0.0f);
        };
        control = std::move(toggle);
    }
    else
    {
        auto slider = std::make_unique<atom::Slider>();
        kbuss::ParameterDescriptor desc;
        desc.id = meta.id.toStdString();
        desc.label = meta.label.toStdString();
        desc.min_domain = meta.minDomain;
        desc.max_domain = meta.maxDomain;
        desc.default_domain = meta.defaultDomain;
        aef::kbuss_param_ui::configureKbussParamSlider(
            *slider,
            atomLookAndFeel_,
            meta.minDomain,
            meta.maxDomain,
            aef::kbuss_param_ui::paramSliderInterval(desc),
            aef::kbuss_param_ui::paramUnitSuffix(desc));
        slider->setValue(initial, juce::dontSendNotification);
        slider->onValueChange = [this, paramId, raw = slider.get()]() {
            processor_.setMiddleParamDomain(paramId, static_cast<float>(raw->getValue()));
        };
        control = std::move(slider);
    }

    auto row = std::make_unique<ParamRow>(labelText, std::move(control));
    addAndMakeVisible(row.get());
    rowComponents_.add(row.release());
}

void EffectUserParamsPanel::syncGraphicState()
{
    if (gfxPanel_ != nullptr)
        gfxPanel_->syncBypassIndicator();
}

void EffectUserParamsPanel::rebuildFromMiddleProcessor()
{
    if (! processor_.getKbussEngine().isReady())
    {
        clearRows();
        return;
    }

    const auto packDir = resolveGfxPackDirectory(processor_);
    if (gfxPanel_ != nullptr && packDir.isDirectory() && gfxPanel_->isLoadedFrom(packDir))
    {
        gfxPanel_->syncBypassIndicator();
        preferredWidth_ = gfxPanel_->preferredWidth();
        preferredHeight_ = gfxPanel_->preferredHeight();
        if (onGeometryChanged)
            onGeometryChanged();
        gfxPanel_->setVisible(gfxPanel_->isFaceReady());
        return;
    }

    detachGraphic();
    rowComponents_.clear();
    removeAllChildren();

    if (tryLoadGraphicPanel())
        return;

    preferredWidth_ = 0;
    preferredHeight_ = kMinHeight;

    auto* middle = processor_.getKbussEngine().getMiddleProcessor();
    if (middle == nullptr)
        return;

    const auto& params = middle->parameters();
    if (! params.empty())
    {
        const auto topLevelIds = aef::kbuss_param_ui::collectTopLevelParamIds(params);
        for (const auto& desc : params)
        {
            if (aef::kbuss_param_ui::isFooterQualityParam(desc.id))
                continue;
            if (processor_.isInsideViewParam(juce::String(desc.id)))
                continue;
            if (aef::kbuss_param_ui::isUserFacingParam(desc, topLevelIds))
                addParamRow(desc);
        }
    }
    else
    {
        const auto dynamicParams = processor_.getDynamicMiddleParamMetadata();
        if (! dynamicParams.empty())
        {
            for (const auto& meta : dynamicParams)
            {
                if (aef::kbuss_param_ui::isFooterQualityParam(meta.id.toStdString()))
                    continue;
                if (processor_.isInsideViewParam(meta.id))
                    continue;
                addMetaParamRow(meta, *middle);
            }
        }
        else
        {
            rebuildIndexedParams(*middle);
        }
    }

    if (rowComponents_.isEmpty())
    {
        preferredHeight_ = kMinHeight;
        return;
    }

    preferredHeight_ = juce::jmax(kMinHeight, rowComponents_.size() * kRowHeight + kPanelPadding * 2);
    resized();
}

void EffectUserParamsPanel::addIndexedParamRow(std::uint32_t index, float initialValue)
{
    const juce::String labelText = "Parameter " + juce::String((int) index + 1);

    auto slider = std::make_unique<atom::Slider>();
    aef::kbuss_param_ui::configureKbussParamSlider(
        *slider,
        atomLookAndFeel_,
        0.0f,
        1.0f,
        0.0f,
        juce::String());
    slider->setValue(juce::jlimit(0.0, 1.0, (double) initialValue), juce::dontSendNotification);
    slider->onValueChange = [this, index, raw = slider.get()]() {
        if (auto* middle = processor_.getKbussEngine().getMiddleProcessor())
            (void) middle->set_parameter(index, static_cast<float>(raw->getValue()));
    };

    auto row = std::make_unique<ParamRow>(labelText, std::move(slider));
    addAndMakeVisible(row.get());
    rowComponents_.add(row.release());
}

void EffectUserParamsPanel::rebuildIndexedParams(kbuss::Processor& middle)
{
    const auto count = middle.parameter_count();
    for (std::uint32_t i = 0; i < count; ++i)
    {
        float value = 0.5f;
        if (middle.get_parameter(i, value) != kbuss::Status::Ok)
            value = 0.5f;
        addIndexedParamRow(i, value);
    }
}

void EffectUserParamsPanel::resized()
{
    if (gfxPanel_ != nullptr)
    {
        gfxPanel_->setBounds(getLocalBounds());
        return;
    }

    auto area = getLocalBounds().reduced(kPanelPadding);
    for (auto* row : rowComponents_)
        row->setBounds(area.removeFromTop(kRowHeight));
}
