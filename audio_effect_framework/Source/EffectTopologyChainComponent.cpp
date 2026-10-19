#include "AefJuceIncludes.h"
#include "EffectTopologyChainComponent.h"

namespace
{
constexpr int kRowHeight = 56;
constexpr int kConnectorHeight = 18;
constexpr int kEndpointLabelHeight = 18;
} // namespace

EffectTopologyChainComponent::ModuleRow::ModuleRow(const EffectTopologyModule& module,
                                                   ModuleBypassChangedFn onChangeIn,
                                                   ModuleSelectedFn onSelectIn)
    : moduleId(module.id),
      onChange(std::move(onChangeIn)),
      onSelect(std::move(onSelectIn)),
      module_(module),
      label(module.id + "Label", module.displayName),
      operatorLabel(module.id + "Op", module.operatorId),
      toggle(module.id + "Toggle", module.bypassed ? "OFF" : "ON")
{
    card.setMinPanelHeight(kRowHeight);
    label.setJustificationType(juce::Justification::centredLeft);
    label.setFont(AtomLookAndFeel::getUIFont(AtomLookAndFeel::getSystemUIFontHeight(), juce::Font::plain));
    label.setMinimumHorizontalScale(1.0f);
    label.setAutoResizeEnabled(false);
    label.setAlpha(module.bypassed ? 0.5f : 1.0f);
    label.setInterceptsMouseClicks(false, false);

    operatorLabel.setJustificationType(juce::Justification::centredLeft);
    operatorLabel.setFont(AtomLookAndFeel::getUIFont(11.0f, juce::Font::plain));
    operatorLabel.setAlpha(module.bypassed ? 0.35f : 0.6f);
    operatorLabel.setInterceptsMouseClicks(false, false);
    operatorLabel.setVisible(module.operatorId.isNotEmpty());

    toggle.setToggleState(!module.bypassed, juce::dontSendNotification);
    toggle.setEnabled(module.bypassable);
    toggle.onClick = [this]
    {
        label.setAlpha(toggle.getToggleState() ? 1.0f : 0.5f);
        operatorLabel.setAlpha(toggle.getToggleState() ? 0.6f : 0.35f);
        if (this->onChange)
            this->onChange(moduleId, !toggle.getToggleState());
    };

    card.setInterceptsMouseClicks(false, true);
    card.addAndMakeVisible(label);
    card.addAndMakeVisible(operatorLabel);
    card.addAndMakeVisible(toggle);
    addAndMakeVisible(card);
    setSize(0, kRowHeight);
}

void EffectTopologyChainComponent::ModuleRow::syncFromModule(const EffectTopologyModule& module,
                                                             bool selected)
{
    module_ = module;
    selected_ = selected;
    label.setText(module.displayName, juce::dontSendNotification);
    operatorLabel.setText(module.operatorId, juce::dontSendNotification);
    operatorLabel.setVisible(module.operatorId.isNotEmpty());
    label.setAlpha(module.bypassed ? 0.5f : 1.0f);
    operatorLabel.setAlpha(module.bypassed ? 0.35f : 0.6f);
    toggle.setEnabled(module.bypassable);
    toggle.setButtonText(module.bypassed ? "OFF" : "ON");
    toggle.setToggleState(!module.bypassed, juce::dontSendNotification);
    repaint();
}

void EffectTopologyChainComponent::ModuleRow::paint(juce::Graphics& g)
{
    if (!selected_)
        return;

    g.setColour(findColour(juce::Label::textColourId).withAlpha(0.22f));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), 8.0f, 2.0f);
}

void EffectTopologyChainComponent::ModuleRow::resized()
{
    card.setBounds(getLocalBounds());
    auto area = card.getLocalBounds().reduced(12, 6);
    toggle.setBounds(area.removeFromRight(72).withSizeKeepingCentre(72, 28));
    area.removeFromRight(10);
    if (operatorLabel.isVisible())
    {
        label.setBounds(area.removeFromTop(22));
        operatorLabel.setBounds(area);
    }
    else
    {
        label.setBounds(area);
    }
}

void EffectTopologyChainComponent::ModuleRow::mouseUp(const juce::MouseEvent&)
{
    if (onSelect)
        onSelect(module_);
}

EffectTopologyChainComponent::EffectTopologyChainComponent()
    : inLabel("topologyIn", "IN"),
      outLabel("topologyOut", "OUT")
{
    inLabel.setJustificationType(juce::Justification::centred);
    outLabel.setJustificationType(juce::Justification::centred);
    for (auto* label : {&inLabel, &outLabel})
    {
        label->setFont(AtomLookAndFeel::getUIFont(11.0f, juce::Font::plain));
        label->setAlpha(0.55f);
        addAndMakeVisible(*label);
    }
}

EffectTopologyChainComponent::~EffectTopologyChainComponent() = default;

