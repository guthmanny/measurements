#include "BjtCurvePanel.h"

#include <atom/CurveControl.h>
#include <utility>

namespace
{
constexpr int kFamilyNumPoints = 96;
constexpr int kTransferNumPoints = 256;

juce::Colour curveColourForIndex(const juce::Colour& base, int index, int total)
{
    if (total <= 1)
        return base;

    const float hue = base.getHue();
    const float sat = base.getSaturation();
    const float shift = static_cast<float>(index) / static_cast<float>(juce::jmax(1, total - 1));
    return juce::Colour::fromHSV(std::fmod(hue + shift * 0.18f, 1.0f), sat, base.getBrightness(), 1.0f);
}

bool ibSweepEqual(const bjt_curve::IbSweepParams& a, const bjt_curve::IbSweepParams& b) noexcept
{
    return a.count == b.count
        && juce::approximatelyEqual(a.minAmps, b.minAmps)
        && juce::approximatelyEqual(a.stepAmps, b.stepAmps);
}
}  // namespace

BjtCurvePanel::BjtCurvePanel()
{
    setOpaque(true);
    curveControl.setMode(atom::CurveControl::Mode::DisplayOnly);
    addAndMakeVisible(curveControl);
    scheduleRebuild();
}

BjtCurvePanel::~BjtCurvePanel()
{
    // Bump generation so in-flight workers discard results; wait briefly for flight flag.
    rebuildGeneration_.fetch_add(1, std::memory_order_acq_rel);
    cancelPendingUpdate();
    for (int i = 0; i < 200 && rebuildInFlight_.load(std::memory_order_acquire); ++i)
        juce::Thread::sleep(1);
}

void BjtCurvePanel::setModel(nx_bjt_npn_model_e model)
{
    if (model_ == model)
        return;

    model_ = model;
    scheduleRebuild();
}

void BjtCurvePanel::setCircuit(bjt_curve::CircuitKind circuit)
{
    if (circuit_ == circuit)
        return;

    circuit_ = circuit;
    scheduleRebuild();
}

void BjtCurvePanel::setCurveKind(bjt_curve::CurveKind kind)
{
    if (curveKind_ == kind)
        return;

    curveKind_ = kind;
    scheduleRebuild();
}

void BjtCurvePanel::setIbSweep(const bjt_curve::IbSweepParams& params)
{
    bjt_curve::IbSweepParams next = params;
    next.sanitise();
    if (ibSweepEqual(ibSweep_, next))
        return;

    ibSweep_ = next;
    scheduleRebuild();
}

void BjtCurvePanel::setVceMaxVolts(float vceMaxVolts)
{
    const float next = juce::jmax(0.05f, vceMaxVolts);
    if (juce::approximatelyEqual(vceMaxVolts_, next))
        return;

    vceMaxVolts_ = next;
    scheduleRebuild();
}

void BjtCurvePanel::setVbeMaxVolts(float vbeMaxVolts)
{
    const float next = juce::jlimit(0.20f, 1.20f, vbeMaxVolts);
    if (juce::approximatelyEqual(vbeMaxVolts_, next))
        return;

    vbeMaxVolts_ = next;
    scheduleRebuild();
}

void BjtCurvePanel::setCurrentMaxAmps(float currentMaxAmps)
{
    const float next = juce::jmax(1.0e-12f, currentMaxAmps);
    if (juce::approximatelyEqual(currentMaxAmps_, next))
        return;

    currentMaxAmps_ = next;
    scheduleRebuild();
}

void BjtCurvePanel::applyTheme(const atom::ThemeColors& themeColors)
{
    style_ = atom::CurveControl::Style::fromTheme(themeColors);
    style_.metrics.numGridDivisions = 5;
    style_.metrics.plotBufferX = 6;
    style_.metrics.plotBufferY = 6;
    style_.metrics.pathStrokeSize = 2.25f;
    curveControl.setStyle(style_);
    // Theme only — do not recompute NuDSP tables / EM families.
    repaint();
}

void BjtCurvePanel::scheduleRebuild()
{
    rebuildGeneration_.fetch_add(1, std::memory_order_acq_rel);
    busy_ = true;
    triggerAsyncUpdate();
    repaint();
}

