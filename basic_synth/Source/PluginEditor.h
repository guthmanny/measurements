#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include <juce_atom_theme/juce_atom_theme.h>

#include "PluginProcessor.h"

#if JucePlugin_Build_Standalone
#include "AppSettingsPanel.h"
#endif

class BasicSynthAudioProcessorEditor : public juce::AudioProcessorEditor
#if JucePlugin_Build_Standalone
                                     , private juce::DarkModeSettingListener
#endif
{
public:
    explicit BasicSynthAudioProcessorEditor (BasicSynthAudioProcessor&);
    ~BasicSynthAudioProcessorEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

#if JucePlugin_Build_Standalone
    void showAppSettingsDialog (AppSettingsPanel::Page initialPage = AppSettingsPanel::Page::AudioSettings);
#endif

private:
#if JucePlugin_Build_Standalone
    void darkModeSettingChanged() override;
    void applyAppSettingsDialogTitleBarTheme();
#endif

    void configureRotarySlider (atom::Slider& slider);

    BasicSynthAudioProcessor& processor;
    AtomLookAndFeel atomLookAndFeel { atom::ThemeType::Dark };

    atom::ShapeButton btnSettings { "btnSettings", AtomIconLibrary::Icon::CogWheel };
    atom::Slider waveSlider;
    atom::Slider cutoffSlider;
    atom::Slider gainSlider;
    atom::Slider eg1AttackSlider;
    atom::Slider eg1ReleaseSlider;
    atom::Slider eg2AttackSlider;
    atom::Slider eg2ReleaseSlider;
    atom::Slider eg3AttackSlider;
    atom::Slider eg3ReleaseSlider;
    atom::Slider lfo1RateSlider;
    atom::Slider lfo2RateSlider;
    std::unique_ptr<juce::Component> oscSection;
    std::unique_ptr<juce::Component> filterSection;
    std::unique_ptr<juce::Component> ampSection;
    std::unique_ptr<juce::Component> modulatorsSection;
    juce::MidiKeyboardComponent keyboard;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> waveAttachment;
    std::unique_ptr<SliderAttachment> cutoffAttachment;
    std::unique_ptr<SliderAttachment> gainAttachment;
    std::unique_ptr<SliderAttachment> eg1AttackAttachment;
    std::unique_ptr<SliderAttachment> eg1ReleaseAttachment;
    std::unique_ptr<SliderAttachment> eg2AttackAttachment;
    std::unique_ptr<SliderAttachment> eg2ReleaseAttachment;
    std::unique_ptr<SliderAttachment> eg3AttackAttachment;
    std::unique_ptr<SliderAttachment> eg3ReleaseAttachment;
    std::unique_ptr<SliderAttachment> lfo1RateAttachment;
    std::unique_ptr<SliderAttachment> lfo2RateAttachment;

#if JucePlugin_Build_Standalone
    juce::Component::SafePointer<juce::DialogWindow> appSettingsDialog;
#endif
};
