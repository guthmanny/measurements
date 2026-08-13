#include "AefJuceIncludes.h"
#include "CalibrationSettingsPanel.h"

#include <cmath>

#include "AudioEffectFrameworkProcessor.h"

namespace
{
constexpr int kRowHeight = 48;
constexpr int kLabelColumnWidth = 160;
constexpr float kIntroFontHeight = 16.0f;
constexpr float kValueLabelReserveDlu = 72.0f;

juce::String formatRms (float rms)
{
    return juce::String (rms, 4);
}

juce::String formatVoltage (float volts)
{
    if (volts >= 100.0f)
        return juce::String (volts, 1) + " V";
    if (volts >= 1.0f)
        return juce::String (volts, 3) + " V";
    if (volts >= 0.001f)
        return juce::String (volts * 1000.0f, 2) + " mV";
    return juce::String (volts * 1.0e6f, 1) + " uV";
}

juce::String formatCoeff (float k)
{
    if (k >= 1000.0f)
        return juce::String (k, 1);
    if (k >= 10.0f)
        return juce::String (k, 2);
    return juce::String (k, 4);
}
} // namespace

namespace calibration_settings
{
class SettingsCardRow final : public juce::Component
{
public:
    SettingsCardRow (const juce::String& rowName, const juce::String& title, juce::Component& controlToEmbed, int height)
        : label (rowName + "Label", title), control (controlToEmbed), rowHeight (height)
    {
        card.setMinPanelHeight (rowHeight);
        label.setJustificationType (juce::Justification::centredLeft);
        label.setFont (AtomLookAndFeel::getUIFont (AtomLookAndFeel::getSystemUIFontHeight(), juce::Font::plain));
        label.setMinimumHorizontalScale (1.0f);
        label.setAutoResizeEnabled (false);
        card.addAndMakeVisible (label);
        card.addAndMakeVisible (control);
        addAndMakeVisible (card);
        setSize (0, rowHeight);
    }

    int getRowHeight() const noexcept { return rowHeight; }

    void resized() override
    {
        card.setBounds (getLocalBounds());
        auto area = card.getLocalBounds().reduced (12, 8);
        auto labelArea = area.removeFromLeft (kLabelColumnWidth);
        area.removeFromLeft (10);
        control.setBounds (area);
        label.setBounds (labelArea);
    }

private:
    atom::SettingsCard card;
    atom::Label label;
    juce::Component& control;
    int rowHeight;
};

class SettingsSection final : public juce::Component
{
public:
    explicit SettingsSection (const juce::String& title)
    {
        groupedList.setTitle (title);
        groupedList.setHeaderFont (AtomLookAndFeel::getSystemUIFont (juce::Font::bold));
        addAndMakeVisible (groupedList);
    }

    void addRow (SettingsCardRow& row)
    {
        rows.push_back (&row);
        groupedList.addItem (&row);
    }

    void clearRows()
    {
        groupedList.clearItems();
        rows.clear();
    }

    int getPreferredHeight() const
    {
        const int headerH = juce::roundToInt (AtomLookAndFeel::getSystemUIFontHeight()) + 10;
        int total = headerH + padding * 2;
        for (size_t i = 0; i < rows.size(); ++i)
        {
            total += rows[i]->getRowHeight();
            if (i + 1 < rows.size())
                total += itemGap;
        }
        return total;
    }

    void resized() override { groupedList.setBounds (getLocalBounds()); }

private:
    static constexpr int padding = 12;
    static constexpr int itemGap = 8;

    atom::GroupedList groupedList;
    std::vector<SettingsCardRow*> rows;
};
} // namespace calibration_settings

void CalibrationSettingsPanel::configureHorizontalSlider (atom::Slider& slider, double minV, double maxV,
                                                          double interval, const juce::String& suffix)
{
    slider.setRange (minV, maxV, interval);
    slider.setTextValueSuffix (suffix);
    slider.setSliderSnapsToMousePosition (false);
    slider.setValueLabelPos (atom::Slider::ValueLabelPos::Right);

    atom::SliderStyleOverride styleOverride;
    styleOverride.metrics.linearHorizontalValueLabelReserveDlu = kValueLabelReserveDlu;
    atomLookAndFeel.setSliderStyleOverride (slider, styleOverride);
}

