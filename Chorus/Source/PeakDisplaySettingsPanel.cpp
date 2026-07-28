#include "PeakDisplaySettingsPanel.h"

#include <cmath>

#include "MeterDisplayUtils.h"
#include "PluginProcessor.h"

namespace
{
constexpr int kRowHeight = 48;
constexpr int kLabelColumnWidth = 160;
constexpr float kIntroFontHeight = 16.0f;
constexpr float kValueLabelReserveDlu = 72.0f;

using meter_display::displayRangeDbFromChoiceIndex;
using meter_display::kDisplayRangeDbChoices;
using meter_display::linearToDb;
using meter_display::linearToDbNormalized;

juce::String formatDbTick (float db)
{
    return juce::String (juce::roundToInt (db)) + " dB";
}

void fillDisplayRangeCombo (atom::ComboBox& combo)
{
    combo.clear (juce::dontSendNotification);
    for (size_t i = 0; i < kDisplayRangeDbChoices.size(); ++i)
        combo.addItem (juce::String (kDisplayRangeDbChoices[i]) + " dB", (int) i + 1);
}

std::vector<float> buildNiceAxisTicks (float minVal, float maxVal, int targetTickCount = 5)
{
    std::vector<float> ticks;
    const float span = maxVal - minVal;
    if (span <= 0.0f)
        return { minVal };

    const float roughStep = span / (float) juce::jmax (1, targetTickCount - 1);
    const float magnitude = std::pow (10.0f, std::floor (std::log10 (roughStep)));
    const float normStep = roughStep / magnitude;
    float niceNorm = 10.0f;
    if (normStep <= 1.0f)
        niceNorm = 1.0f;
    else if (normStep <= 2.0f)
        niceNorm = 2.0f;
    else if (normStep <= 5.0f)
        niceNorm = 5.0f;

    const float step = niceNorm * magnitude;
    ticks.push_back (minVal);

    float v = std::ceil ((minVal + step * 0.001f) / step) * step;
    for (; v < maxVal - step * 0.01f; v += step)
        ticks.push_back (v);

    if (ticks.back() < maxVal - step * 0.001f)
        ticks.push_back (maxVal);

    return ticks;
}

juce::String formatTimeTick (float timeSec)
{
    if (timeSec >= 1.0f)
    {
        const int wholeSec = juce::roundToInt (timeSec);
        if (std::abs (timeSec - (float) wholeSec) < 0.001f)
            return juce::String (wholeSec) + " s";
        return juce::String (timeSec, 1) + " s";
    }

    return juce::String (juce::roundToInt (timeSec * 1000.0f)) + " ms";
}
} // namespace

namespace peak_display_settings
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
} // namespace peak_display_settings

