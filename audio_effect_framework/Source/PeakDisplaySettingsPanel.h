#pragma once

#include <juce_atom_theme/juce_atom_theme.h>
#include "AefJuceIncludes.h"
#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>
#include <utility>
#include <vector>

#include "MeterDisplayUtils.h"

class AudioEffectFrameworkProcessor;

/** Settings → Peak Display: meter attack/release + live envelope stream. */
class PeakDisplaySettingsPanel final : public juce::Component, private juce::Timer
{
public:
    PeakDisplaySettingsPanel (AudioEffectFrameworkProcessor& processor, AtomLookAndFeel& lookAndFeel);
    ~PeakDisplaySettingsPanel() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    int getPreferredPanelHeight() const noexcept;

private:
    void timerCallback() override;
    void pushLiveSample();
    void syncStreamCurve();
    void syncPreviewMeter();
    void setupPreviewMeter();
    void layoutPreviewMeter (int meterX, int meterW);
    void ensureHistorySize();
    float readParam (const juce::String& paramId, float fallback) const;
    float getDisplayRangeDbSpan() const;
    float getDisplayDbMin() const;
    float getDisplayDbMax() const noexcept { return meter_display::kDbMax; }
    float linearToDisplayDbNormalized (float linear) const;
    void configureHorizontalSlider (atom::Slider& slider, double minV, double maxV, double interval,
                                    const juce::String& suffix);

    AudioEffectFrameworkProcessor& processor;
    AtomLookAndFeel& atomLookAndFeel;

    atom::Label introLabel;
    atom::CurveControl streamCurve { atom::CurveControl::Direction::Speedup };
    atom::MeterBar previewMeter;
    atom::ComboBox rangeCombo { "meterDisplayRangeCombo" };
    atom::Slider attackSlider { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    atom::Slider releaseSlider { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };

    std::unique_ptr<juce::Component> rangeRow;
    std::unique_ptr<juce::Component> attackRow;
    std::unique_ptr<juce::Component> releaseRow;
    std::unique_ptr<juce::Component> section;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<ComboBoxAttachment> rangeAttachment;
    std::unique_ptr<SliderAttachment> attackAttachment;
    std::unique_ptr<SliderAttachment> releaseAttachment;

    std::vector<float> inputHistory;
    std::vector<float> envelopeHistory;
    int historyWriteIndex = 0;
    int historyCount = 0;

    static constexpr int kTimerHz = meter_display::kRefreshHz;
    static constexpr float kWindowSec = 3.0f;
    static constexpr int kHistoryPoints = (int) (kWindowSec * (float) kTimerHz);
    static constexpr int kCurveMinHeight = 220;
    static constexpr int kPreviewMeterWidth = 28;
    static constexpr int kPreviewMeterGap = 10;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PeakDisplaySettingsPanel)
};
