#include "EffectFooterComponent.h"

EffectFooterComponent::EffectFooterComponent()
{
  btnMidiPort.setTooltip("MIDI Port");

  qualityLabel.setJustificationType(juce::Justification::centredLeft);
  qualityLabel.setMinimumHorizontalScale(1.0f);
  qualityLabel.setBorderSize({});
  qualityLabel.setAutoResizeEnabled(false);

  qualityComboBox.addItem("STANDARD", 1);
  qualityComboBox.addItem("HIGH", 2);
  qualityComboBox.addItem("ULTRA", 3);
  qualityComboBox.setSelectedId(1, juce::dontSendNotification);
  qualityComboBox.setJustificationType(juce::Justification::centredLeft);

  viewLabel.setJustificationType(juce::Justification::centredLeft);
  viewLabel.setMinimumHorizontalScale(1.0f);
  viewLabel.setBorderSize({});
  viewLabel.setAutoResizeEnabled(false);

  viewComboBox.addItem("75%", 1);
  viewComboBox.addItem("100%", 2);
  viewComboBox.addItem("125%", 3);
  viewComboBox.setSelectedId(2, juce::dontSendNotification);
  viewComboBox.setJustificationType(juce::Justification::centredLeft);

  viewComboBox.onChange = [this]
  {
    if (!onZoomChanged) return;

    switch (viewComboBox.getSelectedId())
    {
      case 1:
        onZoomChanged(0.75f);
        break;
      case 2:
        onZoomChanged(1.00f);
        break;
      case 3:
        onZoomChanged(1.25f);
        break;
      default:
        break;
    }
  };

  applyFonts();

  // ---- Register child controls with the generic FooterBar layout skeleton ----
  const int h = 32;  // reference height used to derive control sizes
  const int btnSize = juce::jmin(h, 64);
  const auto font = AtomLookAndFeel::getUIFont(AtomLookAndFeel::getSystemUIFontHeight(), juce::Font::plain);
  const int qualityTextW = juce::roundToInt(font.getStringWidthFloat("QUALITY:")) + 6;
  const int viewTextW = juce::roundToInt(font.getStringWidthFloat("VIEW:")) + 6;
  const int comboW = 100;

  qualityComboBox.setSize(comboW, h);
  viewComboBox.setSize(comboW, h);

  using atom::core::FlexRowItem;

  addItem(&btnMidiPort, {btnSize, btnSize, 0, 8});

  FlexRowItem spacer;
  spacer.flexGrow = 1.0f;
  addItem(nullptr, spacer);

  addItem(&qualityLabel, {qualityTextW, h, 0, 6});
  addItem(&qualityComboBox, {comboW, h, 0, 10});
  addItem(&viewLabel, {viewTextW, h, 0, 6});
  addItem(&viewComboBox, {comboW, h, 0, 0});

  setContentPadding(16);
}

void EffectFooterComponent::applyFonts()
{
  const auto font = AtomLookAndFeel::getUIFont(AtomLookAndFeel::getSystemUIFontHeight(), juce::Font::plain);
  qualityLabel.setFont(font);
  viewLabel.setFont(font);
}
