#include "AefJuceIncludes.h"
#include "NoiseGateSettingsPanel.h"

#include <cmath>

#include "CompressorKnee.h"
#include "AudioEffectFrameworkProcessor.h"

namespace
{
constexpr int kRowHeight = 48;
constexpr int kLabelColumnWidth = 160;
constexpr float kIntroFontHeight = 16.0f;
constexpr float kThreshAbsMin = -80.0f;
constexpr float kThreshAbsMax = 0.0f;
constexpr float kThreshRangeGap = 0.1f;
/** Shared value-column width so every horizontal slider track is the same length. */
constexpr float kValueLabelReserveDlu = 72.0f;

float linearToDb (float linear) noexcept
{
    return 20.0f * std::log10 (juce::jmax (linear, 1.0e-6f));
}
} // namespace

namespace noise_gate_settings
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

        if (auto* toggle = dynamic_cast<atom::ToggleButton*> (&control))
        {
            toggle->setFontHeight ((float) AtomLookAndFeel::getSystemUIFontHeight());
            control.setBounds (area);
            const int labelH = juce::jmax (1, juce::roundToInt (label.getFont().getHeight()));
            const float tickCentreY = (float) area.getY() + (float) toggle->getFixedHeight() * 0.5f;
            const int labelY = juce::roundToInt (tickCentreY - (float) labelH * 0.5f);
            label.setBounds (labelArea.getX(), labelY, labelArea.getWidth(), labelH);
        }
        else
        {
            control.setBounds (area);
            label.setBounds (labelArea);
        }
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
} // namespace noise_gate_settings

