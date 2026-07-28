#pragma once

#include <memory>

#include <juce_atom_theme/juce_atom_theme.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "Ds1OpampAcMath.h"
#include "Ds1OpampAcPanel.h"

class MainComponent final : public juce::Component
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void applyTheme();
    void updatePlotView();
    void configureCombo(atom::ComboBox& combo);
    void configureKnob(atom::Slider& knob);
    void layoutKnobColumn(juce::Rectangle<int>& area, atom::Label& label, atom::Slider& knob) const;
    ds1_ac::CircuitKind getCircuitFromSelection() const;
    nx_pot_taper_e getPotTaperFromSelection() const;
    void syncPotTaperToCircuitDefault(ds1_ac::CircuitKind circuit);
    void syncDeviceModelCombo(ds1_ac::CircuitKind circuit);
    nx_opamp_model_e getOpampModelFromSelection() const;
    nx_bjt_npn_model_e getBjtModelFromSelection() const;
    nx_jfet_n_model_e getJfetModelFromSelection() const;
    ds1_ac::PlotKind getPlotKindFromSelection() const;
    double getSampleRateFromSelection() const;

    AtomLookAndFeel atomLookAndFeel;

    atom::Label titleLabel;
    atom::Label subtitleLabel;
    atom::TextButton themeButton{"themeButton", "Switch theme"};
    atom::ComboBox circuitBox{"circuitBox"};
    atom::ComboBox plotKindBox{"plotKindBox"};
    atom::ComboBox opampModelBox{"opampModelBox"};
    atom::ComboBox sampleRateBox{"sampleRateBox"};
    atom::ComboBox taperBox{"taperBox"};

    atom::Label sampleRateLabel;
    atom::Label taperLabel;
    atom::Label circuitLabel;

    atom::Label gainLabel;
    atom::Slider gainKnob{juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox};

    atom::Label secondaryLabel;
    atom::Slider secondaryKnob{juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox};

    atom::Label tertiaryLabel;
    atom::Slider tertiaryKnob{juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox};

    atom::Label sineFreqLabel;
    atom::Slider sineFreqKnob{juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox};

    std::unique_ptr<Ds1OpampAcPanel> acPanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
