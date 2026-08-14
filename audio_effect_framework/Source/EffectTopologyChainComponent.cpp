#include "AefJuceIncludes.h"
#include "EffectTopologyChainComponent.h"

namespace
{
constexpr int kRowHeight = 48;
constexpr int kConnectorHeight = 20;
constexpr int kEndpointLabelHeight = 18;
} // namespace

EffectTopologyChainComponent::ModuleRow::ModuleRow (const EffectTopologyModule& module,
                                                    ModuleBypassChangedFn onChangeIn)
    : moduleId (module.id),
      onChange (std::move (onChangeIn)),
      label (module.id + "Label", module.displayName),
      toggle (module.id + "Toggle", module.bypassed ? "OFF" : "ON")
{
    card.setMinPanelHeight (kRowHeight);
    label.setJustificationType (juce::Justification::centredLeft);
    label.setFont (AtomLookAndFeel::getUIFont (AtomLookAndFeel::getSystemUIFontHeight(), juce::Font::plain));
    label.setMinimumHorizontalScale (1.0f);
    label.setAutoResizeEnabled (false);
    label.setAlpha (module.bypassed ? 0.5f : 1.0f);

    toggle.setToggleState (! module.bypassed, juce::dontSendNotification);
    toggle.onClick = [this]
    {
        label.setAlpha (toggle.getToggleState() ? 1.0f : 0.5f);
        if (this->onChange)
            this->onChange (moduleId, ! toggle.getToggleState());
    };

    card.addAndMakeVisible (label);
    card.addAndMakeVisible (toggle);
    addAndMakeVisible (card);
    setSize (0, kRowHeight);
}

void EffectTopologyChainComponent::ModuleRow::syncFromModule (const EffectTopologyModule& module)
{
    label.setAlpha (module.bypassed ? 0.5f : 1.0f);
    toggle.setToggleState (! module.bypassed, juce::dontSendNotification);
}

void EffectTopologyChainComponent::ModuleRow::resized()
{
    card.setBounds (getLocalBounds());
    auto area = card.getLocalBounds().reduced (12, 8);
    auto labelArea = area.removeFromLeft (180);
    area.removeFromLeft (10);
    toggle.setBounds (area.removeFromRight (72));
    label.setBounds (labelArea);
}

EffectTopologyChainComponent::EffectTopologyChainComponent()
    : inLabel ("topologyIn", "IN"),
      outLabel ("topologyOut", "OUT")
{
    inLabel.setJustificationType (juce::Justification::centred);
    outLabel.setJustificationType (juce::Justification::centred);
    for (auto* label : { &inLabel, &outLabel })
    {
        label->setFont (AtomLookAndFeel::getUIFont (11.0f, juce::Font::plain));
        label->setAlpha (0.55f);
        addAndMakeVisible (*label);
    }
}

EffectTopologyChainComponent::~EffectTopologyChainComponent() = default;

void EffectTopologyChainComponent::setModules (const juce::Array<EffectTopologyModule>& modules)
{
    if (modules.size() == modules_.size())
    {
        bool same = true;
        for (int i = 0; i < modules.size(); ++i)
        {
            if (modules_[i].id != modules[i].id || modules_[i].bypassed != modules[i].bypassed)
            {
                same = false;
                break;
            }
        }
        if (same)
            return;
    }

    modules_ = modules;

    if (rows_.size() == (size_t) modules_.size())
    {
        for (int i = 0; i < modules_.size(); ++i)
            rows_[(size_t) i]->syncFromModule (modules_[i]);
        repaint();
        return;
    }

    rebuildRows();
}

void EffectTopologyChainComponent::setModuleBypassChangedCallback (ModuleBypassChangedFn callback)
{
    bypassChangedCallback_ = std::move (callback);
}

int EffectTopologyChainComponent::getPreferredHeight() const noexcept
{
    if (modules_.isEmpty())
        return 0;

    return kEndpointLabelHeight + kConnectorHeight
         + modules_.size() * kRowHeight
         + juce::jmax (0, modules_.size() - 1) * kConnectorHeight
         + kConnectorHeight + kEndpointLabelHeight;
}

void EffectTopologyChainComponent::rebuildRows()
{
    rows_.clear();
    removeAllChildren();
    addAndMakeVisible (inLabel);
    addAndMakeVisible (outLabel);

    for (const auto& mod : modules_)
    {
        auto row = std::make_unique<ModuleRow> (mod, bypassChangedCallback_);
        addAndMakeVisible (*row);
        rows_.push_back (std::move (row));
    }

    resized();
    repaint();
}

void EffectTopologyChainComponent::paint (juce::Graphics& g)
{
    if (rows_.empty())
        return;

    const auto lineColour = findColour (juce::Label::textColourId).withAlpha (0.35f);
    g.setColour (lineColour);

    const float cx = (float) getWidth() * 0.5f;

    auto drawConnector = [&] (int yTop, int yBottom)
    {
        g.drawLine (cx, (float) yTop, cx, (float) yBottom, 1.0f);
        juce::Path arrow;
        arrow.addTriangle (cx - 4.0f, (float) yBottom - 6.0f,
                           cx + 4.0f, (float) yBottom - 6.0f,
                           cx, (float) yBottom);
        g.fillPath (arrow);
    };

    int y = kEndpointLabelHeight;
    drawConnector (y, y + kConnectorHeight);
    y += kConnectorHeight;

    for (size_t i = 0; i < rows_.size(); ++i)
    {
        y += kRowHeight;
        if (i + 1 < rows_.size())
        {
            drawConnector (y, y + kConnectorHeight);
            y += kConnectorHeight;
        }
    }

    drawConnector (y, y + kConnectorHeight);
}

void EffectTopologyChainComponent::resized()
{
    if (rows_.empty())
    {
        inLabel.setBounds ({});
        outLabel.setBounds ({});
        return;
    }

    auto bounds = getLocalBounds();
    inLabel.setBounds (bounds.removeFromTop (kEndpointLabelHeight));
    bounds.removeFromTop (kConnectorHeight);

    for (size_t i = 0; i < rows_.size(); ++i)
    {
        rows_[i]->setBounds (bounds.removeFromTop (kRowHeight));
        if (i + 1 < rows_.size())
            bounds.removeFromTop (kConnectorHeight);
    }

    bounds.removeFromTop (kConnectorHeight);
    outLabel.setBounds (bounds.removeFromTop (kEndpointLabelHeight));
}
