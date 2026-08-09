#include <JuceHeader.h>

#include "AudioEffectFrameworkEditor.h"

#include <algorithm>
#include <cmath>

#include "MeterDisplayUtils.h"

#if JucePlugin_Build_Standalone
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>

#include "AppSettingsPanel.h"
#include "JackCaptureRouting.h"
#endif

namespace
{
class SettingsCardRow final : public juce::Component
{
 public:
  SettingsCardRow(const juce::String& rowName, const juce::String& title, juce::Component& controlToEmbed, int height)
      : label(rowName + "Label", title), control(controlToEmbed), rowHeight(height)
  {
    card.setMinPanelHeight(rowHeight);
    label.setJustificationType(juce::Justification::centredLeft);
    label.setFont(AtomLookAndFeel::getUIFont(AtomLookAndFeel::getSystemUIFontHeight(), juce::Font::plain));
    label.setMinimumHorizontalScale(1.0f);
    label.setAutoResizeEnabled(false);
    card.addAndMakeVisible(label);
    card.addAndMakeVisible(control);
    addAndMakeVisible(card);

    if (auto* combo = dynamic_cast<atom::ComboBox*>(&control))
    {
      combo->onChange = [this] { resized(); };
    }
  }

  void resized() override
  {
    card.setBounds(getLocalBounds());
    auto area = card.getLocalBounds().reduced(12, 8);
    const float rowH = (float) area.getHeight();

    const auto& font = label.getFont();
    const int labelTextWidth =
        juce::roundToInt(font.getStringWidthFloat(label.getText()) + 4.0f);

    if (auto* combo = dynamic_cast<atom::ComboBox*>(&control))
    {
      combo->setSize(combo->getIdealWidth(), juce::roundToInt(rowH));

      juce::FlexBox flex;
      flex.flexDirection = juce::FlexBox::Direction::row;
      flex.alignItems = juce::FlexBox::AlignItems::center;

      juce::FlexItem labelItem((float) labelTextWidth, rowH, label);
      labelItem.flexShrink = 0.0f;

      juce::FlexItem spacer(0.0f, rowH);
      spacer.flexGrow = 1.0f;

      juce::FlexItem comboItem((float) combo->getWidth(), rowH, *combo);
      comboItem.flexShrink = 0.0f;

      flex.items.addArray({labelItem, spacer, comboItem});
      flex.performLayout(area);
      return;
    }

    constexpr int labelGap = 12;
    label.setBounds(area.getX(), area.getY(), labelTextWidth, area.getHeight());
    area.removeFromLeft(labelTextWidth + labelGap);
    control.setBounds(area);
  }

 private:
  atom::SettingsCard card;
  atom::Label label;
  juce::Component& control;
  int rowHeight;
};

#if JucePlugin_Build_Standalone
void applySystemNativeTitleBarTheme(juce::Component& target)
{
  atom::setNativeTitleBarDarkMode(target, juce::Desktop::getInstance().isDarkModeActive());
}
#endif
}  // namespace

