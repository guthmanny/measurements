#pragma once

#include <juce_atom_theme/juce_atom_theme.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>

#include "BjtCurveMath.h"
#include "BjtCurvePanel.h"

class MainComponent final : public juce::Component
{
 public:
  MainComponent();
  ~MainComponent() override;

  void paint(juce::Graphics& g) override;
  void resized() override;

 private:
  void applyTheme();
  void updateCurveViews();
  void configureCombo(atom::ComboBox& combo);
  bjt_curve::CurveKind getCurveKindFromSelection() const;
  bjt_curve::CircuitKind getCircuitFromSelection() const;
  bjt_curve::BjtModelKind getModelFromSelection() const;
  bjt_curve::IbSweepParams getIbSweepFromControls() const;
  void configureMicroAmpSlider(atom::Slider& slider, double value, const juce::String& suffix);
  void configureMilliAmpSlider(atom::Slider& slider, double value);
  void configureVoltSlider(atom::Slider& slider, double value);
  void configureVbeSlider(atom::Slider& slider, double value);
  void syncTransferCurrentSlider(bjt_curve::CurveKind kind);

  AtomLookAndFeel atomLookAndFeel;

  atom::Label titleLabel;
  atom::Label subtitleLabel;
  atom::TextButton themeButton{"themeButton", "Switch theme"};
  atom::ComboBox curveKindBox{"curveKindBox"};
  atom::ComboBox circuitBox{"circuitBox"};
  atom::ComboBox modelBox{"modelBox"};

  atom::Label ibCountLabel;
  atom::Slider ibCountSlider{juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight};
  atom::Label ibMinLabel;
  atom::Slider ibMinSlider{juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight};
  atom::Label ibStepLabel;
  atom::Slider ibStepSlider{juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight};
  atom::Label vceMaxLabel;
  atom::Slider vceMaxSlider{juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight};

  atom::Label vbeMaxLabel;
  atom::Slider vbeMaxSlider{juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight};
  atom::Label iMaxLabel;
  atom::Slider iMaxSlider{juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight};

  std::unique_ptr<BjtCurvePanel> curvePanel;
  bjt_curve::CurveKind lastTransferKind_{bjt_curve::CurveKind::IcVsVbe};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