void BjtCurvePanel::handleAsyncUpdate()
{
    RebuildParams params;
    params.model = model_;
    params.circuit = circuit_;
    params.curveKind = curveKind_;
    params.ibSweep = ibSweep_;
    params.vceMaxVolts = vceMaxVolts_;
    params.vbeMaxVolts = vbeMaxVolts_;
    params.currentMaxAmps = currentMaxAmps_;
    params.generation = rebuildGeneration_.load(std::memory_order_acquire);

    // Coalesce: if a worker is already running, it will re-check generation and
    // we schedule another pass when it finishes if still stale.
    if (rebuildInFlight_.exchange(true, std::memory_order_acq_rel))
        return;

    juce::Component::SafePointer<BjtCurvePanel> safe(this);
    juce::Thread::launch([safe, params]()
    {
        RebuildResult result = computeRebuild(params);

        juce::MessageManager::callAsync([safe, result = std::move(result)]() mutable
        {
            if (safe == nullptr)
                return;

            safe->rebuildInFlight_.store(false, std::memory_order_release);

            const auto latest = safe->rebuildGeneration_.load(std::memory_order_acquire);
            if (result.generation != latest)
            {
                // Params changed while we were computing — run again with latest.
                safe->triggerAsyncUpdate();
                return;
            }

            safe->applyRebuildResult(std::move(result));
        });
    });
}

BjtCurvePanel::RebuildResult BjtCurvePanel::computeRebuild(const RebuildParams& params)
{
    RebuildResult result;
    result.generation = params.generation;
    result.curveKind = params.curveKind;
    result.circuit = params.circuit;

    const auto& bjt = bjt_curve::modelParams(params.model);
    result.ibValues = bjt_curve::buildIbSweepValues(params.ibSweep);
    bjt_curve::curveAxisLabels(params.curveKind, result.xLabel, result.yLabel);

    result.title = bjt_curve::circuitDisplayName(params.circuit);
    result.title += "  |  ";
    result.title += bjt_curve::modelDisplayName(params.model);
    result.title += "  |  " + bjt_curve::formatModelSummary(bjt);

    result.useFamilyPlot = params.curveKind == bjt_curve::CurveKind::IcVsVce
                        && params.circuit == bjt_curve::CircuitKind::CommonEmitter;
    result.unsupportedOutputCurve = params.curveKind == bjt_curve::CurveKind::IcVsVce
                                 && params.circuit != bjt_curve::CircuitKind::CommonEmitter;

    if (result.useFamilyPlot)
    {
        result.curveFamilies = bjt_curve::buildIcVsVceFamily(params.model,
                                                             params.circuit,
                                                             result.ibValues,
                                                             kFamilyNumPoints,
                                                             params.vceMaxVolts);
        result.dcOverlay = bjt_curve::buildCommonEmitterDcLoadLineOverlay(params.model,
                                                                          result.ibValues,
                                                                          result.curveFamilies,
                                                                          params.vceMaxVolts);
        result.axisRange = bjt_curve::computeAxisRangeForFamily(result.curveFamilies);
        bjt_curve::expandAxisRangeForOverlay(result.axisRange, result.dcOverlay, params.vceMaxVolts);
        // After final Y scale is known: extend/trim load line to plot edges (top/sides).
        bjt_curve::clipOverlayLoadLinesToAxis(result.dcOverlay, result.axisRange);
        result.currentDisplayUnit = bjt_curve::chooseCurrentDisplayUnit(result.axisRange.maxY);
        result.yLabel = bjt_curve::currentAxisLabel(result.currentDisplayUnit, "Ic");
        result.title += "  |  NuDSP Ebers-Moll @ const Ib";
        result.title += "  |  " + bjt_curve::formatIbSweepSummary(params.ibSweep);
        result.title += "  |  Vce 0-" + bjt_curve::formatVoltageTick(params.vceMaxVolts);
    }
    else if (result.unsupportedOutputCurve)
    {
        result.axisRange = { 0.0f, params.vceMaxVolts, 0.0f, 1.0e-3f };
        result.currentDisplayUnit = bjt_curve::chooseCurrentDisplayUnit(result.axisRange.maxY);
        result.xLabel = "Vce (V)";
        result.yLabel = bjt_curve::currentAxisLabel(result.currentDisplayUnit, "Ic");
        result.title += "  |  Ic vs Vce only for Common Emitter";
    }
    else
    {
        const float ib = result.ibValues.empty() ? 0.0f : result.ibValues.front();
        result.samples = bjt_curve::buildCurve(params.model,
                                               params.circuit,
                                               params.curveKind,
                                               ib,
                                               kTransferNumPoints,
                                               params.vceMaxVolts,
                                               params.vbeMaxVolts);
        result.dcOverlay.qPoint = bjt_curve::computeStaticOperatingPoint(params.model, params.circuit);
        result.axisRange = bjt_curve::computeAxisRange(params.curveKind, 0.0f, result.samples);
        bjt_curve::applyTransferAxisLimits(result.axisRange,
                                           params.curveKind,
                                           params.vbeMaxVolts,
                                           params.currentMaxAmps,
                                           result.dcOverlay.qPoint);
        result.currentDisplayUnit = bjt_curve::chooseCurrentDisplayUnit(result.axisRange.maxY);
        const juce::String quantity = params.curveKind == bjt_curve::CurveKind::IbVsVbe ? "Ib" : "Ic";
        result.yLabel = bjt_curve::currentAxisLabel(result.currentDisplayUnit, quantity);
        result.title += "  |  NuDSP Ebers-Moll";
        result.title += "  |  Vbe 0-" + bjt_curve::formatVoltageTick(params.vbeMaxVolts);
        result.title += "  |  " + quantity + " max " + bjt_curve::formatCurrentWithUnit(params.currentMaxAmps);
        if (result.dcOverlay.qPoint.valid)
            result.title += "  |  " + bjt_curve::formatOperatingPointLabel(result.dcOverlay.qPoint, params.curveKind);
    }

    return result;
}