AudioEffectFrameworkEditor::AudioEffectFrameworkEditor(AudioEffectFrameworkProcessor& p, bool deferBodyBuild)
    : juce::AudioProcessorEditor(&p), processor(p)
{
  juce::LookAndFeel::setDefaultLookAndFeel(&atomLookAndFeel);

  addAndMakeVisible(headerBar);
  addAndMakeVisible(footerBar);
  addAndMakeVisible(bodyViewport);
  bodyViewport.setViewedComponent(&bodyContent, false);
  bodyViewport.setScrollBarsShown(true, false);
  bodyViewport.getVerticalScrollBar().setColour(juce::ScrollBar::thumbColourId, juce::Colours::grey);

  headerBar.getBtnSettings().onClick = [this]
  {
#if JucePlugin_Build_Standalone
    showAppSettingsDialog();
#endif
  };

  headerBar.getBtnTuner().onClick = [this]
  {
    setTunerVisible (! tunerOverlay.isVisible());
  };

  headerBar.getBtnSpectrum().onClick = [this]
  {
    setSpectrumVisible (! spectrumOverlay.isVisible());
  };

  tunerOverlay.getContent().onCloseRequested = [this] { setTunerVisible (false); };
  tunerOverlay.getContent().onPeriodicityThresholdChanged = [this] (float threshold)
  {
    processor.setTunerPeriodicityThreshold (threshold);
  };
  tunerOverlay.getContent().setPeriodicityThreshold (processor.getTunerPeriodicityThreshold());
  addChildComponent (tunerOverlay);
  tunerOverlay.setAlwaysOnTop (true);

  spectrumOverlay.getContent().onCloseRequested = [this] { setSpectrumVisible (false); };
  spectrumOverlay.getContent().onFftSizeChanged = [this] (int fftSize)
  {
    processor.setSpectrumFftSize (fftSize);
    spectrumOverlay.getContent().clearSpectrum();
  };
  spectrumOverlay.getContent().setFftSize (processor.getSpectrumFftSize());
  addChildComponent (spectrumOverlay);
  spectrumOverlay.setAlwaysOnTop (true);

  footerBar.getBtnMidiPort().setVisible (false);
  footerBar.onZoomChanged = [this](float scale) { applyZoom(scale); };

  {
    auto& quality = footerBar.getQualityComboBox();
    float qualityChoice = (float) processor.paramOversampleQuality.defaultChoice;
    if (auto* param = processor.parameters.valueTreeState.getParameter (processor.paramOversampleQuality.paramID))
      qualityChoice = param->convertFrom0to1 (param->getValue());
    quality.setSelectedId (juce::jlimit (1, 3, juce::roundToInt (qualityChoice) + 1), juce::dontSendNotification);
    quality.onChange = [this]
    {
      auto* param = processor.parameters.valueTreeState.getParameter (processor.paramOversampleQuality.paramID);
      if (param == nullptr)
        return;
      const int id = footerBar.getQualityComboBox().getSelectedId();
      const float choice = (float) juce::jlimit (0, 2, id - 1);
      param->beginChangeGesture();
      param->setValueNotifyingHost (param->convertTo0to1 (choice));
      param->endChangeGesture();
    };
  }

  sliderAttachments.add(
      new SliderAttachment(processor.parameters.valueTreeState, "inputgain", headerBar.getSliderInput()));
  sliderAttachments.add(
      new SliderAttachment(processor.parameters.valueTreeState, "gatethreshold", headerBar.getSliderGate()));
  sliderAttachments.add(
      new SliderAttachment(processor.parameters.valueTreeState, "outputgain", headerBar.getSliderOutput()));

  bodyContentHeight = bodyMargin;

  if (! deferBodyBuild)
    completeBodyConstruction();

#if JucePlugin_Build_Standalone
  juce::Desktop::getInstance().addDarkModeSettingListener(this);
#endif
}

void AudioEffectFrameworkEditor::completeBodyConstruction()
{
  buildParameterBodyRows();
  bodyContentHeight += bodyMargin;
  applyZoom(1.0f);
  startTimerHz (meter_display::kRefreshHz);
}

int AudioEffectFrameworkEditor::getBodyComponentBaseHeight (const juce::Component* component) const noexcept
{
  if (dynamic_cast<const atom::Slider*> (component) != nullptr)
    return sliderRowHeight;
  return cardRowHeight;
}

