#pragma once

#include <vector>

#include "../JuceLibraryCode/JuceHeader.h"
#include <juce_atom_theme/juce_atom_theme.h>

//==============================================================================
/** Chorus LFO delay modulation preview (Atom CurveControl display-only mode). */
class ChorusLfoCurveComponent final : public juce::Component
{
public:
    ChorusLfoCurveComponent();

    /** Current LFO/delay parameters (curve shape). */
    void setParameters (float rateHz, float delayMs, float amountMs, bool triangleLfo);

    /** Fixed plot axes: X = one LFO period at Settings min rate; Y from delay/amount caps. */
    void setAxisLimits (float minLfoFreqHz, float maxDelayMs, float maxAmountMs, float minDelayMs);

    void resized() override;

private:
    void rebuildCurve();
    void syncCurveControl();

    [[nodiscard]] int computeCurvePointCount() const noexcept;

    static float evalLfo (float phaseRadians, bool triangleLfo) noexcept;
    static juce::String formatTimeTick (float timeSec);
    static juce::String formatDelayTick (float delayMs);

    atom::CurveControl curveControl_;

    float rateHz = 1.0f;
    float delayMs = 25.0f;
    float amountMs = 10.0f;
    bool triangleLfo = false;

    float axisMinLfoFreqHz = 0.01f;
    float axisMaxDelayMs = 100.0f;
    float axisMaxAmountMs = 50.0f;
    float axisMinDelayMs = 1.0f;

    float timeMinSec = 0.0f;
    float timeMaxSec = 1.0f;
    float delayMinMs = 0.0f;
    float delayMaxMs = 150.0f;

    std::vector<std::pair<float, float>> modulationCurve;

    static constexpr int minCurvePoints = 256;
    static constexpr int maxCurvePoints = 8192;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChorusLfoCurveComponent)
};
