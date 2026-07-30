#include "AefJuceIncludes.h"
#include "TunerOverlayComponent.h"

#include <cmath>

namespace
{
constexpr float kDefaultSmoothMs = 12.0f;
constexpr float kDefaultLevelGateDb = -48.0f;
constexpr float kCentsRange = 50.0f;       // meter maps [-50, +50]
constexpr float kInTuneCents = 10.0f;      // ±10 cents guide box
constexpr float kHomeNorm = 0.0f;          // leftmost = -50 cents
constexpr float kHomeArriveEps = 0.02f;

void setupCentsMeterBase (atom::MeterBar& meter)
{
    meter.setBarCount (1);
    meter.setOrientation (atom::MeterBar::Orientation::Horizontal);
    meter.setPeakHoldEnabled (false);
    meter.setSegmentCount (0);
    meter.setRefreshRateHz (60);
    meter.setShowValueText (false);
    meter.setValueRange (-(double) kCentsRange, (double) kCentsRange);
    meter.setValueSuffix (" ct");
    meter.setValueDecimals (0);
}

float centsToNorm (float cents) noexcept
{
    const float clamped = juce::jlimit (-kCentsRange, kCentsRange, cents);
    return (clamped + kCentsRange) / (2.0f * kCentsRange);
}
} // namespace

void TunerOverlayComponent::applyCentsMeterStyle()
{
    atom::MeterBarStyleOverride style;
    // Match rotary knob indicator (AtomTheme SliderStyle::thumb = 0xFF34FF34).
    style.colors.peak = juce::Colour (0xff34ff34);
    style.metrics.fillEnabled = false;
    style.metrics.peakThickness = pointerVisible ? 3.0f : 0.0f;
    style.metrics.pointerSoftness = 2.25f;
    style.metrics.pointerMotionBlur = 0.45f;
    style.metrics.pointerSmoothMs = getPointerSmoothMs();
    // ±10 cents within [-50, +50] → normalized [0.4, 0.6]
    style.metrics.guideZoneStart = (kCentsRange - kInTuneCents) / (2.0f * kCentsRange);
    style.metrics.guideZoneEnd = (kCentsRange + kInTuneCents) / (2.0f * kCentsRange);
    style.metrics.roundness = 4.0f;
    style.metrics.outerPadding = 2.0f;
    style.metrics.barGap = 0.0f;
    style.metrics.segmentGap = 0.0f;
    style.metrics.clipZoneSize = 0.0f;
    style.metrics.clipZoneThreshold = 1.0f;
    style.metrics.clipHoldTimeSec = 0.0f;
    centsMeter.setStyleOverride (style);
}

void TunerOverlayComponent::setPointerVisible (bool shouldShow)
{
    if (pointerVisible == shouldShow)
        return;

    pointerVisible = shouldShow;
    applyCentsMeterStyle();
}

bool TunerOverlayComponent::isPointerAtHomeRest() const
{
    if (! pointerVisible)
        return true;

    const auto& displayed = centsMeter.getDisplayedLevels();
    if (displayed.empty())
        return true;

    return displayed[0] <= kHomeArriveEps && ! pointerActive && ! retreatingHome;
}

void TunerOverlayComponent::beginPointerFromLeft (float targetNorm)
{
    // Cold start only: snap to home rest, then ease toward target.
    setPointerVisible (true);
    centsMeter.snapLevels ({ kHomeNorm });
    centsMeter.setLevels ({ juce::jlimit (0.0f, 1.0f, targetNorm) });
    pointerActive = true;
    retreatingHome = false;
}

void TunerOverlayComponent::chasePointer (float centsNorm)
{
    // Analog-style: never interrupt motion with a snap — only retarget.
    setPointerVisible (true);
    retreatingHome = false;
    pointerActive = true;
    centsMeter.setLevels ({ juce::jlimit (0.0f, 1.0f, centsNorm) });
}