void AudioEffectFrameworkEditor::buildParameterBodyRows()
{
  const juce::Array<juce::AudioProcessorParameter*>& parameters = processor.getParameters();

  const juce::StringArray headerParamIds{"inputgain", "gatethreshold", "outputgain"};
  const juce::StringArray settingsOnlyParamIds{"gatethreshmin", "gatethreshmax", "gateoffatmin", "gateratio",
                                               "gateattack", "gaterelease", "gateknee", "gatekneewidth",
                                               "meterattack", "meterrelease", "meterdisplayrange",
                                               "oversamplequality", "upsamplermode", "downsamplermode"};

  const auto uiFont = AtomLookAndFeel::getUIFont(AtomLookAndFeel::getSystemUIFontHeight(), juce::Font::plain);
  const float uiFontHeight = AtomLookAndFeel::getSystemUIFontHeight();
  float maxParamLabelWidth = 0.0f;
  float maxValueTextWidth = 0.0f;

  for (int i = 0; i < parameters.size(); ++i)
  {
    if (const auto* parameter = dynamic_cast<const juce::AudioProcessorParameterWithID*>(parameters[i]))
    {
      if (headerParamIds.contains(parameter->paramID) || settingsOnlyParamIds.contains(parameter->paramID))
        continue;

      if (processor.parameters.parameterTypes[i] != "Slider") continue;

      maxParamLabelWidth = juce::jmax(maxParamLabelWidth, uiFont.getStringWidthFloat(parameter->name));

      if (auto* param = processor.parameters.valueTreeState.getParameter(parameter->paramID))
      {
        const float numW0 = uiFont.getStringWidthFloat(param->getText(0.0f, 0));
        const float numW1 = uiFont.getStringWidthFloat(param->getText(1.0f, 0));
        const float suffixW = uiFont.getStringWidthFloat(parameter->label);
        maxValueTextWidth = juce::jmax(maxValueTextWidth, numW0 + suffixW, numW1 + suffixW);
      }
    }
  }

  const float labelReserveDlu = maxParamLabelWidth > 0.0f ? maxParamLabelWidth * 8.0f / uiFontHeight : 0.0f;
  const float valueReserveDlu = maxValueTextWidth > 0.0f ? (maxValueTextWidth + 12.0f) * 8.0f / uiFontHeight : 0.0f;

  for (int i = 0; i < parameters.size(); ++i)
  {
    if (const auto* parameter = dynamic_cast<const juce::AudioProcessorParameterWithID*>(parameters[i]))
    {
      if (headerParamIds.contains(parameter->paramID) || settingsOnlyParamIds.contains(parameter->paramID))
        continue;

      if (processor.parameters.parameterTypes[i] == "Slider")
      {
        auto* aSlider = sliders.add(new atom::Slider());
        aSlider->setTextValueSuffix(parameter->label);
        aSlider->setValueLabelPos(atom::Slider::ValueLabelPos::Right);

        atom::SliderStyleOverride styleOverride;
        styleOverride.colors.labelText = parameter->name;
        styleOverride.metrics.linearHorizontalLabelReserveDlu = labelReserveDlu;
        styleOverride.metrics.linearHorizontalValueLabelReserveDlu = valueReserveDlu;
        atomLookAndFeel.setSliderStyleOverride(*aSlider, styleOverride);

        sliderAttachments.add(
            new SliderAttachment(processor.parameters.valueTreeState, parameter->paramID, *aSlider));

        bodyContent.addAndMakeVisible(aSlider);
        bodyComponents.add(aSlider);
        bodyContentHeight += getBodyComponentBaseHeight (aSlider) + bodyPadding;
      }
      else if (processor.parameters.parameterTypes[i] == "ToggleButton")
      {
        auto* aButton = toggles.add(new atom::ToggleButton(parameter->paramID, {}));
        aButton->setToggleState(parameter->getDefaultValue(), juce::dontSendNotification);

        buttonAttachments.add(
            new ButtonAttachment(processor.parameters.valueTreeState, parameter->paramID, *aButton));

        auto* row = new SettingsCardRow(parameter->paramID + "Row", parameter->name, *aButton, cardRowHeight);
        settingRows.add(row);
        bodyContent.addAndMakeVisible(row);
        bodyComponents.add(row);
        bodyContentHeight += getBodyComponentBaseHeight (row) + bodyPadding;
      }
      else if (processor.parameters.parameterTypes[i] == "ComboBox")
      {
        int comboListIndex = 0;
        for (int j = 0; j < i; ++j)
          if (processor.parameters.parameterTypes[j] == "ComboBox")
            ++comboListIndex;

        auto* aComboBox = comboBoxes.add(new atom::ComboBox());
        aComboBox->setEditableText(false);
        aComboBox->setJustificationType(juce::Justification::centredLeft);
        aComboBox->addItemList(processor.parameters.comboBoxItemLists[comboListIndex], 1);

        comboBoxAttachments.add(
            new ComboBoxAttachment(processor.parameters.valueTreeState, parameter->paramID, *aComboBox));

        auto* row = new SettingsCardRow(parameter->paramID + "Row", parameter->name, *aComboBox, cardRowHeight);
        settingRows.add(row);
        bodyContent.addAndMakeVisible(row);
        bodyComponents.add(row);
        bodyContentHeight += getBodyComponentBaseHeight (row) + bodyPadding;
      }
    }
  }
}

