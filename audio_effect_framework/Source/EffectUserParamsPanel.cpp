#include "EffectUserParamsPanel.h"

#include "AudioEffectFrameworkProcessor.h"
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
}

void EffectUserParamsPanel::clearRows()
{
    rowComponents_.clear();
    removeAllChildren();
    preferredHeight_ = kMinHeight;
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

void EffectUserParamsPanel::rebuildFromMiddleProcessor()
{
    clearRows();

    if (! processor_.getKbussEngine().isReady())
        return;

    auto* middle = processor_.getKbussEngine().getMiddleProcessor();
    if (middle == nullptr)
        return;

    for (const auto& desc : middle->parameters())
    {
        if (aef::kbuss_param_ui::isUserFacingParam(desc))
            addParamRow(desc);
    }

    if (rowComponents_.isEmpty())
    {
        preferredHeight_ = kMinHeight;
        return;
    }

    preferredHeight_ = juce::jmax(kMinHeight, rowComponents_.size() * kRowHeight + kPanelPadding * 2);
    resized();
}

void EffectUserParamsPanel::resized()
{
    auto area = getLocalBounds().reduced(kPanelPadding);
    for (auto* row : rowComponents_)
        row->setBounds(area.removeFromTop(kRowHeight));
}