TunerOverlayComponent::TunerOverlayComponent()
{
    titleLabel.setText ("Tuner", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setFont (AtomLookAndFeel::getUIFont (18.0f, juce::Font::bold));

    tuningModeLabel.setText ("Tuning mode — output muted; use direct monitoring on your interface",
                             juce::dontSendNotification);
    tuningModeLabel.setJustificationType (juce::Justification::centred);
    tuningModeLabel.setFont (AtomLookAndFeel::getUIFont (12.0f, juce::Font::plain));
    tuningModeLabel.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.65f));
    tuningModeLabel.setMinimumHorizontalScale (1.0f);

    inputLevelLabel.setText ("Input: -- dBFS", juce::dontSendNotification);
    inputLevelLabel.setJustificationType (juce::Justification::centred);
    inputLevelLabel.setFont (AtomLookAndFeel::getUIFont (12.0f, juce::Font::plain));
    inputLevelLabel.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.65f));

    noteLabel.setText ("--", juce::dontSendNotification);
    noteLabel.setJustificationType (juce::Justification::centred);
    noteLabel.setFont (AtomLookAndFeel::getUIFont (88.0f, juce::Font::bold));
    // Match rotary knob indicator / tuner pointer (SliderStyle::thumb).
    noteLabel.setColour (juce::Label::textColourId, juce::Colour (0xff34ff34));

    freqLabel.setText ("-- Hz", juce::dontSendNotification);
    freqLabel.setJustificationType (juce::Justification::centred);
    freqLabel.setFont (AtomLookAndFeel::getUIFont (14.0f, juce::Font::plain));

    centsLabel.setText ("-- cents", juce::dontSendNotification);
    centsLabel.setJustificationType (juce::Justification::centred);
    centsLabel.setFont (AtomLookAndFeel::getUIFont (13.0f, juce::Font::plain));

    periodicityValueLabel.setText ("P --", juce::dontSendNotification);
    periodicityValueLabel.setJustificationType (juce::Justification::centred);
    periodicityValueLabel.setFont (AtomLookAndFeel::getUIFont (12.0f, juce::Font::plain));

    flatLabel.setText ("b", juce::dontSendNotification);
    flatLabel.setJustificationType (juce::Justification::centredRight);
    flatLabel.setFont (AtomLookAndFeel::getUIFont (16.0f, juce::Font::bold));

    sharpLabel.setText ("#", juce::dontSendNotification);
    sharpLabel.setJustificationType (juce::Justification::centredLeft);
    sharpLabel.setFont (AtomLookAndFeel::getUIFont (16.0f, juce::Font::bold));

    setupCentsMeterBase (centsMeter);
    applyCentsMeterStyle();
    centsMeter.snapLevels ({ kHomeNorm });

    periodicitySlider.setRange (0.0, 1.0, 0.01);
    periodicitySlider.setValue (0.70, juce::dontSendNotification);
    periodicitySlider.setSliderStyle (juce::Slider::LinearHorizontal);
    periodicitySlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 56, 22);
    periodicitySlider.setCustomText ("Periodicity");
    periodicitySlider.setValueLabelPos (atom::Slider::ValueLabelPos::Left);
    periodicitySlider.onValueChange = [this]
    {
        if (onPeriodicityThresholdChanged)
            onPeriodicityThresholdChanged ((float) periodicitySlider.getValue());
    };

    smoothSlider.setRange (0.0, 200.0, 1.0);
    smoothSlider.setValue ((double) kDefaultSmoothMs, juce::dontSendNotification);
    smoothSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    smoothSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 56, 22);
    smoothSlider.setCustomText ("Smooth");
    smoothSlider.setTextValueSuffix (" ms");
    smoothSlider.setValueLabelPos (atom::Slider::ValueLabelPos::Left);
    smoothSlider.onValueChange = [this]
    {
        applyCentsMeterStyle();
        if (onPointerSmoothMsChanged)
            onPointerSmoothMsChanged (getPointerSmoothMs());
    };

    levelGateSlider.setRange (-80.0, -12.0, 0.1);
    levelGateSlider.setValue ((double) kDefaultLevelGateDb, juce::dontSendNotification);
    levelGateSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    levelGateSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 64, 22);
    levelGateSlider.setCustomText ("Level Gate");
    levelGateSlider.setTextValueSuffix (" dB");
    levelGateSlider.setValueLabelPos (atom::Slider::ValueLabelPos::Left);

    closeButton.onClick = [this]
    {
        if (onCloseRequested)
            onCloseRequested();
    };

    panel.addAndMakeVisible (titleLabel);
    panel.addAndMakeVisible (tuningModeLabel);
    panel.addAndMakeVisible (inputLevelLabel);
    panel.addAndMakeVisible (noteLabel);
    panel.addAndMakeVisible (freqLabel);
    panel.addAndMakeVisible (centsLabel);
    panel.addAndMakeVisible (periodicityValueLabel);
    panel.addAndMakeVisible (flatLabel);
    panel.addAndMakeVisible (sharpLabel);
    panel.addAndMakeVisible (centsMeter);
    panel.addAndMakeVisible (periodicitySlider);
    panel.addAndMakeVisible (smoothSlider);
    panel.addAndMakeVisible (levelGateSlider);
    panel.addAndMakeVisible (closeButton);
    addAndMakeVisible (panel);
}