CalibrationSettingsPanel::CalibrationSettingsPanel (AudioEffectFrameworkProcessor& processorIn, AtomLookAndFeel& lookAndFeel)
    : processor (processorIn),
      atomLookAndFeel (lookAndFeel),
      introLabel ("calibrationIntro", "Calibration")
{
    setLookAndFeel (&atomLookAndFeel);

    introLabel.setHintText (
        "Input (before FX): enter Reference V, Calibrate Ki (Ki = V_ref / RMS; multiplied into effect input). "
        "Output (after FX): enter Measured Out V, Calibrate Ko (Ko = V_ref / V_out; multiplied after FX).");
    introLabel.setFont (AtomLookAndFeel::getUIFont (kIntroFontHeight, juce::Font::bold));
    addAndMakeVisible (introLabel);

    const auto valueFont = AtomLookAndFeel::getUIFont (AtomLookAndFeel::getSystemUIFontHeight(), juce::Font::plain);
    for (auto* label : { &rmsValueLabel, &voltageValueLabel, &kiValueLabel, &koValueLabel, &statusLabel })
    {
        label->setFont (valueFont);
        label->setJustificationType (juce::Justification::centredLeft);
        label->setMinimumHorizontalScale (1.0f);
        label->setAutoResizeEnabled (false);
    }
    statusLabel.setHintText (" ");

    configureHorizontalSlider (referenceSlider, 0.001, 100.0, 0.001, " V");
    referenceSlider.setSkewFactorFromMidPoint (1.0);
    referenceSlider.setValue (processor.getCalibrationReferenceVoltage(), juce::dontSendNotification);
    referenceSlider.onValueChange = [this]
    {
        processor.setCalibrationReferenceVoltage ((float) referenceSlider.getValue());
    };

    configureHorizontalSlider (measuredOutputSlider, 0.001, 100.0, 0.001, " V");
    measuredOutputSlider.setSkewFactorFromMidPoint (1.0);
    measuredOutputSlider.setValue (processor.getCalibrationMeasuredOutputVoltage(), juce::dontSendNotification);
    measuredOutputSlider.onValueChange = [this]
    {
        processor.setCalibrationMeasuredOutputVoltage ((float) measuredOutputSlider.getValue());
    };

    calibrateInputButton.onClick = [this] { calibrateInputFromUi(); };
    resetInputButton.onClick = [this] { resetInputCalibration(); };
    calibrateOutputButton.onClick = [this] { calibrateOutputFromUi(); };
    resetOutputButton.onClick = [this] { resetOutputCalibration(); };

    rmsRow = std::make_unique<calibration_settings::SettingsCardRow> ("rmsRow", "INPUT RMS", rmsValueLabel, kRowHeight);
    voltageRow = std::make_unique<calibration_settings::SettingsCardRow> ("voltageRow", "MAPPED VOLTAGE",
                                                                         voltageValueLabel, kRowHeight);
    kiRow = std::make_unique<calibration_settings::SettingsCardRow> ("kiRow", "COEFFICIENT Ki", kiValueLabel, kRowHeight);
    referenceRow = std::make_unique<calibration_settings::SettingsCardRow> ("referenceRow", "REFERENCE V",
                                                                           referenceSlider, kRowHeight);
    calibrateInputRow = std::make_unique<calibration_settings::SettingsCardRow> ("calibrateInputRow", "ACTION",
                                                                                 calibrateInputButton, kRowHeight);
    resetInputRow = std::make_unique<calibration_settings::SettingsCardRow> ("resetInputRow", "RESET",
                                                                             resetInputButton, kRowHeight);

    measuredOutputRow = std::make_unique<calibration_settings::SettingsCardRow> (
        "measuredOutputRow", "MEASURED OUT V", measuredOutputSlider, kRowHeight);
    koRow = std::make_unique<calibration_settings::SettingsCardRow> ("koRow", "COEFFICIENT Ko", koValueLabel, kRowHeight);
    calibrateOutputRow = std::make_unique<calibration_settings::SettingsCardRow> (
        "calibrateOutputRow", "ACTION", calibrateOutputButton, kRowHeight);
    resetOutputRow = std::make_unique<calibration_settings::SettingsCardRow> ("resetOutputRow", "RESET",
                                                                               resetOutputButton, kRowHeight);

    auto inSection = std::make_unique<calibration_settings::SettingsSection> ("Input Calibration");
    inSection->addRow (static_cast<calibration_settings::SettingsCardRow&> (*rmsRow));
    inSection->addRow (static_cast<calibration_settings::SettingsCardRow&> (*voltageRow));
    inSection->addRow (static_cast<calibration_settings::SettingsCardRow&> (*kiRow));
    inSection->addRow (static_cast<calibration_settings::SettingsCardRow&> (*referenceRow));
    inSection->addRow (static_cast<calibration_settings::SettingsCardRow&> (*calibrateInputRow));
    inSection->addRow (static_cast<calibration_settings::SettingsCardRow&> (*resetInputRow));
    inputSection = std::move (inSection);

    auto outSection = std::make_unique<calibration_settings::SettingsSection> ("Output Calibration");
    outSection->addRow (static_cast<calibration_settings::SettingsCardRow&> (*measuredOutputRow));
    outSection->addRow (static_cast<calibration_settings::SettingsCardRow&> (*koRow));
    outSection->addRow (static_cast<calibration_settings::SettingsCardRow&> (*calibrateOutputRow));
    outSection->addRow (static_cast<calibration_settings::SettingsCardRow&> (*resetOutputRow));
    outputSection = std::move (outSection);

    addAndMakeVisible (*inputSection);
    addAndMakeVisible (*outputSection);
    addAndMakeVisible (statusLabel);

    refreshReadouts();
    startTimerHz (kTimerHz);
    syncMeteringEnabled();
}