void NoiseGateSettingsPanel::configureHorizontalSlider (atom::Slider& slider, double minV, double maxV,
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

NoiseGateSettingsPanel::NoiseGateSettingsPanel (AudioEffectFrameworkProcessor& processorIn, AtomLookAndFeel& lookAndFeel)
    : processor (processorIn),
      atomLookAndFeel (lookAndFeel),
      introLabel ("noiseGateIntro", "Noise Gate")
{
    setLookAndFeel (&atomLookAndFeel);

    introLabel.setHintText ("Advanced gate controls. Threshold remains on the main header; GATE AT MIN / OFF bypasses the gate at THRESH MIN.");
    introLabel.setFont (AtomLookAndFeel::getUIFont (kIntroFontHeight, juce::Font::bold));
    addAndMakeVisible (introLabel);

    transferCurve.setCurveTitle ("Noise Gate");
    transferCurve.setPlotRange (-80.0f, 0.0f, -80.0f, 0.0f);
    addAndMakeVisible (transferCurve);

    configureHorizontalSlider (threshMinSlider, kThreshAbsMin, kThreshAbsMax, 0.1, " dB");
    configureHorizontalSlider (threshMaxSlider, kThreshAbsMin, kThreshAbsMax, 0.1, " dB");
    configureHorizontalSlider (kneeWidthSlider, 0.1, 24.0, 0.1, " dB");
    configureHorizontalSlider (ratioSlider, 1.0, 10.0, 0.1, "");
    configureHorizontalSlider (attackSlider, 5.0, 100.0, 0.1, " ms");
    configureHorizontalSlider (releaseSlider, 100.0, 10000.0, 1.0, " ms");

    gateAtMinToggle.setFontHeight ((float) AtomLookAndFeel::getSystemUIFontHeight());
    kneeTypeToggle.setFontHeight ((float) AtomLookAndFeel::getSystemUIFontHeight());
    kneeTypeToggle.onClick = [this] { updateKneeWidthEnabled(); };

    threshMinRow = std::make_unique<noise_gate_settings::SettingsCardRow> ("threshMinRow", "THRESH MIN", threshMinSlider, kRowHeight);
    threshMaxRow = std::make_unique<noise_gate_settings::SettingsCardRow> ("threshMaxRow", "THRESH MAX", threshMaxSlider, kRowHeight);
    gateAtMinRow = std::make_unique<noise_gate_settings::SettingsCardRow> ("gateAtMinRow", "GATE AT MIN", gateAtMinToggle, kRowHeight);
    kneeTypeRow = std::make_unique<noise_gate_settings::SettingsCardRow> ("kneeTypeRow", "KNEE TYPE", kneeTypeToggle, kRowHeight);
    kneeWidthRow = std::make_unique<noise_gate_settings::SettingsCardRow> ("kneeWidthRow", "KNEE WIDTH", kneeWidthSlider, kRowHeight);
    ratioRow = std::make_unique<noise_gate_settings::SettingsCardRow> ("ratioRow", "RATIO", ratioSlider, kRowHeight);
    attackRow = std::make_unique<noise_gate_settings::SettingsCardRow> ("attackRow", "ATTACK", attackSlider, kRowHeight);
    releaseRow = std::make_unique<noise_gate_settings::SettingsCardRow> ("releaseRow", "RELEASE", releaseSlider, kRowHeight);

    auto gateSection = std::make_unique<noise_gate_settings::SettingsSection> ("Hidden Settings");
    gateSection->addRow (static_cast<noise_gate_settings::SettingsCardRow&> (*threshMinRow));
    gateSection->addRow (static_cast<noise_gate_settings::SettingsCardRow&> (*threshMaxRow));
    gateSection->addRow (static_cast<noise_gate_settings::SettingsCardRow&> (*gateAtMinRow));
    gateSection->addRow (static_cast<noise_gate_settings::SettingsCardRow&> (*kneeTypeRow));
    gateSection->addRow (static_cast<noise_gate_settings::SettingsCardRow&> (*kneeWidthRow));
    gateSection->addRow (static_cast<noise_gate_settings::SettingsCardRow&> (*ratioRow));
    gateSection->addRow (static_cast<noise_gate_settings::SettingsCardRow&> (*attackRow));
    gateSection->addRow (static_cast<noise_gate_settings::SettingsCardRow&> (*releaseRow));
    section = std::move (gateSection);
    addAndMakeVisible (*section);

    auto& vts = processor.parameters.valueTreeState;
    threshMinAttachment = std::make_unique<SliderAttachment> (vts, processor.paramGateThreshMin.paramID, threshMinSlider);
    threshMaxAttachment = std::make_unique<SliderAttachment> (vts, processor.paramGateThreshMax.paramID, threshMaxSlider);
    gateAtMinAttachment = std::make_unique<ButtonAttachment> (vts, processor.paramGateOffAtMin.paramID, gateAtMinToggle);
    kneeTypeAttachment = std::make_unique<ButtonAttachment> (vts, processor.paramGateKnee.paramID, kneeTypeToggle);
    kneeWidthAttachment = std::make_unique<SliderAttachment> (vts, processor.paramGateKneeWidth.paramID, kneeWidthSlider);
    ratioAttachment = std::make_unique<SliderAttachment> (vts, processor.paramGateRatio.paramID, ratioSlider);
    attackAttachment = std::make_unique<SliderAttachment> (vts, processor.paramGateAttack.paramID, attackSlider);
    releaseAttachment = std::make_unique<SliderAttachment> (vts, processor.paramGateRelease.paramID, releaseSlider);

    updateKneeWidthEnabled();
    enforceThresholdRange();
    updateTransferCurve();
    startTimerHz (60);
}

NoiseGateSettingsPanel::~NoiseGateSettingsPanel()
{
    stopTimer();
    threshMinAttachment.reset();
    threshMaxAttachment.reset();
    gateAtMinAttachment.reset();
    kneeTypeAttachment.reset();
    kneeWidthAttachment.reset();
    ratioAttachment.reset();
    attackAttachment.reset();
    releaseAttachment.reset();

    if (auto* gateSection = dynamic_cast<noise_gate_settings::SettingsSection*> (section.get()))
        gateSection->clearRows();

    section.reset();
    threshMinRow.reset();
    threshMaxRow.reset();
    gateAtMinRow.reset();
    kneeTypeRow.reset();
    kneeWidthRow.reset();
    ratioRow.reset();
    attackRow.reset();
    releaseRow.reset();
    setLookAndFeel (nullptr);
}

float NoiseGateSettingsPanel::readParam (const juce::String& paramId, float fallback) const
{
    if (auto* param = processor.parameters.valueTreeState.getParameter (paramId))
        return param->convertFrom0to1 (param->getValue());

    if (auto* value = processor.parameters.valueTreeState.getRawParameterValue (paramId))
        return value->load();

    return fallback;
}

void NoiseGateSettingsPanel::writeParam (const juce::String& paramId, float value)
{
    if (auto* param = processor.parameters.valueTreeState.getParameter (paramId))
        param->setValueNotifyingHost (param->convertTo0to1 (value));
}

void NoiseGateSettingsPanel::enforceThresholdRange()
{
    float minDb = readParam (processor.paramGateThreshMin.paramID, processor.paramGateThreshMin.defaultValue);
    float maxDb = readParam (processor.paramGateThreshMax.paramID, processor.paramGateThreshMax.defaultValue);

    minDb = juce::jlimit (kThreshAbsMin, kThreshAbsMax, minDb);
    maxDb = juce::jlimit (kThreshAbsMin, kThreshAbsMax, maxDb);

    if (minDb > maxDb - kThreshRangeGap)
    {
        // Prefer keeping the user's last max when range collapses.
        maxDb = juce::jmin (kThreshAbsMax, minDb + kThreshRangeGap);
        if (maxDb - minDb < kThreshRangeGap)
            minDb = juce::jmax (kThreshAbsMin, maxDb - kThreshRangeGap);
    }

    if (std::abs (minDb - readParam (processor.paramGateThreshMin.paramID, minDb)) > 1.0e-3f)
        writeParam (processor.paramGateThreshMin.paramID, minDb);
    if (std::abs (maxDb - readParam (processor.paramGateThreshMax.paramID, maxDb)) > 1.0e-3f)
        writeParam (processor.paramGateThreshMax.paramID, maxDb);

    const float thresholdDb = readParam (processor.paramGateThreshold.paramID, processor.paramGateThreshold.defaultValue);
    const float clamped = juce::jlimit (minDb, maxDb, thresholdDb);
    if (std::abs (clamped - thresholdDb) > 1.0e-3f)
        writeParam (processor.paramGateThreshold.paramID, clamped);
}

void NoiseGateSettingsPanel::updateKneeWidthEnabled()
{
    const bool softKnee = readParam (processor.paramGateKnee.paramID, (float) processor.paramGateKnee.defaultChoice) >= 0.5f;
    kneeWidthSlider.setEnabled (softKnee);
    if (kneeWidthRow != nullptr)
        kneeWidthRow->setAlpha (softKnee ? 1.0f : 0.45f);
}

void NoiseGateSettingsPanel::updateTransferCurve()
{
    const float thresholdDb = readParam (processor.paramGateThreshold.paramID, processor.paramGateThreshold.defaultValue);
    const float ratio = juce::jmax (1.0f, readParam (processor.paramGateRatio.paramID, processor.paramGateRatio.defaultValue));
    const bool softKnee = readParam (processor.paramGateKnee.paramID, (float) processor.paramGateKnee.defaultChoice) >= 0.5f;
    const float kneeWidthDb = readParam (processor.paramGateKneeWidth.paramID, processor.paramGateKneeWidth.defaultValue);

    transferCurve.setParameters (thresholdDb, ratio, 0.0f, true, softKnee, kneeWidthDb);
}

void NoiseGateSettingsPanel::timerCallback()
{
    enforceThresholdRange();
    updateKneeWidthEnabled();
    updateTransferCurve();

    const float thresholdDb = readParam (processor.paramGateThreshold.paramID, processor.paramGateThreshold.defaultValue);
    const float ratio = juce::jmax (1.0f, readParam (processor.paramGateRatio.paramID, processor.paramGateRatio.defaultValue));
    const bool softKnee = readParam (processor.paramGateKnee.paramID, (float) processor.paramGateKnee.defaultChoice) >= 0.5f;
    const float kneeWidthDb = readParam (processor.paramGateKneeWidth.paramID, processor.paramGateKneeWidth.defaultValue);
    const float attackSec = readParam (processor.paramGateAttack.paramID, processor.paramGateAttack.defaultValue) * 0.001f;
    const float releaseSec = readParam (processor.paramGateRelease.paramID, processor.paramGateRelease.defaultValue) * 0.001f;

    const float inputDb = juce::jlimit (-80.0f, 0.0f, linearToDb (processor.getMeterLevelMono()));
    const float outputDb = CompressorKnee::computeOutputDb (inputDb, thresholdDb, ratio, 0.0f, true, softKnee, kneeWidthDb);
    const float gainReductionDb = juce::jmax (0.0f, inputDb - outputDb);

    transferCurve.setDynamicMeter (inputDb, gainReductionDb, attackSec, releaseSec);
}

void NoiseGateSettingsPanel::paint (juce::Graphics& g)
{
    g.fillAll (findColour (juce::ResizableWindow::backgroundColourId));
}

int NoiseGateSettingsPanel::getPreferredPanelHeight() const noexcept
{
    int sectionH = 0;
    if (auto* gateSection = dynamic_cast<const noise_gate_settings::SettingsSection*> (section.get()))
        sectionH = gateSection->getPreferredHeight();

    constexpr int pad = 16;
    constexpr int introH = 48;
    constexpr int gapAfterIntro = 8;
    constexpr int gapAfterCurve = 12;
    return pad + introH + gapAfterIntro + kCurveMinHeight + gapAfterCurve + sectionH + pad;
}

void NoiseGateSettingsPanel::resized()
{
    auto area = getLocalBounds().reduced (16);
    introLabel.setBounds (area.removeFromTop (48));
    area.removeFromTop (8);

    int sectionH = 0;
    if (auto* gateSection = dynamic_cast<noise_gate_settings::SettingsSection*> (section.get()))
        sectionH = gateSection->getPreferredHeight();

    const int curveH = juce::jmax (kCurveMinHeight, area.getHeight() - sectionH - 12);
    transferCurve.setBounds (area.removeFromTop (curveH));
    area.removeFromTop (12);

    if (auto* gateSection = dynamic_cast<noise_gate_settings::SettingsSection*> (section.get()))
        gateSection->setBounds (area.removeFromTop (sectionH));
}