void TunerOverlayComponent::setPeriodicityThreshold (float threshold)
{
    periodicitySlider.setValue ((double) juce::jlimit (0.0f, 1.0f, threshold), juce::dontSendNotification);
}

float TunerOverlayComponent::getPeriodicityThreshold() const
{
    return (float) periodicitySlider.getValue();
}

void TunerOverlayComponent::setPointerSmoothMs (float milliseconds)
{
    smoothSlider.setValue ((double) juce::jlimit (0.0f, 200.0f, milliseconds), juce::dontSendNotification);
    applyCentsMeterStyle();
}

float TunerOverlayComponent::getPointerSmoothMs() const
{
    return (float) smoothSlider.getValue();
}

void TunerOverlayComponent::setLevelGateDbFs (float dbFs)
{
    levelGateSlider.setValue ((double) juce::jlimit (-80.0f, -12.0f, dbFs), juce::dontSendNotification);
}

float TunerOverlayComponent::getLevelGateDbFs() const
{
    return (float) levelGateSlider.getValue();
}

void TunerOverlayComponent::setTuningModeActive (bool active)
{
    tuningModeLabel.setVisible (active);
}

void TunerOverlayComponent::setInputDbFs (float dbFsLeft, double sampleRateHz, float dbFsRight)
{
    const bool hasLeft = dbFsLeft > -90.0f;
    const bool hasRight = dbFsRight > -90.0f;
    lastInputDbFs = hasLeft || hasRight ? juce::jmax (dbFsLeft, dbFsRight) : -100.0f;

    juce::String line;
    if (! hasLeft && ! hasRight)
        line = "Input: no signal";
    else if (hasRight)
        line = "Input: L " + juce::String (dbFsLeft, 1) + " / R " + juce::String (dbFsRight, 1) + " dBFS";
    else
        line = "Input: " + juce::String (dbFsLeft, 1) + " dBFS";

    if (sampleRateHz > 0.0)
        line += " @ " + juce::String (sampleRateHz, 0) + " Hz";

    inputLevelLabel.setText (line, juce::dontSendNotification);
}

void TunerOverlayComponent::clearNoteDisplay()
{
    noteLabel.setText ("--", juce::dontSendNotification);
    freqLabel.setText ("-- Hz", juce::dontSendNotification);
    centsLabel.setText ("-- cents", juce::dontSendNotification);
    lastNoteIndex = -1;
    lastOctave = -1;
}

void TunerOverlayComponent::retreatPointerHome()
{
    pointerActive = false;
    retreatingHome = true;
    // Keep last note name until the pointer finishes returning home and hides.
    setPointerVisible (true);
    centsMeter.setLevels ({ kHomeNorm });
}

