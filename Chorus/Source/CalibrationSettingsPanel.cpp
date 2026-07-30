#include "CalibrationSettingsPanel.h"

#include <cmath>

#include "PluginProcessor.h"

namespace
{
constexpr int kRowHeight = 48;
constexpr int kLabelColumnWidth = 160;
constexpr float kIntroFontHeight = 16.0f;
constexpr float kValueLabelReserveDlu = 72.0f;

juce::String formatRms (float rms)
{
    // 4 decimals is enough once RMS is smoothed; fewer digits = less visual jitter.
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

juce::String formatK (float k)
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

CalibrationSettingsPanel::CalibrationSettingsPanel (ChorusAudioProcessor& processorIn, AtomLookAndFeel& lookAndFeel)
    : processor (processorIn),
      atomLookAndFeel (lookAndFeel),
      introLabel ("calibrationIntro", "Input Calibration")
{
    setLookAndFeel (&atomLookAndFeel);

    introLabel.setHintText ("Play a known reference signal, enter its external RMS voltage, then Calibrate. "
                            "Display RMS is UI-smoothed (~350 ms). K = V_ref / RMS; mapped voltage = RMS * K.");
    introLabel.setFont (AtomLookAndFeel::getUIFont (kIntroFontHeight, juce::Font::bold));
    addAndMakeVisible (introLabel);

    const auto valueFont = AtomLookAndFeel::getUIFont (AtomLookAndFeel::getSystemUIFontHeight(), juce::Font::plain);
    for (auto* label : { &rmsValueLabel, &voltageValueLabel, &kValueLabel, &statusLabel })
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

    calibrateButton.onClick = [this] { calibrateFromUi(); };
    resetButton.onClick = [this] { resetCalibration(); };

    rmsRow = std::make_unique<calibration_settings::SettingsCardRow> ("rmsRow", "INPUT RMS", rmsValueLabel, kRowHeight);
    voltageRow = std::make_unique<calibration_settings::SettingsCardRow> ("voltageRow", "MAPPED VOLTAGE",
                                                                         voltageValueLabel, kRowHeight);
    kRow = std::make_unique<calibration_settings::SettingsCardRow> ("kRow", "COEFFICIENT K", kValueLabel, kRowHeight);
    referenceRow = std::make_unique<calibration_settings::SettingsCardRow> ("referenceRow", "REFERENCE V",
                                                                           referenceSlider, kRowHeight);
    calibrateRow = std::make_unique<calibration_settings::SettingsCardRow> ("calibrateRow", "ACTION",
                                                                           calibrateButton, kRowHeight);
    resetRow = std::make_unique<calibration_settings::SettingsCardRow> ("resetRow", "RESET", resetButton, kRowHeight);

    auto calSection = std::make_unique<calibration_settings::SettingsSection> ("RMS → Voltage");
    calSection->addRow (static_cast<calibration_settings::SettingsCardRow&> (*rmsRow));
    calSection->addRow (static_cast<calibration_settings::SettingsCardRow&> (*voltageRow));
    calSection->addRow (static_cast<calibration_settings::SettingsCardRow&> (*kRow));
    calSection->addRow (static_cast<calibration_settings::SettingsCardRow&> (*referenceRow));
    calSection->addRow (static_cast<calibration_settings::SettingsCardRow&> (*calibrateRow));
    calSection->addRow (static_cast<calibration_settings::SettingsCardRow&> (*resetRow));
    section = std::move (calSection);
    addAndMakeVisible (*section);
    addAndMakeVisible (statusLabel);

    refreshReadouts();
    startTimerHz (kTimerHz);
    syncMeteringEnabled();
}

CalibrationSettingsPanel::~CalibrationSettingsPanel()
{
    stopTimer();
    processor.setInputRmsMeteringEnabled (false);

    if (auto* calSection = dynamic_cast<calibration_settings::SettingsSection*> (section.get()))
        calSection->clearRows();

    section.reset();
    rmsRow.reset();
    voltageRow.reset();
    kRow.reset();
    referenceRow.reset();
    calibrateRow.reset();
    resetRow.reset();
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

void CalibrationSettingsPanel::calibrateFromUi()
{
    const float refV = (float) referenceSlider.getValue();
    processor.setCalibrationReferenceVoltage (refV);

    if (processor.calibrateInputFromReference (refV, smoothedRms))
    {
        statusLabel.setText ("Calibrated: K = V_ref / RMS", juce::dontSendNotification);
        statusLabel.setHintText ("Mapped voltage should now match the reference while the signal is steady.");
    }
    else
    {
        statusLabel.setText ("Calibration failed — input RMS too low", juce::dontSendNotification);
        statusLabel.setHintText ("Play a steady reference signal into the selected input channels, then try again.");
    }

    refreshReadouts();
}

void CalibrationSettingsPanel::resetCalibration()
{
    processor.resetCalibration();
    statusLabel.setText ("K reset to 1.0", juce::dontSendNotification);
    statusLabel.setHintText ("Mapped voltage equals digital RMS until you calibrate again.");
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

    const float mappedV = smoothedRms * processor.getCalibrationK();
    const auto rmsText = formatRms (smoothedRms);
    const auto voltageText = formatVoltage (mappedV);
    const auto kText = formatK (processor.getCalibrationK());

    // Avoid redundant Label::setText → repaint storms that can hitch ASIO.
    if (rmsValueLabel.getText() != rmsText)
        rmsValueLabel.setText (rmsText, juce::dontSendNotification);
    if (voltageValueLabel.getText() != voltageText)
        voltageValueLabel.setText (voltageText, juce::dontSendNotification);
    if (kValueLabel.getText() != kText)
        kValueLabel.setText (kText, juce::dontSendNotification);
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
    if (auto* calSection = dynamic_cast<const calibration_settings::SettingsSection*> (section.get()))
        sectionH = calSection->getPreferredHeight();

    constexpr int pad = 16;
    constexpr int introH = 56;
    constexpr int gapAfterIntro = 8;
    constexpr int statusH = 40;
    constexpr int gapBeforeStatus = 8;
    return pad + introH + gapAfterIntro + sectionH + gapBeforeStatus + statusH + pad;
}

void CalibrationSettingsPanel::resized()
{
    auto area = getLocalBounds().reduced (16);
    introLabel.setBounds (area.removeFromTop (56));
    area.removeFromTop (8);

    int sectionH = 0;
    if (auto* calSection = dynamic_cast<calibration_settings::SettingsSection*> (section.get()))
        sectionH = calSection->getPreferredHeight();

    auto statusArea = area.removeFromBottom (40);
    if (area.getHeight() > 8)
        area.removeFromBottom (8);

    if (section != nullptr)
        section->setBounds (area.removeFromTop (sectionH));

    statusLabel.setBounds (statusArea);
}