void EffectTopologyChainComponent::setModules(const juce::Array<EffectTopologyModule>& modules)
{
    bool sameIds = modules.size() == modules_.size();
    if (sameIds)
    {
        for (int i = 0; i < modules.size(); ++i)
        {
            if (modules_[i].id != modules[i].id)
            {
                sameIds = false;
                break;
            }
        }
    }

    modules_ = modules;

    if (sameIds && rows_.size() == (size_t)modules_.size())
    {
        if (selectedId_.isNotEmpty())
        {
            bool stillThere = false;
            for (const auto& mod : modules_)
                stillThere = stillThere || mod.id == selectedId_;
            if (!stillThere)
                selectedId_.clear();
        }

        for (int i = 0; i < modules_.size(); ++i)
            rows_[(size_t)i]->syncFromModule(modules_[i], modules_[i].id == selectedId_);
        repaint();
        return;
    }

    if (selectedId_.isNotEmpty())
    {
        bool stillThere = false;
        for (const auto& mod : modules_)
            stillThere = stillThere || mod.id == selectedId_;
        if (!stillThere)
            selectedId_.clear();
    }

    rebuildRows();
}

void EffectTopologyChainComponent::setModuleBypassChangedCallback(ModuleBypassChangedFn callback)
{
    bypassChangedCallback_ = std::move(callback);
}

void EffectTopologyChainComponent::setModuleSelectedCallback(ModuleSelectedFn callback)
{
    selectedCallback_ = std::move(callback);
}

void EffectTopologyChainComponent::setSelectedModuleId(const juce::String& moduleId)
{
    selectedId_ = moduleId;
    for (int i = 0; i < modules_.size() && i < (int)rows_.size(); ++i)
        rows_[(size_t)i]->syncFromModule(modules_[i], modules_[i].id == selectedId_);
}

int EffectTopologyChainComponent::getPreferredHeight() const noexcept
{
    if (modules_.isEmpty())
        return 0;

    return kEndpointLabelHeight + kConnectorHeight
         + modules_.size() * kRowHeight
         + juce::jmax(0, modules_.size() - 1) * kConnectorHeight
         + kConnectorHeight + kEndpointLabelHeight;
}

void EffectTopologyChainComponent::rebuildRows()
{
    rows_.clear();
    removeAllChildren();
    addAndMakeVisible(inLabel);
    addAndMakeVisible(outLabel);

    for (const auto& mod : modules_)
    {
        auto row = std::make_unique<ModuleRow>(mod, bypassChangedCallback_,
                                               [this](const EffectTopologyModule& m)
                                               {
                                                   setSelectedModuleId(m.id);
                                                   if (selectedCallback_)
                                                       selectedCallback_(m);
                                               });
        row->syncFromModule(mod, mod.id == selectedId_);
        addAndMakeVisible(*row);
        rows_.push_back(std::move(row));
    }

    resized();
    repaint();
}

void EffectTopologyChainComponent::paint(juce::Graphics& g)
{
    if (rows_.empty())
        return;

    const auto lineColour = findColour(juce::Label::textColourId).withAlpha(0.35f);
    g.setColour(lineColour);

    const float cx = (float)getWidth() * 0.5f;

    auto drawConnector = [&](int yTop, int yBottom)
    {
        g.drawLine(cx, (float)yTop, cx, (float)yBottom, 1.0f);
        juce::Path arrow;
        arrow.addTriangle(cx - 4.0f, (float)yBottom - 6.0f,
                          cx + 4.0f, (float)yBottom - 6.0f,
                          cx, (float)yBottom);
        g.fillPath(arrow);
    };

    int y = kEndpointLabelHeight;
    drawConnector(y, y + kConnectorHeight);
    y += kConnectorHeight;

    for (size_t i = 0; i < rows_.size(); ++i)
    {
        y += kRowHeight;
        if (i + 1 < rows_.size())
        {
            drawConnector(y, y + kConnectorHeight);
            y += kConnectorHeight;
        }
    }

    drawConnector(y, y + kConnectorHeight);
}

void EffectTopologyChainComponent::resized()
{
    if (rows_.empty())
    {
        inLabel.setBounds({});
        outLabel.setBounds({});
        return;
    }

    auto bounds = getLocalBounds();
    inLabel.setBounds(bounds.removeFromTop(kEndpointLabelHeight));
    bounds.removeFromTop(kConnectorHeight);

    for (size_t i = 0; i < rows_.size(); ++i)
    {
        rows_[i]->setBounds(bounds.removeFromTop(kRowHeight));
        if (i + 1 < rows_.size())
            bounds.removeFromTop(kConnectorHeight);
    }

    bounds.removeFromTop(kConnectorHeight);
    outLabel.setBounds(bounds.removeFromTop(kEndpointLabelHeight));
}
