#pragma once

#include "AudioEffectFrameworkEditor.h"
#include "PluginProcessor.h"

class SchematicEditorPanel;

class LittleHostProcessorEditor final : public AudioEffectFrameworkEditor
{
public:
    explicit LittleHostProcessorEditor (LittleHostProcessor& processor);
    ~LittleHostProcessorEditor() override;

protected:
    int getBodyComponentBaseHeight (const juce::Component* component) const noexcept override;
    int getMaxBodyViewportHeight() const noexcept override;
    void onEditorTimerTick() override;

private:
    LittleHostProcessor& hostProcessor_;
    atom::ComboBox pluginCombo;
    std::unique_ptr<juce::Component> pluginSelectorRow;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> pluginAttachment;
    std::unique_ptr<SchematicEditorPanel> schematic_;
};
