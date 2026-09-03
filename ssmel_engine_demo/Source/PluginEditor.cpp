#include "PluginEditor.h"

#include "ssmel/ssmel_engine.h"

#if JucePlugin_Build_Standalone
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>

namespace
{
void applySystemNativeTitleBarTheme (juce::Component& target)
{
    atom::setNativeTitleBarDarkMode (target, juce::Desktop::getInstance().isDarkModeActive());
}
} // namespace
#endif

SsmelDemoEditor::SsmelDemoEditor (SsmelDemoProcessor& p)
    : AudioProcessorEditor (p),
      processor (p),
      versionLabel ("versionLabel", juce::String (ssmel_engine_version_string())),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    juce::LookAndFeel::setDefaultLookAndFeel (&atomLookAndFeel);
    setLookAndFeel (&atomLookAndFeel);

    btnSettings.setTooltip ("Settings");
    addAndMakeVisible (btnSettings);

    versionLabel.setJustificationType (juce::Justification::centredLeft);
    versionLabel.setFont (AtomLookAndFeel::getUIFont (12.0f, juce::Font::plain));
    addAndMakeVisible (versionLabel);

    presetCombo.addItem ("Piano", 1);
    presetCombo.addItem ("EP", 2);
    presetCombo.setTooltip ("Piano2ry.XDI / Ep2.XDI (client-built sample maps)");
    addAndMakeVisible (presetCombo);

    configureRotarySlider (outputGainSlider);
    configureRotarySlider (filterCutoffSlider);
    filterCutoffSlider.setTooltip ("Forte cutoff (Hz). Soft notes close from this.");
    addAndMakeVisible (outputGainSlider);
    addAndMakeVisible (filterCutoffSlider);

    keyboard.setKeyWidth (26.0f);
    keyboard.setScrollButtonsVisible (false);
    keyboard.setOctaveForMiddleC (4);
    keyboard.setAvailableRange (21, 108);
    keyboard.setLowestVisibleKey (36);
    addAndMakeVisible (keyboard);

    presetAttachment = std::make_unique<ComboAttachment> (
        processor.parameters, SsmelDemoProcessor::kPresetParamId, presetCombo);
    outputGainAttachment = std::make_unique<SliderAttachment> (
        processor.parameters, SsmelDemoProcessor::kOutputGainParamId, outputGainSlider);
    filterCutoffAttachment = std::make_unique<SliderAttachment> (
        processor.parameters, SsmelDemoProcessor::kFilterCutoffParamId, filterCutoffSlider);

#if JucePlugin_Build_Standalone
    btnSettings.onClick = [this] { showAppSettingsDialog(); };
    juce::Desktop::getInstance().addDarkModeSettingListener (this);
#endif

    setSize (720, 420);
}

SsmelDemoEditor::~SsmelDemoEditor()
{
#if JucePlugin_Build_Standalone
    juce::Desktop::getInstance().removeDarkModeSettingListener (this);
#endif
    setLookAndFeel (nullptr);
}

void SsmelDemoEditor::configureRotarySlider (atom::Slider& slider)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 18);
}

void SsmelDemoEditor::paint (juce::Graphics& g)
{
    g.fillAll (findColour (juce::ResizableWindow::backgroundColourId));
}

void SsmelDemoEditor::resized()
{
    auto area = getLocalBounds().reduced (12);

    auto header = area.removeFromTop (28);
    btnSettings.setBounds (header.removeFromRight (28));
    header.removeFromRight (8);
    presetCombo.setBounds (header.removeFromLeft (160));
    header.removeFromLeft (12);
    versionLabel.setBounds (header);

    auto knobs = area.removeFromTop (180);
    const int knobW = knobs.getWidth() / 2;
    outputGainSlider.setBounds (knobs.removeFromLeft (knobW).reduced (24, 8));
    filterCutoffSlider.setBounds (knobs.reduced (24, 8));

    keyboard.setBounds (area.reduced (0, 8));
}

#if JucePlugin_Build_Standalone
void SsmelDemoEditor::applyAppSettingsDialogTitleBarTheme()
{
    if (appSettingsDialog == nullptr)
        return;

    applySystemNativeTitleBarTheme (*appSettingsDialog);
}

void SsmelDemoEditor::darkModeSettingChanged()
{
    applyAppSettingsDialogTitleBarTheme();
}

void SsmelDemoEditor::showAppSettingsDialog (AppSettingsPanel::Page initialPage)
{
    if (appSettingsDialog != nullptr)
    {
        if (auto* panel = dynamic_cast<AppSettingsPanel*> (appSettingsDialog->getContentComponent()))
            panel->selectPage (initialPage);

        appSettingsDialog->toFront (true);
        appSettingsDialog->grabKeyboardFocus();
        return;
    }

    auto* window = findParentComponentOfClass<juce::StandaloneFilterWindow>();
    if (window == nullptr)
        return;

    auto* panel = new AppSettingsPanel (window->getDeviceManager(), atomLookAndFeel);
    panel->selectPage (initialPage);

    const int prefW = panel->getPreferredWidth();
    const int prefH = panel->getPreferredHeight();
    panel->setSize (prefW, prefH);

    const int minPanelW = panel->getMinimumWidth();
    constexpr int kMaxMinW = 720;
    const int clampedMinW = juce::jmin (minPanelW, kMaxMinW);
    const int minDialogW = juce::jmax (560, clampedMinW + 20);
    const int minDialogH = juce::jmax (360, panel->getMinimumHeight());

    juce::DialogWindow::LaunchOptions options;
    options.dialogTitle = "Settings";
    options.dialogBackgroundColour =
        getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId);
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = true;
    options.useBottomRightCornerResizer = false;
    options.content.setOwned (panel);
    options.componentToCentreAround = window;

    auto* dialog = options.create();
    appSettingsDialog = dialog;

    if (dialog != nullptr)
    {
        dialog->setResizeLimits (minDialogW, minDialogH, 1600, 1200);
        dialog->setAlwaysOnTop (true);
        applyAppSettingsDialogTitleBarTheme();

        juce::Component::SafePointer<juce::Component> safeDialog (dialog);
        juce::Timer::callAfterDelay (0, [safeDialog]()
        {
            if (safeDialog != nullptr)
                applySystemNativeTitleBarTheme (*safeDialog);
        });

        juce::Component::SafePointer<SsmelDemoEditor> safeEditor (this);
        dialog->enterModalState (true,
                                juce::ModalCallbackFunction::create ([safeEditor] (int)
                                {
                                    if (safeEditor != nullptr)
                                        safeEditor->appSettingsDialog = nullptr;
                                }),
                                true);
    }
}
#endif
