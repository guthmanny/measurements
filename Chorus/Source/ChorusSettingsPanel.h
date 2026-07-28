#pragma once

#include <juce_atom_theme/juce_atom_theme.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>

class ChorusAudioProcessor;

/** Settings page: Chorus engine limits (max delay / max LFO rate / max amount). */
class ChorusSettingsPanel final : public juce::Component,
                                  private juce::AudioProcessorValueTreeState::Listener
{
public:
    ChorusSettingsPanel (ChorusAudioProcessor& processor, AtomLookAndFeel& lookAndFeel);
    ~ChorusSettingsPanel() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    int getPreferredPanelHeight() const noexcept;

private:
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void configureHorizontalSlider (atom::Slider& slider, double minV, double maxV, double interval,
                                    const juce::String& suffix);

    ChorusAudioProcessor& processor;
    AtomLookAndFeel& atomLookAndFeel;

    atom::Label introLabel;
    atom::Slider maxDelaySlider { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    atom::Slider minLfoFreqSlider { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    atom::Slider maxLfoFreqSlider { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    atom::Slider maxAmountSlider { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };

    std::unique_ptr<juce::Component> maxDelayRow;
    std::unique_ptr<juce::Component> minLfoFreqRow;
    std::unique_ptr<juce::Component> maxLfoFreqRow;
    std::unique_ptr<juce::Component> maxAmountRow;
    std::unique_ptr<juce::Component> section;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> maxDelayAttachment;
    std::unique_ptr<SliderAttachment> minLfoFreqAttachment;
    std::unique_ptr<SliderAttachment> maxLfoFreqAttachment;
    std::unique_ptr<SliderAttachment> maxAmountAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChorusSettingsPanel)
};
