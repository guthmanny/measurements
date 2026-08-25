#pragma once

#include "AudioEffectFrameworkEditor.h"
#include "CamelCatalog.h"
#include "PluginProcessor.h"

class CamelAudioProcessorEditor final : public AudioEffectFrameworkEditor
{
public:
    explicit CamelAudioProcessorEditor(CamelAudioProcessor& processor);

protected:
    int getBodyComponentBaseHeight(const juce::Component* component) const noexcept override;
    int getMaxBodyViewportHeight() const noexcept override;

private:
    CamelAudioProcessor& camelProcessor;
    atom::Label effectLabel;
    atom::ComboBox effectCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> effectAttachment;
};
