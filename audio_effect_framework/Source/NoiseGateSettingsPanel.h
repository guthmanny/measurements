#pragma once

#include <juce_atom_theme/juce_atom_theme.h>
#include "AefJuceIncludes.h"
#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>

#include "CompressorCurveComponent.h"

class AudioEffectFrameworkProcessor;

/** Advanced Noise Gate controls + transfer curve. Threshold stays on the header. */
class NoiseGateSettingsPanel final : public juce::Component, private juce::Timer
{
public:
    NoiseGateSettingsPanel (AudioEffectFrameworkProcessor& processor, AtomLookAndFeel& lookAndFeel);
    ~NoiseGateSettingsPanel() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    /** Natural height: intro + curve + controls section. */
    int getPreferredPanelHeight() const noexcept;

private:
    void timerCallback() override;
    void updateTransferCurve();
    void updateKneeWidthEnabled();
    void enforceThresholdRange();
    float readParam (const juce::String& paramId, float fallback) const;
    void writeParam (const juce::String& paramId, float value);
    void configureHorizontalSlider (atom::Slider& slider, double minV, double maxV, double interval,
                                    const juce::String& suffix);

    AudioEffectFrameworkProcessor& processor;
    AtomLookAndFeel& atomLookAndFeel;

    atom::Label introLabel;
    CompressorCurveComponent transferCurve;
    atom::Slider threshMinSlider { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    atom::Slider threshMaxSlider { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    atom::ToggleButton gateAtMinToggle { "gateAtMinToggle", "OFF" };
    atom::ToggleButton kneeTypeToggle { "kneeTypeToggle", "SOFT" };
    atom::Slider kneeWidthSlider { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    atom::Slider ratioSlider { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    atom::Slider attackSlider { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    atom::Slider releaseSlider { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };

    std::unique_ptr<juce::Component> threshMinRow;
    std::unique_ptr<juce::Component> threshMaxRow;
    std::unique_ptr<juce::Component> gateAtMinRow;
    std::unique_ptr<juce::Component> kneeTypeRow;
    std::unique_ptr<juce::Component> kneeWidthRow;
    std::unique_ptr<juce::Component> ratioRow;
    std::unique_ptr<juce::Component> attackRow;
    std::unique_ptr<juce::Component> releaseRow;
    std::unique_ptr<juce::Component> section;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SliderAttachment> threshMinAttachment;
    std::unique_ptr<SliderAttachment> threshMaxAttachment;
    std::unique_ptr<ButtonAttachment> gateAtMinAttachment;
    std::unique_ptr<ButtonAttachment> kneeTypeAttachment;
    std::unique_ptr<SliderAttachment> kneeWidthAttachment;
    std::unique_ptr<SliderAttachment> ratioAttachment;
    std::unique_ptr<SliderAttachment> attackAttachment;
    std::unique_ptr<SliderAttachment> releaseAttachment;

    static constexpr int kCurveMinHeight = 220;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoiseGateSettingsPanel)
};
