#pragma once

#include "AllParamsPanel.h"
#include "AudioEffectFrameworkEditor.h"
#include "EffectTopologyChainComponent.h"
#include "PluginProcessor.h"
#include "SchematicEditorPanel.h"

class WhiteBoxLabEditor final : public AudioEffectFrameworkEditor
{
public:
    explicit WhiteBoxLabEditor(WhiteBoxLabProcessor& processor);

protected:
    int getBodyComponentBaseHeight(const juce::Component* component) const noexcept override;
    int getMaxBodyViewportHeight() const noexcept override;
    void onEditorTimerTick() override;

private:
    WhiteBoxLabProcessor& labProcessor;
    atom::Label modelLabel;
    atom::ComboBox modelCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modelAttachment;
    atom::Label topologyTitle;
    EffectTopologyChainComponent topologyChain;
    AllParamsPanel allParams;
    SchematicEditorPanel schematic;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WhiteBoxLabEditor)
};
