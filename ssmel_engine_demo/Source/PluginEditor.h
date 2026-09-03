#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include <juce_atom_theme/juce_atom_theme.h>

#include "PluginProcessor.h"

#if JucePlugin_Build_Standalone
#include "AppSettingsPanel.h"
#endif

class SsmelDemoEditor : public juce::AudioProcessorEditor
#if JucePlugin_Build_Standalone
                      , private juce::DarkModeSettingListener
#endif
{
public:
    explicit SsmelDemoEditor (SsmelDemoProcessor&);
    ~SsmelDemoEditor() override;

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

    SsmelDemoProcessor& processor;
    AtomLookAndFeel atomLookAndFeel { atom::ThemeType::Dark };

    atom::ShapeButton btnSettings { "btnSettings", AtomIconLibrary::Icon::CogWheel };
    atom::Label versionLabel;
    atom::ComboBox presetCombo;
    atom::Slider outputGainSlider;
    atom::Slider filterCutoffSlider;
    juce::MidiKeyboardComponent keyboard;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<ComboAttachment> presetAttachment;
    std::unique_ptr<SliderAttachment> outputGainAttachment;
    std::unique_ptr<SliderAttachment> filterCutoffAttachment;

#if JucePlugin_Build_Standalone
    juce::Component::SafePointer<juce::DialogWindow> appSettingsDialog;
#endif
};