void BjtCurvePanel::applyRebuildResult(RebuildResult&& result)
{
    curveKind_ = result.curveKind;
    circuit_ = result.circuit;
    ibValues_ = std::move(result.ibValues);
    samples_ = std::move(result.samples);
    curveFamilies_ = std::move(result.curveFamilies);
    dcOverlay_ = std::move(result.dcOverlay);
    axisRange_ = result.axisRange;
    currentDisplayUnit_ = result.currentDisplayUnit;
    title_ = std::move(result.title);
    xLabel_ = std::move(result.xLabel);
    yLabel_ = std::move(result.yLabel);
    busy_ = false;

    if (result.useFamilyPlot || result.unsupportedOutputCurve)
    {
        curveControl.setVisible(false);
        curveControl.setInterceptsMouseClicks(false, false);
        if (result.unsupportedOutputCurve)
            curveControl.clearSecondaryCustomCurve();
    }
    else
    {
        curveControl.setVisible(true);
        const auto currentUnit = currentDisplayUnit_;
        atom::CurveControl::DataAxis xAxis;
        xAxis.minValue = axisRange_.minX;
        xAxis.maxValue = axisRange_.maxX;
        xAxis.formatTick = [](float value) { return bjt_curve::formatVoltageTick(value); };

        atom::CurveControl::DataAxis yAxis;
        yAxis.minValue = axisRange_.minY;
        yAxis.maxValue = axisRange_.maxY;
        yAxis.formatTick = [currentUnit](float value)
        { return bjt_curve::formatCurrentTick(value, currentUnit); };

        curveControl.setTitle(title_);
        curveControl.setXLabel(xLabel_);
        curveControl.setYLabel(yLabel_);
        curveControl.setDataAxes(xAxis, yAxis);
        curveControl.setCustomAxisTicks(bjt_curve::buildNiceTicks(axisRange_.minX, axisRange_.maxX),
                                        bjt_curve::buildNiceTicks(axisRange_.minY, axisRange_.maxY));
        curveControl.setCustomCurve(samples_);
        curveControl.clearSecondaryCustomCurve();
    }

    repaint();
}

