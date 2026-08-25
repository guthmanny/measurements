#include "PluginEditor.h"

#include "CamelCatalog.h"

CamelAudioProcessorEditor::CamelAudioProcessorEditor(CamelAudioProcessor& p)
    : AudioEffectFrameworkEditor(p, true),
      camelProcessor(p),
      effectLabel("effectLabel", "CAMEL Effect")
{
    effectLabel.setJustificationType(juce::Justification::centredLeft);
    effectLabel.setFont(AtomLookAndFeel::getUIFont(13.0f, juce::Font::bold));

    for (int i = 0; i < (int)kCamelEffectCount; ++i)
        effectCombo.addItem(kCamelEffects[(std::size_t)i].displayName, i + 1);

    effectCombo.setTooltip("Select MuDSP CAMEL product effect");
    effectAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        camelProcessor.parameters.valueTreeState, CamelAudioProcessor::kCamelEffectParamId, effectCombo);

    bodyContent.addAndMakeVisible(effectLabel);
    bodyContent.addAndMakeVisible(effectCombo);
    bodyComponents.add(&effectLabel);
    bodyComponents.add(&effectCombo);

    completeBodyConstruction();
    recalculateBodyContentHeight();
    applyZoom(1.0f);
}

int CamelAudioProcessorEditor::getBodyComponentBaseHeight(const juce::Component* component) const noexcept
{
    if (component == &effectLabel)
        return 28;

    if (component == &effectCombo)
        return cardRowHeight;

    return AudioEffectFrameworkEditor::getBodyComponentBaseHeight(component);
}

int CamelAudioProcessorEditor::getMaxBodyViewportHeight() const noexcept
{
    return 720;
}
