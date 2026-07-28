#include "PluginEditor.h"

#include <array>
#include <vector>

#if JucePlugin_Build_Standalone
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>

#include "AppSettingsPanel.h"

namespace
{
void applySystemNativeTitleBarTheme (juce::Component& target)
{
    atom::setNativeTitleBarDarkMode (target, juce::Desktop::getInstance().isDarkModeActive());
}
} // namespace
#endif

namespace
{
constexpr int kSectionTitleHeight = 22;
constexpr int kParamLabelHeight = 20;
constexpr int kSectionPad = 10;
} // namespace

class SectionPanel final : public juce::Component
{
public:
    SectionPanel (const juce::String& sectionName,
                  const juce::String& paramName,
                  atom::Slider& slider)
        : sectionLabel ("sectionLabel", sectionName),
          paramLabel ("paramLabel", paramName),
          sliderRef (slider)
    {
        sectionLabel.setJustificationType (juce::Justification::centred);
        sectionLabel.setFont (AtomLookAndFeel::getUIFont (13.0f, juce::Font::bold));

        paramLabel.setJustificationType (juce::Justification::centred);
        paramLabel.setFont (AtomLookAndFeel::getUIFont (AtomLookAndFeel::getSystemUIFontHeight(),
                                                        juce::Font::plain));

        card.addAndMakeVisible (sectionLabel);
        card.addAndMakeVisible (paramLabel);
        card.addAndMakeVisible (sliderRef);
        addAndMakeVisible (card);
    }

    void resized() override
    {
        card.setBounds (getLocalBounds());

        auto area = card.getLocalBounds().reduced (kSectionPad, 8);
        sectionLabel.setBounds (area.removeFromTop (kSectionTitleHeight));
        area.removeFromTop (6);
        paramLabel.setBounds (area.removeFromTop (kParamLabelHeight));
        area.removeFromTop (4);
        sliderRef.setBounds (area);
    }

private:
    atom::SettingsCard card;
    atom::Label sectionLabel;
    atom::Label paramLabel;
    atom::Slider& sliderRef;
};

class ModulatorsPanel final : public juce::Component
{
public:
    struct KnobBinding
    {
        juce::String groupName;
        juce::String paramName;
        atom::Slider& slider;
    };

    ModulatorsPanel (juce::Span<const KnobBinding> knobs)
    {
        sectionLabel.setText ("Modulators", juce::dontSendNotification);
        sectionLabel.setJustificationType (juce::Justification::centred);
        sectionLabel.setFont (AtomLookAndFeel::getUIFont (13.0f, juce::Font::bold));
        card.addAndMakeVisible (sectionLabel);

        for (const auto& knob : knobs)
        {
            auto column = std::make_unique<KnobColumn> (knob.groupName, knob.paramName, knob.slider);
            card.addAndMakeVisible (*column);
            columns.push_back (std::move (column));
        }

        addAndMakeVisible (card);
    }

    void resized() override
    {
        card.setBounds (getLocalBounds());

        auto area = card.getLocalBounds().reduced (kSectionPad, 8);
        sectionLabel.setBounds (area.removeFromTop (kSectionTitleHeight));
        area.removeFromTop (6);

        const int colCount = (int) columns.size();
        if (colCount <= 0)
            return;

        const int colW = area.getWidth() / colCount;
        for (auto& column : columns)
            column->setBounds (area.removeFromLeft (colW).reduced (2, 0));
    }

private:
    class KnobColumn final : public juce::Component
    {
    public:
        KnobColumn (const juce::String& groupName,
                    const juce::String& paramName,
                    atom::Slider& slider)
            : groupLabel ("groupLabel", groupName),
              paramLabel ("paramLabel", paramName),
              sliderRef (slider)
        {
            groupLabel.setJustificationType (juce::Justification::centred);
            groupLabel.setFont (AtomLookAndFeel::getUIFont (11.0f, juce::Font::bold));

            paramLabel.setJustificationType (juce::Justification::centred);
            paramLabel.setFont (AtomLookAndFeel::getUIFont (AtomLookAndFeel::getSystemUIFontHeight(),
                                                            juce::Font::plain));

            addAndMakeVisible (groupLabel);
            addAndMakeVisible (paramLabel);
            addAndMakeVisible (sliderRef);
        }

        void resized() override
        {
            auto area = getLocalBounds();
            groupLabel.setBounds (area.removeFromTop (16));
            paramLabel.setBounds (area.removeFromTop (kParamLabelHeight));
            area.removeFromTop (2);
            sliderRef.setBounds (area);
        }

    private:
        atom::Label groupLabel;
        atom::Label paramLabel;
        atom::Slider& sliderRef;
    };

    atom::SettingsCard card;
    atom::Label sectionLabel;
    std::vector<std::unique_ptr<KnobColumn>> columns;
};

void BasicSynthAudioProcessorEditor::configureRotarySlider (atom::Slider& slider)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setValueLabelPos (atom::Slider::ValueLabelPos::Below);
    slider.setRequiredWidthMode (atom::Slider::RequiredWidthMode::Content);
    slider.setValueLabelGap (2);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                juce::MathConstants<float>::pi * 2.75f,
                                true);
}