void BjtCurvePanel::paintBusyOverlay(juce::Graphics& g)
{
    if (!busy_)
        return;

    g.setColour(style_.colors.background.withAlpha(0.35f));
    g.fillRect(getLocalBounds());
    g.setFont(13.0f);
    g.setColour(style_.colors.label);
    g.drawText("Computing...", getLocalBounds(), juce::Justification::centred);
}

juce::Rectangle<float> BjtCurvePanel::getPlotArea() const noexcept
{
    const auto bounds = getLocalBounds().toFloat();
    const float titleBand = 22.0f;
    const float xLabelBand = 18.0f;
    const float yLabelBand = 36.0f;
    const float xTickBand = 16.0f;
    const float yTickBand = 52.0f;
    const float pad = static_cast<float>(style_.metrics.outerPadding);

    return bounds.withTrimmedTop(titleBand + pad)
        .withTrimmedBottom(xLabelBand + xTickBand + pad)
        .withTrimmedLeft(yLabelBand + yTickBand + pad)
        .withTrimmedRight(pad)
        .reduced(static_cast<float>(style_.metrics.plotBufferX), static_cast<float>(style_.metrics.plotBufferY));
}

float BjtCurvePanel::dataToX(float x) const noexcept
{
    const auto area = getPlotArea();
    if (area.getWidth() <= 0.0f || axisRange_.maxX <= axisRange_.minX)
        return area.getX();

    const float norm = juce::jlimit(0.0f, 1.0f, (x - axisRange_.minX) / (axisRange_.maxX - axisRange_.minX));
    return area.getX() + norm * area.getWidth();
}

float BjtCurvePanel::dataToY(float y) const noexcept
{
    const auto area = getPlotArea();
    if (area.getHeight() <= 0.0f || axisRange_.maxY <= axisRange_.minY)
        return area.getY();

    const float norm = juce::jlimit(0.0f, 1.0f, (y - axisRange_.minY) / (axisRange_.maxY - axisRange_.minY));
    return area.getBottom() - norm * area.getHeight();
}

