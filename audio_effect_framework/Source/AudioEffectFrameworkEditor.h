#pragma once

#include "AefJuceIncludes.h"
#include <juce_atom_theme/juce_atom_theme.h>

#include "EffectFooterComponent.h"
#include "EffectHeaderComponent.h"
#include "AudioEffectFrameworkProcessor.h"
#include "SpectrumOverlayComponent.h"
#include "TunerOverlayComponent.h"

#if JucePlugin_Build_Standalone
#include "AppSettingsPanel.h"
#endif

class EffectBodyContent final : public juce::Component
{
public:
    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colours::black);
    }
};

class AudioEffectFrameworkEditor : public juce::AudioProcessorEditor,
                                           private juce::Timer
#if JucePlugin_Build_Standalone
                                           , private juce::DarkModeSettingListener
#endif
{
public:
    /** @param deferBodyBuild If true, call completeBodyConstruction() from the
        derived constructor after inserting custom body rows (virtuals are not
        available during the base constructor). */
    explicit AudioEffectFrameworkEditor (AudioEffectFrameworkProcessor&,
                                         bool deferBodyBuild = false);
    ~AudioEffectFrameworkEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

#if JucePlugin_Build_Standalone
    void showStandaloneOptionsMenu();
    void showAppSettingsDialog (AppSettingsPanel::Page initialPage = AppSettingsPanel::Page::AudioSettings);
#endif

    void setTunerVisible (bool shouldShow);
    void setSpectrumVisible (bool shouldShow);

protected:
    /** Finish APVTS body rows + zoom/timer. Call once from a derived ctor when
        constructed with deferBodyBuild=true. */
    void completeBodyConstruction();

    /** Unzoomed height for a body row (override for custom components). */
    virtual int getBodyComponentBaseHeight (const juce::Component* component) const noexcept;

    /** Extra per-tick UI work after meters/overlays update. */
    virtual void onEditorTimerTick() {}

    AudioEffectFrameworkProcessor& processor;

    static constexpr int headerBaseHeight = 80;
    static constexpr int footerBaseHeight = 32;
    static constexpr int bodyPadding = 10;
    static constexpr int bodyMargin = 20;
    static constexpr int sliderRowHeight = 50;
    static constexpr int cardRowHeight = 48;

    float zoomFactor = 1.0f;
    int bodyContentHeight = 0;

    AtomLookAndFeel atomLookAndFeel { atom::ThemeType::Dark };
    EffectHeaderComponent headerBar;
    EffectFooterComponent footerBar;
    EffectBodyContent bodyContent;
    juce::Viewport bodyViewport;
    juce::Array<juce::Component*> bodyComponents;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    juce::OwnedArray<SliderAttachment> sliderAttachments;
    juce::OwnedArray<ButtonAttachment> buttonAttachments;
    juce::OwnedArray<ComboBoxAttachment> comboBoxAttachments;

private:
    void timerCallback() override;
#if JucePlugin_Build_Standalone
    void darkModeSettingChanged() override;
    void applyAppSettingsDialogTitleBarTheme();
#endif
    void applyZoom (float newZoom);
    int getEditorWidth();
    int getNaturalHeight() const noexcept;
    int getHeaderHeight() const noexcept;
    int getFooterHeight() const noexcept;
    int getBodyContentHeight() const noexcept;
    void buildParameterBodyRows();

    TunerOverlay tunerOverlay;
    SpectrumOverlay spectrumOverlay;
    std::vector<float> spectrumScratch;
    uint32_t lastSpectrumFrameId = 0;

    juce::OwnedArray<atom::Slider> sliders;
    juce::OwnedArray<atom::ToggleButton> toggles;
    juce::OwnedArray<atom::ComboBox> comboBoxes;
    juce::OwnedArray<juce::Component> settingRows;

#if JucePlugin_Build_Standalone
    juce::Component::SafePointer<juce::DialogWindow> appSettingsDialog;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioEffectFrameworkEditor)
};
