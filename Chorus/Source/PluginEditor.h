#pragma once

#include "AudioEffectFrameworkEditor.h"
#include "PluginProcessor.h"

class ChorusAudioProcessorEditor final : public AudioEffectFrameworkEditor
{
public:
    explicit ChorusAudioProcessorEditor(ChorusAudioProcessor& processor);
    ~ChorusAudioProcessorEditor() override;

protected:
    int getBodyComponentBaseHeight(const juce::Component* component) const noexcept override;
    void onEditorTimerTick() override;

    juce::Component* getInsidePage() override;
    const juce::Component* getInsidePage() const override;
    int getInsidePreferredWidth() const override;
    int getInsidePreferredHeight() const override;
    void onInsidePageShown() override;

private:
    void configureUserSlider(atom::Slider& slider, const juce::String& suffix);
    void refreshUserControls();
    void refreshDelayModView();
    void syncTapTempo();
    void applyRateSliderRange();
    void updateChorusRowVisibility();

    ChorusAudioProcessor& chorusProcessor;
    atom::ComboBox modelCombo;
    std::unique_ptr<juce::Component> modelSelectorRow;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modelAttachment;
    atom::Slider rateSlider;
    atom::Slider depthSlider;
    atom::Slider mixSlider;
    std::unique_ptr<juce::Component> rateRow;
    std::unique_ptr<juce::Component> depthRow;
    std::unique_ptr<juce::Component> mixRow;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    atom::DelayModView delayModView;
    int lastModelIndex_ = -1;
    int lastMiddleGeneration_ = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChorusAudioProcessorEditor)
};