void BjtCurvePanel::paintFamilyCurves(juce::Graphics& g)
{
    const auto& colors = style_.colors;
    const auto& metrics = style_.metrics;
    const auto plotArea = getPlotArea();
    if (plotArea.isEmpty())
        return;

    g.fillAll(colors.background);

    g.setColour(colors.frame);
    g.drawRect(plotArea, metrics.frameStrokeSize);

    g.setColour(colors.grid);
    for (const float xTick : bjt_curve::buildNiceTicks(axisRange_.minX, axisRange_.maxX))
    {
        const float x = dataToX(xTick);
        g.drawVerticalLine(juce::roundToInt(x), plotArea.getY(), plotArea.getBottom());
    }
    for (const float yTick : bjt_curve::buildNiceTicks(axisRange_.minY, axisRange_.maxY))
    {
        const float y = dataToY(yTick);
        g.drawHorizontalLine(juce::roundToInt(y), plotArea.getX(), plotArea.getRight());
    }

    const auto mapPoint = [this](float x, float y) { return juce::Point<float>(dataToX(x), dataToY(y)); };
    const int total = static_cast<int>(curveFamilies_.size());

    for (int i = 0; i < total; ++i)
    {
        juce::Path strokePath;
        atom::buildVerticallyClippedPolylinePaths(curveFamilies_[static_cast<size_t>(i)],
                                                  axisRange_.minY,
                                                  axisRange_.maxY,
                                                  mapPoint,
                                                  strokePath);

        g.setColour(curveColourForIndex(colors.path, i, total));
        g.strokePath(strokePath, juce::PathStrokeType(metrics.pathStrokeSize));
    }

    if (!dcOverlay_.loadLines.empty() && dcOverlay_.loadLines.front().size() >= 2)
    {
        const auto& segment = dcOverlay_.loadLines.front();
        juce::Path loadLinePath;
        loadLinePath.startNewSubPath(mapPoint(segment.front().first, segment.front().second));
        for (size_t j = 1; j < segment.size(); ++j)
            loadLinePath.lineTo(mapPoint(segment[j].first, segment[j].second));

        juce::Path dashedLoadLine;
        const float dashLengths[] = { 6.0f, 4.0f };
        juce::PathStrokeType(1.75f).createDashedStroke(dashedLoadLine, loadLinePath, dashLengths, 2);

        g.setColour(colors.label.withAlpha(0.9f));
        g.strokePath(dashedLoadLine, juce::PathStrokeType(1.75f));
    }

    if (dcOverlay_.qPoint.valid)
    {
        constexpr float kQPointRadius = 7.0f;
        const float x = dataToX(dcOverlay_.qPoint.vce);
        const float y = dataToY(dcOverlay_.qPoint.ic);
        g.setColour(colors.label);
        g.fillEllipse(x - kQPointRadius, y - kQPointRadius, kQPointRadius * 2.0f, kQPointRadius * 2.0f);
        g.setColour(colors.background);
        g.drawEllipse(x - kQPointRadius, y - kQPointRadius, kQPointRadius * 2.0f, kQPointRadius * 2.0f, 1.5f);

        const auto qLabel = bjt_curve::formatOperatingPointLabel(dcOverlay_.qPoint, bjt_curve::CurveKind::IcVsVce);
        g.setFont(11.0f);
        g.setColour(colors.label);
        const float tw = g.getCurrentFont().getStringWidthFloat(qLabel);
        float labelX = x + 10.0f;
        float labelY = y - 16.0f;
        if (labelX + tw > plotArea.getRight() - 4.0f)
            labelX = x - tw - 10.0f;
        if (labelY < plotArea.getY() + 2.0f)
            labelY = y + 10.0f;
        g.drawText(qLabel, juce::Rectangle<float>(labelX, labelY, tw + 2.0f, 14.0f), juce::Justification::centredLeft);
    }

    if (total == 0 && !busy_)
    {
        g.setFont(13.0f);
        g.setColour(colors.label);
        const juce::String message = circuit_ == bjt_curve::CircuitKind::CommonEmitter
            ? "No valid operating points for this Ib sweep."
            : "Ic vs Vce is only available for Common Emitter.\n"
              "Switch Circuit back to Common Emitter, or choose Ic vs Vbe / Ib vs Vbe.";
        g.drawFittedText(message, plotArea.toNearestInt().reduced(24), juce::Justification::centred, 4);
    }

    g.setFont(11.0f);
    g.setColour(colors.label);

    for (const float xTick : bjt_curve::buildNiceTicks(axisRange_.minX, axisRange_.maxX))
    {
        const auto label = bjt_curve::formatVoltageTick(xTick);
        const float x = dataToX(xTick);
        const float tw = g.getCurrentFont().getStringWidthFloat(label);
        g.drawText(label,
                   juce::Rectangle<float>(x - tw * 0.5f, plotArea.getBottom() + 4.0f, tw, 12.0f),
                   juce::Justification::centred);
    }

    for (const float yTick : bjt_curve::buildNiceTicks(axisRange_.minY, axisRange_.maxY))
    {
        const auto label = bjt_curve::formatCurrentTick(yTick, currentDisplayUnit_);
        const float y = dataToY(yTick);
        const float tw = g.getCurrentFont().getStringWidthFloat(label);
        g.drawText(label,
                   juce::Rectangle<float>(4.0f, y - 6.0f, juce::jmax(48.0f, tw + 4.0f), 12.0f),
                   juce::Justification::centredRight);
    }

    g.drawText(xLabel_,
               plotArea.withY(plotArea.getBottom() + 18.0f).withHeight(14.0f),
               juce::Justification::centred);
    g.drawText(yLabel_,
               juce::Rectangle<float>(4.0f, plotArea.getY() - 18.0f, 56.0f, 12.0f),
               juce::Justification::centredLeft);

    g.setFont(12.0f);
    g.setColour(colors.path);
    g.drawText(title_, getLocalBounds().removeFromTop(20).reduced(6, 0), juce::Justification::centredLeft);

    const float legendX = plotArea.getRight() - 92.0f;
    float legendY = plotArea.getY() + 6.0f;
    g.setFont(10.0f);

    if (!dcOverlay_.loadLines.empty())
    {
        g.setColour(colors.label);
        g.drawText("--- DC load line",
                   juce::Rectangle<float>(legendX + 14.0f, legendY - 1.0f, 100.0f, 12.0f),
                   juce::Justification::centredLeft);
        legendY += 14.0f;
    }

    if (dcOverlay_.qPoint.valid)
    {
        g.setColour(colors.label);
        g.fillEllipse(legendX, legendY, 10.0f, 10.0f);
        g.setColour(colors.label);
        g.drawText("Q (bias)",
                   juce::Rectangle<float>(legendX + 14.0f, legendY - 1.0f, 76.0f, 12.0f),
                   juce::Justification::centredLeft);
        legendY += 14.0f;
    }

    for (int i = 0; i < total; ++i)
    {
        const auto colour = curveColourForIndex(colors.path, i, total);
        g.setColour(colour);
        g.fillRect(legendX, legendY, 10.0f, 10.0f);
        g.setColour(colors.label);
        g.drawText("Ib " + bjt_curve::formatCurrentWithUnit(ibValues_[static_cast<size_t>(i)]),
                   juce::Rectangle<float>(legendX + 14.0f, legendY - 1.0f, 76.0f, 12.0f),
                   juce::Justification::centredLeft);
        legendY += 14.0f;
    }
}

