#pragma once

#include <juce_atom_theme/juce_atom_theme.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>

class ChorusAudioProcessor;

/**
 * Settings → Input Calibration:
 * User enters a known external voltage while a reference signal is playing.
 * K = V_ref / RMS_digital; mapped voltage = digital * K.
 */
class CalibrationSettingsPanel final : public juce::Component, private juce::Timer
{
public:
    CalibrationSettingsPanel (ChorusAudioProcessor& processor, AtomLookAndFeel& lookAndFeel);
    ~CalibrationSettingsPanel() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void visibilityChanged() override;

    int getPreferredPanelHeight() const noexcept;

private:
    void timerCallback() override;
    void refreshReadouts();
    void calibrateFromUi();
    void resetCalibration();
    void syncMeteringEnabled();
    void configureHorizontalSlider (atom::Slider& slider, double minV, double maxV, double interval,
                                    const juce::String& suffix);

    ChorusAudioProcessor& processor;
    AtomLookAndFeel& atomLookAndFeel;

    atom::Label introLabel;
    atom::Label rmsValueLabel { "rmsValueLabel", "0.0000" };
    atom::Label voltageValueLabel { "voltageValueLabel", "0.000 V" };
    atom::Label kValueLabel { "kValueLabel", "1.000" };
    atom::Slider referenceSlider { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    atom::TextButton calibrateButton { "calibrateButton", "Calibrate" };
    atom::TextButton resetButton { "resetButton", "Reset K" };
    atom::Label statusLabel { "statusLabel", {} };

    std::unique_ptr<juce::Component> rmsRow;
    std::unique_ptr<juce::Component> voltageRow;
    std::unique_ptr<juce::Component> kRow;
    std::unique_ptr<juce::Component> referenceRow;
    std::unique_ptr<juce::Component> calibrateRow;
    std::unique_ptr<juce::Component> resetRow;
    std::unique_ptr<juce::Component> section;

    float smoothedRms = 0.0f;
    bool rmsSmoothPrimed = false;

    static constexpr int kTimerHz = 10;
    static constexpr float kRmsSmoothSec = 0.35f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CalibrationSettingsPanel)
};
