#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_atom_theme/juce_atom_theme.h>

class ChorusHeaderComponent final : public juce::Component
{
public:
    ChorusHeaderComponent();
    ~ChorusHeaderComponent() override;

    atom::Slider& getSliderInput() { return sliderInput; }
    atom::Slider& getSliderGate() { return sliderGate; }
    atom::Slider& getSliderOutput() { return sliderOutput; }
    atom::ShapeButton& getBtnSettings() { return btnSettings; }
    atom::ShapeButton& getBtnTuner() { return btnTuner; }
    atom::ShapeButton& getBtnSpectrum() { return btnSpectrum; }
    atom::TapCircle& getTapTempo() { return tapTempo; }
    atom::MeterBar& getMeterLeft() { return meterLeft; }
    atom::MeterBar& getMeterRight() { return meterRight; }

    void setMeterDisplayLevels (float monoNorm, float leftNorm, float rightNorm);
    void applyMeterSettings (int displayRangeDbSpan);

    void lookAndFeelChanged() override;
    void paint (juce::Graphics& g) override;
    void resized() override;

    int getMinimumContentWidth (int heightHint = 0);

private:
    atom::MeterBar meterLeft;
    atom::MeterBar meterRight;
    atom::ShapeButton btnSettings { "btnSettings", AtomIconLibrary::Icon::CogWheel };
    atom::ShapeButton btnTuner { "btnTuner", AtomIconLibrary::Icon::PitchFork };
    atom::ShapeButton btnSpectrum { "btnSpectrum", AtomIconLibrary::Icon::Spectrum };
    atom::TapCircle tapTempo;
    atom::Slider sliderInput { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
    atom::Slider sliderGate { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
    atom::Slider sliderOutput { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
};