BasicSynthAudioProcessorEditor::BasicSynthAudioProcessorEditor (BasicSynthAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      oscSection (std::make_unique<SectionPanel> ("OSC", "Wave", waveSlider)),
      filterSection (std::make_unique<SectionPanel> ("Filter", "Cutoff", cutoffSlider)),
      ampSection (std::make_unique<SectionPanel> ("Amp", "Gain", gainSlider)),
      modulatorsSection (std::make_unique<ModulatorsPanel> (std::array<ModulatorsPanel::KnobBinding, 8> {{
          { "LFO1", "Rate", lfo1RateSlider },
          { "LFO2", "Rate", lfo2RateSlider },
          { "EG1", "Attack", eg1AttackSlider },
          { "EG1", "Release", eg1ReleaseSlider },
          { "EG2", "Attack", eg2AttackSlider },
          { "EG2", "Release", eg2ReleaseSlider },
          { "EG3", "Attack", eg3AttackSlider },
          { "EG3", "Release", eg3ReleaseSlider },
      }})),
      keyboard (processor.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    juce::LookAndFeel::setDefaultLookAndFeel (&atomLookAndFeel);
    setLookAndFeel (&atomLookAndFeel);

    btnSettings.setTooltip ("Settings");
    addAndMakeVisible (btnSettings);

#if JucePlugin_Build_Standalone
    btnSettings.onClick = [this] { showAppSettingsDialog(); };
#endif

    for (auto* slider : { &waveSlider, &cutoffSlider, &gainSlider,
                          &eg1AttackSlider, &eg1ReleaseSlider,
                          &eg2AttackSlider, &eg2ReleaseSlider,
                          &eg3AttackSlider, &eg3ReleaseSlider,
                          &lfo1RateSlider, &lfo2RateSlider })
        configureRotarySlider (*slider);

    waveAttachment = std::make_unique<SliderAttachment> (
        processor.parameters, "wave", waveSlider);
    cutoffAttachment = std::make_unique<SliderAttachment> (
        processor.parameters, "cutoff", cutoffSlider);
    gainAttachment = std::make_unique<SliderAttachment> (
        processor.parameters, "gain", gainSlider);
    eg1AttackAttachment = std::make_unique<SliderAttachment> (
        processor.parameters, "eg1_attack", eg1AttackSlider);
    eg1ReleaseAttachment = std::make_unique<SliderAttachment> (
        processor.parameters, "eg1_release", eg1ReleaseSlider);
    eg2AttackAttachment = std::make_unique<SliderAttachment> (
        processor.parameters, "eg2_attack", eg2AttackSlider);
    eg2ReleaseAttachment = std::make_unique<SliderAttachment> (
        processor.parameters, "eg2_release", eg2ReleaseSlider);
    eg3AttackAttachment = std::make_unique<SliderAttachment> (
        processor.parameters, "eg3_attack", eg3AttackSlider);
    eg3ReleaseAttachment = std::make_unique<SliderAttachment> (
        processor.parameters, "eg3_release", eg3ReleaseSlider);
    lfo1RateAttachment = std::make_unique<SliderAttachment> (
        processor.parameters, "lfo1_rate", lfo1RateSlider);
    lfo2RateAttachment = std::make_unique<SliderAttachment> (
        processor.parameters, "lfo2_rate", lfo2RateSlider);

    addAndMakeVisible (*oscSection);
    addAndMakeVisible (*filterSection);
    addAndMakeVisible (*ampSection);
    addAndMakeVisible (*modulatorsSection);
    addAndMakeVisible (keyboard);
    setSize (720, 580);

#if JucePlugin_Build_Standalone
    juce::Desktop::getInstance().addDarkModeSettingListener (this);
#endif
}

BasicSynthAudioProcessorEditor::~BasicSynthAudioProcessorEditor()
{
#if JucePlugin_Build_Standalone
    juce::Desktop::getInstance().removeDarkModeSettingListener (this);
#endif
    setLookAndFeel (nullptr);
}

void BasicSynthAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (findColour (juce::ResizableWindow::backgroundColourId));
}

void BasicSynthAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (12);

    auto header = area.removeFromTop (28);
    btnSettings.setBounds (header.removeFromRight (28));

    auto controls = area.removeFromTop (190);
    const int sectionW = controls.getWidth() / 3;
    oscSection->setBounds (controls.removeFromLeft (sectionW).reduced (4, 0));
    filterSection->setBounds (controls.removeFromLeft (sectionW).reduced (4, 0));
    ampSection->setBounds (controls.reduced (4, 0));

    modulatorsSection->setBounds (area.removeFromTop (170).reduced (4, 0));

    keyboard.setBounds (area.reduced (0, 8));
}

#if JucePlugin_Build_Standalone
void BasicSynthAudioProcessorEditor::applyAppSettingsDialogTitleBarTheme()
{
    if (appSettingsDialog == nullptr)
        return;

    applySystemNativeTitleBarTheme (*appSettingsDialog);
}

void BasicSynthAudioProcessorEditor::darkModeSettingChanged()
{
    applyAppSettingsDialogTitleBarTheme();
}

void BasicSynthAudioProcessorEditor::showAppSettingsDialog (AppSettingsPanel::Page initialPage)
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

        juce::Component::SafePointer<BasicSynthAudioProcessorEditor> safeEditor (this);
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