AudioEffectFrameworkEditor::~AudioEffectFrameworkEditor()
{
  stopTimer();

#if JucePlugin_Build_Standalone
  juce::Desktop::getInstance().removeDarkModeSettingListener(this);
#endif

  for (auto* slider : sliders) atomLookAndFeel.clearSliderStyleOverride(*slider);

  juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
}

int AudioEffectFrameworkEditor::getHeaderHeight() const noexcept
{
  return juce::roundToInt((float)headerBaseHeight * zoomFactor);
}

int AudioEffectFrameworkEditor::getFooterHeight() const noexcept
{
  return juce::roundToInt((float)footerBaseHeight * zoomFactor);
}

int AudioEffectFrameworkEditor::getBodyContentHeight() const noexcept
{
  return juce::roundToInt((float)bodyContentHeight * zoomFactor);
}

int AudioEffectFrameworkEditor::getEditorWidth()
{
  const int headerW = headerBar.getMinimumContentWidth(getHeaderHeight());
  const int footerW = footerBar.getMinimumContentWidth(getFooterHeight());
  return juce::jmax(headerW, footerW);
}

int AudioEffectFrameworkEditor::getNaturalHeight() const noexcept
{
  return getHeaderHeight() + getBodyContentHeight() + getFooterHeight();
}

void AudioEffectFrameworkEditor::applyZoom(float newZoom)
{
  zoomFactor = juce::jlimit(0.75f, 1.25f, newZoom);

  const int width = getEditorWidth();
  const int height = getNaturalHeight();

  setResizeLimits(width, height, width, height);
  setSize(width, height);

  bodyContent.setSize(width, getBodyContentHeight());
  resized();
}

void AudioEffectFrameworkEditor::setTunerVisible (bool shouldShow)
{
  processor.setTunerEnabled (shouldShow);
  tunerOverlay.setVisible (shouldShow);
  tunerOverlay.getContent().setTuningModeActive (shouldShow);

#if JucePlugin_Build_Standalone
  if (shouldShow)
  {
    if (auto* window = findParentComponentOfClass<juce::StandaloneFilterWindow>())
      effect_jack::prepareJackInputForTuning (window->getDeviceManager(), processor.getName());
  }
#endif

  if (shouldShow)
  {
    tunerOverlay.setBounds (getLocalBounds());
    tunerOverlay.toFront (false);
    tunerOverlay.getContent().clearPitch();
  }
}

void AudioEffectFrameworkEditor::setSpectrumVisible (bool shouldShow)
{
  processor.setSpectrumEnabled (shouldShow);
  spectrumOverlay.setVisible (shouldShow);

  if (shouldShow)
  {
    spectrumOverlay.setBounds (getLocalBounds());
    spectrumOverlay.toFront (false);
    spectrumOverlay.getContent().setFftSize (processor.getSpectrumFftSize());
    spectrumOverlay.getContent().clearSpectrum();
    lastSpectrumFrameId = 0;
  }
}

