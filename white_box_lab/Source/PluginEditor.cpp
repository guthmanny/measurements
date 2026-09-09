#include "PluginEditor.h"

WhiteBoxLabEditor::WhiteBoxLabEditor(WhiteBoxLabProcessor& p)
    : AudioEffectFrameworkEditor(p, true),
      labProcessor(p),
      modelLabel("modelLabel", "White Box Model"),
      allParams(p, atomLookAndFeel),
      schematic(p, atomLookAndFeel)
{
    modelLabel.setJustificationType(juce::Justification::centredLeft);
    modelLabel.setFont(AtomLookAndFeel::getUIFont(13.0f, juce::Font::bold));

    const auto names = labProcessor.modelDisplayNames();
    for (int i = 0; i < names.size(); ++i)
        modelCombo.addItem(names[i], i + 1);

    modelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        labProcessor.parameters.valueTreeState, WhiteBoxLabProcessor::kModelParamId, modelCombo);

    topologyTitle.setText("Internal signal chain", juce::dontSendNotification);
    topologyTitle.setFont(AtomLookAndFeel::getUIFont(13.0f, juce::Font::bold));
    topologyChain.setModuleBypassChangedCallback(
        [this](const juce::String& moduleId, bool bypassed)
        {
            labProcessor.setEffectTopologyModuleBypassed(moduleId, bypassed);
        });

    bodyContent.addAndMakeVisible(modelLabel);
    bodyContent.addAndMakeVisible(modelCombo);
    bodyContent.addAndMakeVisible(topologyTitle);
    bodyContent.addAndMakeVisible(topologyChain);
    bodyContent.addAndMakeVisible(allParams);
    bodyContent.addAndMakeVisible(schematic);

    bodyComponents.add(&modelLabel);
    bodyComponents.add(&modelCombo);
    bodyComponents.add(&topologyTitle);
    bodyComponents.add(&topologyChain);
    bodyComponents.add(&allParams);
    bodyComponents.add(&schematic);

    schematic.setCompositeKey(labProcessor.currentCompositeKey());
    topologyChain.setModules(labProcessor.getEffectTopologyModules());

    completeBodyConstruction();
    recalculateBodyContentHeight();
    applyZoom(1.0f);
}

int WhiteBoxLabEditor::getBodyComponentBaseHeight(const juce::Component* component) const noexcept
{
    if (component == &modelLabel || component == &topologyTitle)
        return 28;
    if (component == &modelCombo)
        return cardRowHeight;
    if (component == &topologyChain)
        return juce::jmax(80, topologyChain.getPreferredHeight());
    if (component == &allParams)
        return allParams.getPreferredPanelHeight();
    if (component == &schematic)
        return 520;
    return AudioEffectFrameworkEditor::getBodyComponentBaseHeight(component);
}

int WhiteBoxLabEditor::getMaxBodyViewportHeight() const noexcept
{
    return 900;
}

void WhiteBoxLabEditor::onEditorTimerTick()
{
    topologyChain.setModules(labProcessor.getEffectTopologyModules());
    schematic.setCompositeKey(labProcessor.currentCompositeKey());
}
