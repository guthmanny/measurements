#pragma once

#include <juce_atom_theme/juce_atom_theme.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>

class AudioEffectFrameworkProcessor;

/**
 * Settings -> Calibration:
 * Input (pre-FX): Ki = V_ref / RMS_digital; audio *= Ki before effect.
 * Output (post-FX): Ko = V_ref / V_out_measured; audio *= Ko after effect.
 */
class CalibrationSettingsPanel final : public juce::Component, private juce::Timer
{
public:
    CalibrationSettingsPanel (AudioEffectFrameworkProcessor& processor, AtomLookAndFeel& lookAndFeel);
    ~CalibrationSettingsPanel() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void visibilityChanged() override;

    int getPreferredPanelHeight() const noexcept;

private:
    void timerCallback() override;
    void refreshReadouts();
    void calibrateInputFromUi();
    void calibrateOutputFromUi();
    void resetInputCalibration();
    void resetOutputCalibration();
    void saveCalibrationFromUi();
    void loadCalibrationFromUi();
    void syncUiFromProcessor();
    void syncMeteringEnabled();
    void configureHorizontalSlider (atom::Slider& slider, double minV, double maxV, double interval,
                                    const juce::String& suffix);

    AudioEffectFrameworkProcessor& processor;
    AtomLookAndFeel& atomLookAndFeel;

    atom::Label introLabel;
    atom::Label rmsValueLabel { "rmsValueLabel", "0.0000" };
    atom::Label voltageValueLabel { "voltageValueLabel", "0.000 V" };
    atom::Label kiValueLabel { "kiValueLabel", "1.000" };
    atom::Label koValueLabel { "koValueLabel", "1.000" };
    atom::Slider referenceSlider { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    atom::Slider measuredOutputSlider { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    atom::TextButton calibrateInputButton { "calibrateInputButton", "Calibrate Ki" };
    atom::TextButton resetInputButton { "resetInputButton", "Reset Ki" };
    atom::TextButton calibrateOutputButton { "calibrateOutputButton", "Calibrate Ko" };
    atom::TextButton resetOutputButton { "resetOutputButton", "Reset Ko" };
    atom::TextButton saveCalibrationButton { "saveCalibrationButton", "Save" };
    atom::TextButton loadCalibrationButton { "loadCalibrationButton", "Load" };
    atom::Label statusLabel { "statusLabel", {} };

    std::unique_ptr<juce::Component> rmsRow;
    std::unique_ptr<juce::Component> voltageRow;
    std::unique_ptr<juce::Component> kiRow;
    std::unique_ptr<juce::Component> referenceRow;
    std::unique_ptr<juce::Component> calibrateInputRow;
    std::unique_ptr<juce::Component> resetInputRow;
    std::unique_ptr<juce::Component> measuredOutputRow;
    std::unique_ptr<juce::Component> koRow;
    std::unique_ptr<juce::Component> calibrateOutputRow;
    std::unique_ptr<juce::Component> resetOutputRow;
    std::unique_ptr<juce::Component> saveCalibrationRow;
    std::unique_ptr<juce::Component> loadCalibrationRow;
    std::unique_ptr<juce::Component> inputSection;
    std::unique_ptr<juce::Component> outputSection;
    std::unique_ptr<juce::Component> fileSection;

    float smoothedRms = 0.0f;
    bool rmsSmoothPrimed = false;

    static constexpr int kTimerHz = 10;
    static constexpr float kRmsSmoothSec = 0.35f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CalibrationSettingsPanel)
};