void AudioEffectFrameworkEditor::timerCallback()
{
  auto readParam = [this](const juce::String& id, float fallback) -> float
  {
    if (auto* p = processor.parameters.valueTreeState.getParameter(id))
      return p->convertFrom0to1(p->getValue());
    return fallback;
  };

  headerBar.applyMeterSettings(
      meter_display::displayRangeDbFromChoiceIndex(juce::roundToInt(
          readParam(processor.paramMeterDisplayRange.paramID,
                    (float) processor.paramMeterDisplayRange.defaultChoice))));

  const float attackMs = readParam(processor.paramMeterAttack.paramID, processor.paramMeterAttack.defaultValue);
  const float releaseSec = readParam(processor.paramMeterRelease.paramID, processor.paramMeterRelease.defaultValue);
  const int displayRangeDbSpan = meter_display::displayRangeDbFromChoiceIndex(juce::roundToInt(
      readParam(processor.paramMeterDisplayRange.paramID,
                (float) processor.paramMeterDisplayRange.defaultChoice)));

  processor.updateMeterDisplay(attackMs, releaseSec, displayRangeDbSpan,
                               1.0f / (float) meter_display::kRefreshHz);

  headerBar.setMeterDisplayLevels(processor.getMeterDisplayMonoNormalized(),
                                  processor.getMeterDisplayLeftNormalized(),
                                  processor.getMeterDisplayRightNormalized());

  {
    float minDb = readParam(processor.paramGateThreshMin.paramID, processor.paramGateThreshMin.defaultValue);
    float maxDb = readParam(processor.paramGateThreshMax.paramID, processor.paramGateThreshMax.defaultValue);
    if (minDb > maxDb)
      std::swap(minDb, maxDb);
    if (maxDb - minDb < 0.1f)
      maxDb = juce::jmin(0.0f, minDb + 0.1f);

    auto& gateSlider = headerBar.getSliderGate();
    if (std::abs(gateSlider.getMinimum() - (double)minDb) > 1.0e-4
        || std::abs(gateSlider.getMaximum() - (double)maxDb) > 1.0e-4)
      gateSlider.setRange((double)minDb, (double)maxDb, 0.01);

    if (auto* threshParam = processor.parameters.valueTreeState.getParameter(processor.paramGateThreshold.paramID))
    {
      const float threshDb = threshParam->convertFrom0to1(threshParam->getValue());
      const float clamped = juce::jlimit(minDb, maxDb, threshDb);
      if (std::abs(clamped - threshDb) > 1.0e-3f)
        threshParam->setValueNotifyingHost(threshParam->convertTo0to1(clamped));
    }
  }

  if (tunerOverlay.isVisible())
  {
    tunerOverlay.getContent().setInputDbFs (processor.getTunerInputDbFs(),
                                            processor.getTunerSampleRate(),
                                            processor.getTunerInputDbFsRight());
    tunerOverlay.getContent().setPitchResult (processor.getTunerResult());
  }

  if (spectrumOverlay.isVisible()
      && processor.copySpectrumMagnitudesIfNew (lastSpectrumFrameId, spectrumScratch))
  {
    spectrumOverlay.getContent().setSpectrumMagnitudes (spectrumScratch,
                                                        processor.getSpectrumSampleRate(),
                                                        processor.getSpectrumFftSize());
  }

  onEditorTimerTick();
}