CalibrationSettingsPanel::~CalibrationSettingsPanel()
{
    stopTimer();
    processor.setInputRmsMeteringEnabled (false);

    if (auto* section = dynamic_cast<calibration_settings::SettingsSection*> (inputSection.get()))
        section->clearRows();
    if (auto* section = dynamic_cast<calibration_settings::SettingsSection*> (outputSection.get()))
        section->clearRows();

    inputSection.reset();
    outputSection.reset();
    rmsRow.reset();
    voltageRow.reset();
    kiRow.reset();
    referenceRow.reset();
    calibrateInputRow.reset();
    resetInputRow.reset();
    measuredOutputRow.reset();
    koRow.reset();
    calibrateOutputRow.reset();
    resetOutputRow.reset();
    setLookAndFeel (nullptr);
}

void CalibrationSettingsPanel::syncMeteringEnabled()
{
    const bool enabled = isShowing();
    processor.setInputRmsMeteringEnabled (enabled);
    if (! enabled)
    {
        smoothedRms = 0.0f;
        rmsSmoothPrimed = false;
    }
}

void CalibrationSettingsPanel::visibilityChanged()
{
    syncMeteringEnabled();
}

void CalibrationSettingsPanel::calibrateInputFromUi()
{
    const float refV = (float) referenceSlider.getValue();
    processor.setCalibrationReferenceVoltage (refV);

    if (processor.calibrateInputFromReference (refV, smoothedRms))
    {
        const auto kiText = formatCoeff (processor.getCalibrationKi());
        statusLabel.setText ("Calibrated: Ki = " + kiText + " (pre-FX input)", juce::dontSendNotification);
        statusLabel.setHintText ("Ki scales the signal before the effect chain. Mapped V = RMS * Ki.");
    }
    else
    {
        statusLabel.setText ("Input calibration failed - input RMS too low", juce::dontSendNotification);
        statusLabel.setHintText ("Play a steady reference signal into the selected input channels, then try again.");
    }

    refreshReadouts();
}

void CalibrationSettingsPanel::calibrateOutputFromUi()
{
    const float refV = (float) referenceSlider.getValue();
    const float measuredV = (float) measuredOutputSlider.getValue();
    processor.setCalibrationReferenceVoltage (refV);
    processor.setCalibrationMeasuredOutputVoltage (measuredV);

    if (processor.calibrateOutputFromMeasured (measuredV))
    {
        const auto koText = formatCoeff (processor.getCalibrationKo());
        statusLabel.setText ("Calibrated: Ko = " + koText + " (post-FX output)", juce::dontSendNotification);
        statusLabel.setHintText ("Ko scales the signal after the effect chain, not the effect input.");
    }
    else
    {
        statusLabel.setText ("Output calibration failed - check Reference V and Measured Out V", juce::dontSendNotification);
        statusLabel.setHintText ("Both voltages must be greater than zero.");
    }

    refreshReadouts();
}

