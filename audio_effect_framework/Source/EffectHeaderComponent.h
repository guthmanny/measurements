#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_atom_theme/juce_atom_theme.h>

/**
    The audio-effect-specific Header.

    Inherits the generic atom::HeaderBar layout skeleton and registers this
    framework's own child controls (meters, INPUT/GATE/OUTPUT rotary sliders,
    Settings/Tuner/Spectrum buttons, tap tempo) via addItem().  It also owns the
    audio-domain meter logic (peak display levels / dB range) that is not part
    of the generic AtomTheme skeleton.
*/
class EffectHeaderComponent final : public atom::HeaderBar
{
public:
    EffectHeaderComponent();
    ~EffectHeaderComponent() override;

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