void TunerOverlayComponent::clearPitch()
{
    clearNoteDisplay();
    periodicityValueLabel.setText ("P --", juce::dontSendNotification);
    inputLevelLabel.setText ("Input: -- dBFS", juce::dontSendNotification);
    lastInputDbFs = -100.0f;
    pointerActive = false;
    retreatingHome = false;
    centsMeter.snapLevels ({ kHomeNorm });
    setPointerVisible (false);
}

void TunerOverlayComponent::setPitchResult (const TunerDetector::Result& result)
{
    if (result.periodicity > 0.0f)
    {
        juce::String pText = "P " + juce::String (result.periodicity, 2);
        if (! result.valid)
            pText += "  <  thr " + juce::String (getPeriodicityThreshold(), 2);
        periodicityValueLabel.setText (pText, juce::dontSendNotification);
    }
    else
    {
        periodicityValueLabel.setText ("P --", juce::dontSendNotification);
    }

    const bool signalOk = lastInputDbFs >= getLevelGateDbFs();
    const bool pitchOk = result.valid;
    const float targetNorm = pitchOk ? centsToNorm (result.cents) : kHomeNorm;

    if (signalOk && pitchOk)
    {
        const juce::String noteText = juce::String (TunerDetector::noteName (result.noteIndex))
                                    + juce::String (result.octave);
        noteLabel.setText (noteText, juce::dontSendNotification);
        freqLabel.setText (juce::String (result.frequencyHz, 1) + " Hz", juce::dontSendNotification);
        const juce::String sign = result.cents >= 0.0f ? "+" : "";
        centsLabel.setText (sign + juce::String (result.cents, 1) + " cents", juce::dontSendNotification);

        lastNoteIndex = result.noteIndex;
        lastOctave = result.octave;

        // From hidden rest: walk in from the left once.
        // Mid-release / already moving: only retarget — never snap (analog needle).
        if (isPointerAtHomeRest())
            beginPointerFromLeft (targetNorm);
        else
            chasePointer (targetNorm);

        return;
    }

    // Weak / unlocked: always ease home — never snap during release.
    // Note name stays until the pointer reaches home and disappears.
    if (pointerActive || (pointerVisible && ! retreatingHome))
        retreatPointerHome();

    if (retreatingHome)
    {
        centsMeter.setLevels ({ kHomeNorm });
        const auto& displayed = centsMeter.getDisplayedLevels();
        const float pos = displayed.empty() ? 0.0f : displayed[0];
        if (pos <= kHomeArriveEps)
        {
            retreatingHome = false;
            pointerActive = false;
            setPointerVisible (false);
            clearNoteDisplay();
        }
    }
}

void TunerOverlayComponent::paint (juce::Graphics&)
{
}

void TunerOverlayComponent::resized()
{
    panel.setBounds (getLocalBounds());
    auto area = panel.getLocalBounds().reduced (20, 16);

    titleLabel.setBounds (area.removeFromTop (28));
    area.removeFromTop (2);
    tuningModeLabel.setBounds (area.removeFromTop (36));
    area.removeFromTop (4);
    inputLevelLabel.setBounds (area.removeFromTop (18));
    area.removeFromTop (4);
    noteLabel.setBounds (area.removeFromTop (96));
    freqLabel.setBounds (area.removeFromTop (22));
    centsLabel.setBounds (area.removeFromTop (20));
    periodicityValueLabel.setBounds (area.removeFromTop (18));
    area.removeFromTop (8);

    auto meterRow = area.removeFromTop (36);
    flatLabel.setBounds (meterRow.removeFromLeft (28));
    sharpLabel.setBounds (meterRow.removeFromRight (28));
    meterRow.reduce (8, 4);
    centsMeter.setBounds (meterRow);

    area.removeFromTop (12);
    periodicitySlider.setBounds (area.removeFromTop (36));
    area.removeFromTop (8);
    smoothSlider.setBounds (area.removeFromTop (36));
    area.removeFromTop (8);
    levelGateSlider.setBounds (area.removeFromTop (36));

    area.removeFromTop (12);
    closeButton.setBounds (area.removeFromBottom (34).withSizeKeepingCentre (120, 34));
}