void CalibrationSettingsPanel::resetInputCalibration()
{
    processor.resetInputCalibration();
    statusLabel.setText ("Ki reset to 1.0", juce::dontSendNotification);
    statusLabel.setHintText ("Mapped voltage equals digital RMS until you calibrate again.");
    refreshReadouts();
}

void CalibrationSettingsPanel::resetOutputCalibration()
{
    processor.resetOutputCalibration();
    statusLabel.setText ("Ko reset to 1.0", juce::dontSendNotification);
    statusLabel.setHintText ("Output compensation is unity until you calibrate again.");
    refreshReadouts();
}

void CalibrationSettingsPanel::refreshReadouts()
{
    const float rawRms = processor.getInputRms();
    if (! rmsSmoothPrimed)
    {
        smoothedRms = rawRms;
        rmsSmoothPrimed = true;
    }
    else
    {
        const float alpha = 1.0f - std::exp (-1.0f / ((float) kTimerHz * kRmsSmoothSec));
        smoothedRms += alpha * (rawRms - smoothedRms);
    }

    const float mappedV = smoothedRms * processor.getCalibrationKi();
    const auto rmsText = formatRms (smoothedRms);
    const auto voltageText = formatVoltage (mappedV);
    const auto kiText = formatCoeff (processor.getCalibrationKi());
    const auto koText = formatCoeff (processor.getCalibrationKo());

    if (rmsValueLabel.getText() != rmsText)
        rmsValueLabel.setText (rmsText, juce::dontSendNotification);
    if (voltageValueLabel.getText() != voltageText)
        voltageValueLabel.setText (voltageText, juce::dontSendNotification);
    if (kiValueLabel.getText() != kiText)
        kiValueLabel.setText (kiText, juce::dontSendNotification);
    if (koValueLabel.getText() != koText)
        koValueLabel.setText (koText, juce::dontSendNotification);
}

void CalibrationSettingsPanel::timerCallback()
{
    if (! isShowing())
        return;

    refreshReadouts();
}

void CalibrationSettingsPanel::paint (juce::Graphics& g)
{
    g.fillAll (findColour (juce::ResizableWindow::backgroundColourId));
}

int CalibrationSettingsPanel::getPreferredPanelHeight() const noexcept
{
    int sectionH = 0;
    if (auto* section = dynamic_cast<const calibration_settings::SettingsSection*> (inputSection.get()))
        sectionH += section->getPreferredHeight();
    if (auto* section = dynamic_cast<const calibration_settings::SettingsSection*> (outputSection.get()))
        sectionH += section->getPreferredHeight();

    constexpr int pad = 16;
    constexpr int introH = 72;
    constexpr int gapAfterIntro = 8;
    constexpr int gapBetweenSections = 12;
    constexpr int statusH = 40;
    constexpr int gapBeforeStatus = 8;
    return pad + introH + gapAfterIntro + sectionH + gapBetweenSections + gapBeforeStatus + statusH + pad;
}

void CalibrationSettingsPanel::resized()
{
    auto area = getLocalBounds().reduced (16);
    introLabel.setBounds (area.removeFromTop (72));
    area.removeFromTop (8);

    int inputH = 0;
    int outputH = 0;
    if (auto* section = dynamic_cast<calibration_settings::SettingsSection*> (inputSection.get()))
        inputH = section->getPreferredHeight();
    if (auto* section = dynamic_cast<calibration_settings::SettingsSection*> (outputSection.get()))
        outputH = section->getPreferredHeight();

    auto statusArea = area.removeFromBottom (40);
    if (area.getHeight() > 8)
        area.removeFromBottom (8);

    if (inputSection != nullptr)
        inputSection->setBounds (area.removeFromTop (inputH));

    if (area.getHeight() > 12)
        area.removeFromTop (12);

    if (outputSection != nullptr)
        outputSection->setBounds (area.removeFromTop (outputH));

    statusLabel.setBounds (statusArea);
}