void PeakDisplaySettingsPanel::configureHorizontalSlider (atom::Slider& slider, double minV, double maxV,
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

PeakDisplaySettingsPanel::PeakDisplaySettingsPanel (ChorusAudioProcessor& processorIn, AtomLookAndFeel& lookAndFeel)
    : processor (processorIn),
      atomLookAndFeel (lookAndFeel),
      introLabel ("peakDisplayIntro", "Peak Display")
{
    setLookAndFeel (&atomLookAndFeel);

    introLabel.setHintText ("Live input peak and meter envelope stream. Attack / Release control header meter ballistics.");
    introLabel.setFont (AtomLookAndFeel::getUIFont (kIntroFontHeight, juce::Font::bold));
    addAndMakeVisible (introLabel);

    auto style = atom::CurveControl::Style::fromTheme (atom::Theme::getCurrent());
    style.metrics.numGridDivisions = 4;
    style.metrics.plotBufferX = 0;
    style.metrics.plotBufferY = 0;
    streamCurve.setStyle (style);
    streamCurve.setMode (atom::CurveControl::Mode::DisplayOnly);
    streamCurve.setXLabel ("Time");
    streamCurve.setYLabel ("dB");
    addAndMakeVisible (streamCurve);

    setupPreviewMeter();
    addAndMakeVisible (previewMeter);

    configureHorizontalSlider (attackSlider, 0.0, 10.0, 0.1, " ms");
    configureHorizontalSlider (releaseSlider, 0.5, 10.0, 0.1, " s");

    fillDisplayRangeCombo (rangeCombo);

    rangeRow = std::make_unique<peak_display_settings::SettingsCardRow> ("rangeRow", "DISPLAY RANGE", rangeCombo, kRowHeight);
    attackRow = std::make_unique<peak_display_settings::SettingsCardRow> ("attackRow", "ATTACK", attackSlider, kRowHeight);
    releaseRow = std::make_unique<peak_display_settings::SettingsCardRow> ("releaseRow", "RELEASE", releaseSlider, kRowHeight);

    auto peakSection = std::make_unique<peak_display_settings::SettingsSection> ("Meter Ballistics");
    peakSection->addRow (static_cast<peak_display_settings::SettingsCardRow&> (*rangeRow));
    peakSection->addRow (static_cast<peak_display_settings::SettingsCardRow&> (*attackRow));
    peakSection->addRow (static_cast<peak_display_settings::SettingsCardRow&> (*releaseRow));
    section = std::move (peakSection);
    addAndMakeVisible (*section);

    auto& vts = processor.parameters.valueTreeState;
    rangeAttachment = std::make_unique<ComboBoxAttachment> (vts, processor.paramMeterDisplayRange.paramID, rangeCombo);
    attackAttachment = std::make_unique<SliderAttachment> (vts, processor.paramMeterAttack.paramID, attackSlider);
    releaseAttachment = std::make_unique<SliderAttachment> (vts, processor.paramMeterRelease.paramID, releaseSlider);

    ensureHistorySize();
    syncStreamCurve();
    startTimerHz (kTimerHz);
}

PeakDisplaySettingsPanel::~PeakDisplaySettingsPanel()
{
    stopTimer();
    rangeAttachment.reset();
    attackAttachment.reset();
    releaseAttachment.reset();

    if (auto* peakSection = dynamic_cast<peak_display_settings::SettingsSection*> (section.get()))
        peakSection->clearRows();

    section.reset();
    rangeRow.reset();
    attackRow.reset();
    releaseRow.reset();
    setLookAndFeel (nullptr);
}

float PeakDisplaySettingsPanel::readParam (const juce::String& paramId, float fallback) const
{
    if (auto* param = processor.parameters.valueTreeState.getParameter (paramId))
        return param->convertFrom0to1 (param->getValue());

    if (auto* value = processor.parameters.valueTreeState.getRawParameterValue (paramId))
        return value->load();

    return fallback;
}

float PeakDisplaySettingsPanel::getDisplayRangeDbSpan() const
{
    const float choice = readParam (processor.paramMeterDisplayRange.paramID,
                                    (float) processor.paramMeterDisplayRange.defaultChoice);
    return (float) displayRangeDbFromChoiceIndex (juce::roundToInt (choice));
}

float PeakDisplaySettingsPanel::getDisplayDbMin() const
{
    return -getDisplayRangeDbSpan();
}

float PeakDisplaySettingsPanel::linearToDisplayDbNormalized (float linear) const
{
    return linearToDbNormalized (linear, getDisplayDbMin(), getDisplayDbMax());
}

void PeakDisplaySettingsPanel::setupPreviewMeter()
{
    meter_display::configurePeakMeter (previewMeter, 1);
}

void PeakDisplaySettingsPanel::syncPreviewMeter()
{
    meter_display::applyMeterValueRange (previewMeter, (int) getDisplayRangeDbSpan());
    previewMeter.setLevels ({ processor.getMeterDisplayMonoNormalized() });
}

void PeakDisplaySettingsPanel::layoutPreviewMeter (int meterX, int meterW)
{
    if (streamCurve.getWidth() <= 0 || streamCurve.getHeight() <= 0)
    {
        previewMeter.setBounds (meterX, streamCurve.getY(), meterW, juce::jmax (1, streamCurve.getHeight()));
        return;
    }

    const auto dataBounds = streamCurve.getDataPlotBounds();
    const int meterPad = juce::roundToInt (previewMeter.getResolvedStyle().metrics.outerPadding);

    previewMeter.setBounds (meterX,
                            streamCurve.getY() + dataBounds.getY() - meterPad,
                            meterW,
                            dataBounds.getHeight() + meterPad * 2);
}

void PeakDisplaySettingsPanel::ensureHistorySize()
{
    if ((int) inputHistory.size() != kHistoryPoints)
    {
        inputHistory.assign ((size_t) kHistoryPoints, 0.0f);
        envelopeHistory.assign ((size_t) kHistoryPoints, 0.0f);
        historyWriteIndex = 0;
        historyCount = 0;
    }
}

void PeakDisplaySettingsPanel::pushLiveSample()
{
    ensureHistorySize();

    const float input = juce::jlimit (0.0f, 1.0f, processor.getMeterLevelMono());
    const float envelope = juce::jlimit (0.0f, 1.0f, processor.getMeterEnvelopeMono());

    inputHistory[(size_t) historyWriteIndex] = input;
    envelopeHistory[(size_t) historyWriteIndex] = envelope;
    historyWriteIndex = (historyWriteIndex + 1) % kHistoryPoints;
    historyCount = juce::jmin (kHistoryPoints, historyCount + 1);
}

void PeakDisplaySettingsPanel::syncStreamCurve()
{
    ensureHistorySize();

    const float attackMs = juce::jlimit (0.0f, 10.0f,
                                         readParam (processor.paramMeterAttack.paramID,
                                                    processor.paramMeterAttack.defaultValue));
    const float releaseSec = juce::jlimit (0.5f, 10.0f,
                                           readParam (processor.paramMeterRelease.paramID,
                                                      processor.paramMeterRelease.defaultValue));

    const int count = juce::jmax (2, historyCount);
    std::vector<std::pair<float, float>> inputCurve;
    std::vector<std::pair<float, float>> envelopeCurve;
    inputCurve.reserve ((size_t) count);
    envelopeCurve.reserve ((size_t) count);

    const float dt = kWindowSec / (float) (kHistoryPoints - 1);
    const int start = (historyWriteIndex - count + kHistoryPoints * 2) % kHistoryPoints;

    for (int i = 0; i < count; ++i)
    {
        const int idx = (start + i) % kHistoryPoints;
        const float t = (float) i * dt;
        inputCurve.emplace_back (t, linearToDb (inputHistory[(size_t) idx]));
        envelopeCurve.emplace_back (t, linearToDb (envelopeHistory[(size_t) idx]));
    }

    // Pad to full window so the axis stays stable while the buffer fills.
    if (count < kHistoryPoints)
    {
        const float lastInputDb = inputCurve.back().second;
        const float lastEnvDb = envelopeCurve.back().second;
        for (int i = count; i < kHistoryPoints; ++i)
        {
            const float t = (float) i * dt;
            inputCurve.emplace_back (t, lastInputDb);
            envelopeCurve.emplace_back (t, lastEnvDb);
        }
    }

    const float displayDbMin = getDisplayDbMin();
    const float displayDbMax = getDisplayDbMax();

    streamCurve.setTitle ("Input Peak  |  Meter Envelope  |  "
                          + juce::String ((int) getDisplayRangeDbSpan()) + " dB  |  Attack "
                          + juce::String (attackMs, 1) + " ms  |  Release "
                          + juce::String (releaseSec, 1) + " s");

    atom::CurveControl::DataAxis xAxis;
    xAxis.minValue = 0.0f;
    xAxis.maxValue = kWindowSec;
    xAxis.formatTick = [] (float value) { return formatTimeTick (value); };

    atom::CurveControl::DataAxis yAxis;
    yAxis.minValue = displayDbMin;
    yAxis.maxValue = displayDbMax;
    yAxis.formatTick = [] (float value) { return formatDbTick (value); };

    streamCurve.setDataAxes (xAxis, yAxis);
    streamCurve.setCustomAxisTicks (buildNiceAxisTicks (0.0f, kWindowSec),
                                    buildNiceAxisTicks (displayDbMin, displayDbMax));
    streamCurve.setSecondaryCustomCurve (inputCurve);
    streamCurve.setCustomCurve (envelopeCurve);

    if (previewMeter.getWidth() > 0)
        layoutPreviewMeter (previewMeter.getX(), previewMeter.getWidth());
}

void PeakDisplaySettingsPanel::timerCallback()
{
    if (! isShowing())
        return;

    pushLiveSample();
    syncStreamCurve();
    syncPreviewMeter();
}

void PeakDisplaySettingsPanel::paint (juce::Graphics& g)
{
    g.fillAll (findColour (juce::ResizableWindow::backgroundColourId));
}

int PeakDisplaySettingsPanel::getPreferredPanelHeight() const noexcept
{
    int sectionH = 0;
    if (auto* peakSection = dynamic_cast<const peak_display_settings::SettingsSection*> (section.get()))
        sectionH = peakSection->getPreferredHeight();

    constexpr int pad = 16;
    constexpr int introH = 48;
    constexpr int gapAfterIntro = 8;
    constexpr int gapAfterCurve = 12;
    return pad + introH + gapAfterIntro + kCurveMinHeight + gapAfterCurve + sectionH + pad;
}

void PeakDisplaySettingsPanel::resized()
{
    auto area = getLocalBounds().reduced (16);
    introLabel.setBounds (area.removeFromTop (48));
    area.removeFromTop (8);

    int sectionH = 0;
    if (auto* peakSection = dynamic_cast<peak_display_settings::SettingsSection*> (section.get()))
        sectionH = peakSection->getPreferredHeight();

    // Reserve slider section first so Attack/Release never get clipped by the curve.
    auto sectionArea = area.removeFromBottom (sectionH);
    if (area.getHeight() > 12)
        area.removeFromBottom (12);

    auto meterArea = area.removeFromRight (kPreviewMeterWidth);
    if (area.getWidth() > kPreviewMeterGap)
        area.removeFromRight (kPreviewMeterGap);

    streamCurve.setBounds (area);
    layoutPreviewMeter (meterArea.getX(), meterArea.getWidth());
    if (section != nullptr)
        section->setBounds (sectionArea);
}
