#include "PluginEditor.h"

#include "SchematicEditorPanel.h"

namespace
{
class PluginSelectorRow final : public juce::Component
{
public:
    PluginSelectorRow (const juce::String& title, atom::ComboBox& combo, int rowHeight)
        : label ("pluginSelectorLabel", title),
          comboBox (combo),
          rowHeight_ (rowHeight)
    {
        card.setMinPanelHeight (rowHeight);
        label.setJustificationType (juce::Justification::centredLeft);
        label.setFont (AtomLookAndFeel::getUIFont (AtomLookAndFeel::getSystemUIFontHeight(), juce::Font::plain));
        label.setMinimumHorizontalScale (1.0f);
        label.setAutoResizeEnabled (false);

        comboBox.setEditableText (false);
        comboBox.onChange = [this] { resized(); };

        card.addAndMakeVisible (label);
        card.addAndMakeVisible (comboBox);
        addAndMakeVisible (card);
    }

    void resized() override
    {
        card.setBounds (getLocalBounds());
        auto area = card.getLocalBounds().reduced (12, 8);
        const float rowH = (float) area.getHeight();

        comboBox.setSize (comboBox.getIdealWidth(), juce::roundToInt (rowH));

        const auto& font = label.getFont();
        const int labelTextWidth =
            juce::roundToInt (font.getStringWidthFloat (label.getText()) + 4.0f);

        juce::FlexBox flex;
        flex.flexDirection = juce::FlexBox::Direction::row;
        flex.alignItems = juce::FlexBox::AlignItems::center;

        juce::FlexItem labelItem ((float) labelTextWidth, rowH, label);
        labelItem.flexShrink = 0.0f;

        juce::FlexItem spacer (0.0f, rowH);
        spacer.flexGrow = 1.0f;

        juce::FlexItem comboItem ((float) comboBox.getWidth(), rowH, comboBox);
        comboItem.flexShrink = 0.0f;

        flex.items.addArray ({ labelItem, spacer, comboItem });
        flex.performLayout (area);
    }

private:
    atom::SettingsCard card;
    atom::Label label;
    atom::ComboBox& comboBox;
    int rowHeight_;
};
} // namespace

LittleHostProcessorEditor::LittleHostProcessorEditor (LittleHostProcessor& processor)
    : AudioEffectFrameworkEditor (processor, true),
      hostProcessor_ (processor),
      pluginCombo ("pluginCombo"),
      schematic_ (std::make_unique<SchematicEditorPanel> (processor, atomLookAndFeel))
{
    const auto names = hostProcessor_.pluginCatalog().displayNames();
    for (int i = 0; i < names.size(); ++i)
        pluginCombo.addItem (names[i], i + 1);

    if (pluginCombo.getNumItems() > 0)
        pluginCombo.setSelectedId (1, juce::dontSendNotification);

    pluginCombo.setTooltip ("Select a .kbplug effect from the plugins/ directory");
    pluginAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        hostProcessor_.parameters.valueTreeState,
        LittleHostProcessor::kPluginChoiceParamId,
        pluginCombo);

    pluginSelectorRow = std::make_unique<PluginSelectorRow> ("Plugin", pluginCombo, cardRowHeight);
    bodyContent.addAndMakeVisible (*pluginSelectorRow);
    bodyComponents.add (pluginSelectorRow.get());

    bodyContent.addAndMakeVisible (*schematic_);
    bodyComponents.add (schematic_.get());

    schematic_->setCompositeKey (hostProcessor_.currentCompositeKey());

    completeBodyConstruction();
    recalculateBodyContentHeight();
    applyZoom (1.0f);
}

LittleHostProcessorEditor::~LittleHostProcessorEditor() = default;

int LittleHostProcessorEditor::getBodyComponentBaseHeight (const juce::Component* component) const noexcept
{
    if (pluginSelectorRow != nullptr && component == pluginSelectorRow.get())
        return cardRowHeight;
    if (schematic_ != nullptr && component == schematic_.get())
        return 520;

    return AudioEffectFrameworkEditor::getBodyComponentBaseHeight (component);
}

int LittleHostProcessorEditor::getMaxBodyViewportHeight() const noexcept
{
    return 900;
}

void LittleHostProcessorEditor::onEditorTimerTick()
{
    schematic_->setCompositeKey (hostProcessor_.currentCompositeKey());
}
