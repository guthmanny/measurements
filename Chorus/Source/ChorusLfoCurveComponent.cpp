#include "ChorusLfoCurveComponent.h"

#include <cmath>

namespace
{
std::vector<float> buildNiceAxisTicks (float minVal, float maxVal, int targetTickCount = 5)
{
    std::vector<float> ticks;
    const float span = maxVal - minVal;
    if (span <= 0.0f)
    {
        ticks.push_back (minVal);
        return ticks;
    }

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

std::vector<float> buildTimeAxisTicks (float periodSec)
{
    auto ticks = buildNiceAxisTicks (0.0f, periodSec);
    if (ticks.empty())
        return { 0.0f, periodSec };

    ticks.back() = periodSec;
    return ticks;
}
} // namespace

//==============================================================================

ChorusLfoCurveComponent::ChorusLfoCurveComponent()
    : curveControl_ (atom::CurveControl::Direction::Speedup)
{
    auto style = atom::CurveControl::Style::fromTheme (atom::Theme::getDarkTheme());
    style.metrics.numGridDivisions = 4;
    style.metrics.plotBufferX = 0;
    style.metrics.plotBufferY = 0;
    curveControl_.setStyle (style);
    curveControl_.setMode (atom::CurveControl::Mode::DisplayOnly);
    curveControl_.setXLabel ("Time");
    curveControl_.setYLabel ("Delay");

    addAndMakeVisible (curveControl_);
    rebuildCurve();
    syncCurveControl();
}

float ChorusLfoCurveComponent::evalLfo (float phaseRadians, bool triangleLfo) noexcept
{
    if (triangleLfo)
        return (2.0f / juce::MathConstants<float>::pi)
               * std::asin (std::sin (phaseRadians));

    return std::sin (phaseRadians);
}

juce::String ChorusLfoCurveComponent::formatTimeTick (float timeSec)
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

juce::String ChorusLfoCurveComponent::formatDelayTick (float delayMs)
{
    return juce::String (delayMs, delayMs >= 100.0f ? 0 : 1) + " ms";
}

void ChorusLfoCurveComponent::setParameters (float newRateHz,
                                             float newDelayMs,
                                             float newAmountMs,
                                             bool newTriangleLfo)
{
    rateHz = juce::jmax (0.01f, newRateHz);
    delayMs = juce::jmax (0.0f, newDelayMs);
    amountMs = juce::jmax (0.0f, newAmountMs);
    triangleLfo = newTriangleLfo;

    rebuildCurve();
    syncCurveControl();
}

void ChorusLfoCurveComponent::setAxisLimits (float minLfoFreqHz,
                                             float maxDelayMs,
                                             float maxAmountMs,
                                             float minDelayMs)
{
    axisMinLfoFreqHz = juce::jmax (0.01f, minLfoFreqHz);
    axisMaxDelayMs = juce::jmax (0.0f, maxDelayMs);
    axisMaxAmountMs = juce::jmax (0.0f, maxAmountMs);
    axisMinDelayMs = juce::jmax (0.0f, minDelayMs);

    timeMinSec = 0.0f;
    timeMaxSec = 1.0f / axisMinLfoFreqHz;
    delayMinMs = juce::jmax (0.0f, axisMinDelayMs - axisMaxAmountMs);
    delayMaxMs = axisMaxDelayMs + axisMaxAmountMs;
    if (delayMaxMs - delayMinMs < 1.0f)
        delayMaxMs = delayMinMs + 1.0f;

    rebuildCurve();
    syncCurveControl();
}

int ChorusLfoCurveComponent::computeCurvePointCount() const noexcept
{
    const float timeSpanSec = juce::jmax (1.0e-6f, timeMaxSec);
    const float cyclesInWindow = juce::jmax (1.0f, rateHz * timeSpanSec);
    const int pointsPerCycle = triangleLfo ? 48 : 64;
    const int cyclePoints = (int) std::ceil (cyclesInWindow * (float) pointsPerCycle);

    const float plotWidth = (float) curveControl_.getWidth();
    const int pixelPoints = plotWidth > 1.0f ? (int) std::ceil (plotWidth * 2.0f) : 0;

    return juce::jlimit (minCurvePoints,
                         maxCurvePoints,
                         juce::jmax (cyclePoints, pixelPoints));
}

void ChorusLfoCurveComponent::rebuildCurve()
{
    const int pointCount = computeCurvePointCount();
    modulationCurve.clear();
    modulationCurve.reserve ((size_t) pointCount);

    const float safeRate = juce::jmax (0.01f, rateHz);
    const float timeSpanSec = juce::jmax (1.0e-6f, timeMaxSec);
    const float twoPi = juce::MathConstants<float>::twoPi;

    for (int i = 0; i < pointCount; ++i)
    {
        const float tNorm = (float) i / (float) (pointCount - 1);
        const float timeSec = tNorm * timeSpanSec;
        const float phase = twoPi * safeRate * timeSec;
        const float lfo = evalLfo (phase, triangleLfo);
        const float modDelayMs = delayMs + amountMs * lfo;
        modulationCurve.emplace_back (timeSec, modDelayMs);
    }
}

void ChorusLfoCurveComponent::syncCurveControl()
{
    const auto shapeText = triangleLfo ? juce::String ("Triangle") : juce::String ("Sine");
    curveControl_.setTitle ("LFO " + shapeText + "  |  "
                            + juce::String (rateHz, 2) + " Hz  |  Center "
                            + juce::String (delayMs, 1) + " ms  |  Depth "
                            + juce::String (amountMs, 1) + " ms");

    atom::CurveControl::DataAxis xAxis;
    xAxis.minValue = timeMinSec;
    xAxis.maxValue = timeMaxSec;
    xAxis.formatTick = [] (float value) { return formatTimeTick (value); };

    atom::CurveControl::DataAxis yAxis;
    yAxis.minValue = delayMinMs;
    yAxis.maxValue = delayMaxMs;
    yAxis.formatTick = [] (float value) { return formatDelayTick (value); };

    curveControl_.setDataAxes (xAxis, yAxis);
    curveControl_.setCustomAxisTicks (buildTimeAxisTicks (timeMaxSec),
                                      buildNiceAxisTicks (delayMinMs, delayMaxMs));
    curveControl_.setCustomCurve (modulationCurve);

    atom::CurveControl::ReferenceLine centerLine;
    centerLine.yValue = delayMs;
    centerLine.enabled = delayMs >= delayMinMs && delayMs <= delayMaxMs;
    curveControl_.setReferenceLine (centerLine);
}

void ChorusLfoCurveComponent::resized()
{
    curveControl_.setBounds (getLocalBounds());
    rebuildCurve();
    syncCurveControl();
}