void AudioEffectFrameworkEditor::paint(juce::Graphics& g)
{
  g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void AudioEffectFrameworkEditor::resized()
{
  auto bounds = getLocalBounds();
  const int headerH = getHeaderHeight();
  const int footerH = getFooterHeight();

  headerBar.setBounds(bounds.removeFromTop(headerH));
  footerBar.setBounds(bounds.removeFromBottom(footerH));
  bodyViewport.setBounds(bounds);

  if (tunerOverlay.isVisible())
    tunerOverlay.setBounds(getLocalBounds());

  if (spectrumOverlay.isVisible())
    spectrumOverlay.setBounds(getLocalBounds());

  bodyContent.setSize(juce::jmax(getWidth(), getEditorWidth()), juce::jmax(getBodyContentHeight(), bounds.getHeight()));

  auto area = bodyContent.getLocalBounds().reduced(juce::roundToInt((float)bodyMargin * zoomFactor), 0);

  for (auto* component : bodyComponents)
  {
    if (!component->isVisible()) continue;
    const int rowHeight = juce::roundToInt ((float) getBodyComponentBaseHeight (component) * zoomFactor);
    component->setBounds(area.removeFromTop(rowHeight));
    area.removeFromTop(juce::roundToInt((float)bodyPadding * zoomFactor));
  }
}

#if JucePlugin_Build_Standalone
void AudioEffectFrameworkEditor::applyAppSettingsDialogTitleBarTheme()
{
  if (appSettingsDialog == nullptr) return;

  applySystemNativeTitleBarTheme(*appSettingsDialog);
}

void AudioEffectFrameworkEditor::darkModeSettingChanged() { applyAppSettingsDialogTitleBarTheme(); }

void AudioEffectFrameworkEditor::showAppSettingsDialog (AppSettingsPanel::Page initialPage)
{
  if (appSettingsDialog != nullptr)
  {
    if (auto* panel = dynamic_cast<AppSettingsPanel*> (appSettingsDialog->getContentComponent()))
      panel->selectPage (initialPage);

    appSettingsDialog->toFront(true);
    appSettingsDialog->grabKeyboardFocus();
    return;
  }

  auto* window = findParentComponentOfClass<juce::StandaloneFilterWindow>();
  if (window == nullptr) return;

  auto* panel = new AppSettingsPanel (window->getDeviceManager(), processor, atomLookAndFeel);
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
  options.dialogBackgroundColour = getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId);
  options.escapeKeyTriggersCloseButton = true;
  options.useNativeTitleBar = true;
  options.resizable = true;
  options.useBottomRightCornerResizer = false;
  options.content.setOwned(panel);
  options.componentToCentreAround = window;

  auto* dialog = options.create();
  appSettingsDialog = dialog;

  if (dialog != nullptr)
  {
    dialog->setResizeLimits(minDialogW, minDialogH, 1600, 1200);
    dialog->setAlwaysOnTop(true);
    applyAppSettingsDialogTitleBarTheme();

    juce::Component::SafePointer<juce::Component> safeDialog(dialog);
    juce::Timer::callAfterDelay(0,
                                [safeDialog]()
                                {
                                  if (safeDialog != nullptr) applySystemNativeTitleBarTheme(*safeDialog);
                                });

    juce::Component::SafePointer<AudioEffectFrameworkEditor> safeEditor(this);
    dialog->enterModalState(true,
                            juce::ModalCallbackFunction::create(
                                [safeEditor](int)
                                {
                                  if (safeEditor != nullptr) safeEditor->appSettingsDialog = nullptr;
                                }),
                            true);
  }
}

void AudioEffectFrameworkEditor::showStandaloneOptionsMenu()
{
  auto* window = findParentComponentOfClass<juce::StandaloneFilterWindow>();
  if (window == nullptr) return;

  juce::PopupMenu menu;
  menu.addItem(1, TRANS("Settings..."));
  menu.addSeparator();
  menu.addItem(2, TRANS("Save current state..."));
  menu.addItem(3, TRANS("Load a saved state..."));
  menu.addSeparator();
  menu.addItem(4, TRANS("Reset to default state"));

  juce::Component::SafePointer<juce::StandaloneFilterWindow> safeWindow(window);
  menu.showMenuAsync(
      juce::PopupMenu::Options(),
      [safeWindow, safeEditor = juce::Component::SafePointer<AudioEffectFrameworkEditor>(this)](int result)
      {
        if (result == 0) return;

        if (result == 1)
        {
          if (safeEditor != nullptr) safeEditor->showAppSettingsDialog();
          return;
        }

        if (safeWindow != nullptr) safeWindow->handleMenuResult(result);
      });
}
#endif