void BjtCurvePanel::paintTransferQPoint(juce::Graphics& g)
{
    const auto& q = dcOverlay_.qPoint;
    if (!q.valid || !curveControl.isVisible())
        return;

    const float qX = q.vbe;
    const float qY = curveKind_ == bjt_curve::CurveKind::IbVsVbe ? q.ib : q.ic;
    if (qY <= 0.0f)
        return;

    // Keep the marker aligned with CurveControl's data-domain plot (includes gridRange).
    const auto plotArea = curveControl.getDataPlotBounds().toFloat();
    if (plotArea.isEmpty() || axisRange_.maxX <= axisRange_.minX || axisRange_.maxY <= axisRange_.minY)
        return;

    const float xNorm = juce::jlimit(0.0f, 1.0f, (qX - axisRange_.minX) / (axisRange_.maxX - axisRange_.minX));
    const float yNorm = juce::jlimit(0.0f, 1.0f, (qY - axisRange_.minY) / (axisRange_.maxY - axisRange_.minY));
    const float x = plotArea.getX() + xNorm * plotArea.getWidth();
    const float y = plotArea.getBottom() - yNorm * plotArea.getHeight();

    constexpr float kQPointRadius = 7.0f;
    const auto& colors = style_.colors;
    g.setColour(colors.label);
    g.fillEllipse(x - kQPointRadius, y - kQPointRadius, kQPointRadius * 2.0f, kQPointRadius * 2.0f);
    g.setColour(colors.background);
    g.drawEllipse(x - kQPointRadius, y - kQPointRadius, kQPointRadius * 2.0f, kQPointRadius * 2.0f, 1.5f);

    const auto qLabel = bjt_curve::formatOperatingPointLabel(q, curveKind_);
    g.setFont(11.0f);
    g.setColour(colors.label);
    const float tw = g.getCurrentFont().getStringWidthFloat(qLabel);
    float labelX = x + 10.0f;
    float labelY = y - 16.0f;
    if (labelX + tw > plotArea.getRight() - 4.0f)
        labelX = juce::jmax(plotArea.getX() + 4.0f, x - tw - 10.0f);
    if (labelY < plotArea.getY() + 2.0f)
        labelY = juce::jmin(plotArea.getBottom() - 16.0f, y + 10.0f);
    g.drawText(qLabel, juce::Rectangle<float>(labelX, labelY, tw + 2.0f, 14.0f), juce::Justification::centredLeft);
}

void BjtCurvePanel::paint(juce::Graphics& g)
{
    if (curveKind_ == bjt_curve::CurveKind::IcVsVce)
        paintFamilyCurves(g);
    else if (!curveControl.isVisible())
        g.fillAll(style_.colors.background);
}

void BjtCurvePanel::paintOverChildren(juce::Graphics& g)
{
    if (curveKind_ == bjt_curve::CurveKind::IcVsVbe || curveKind_ == bjt_curve::CurveKind::IbVsVbe)
        paintTransferQPoint(g);

    paintBusyOverlay(g);
}

void BjtCurvePanel::resized()
{
    curveControl.setBounds(getLocalBounds());
}
