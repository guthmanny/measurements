#include "Ds1OpampAcPanel.h"

#include <atom/CurveControl.h>

#include "SineWavePreviewEngine.h"

namespace
{
bool isValidMagnitudeAxis(const ds1_ac::AxisRange& axis) noexcept
{
    return axis.maxY - axis.minY >= 6.0f;
}
std::vector<float> buildMagnitudeTicks(float minDb, float maxDb)
{
    std::vector<float> ticks;
    const float span = maxDb - minDb;
    if (span <= 0.0f)
        return ticks;

    const float step = span > 18.0f ? 6.0f : 3.0f;
    const float start = std::ceil(minDb / step) * step;
    for (float value = start; value <= maxDb + step * 0.01f; value += step)
        ticks.push_back(value);

    return ticks;
}

std::vector<float> buildPhaseTicks(float minDeg, float maxDeg)
{
    juce::ignoreUnused(minDeg, maxDeg);
    return {-180.0f, 0.0f, 180.0f};
}

void applyLogFrequencyStyle(atom::CurveControl::Style& style, double freqMinHz, double freqMaxHz)
{
    style.metrics.gridMode = atom::CurveControl::GridMode::Logarithmic;
    style.metrics.logXConfig.enabled = true;
    style.metrics.logXConfig.minValue = freqMinHz;
    style.metrics.logXConfig.maxValue = freqMaxHz;
    style.metrics.numGridDivisions = 5;
    style.metrics.plotBufferX = 8;
    style.metrics.plotBufferY = 8;
    style.metrics.pathStrokeSize = 2.25f;
    style.metrics.showXLabel = true;
    style.metrics.showYLabel = true;
    style.metrics.showTickLabels = true;
    style.metrics.frameMode = atom::CurveControl::FrameMode::All;
}

void applyLinearChartStyle(atom::CurveControl::Style& style)
{
    style.metrics.gridMode = atom::CurveControl::GridMode::Linear;
    style.metrics.logXConfig.enabled = false;
    style.metrics.numGridDivisions = 5;
    style.metrics.plotBufferX = 8;
    style.metrics.plotBufferY = 8;
    style.metrics.pathStrokeSize = 2.25f;
    style.metrics.showXLabel = true;
    style.metrics.showYLabel = true;
    style.metrics.showTickLabels = true;
    style.metrics.titleDisplayMode = atom::CurveControl::TitleDisplayMode::Show;
    style.metrics.frameMode = atom::CurveControl::FrameMode::All;
}

std::vector<float> buildPeriodTicks()
{
    return {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
}

std::vector<float> buildSupplyVoltageTicks(float vccHalf, bool groundReferenced)
{
    if (groundReferenced)
        return {0.0f, vccHalf, vccHalf * 2.0f};
    return {-vccHalf, 0.0f, vccHalf};
}

juce::Rectangle<int> plotBoundsInParent(const atom::CurveControl& curve)
{
    return curve.getDataPlotBounds().translated(curve.getX(), curve.getY());
}

void alignPlotEdgeInParent(atom::CurveControl& target,
                           const atom::CurveControl& reference,
                           bool alignRight)
{
    const auto refPlot = plotBoundsInParent(reference);
    const auto tgtPlot = plotBoundsInParent(target);
    const int delta = alignRight ? (refPlot.getRight() - tgtPlot.getRight())
                                 : (refPlot.getX() - tgtPlot.getX());
    if (delta == 0)
        return;

    target.setBounds(target.getBounds().translated(delta, 0));
}

void alignCurveControlFrames(atom::CurveControl& left, atom::CurveControl& right)
{
    const int deltaY = right.getDataPlotBounds().getY() - left.getDataPlotBounds().getY();
    const int deltaH = right.getDataPlotBounds().getHeight() - left.getDataPlotBounds().getHeight();
    if (deltaY == 0 && deltaH == 0)
        return;

    auto bounds = left.getBounds();
    left.setBounds(bounds.translated(0, deltaY).withHeight(bounds.getHeight() + deltaH));
}

void alignBottomRowWithMagnitude(atom::CurveControl& magnitude,
                                 atom::CurveControl& sine,
                                 atom::CurveControl& phase)
{
    alignCurveControlFrames(sine, phase);
    alignPlotEdgeInParent(phase, magnitude, true);
    alignPlotEdgeInParent(sine, magnitude, false);
}

void configureSineWaveCurve(atom::CurveControl& curve, const ds1_ac::SineWavePreview& preview)
{
    atom::CurveControl::DataAxis xAxis;
    xAxis.minValue = preview.axis.minX;
    xAxis.maxValue = preview.axis.maxX;
    xAxis.formatTick = [](float t) { return ds1_ac::formatPeriodTick(t); };

    atom::CurveControl::DataAxis yAxis;
    yAxis.minValue = preview.axis.minY;
    yAxis.maxValue = preview.axis.maxY;
    yAxis.formatTick = [](float amp) { return ds1_ac::formatWaveformTick(amp); };

    curve.setTitle(preview.title);
    curve.setXLabel("Period");
    curve.setYLabel("Output");
    curve.setShowXLabel(true);
    curve.setShowYLabel(true);
    curve.setDataAxes(xAxis, yAxis);
    curve.setCustomAxisTicks(buildPeriodTicks(),
                             buildSupplyVoltageTicks(preview.vccHalf, preview.groundReferenced));
    curve.setCustomCurve(preview.outputCurve);
    curve.clearSecondaryCustomCurve();
}

void configureMagnitudeCurve(atom::CurveControl& curve,
                             const ds1_ac::AcResponse& response,
                             bool overlayPhase)
{
    const float logMin = response.magnitudeAxis.minX;
    const float logMax = response.magnitudeAxis.maxX;
    const auto freqTicks = ds1_ac::buildLogFrequencyGridTicks(logMin, logMax);

    atom::CurveControl::DataAxis xAxis;
    xAxis.minValue = logMin;
    xAxis.maxValue = logMax;
    xAxis.formatTick = [](float log10Hz) { return ds1_ac::formatFrequencyTick(log10Hz); };

    atom::CurveControl::DataAxis yAxis;
    yAxis.minValue = response.magnitudeAxis.minY;
    yAxis.maxValue = response.magnitudeAxis.maxY;
    yAxis.formatTick = [](float magDb) { return ds1_ac::formatMagnitudeTick(magDb); };

    curve.setTitle(response.title + (overlayPhase ? "  |  Magnitude" : "  |  Magnitude"));
    curve.setXLabel("Frequency");
    curve.setYLabel("Magnitude");
    curve.setDataAxes(xAxis, yAxis);
    curve.setCustomAxisTicks(freqTicks, buildMagnitudeTicks(yAxis.minValue, yAxis.maxValue));
    curve.setCustomCurve(response.magnitudeCurve);
    curve.clearSecondaryCustomCurve();
}

void configurePhaseCurve(atom::CurveControl& curve, const ds1_ac::AcResponse& response)
{
    const float logMin = response.phaseAxis.minX;
    const float logMax = response.phaseAxis.maxX;
    const auto freqTicks = ds1_ac::buildLogFrequencyGridTicks(logMin, logMax);

    atom::CurveControl::DataAxis xAxis;
    xAxis.minValue = logMin;
    xAxis.maxValue = logMax;
    xAxis.formatTick = [](float log10Hz) { return ds1_ac::formatFrequencyTick(log10Hz); };

    atom::CurveControl::DataAxis yAxis;
    yAxis.minValue = response.phaseAxis.minY;
    yAxis.maxValue = response.phaseAxis.maxY;
    yAxis.formatTick = [](float phase) { return ds1_ac::formatPhaseTick(phase); };

    curve.setTitle(response.title + "  |  Phase");
    curve.setXLabel("Frequency");
    curve.setYLabel("Phase");
    curve.setDataAxes(xAxis, yAxis);
    curve.setCustomAxisTicks(freqTicks, buildPhaseTicks(yAxis.minValue, yAxis.maxValue));
    curve.setCustomCurve(response.phaseCurve);
    curve.clearSecondaryCustomCurve();
}

void layoutPhaseRow(juce::Rectangle<int> area,
                    atom::CurveControl& sineWaveCurve,
                    atom::CurveControl& phaseCurve)
{
    const int gap = 8;
    const int sineWidth = juce::jmax(220, area.getWidth() * 2 / 5);
    sineWaveCurve.setBounds(area.removeFromLeft(sineWidth));
    area.removeFromLeft(gap);
    phaseCurve.setBounds(area);
    alignCurveControlFrames(sineWaveCurve, phaseCurve);
}
}  // namespace

Ds1OpampAcPanel::Ds1OpampAcPanel()
{
    setOpaque(true);

    magnitudeCurve.setMode(atom::CurveControl::Mode::DisplayOnly);
    phaseCurve.setMode(atom::CurveControl::Mode::DisplayOnly);
    sineWaveCurve.setMode(atom::CurveControl::Mode::DisplayOnly);

    addAndMakeVisible(magnitudeCurve);
    addChildComponent(phaseCurve);
    addChildComponent(sineWaveCurve);

    sweep_.freqMinHz = ds1_ac::kDefaultFreqMinHz;
    sweep_.freqMaxHz = ds1_ac::kDefaultFreqMaxHz;
    sweep_.sampleRateHz = ds1_ac::kDefaultSampleRateHz;

    previewEngine_ = std::make_unique<ds1_ac::SineWavePreviewEngine>();

    scheduleRebuild();
}

void Ds1OpampAcPanel::waitForWorkers()
{
    for (int i = 0; i < 2000 && outstandingWorkers_.load(std::memory_order_acquire) > 0; ++i)
        juce::Thread::sleep(1);
}

Ds1OpampAcPanel::~Ds1OpampAcPanel()
{
    rebuildGeneration_.fetch_add(1, std::memory_order_acq_rel);
    cancelPendingUpdate();
    waitForWorkers();
}

void Ds1OpampAcPanel::setCircuitKind(ds1_ac::CircuitKind circuitKind)
{
    if (circuitKind_ == circuitKind)
        return;

    circuitKind_ = circuitKind;
    schematicValues_ = {};
    hasFixedMagnitudeAxis_ = false;
    lastResponse_ = {};
    lastSinePreview_ = {};
    if (previewEngine_ != nullptr)
        previewEngine_->invalidate();
    refreshCurveViews();
    scheduleRebuild();
}

void Ds1OpampAcPanel::setOpampModel(nx_opamp_model_e model)
{
    if (model_ == model)
        return;

    model_ = model;
    hasFixedMagnitudeAxis_ = false;
    scheduleRebuild();
}

void Ds1OpampAcPanel::setDiodeModel(nx_diode_model_t diodeModel)
{
    if (diodeModel_ == diodeModel)
        return;

    diodeModel_ = diodeModel;
    hasFixedMagnitudeAxis_ = false;
    scheduleRebuild();
}

void Ds1OpampAcPanel::setBjtModel(nx_bjt_npn_model_e bjtModel)
{
    if (bjtModel_ == bjtModel)
        return;

    bjtModel_ = bjtModel;
    hasFixedMagnitudeAxis_ = false;
    scheduleRebuild();
}

void Ds1OpampAcPanel::setJfetModel(nx_jfet_n_model_e jfetModel)
{
    if (jfetModel_ == jfetModel)
        return;

    jfetModel_ = jfetModel;
    hasFixedMagnitudeAxis_ = false;
    scheduleRebuild();
}

void Ds1OpampAcPanel::setGainControl(double gainControl)
{
    const double next = juce::jlimit(0.0, 1.0, gainControl);
    if (juce::approximatelyEqual(gainControl_, next))
        return;

    gainControl_ = next;
    scheduleRebuild();
}

void Ds1OpampAcPanel::setSecondaryControl(double secondaryControl)
{
    const double next = juce::jlimit(0.0, 1.0, secondaryControl);
    if (juce::approximatelyEqual(secondaryControl_, next))
        return;

    secondaryControl_ = next;
    if (ds1_ac::circuitHasSecondaryControl(circuitKind_))
        hasFixedMagnitudeAxis_ = false;
    scheduleRebuild();
}

void Ds1OpampAcPanel::setTertiaryControl(double tertiaryControl)
{
    const double next = juce::jlimit(0.0, 1.0, tertiaryControl);
    if (juce::approximatelyEqual(tertiaryControl_, next))
        return;

    tertiaryControl_ = next;
    if (ds1_ac::circuitHasTertiaryControl(circuitKind_))
        hasFixedMagnitudeAxis_ = false;
    scheduleRebuild();
}

void Ds1OpampAcPanel::setPotTaper(nx_pot_taper_e potTaper)
{
    if (potTaper_ == potTaper)
        return;

    potTaper_ = potTaper;
    hasFixedMagnitudeAxis_ = false;
    scheduleRebuild();
}

void Ds1OpampAcPanel::setPreviewFrequencyHz(double freqHz)
{
    previewFreqHz_ = juce::jlimit(ds1_ac::kPreviewFreqMinHz, ds1_ac::kPreviewFreqMaxHz, freqHz);
}

void Ds1OpampAcPanel::setPreviewAmplitude(double amplitude)
{
    previewAmplitude_ = juce::jlimit(ds1_ac::kPreviewAmpMin, ds1_ac::kPreviewAmpMax, amplitude);
}

void Ds1OpampAcPanel::refreshPreview()
{
    recomputePreviewNow();
}

void Ds1OpampAcPanel::setSampleRateHz(double sampleRateHz)
{
    const double next = juce::jmax(1000.0, sampleRateHz);
    if (juce::approximatelyEqual(sweep_.sampleRateHz, next))
        return;

    sweep_.sampleRateHz = next;
    hasFixedMagnitudeAxis_ = false;
    scheduleRebuild();
}

void Ds1OpampAcPanel::setSchematicComponentValues(const ds1_ac::SchematicComponentValues& values)
{
    if (schematicValues_ == values)
        return;

    schematicValues_ = values;
    hasFixedMagnitudeAxis_ = false;
    scheduleRebuild();
}

void Ds1OpampAcPanel::setPlotKind(ds1_ac::PlotKind plotKind)
{
    if (plotKind_ == plotKind)
        return;

    plotKind_ = plotKind;
    refreshCurveViews();
    resized();
    repaint();

    if (plotKind_ != ds1_ac::PlotKind::Magnitude && lastSinePreview_.outputCurve.empty())
    {
        if (rebuildBatchDepth_ > 0)
            rebuildQueuedDuringBatch_ = true;
        else
            launchPreviewJob(buildRebuildParams());
    }
}

void Ds1OpampAcPanel::applyTheme(const atom::ThemeColors& themeColors)
{
    style_ = atom::CurveControl::Style::fromTheme(themeColors);
    applyLogFrequencyStyle(style_, sweep_.freqMinHz, sweep_.freqMaxHz);

    sineStyle_ = style_;
    applyLinearChartStyle(sineStyle_);

    magnitudeCurve.setStyle(style_);
    phaseCurve.setStyle(style_);
    sineWaveCurve.setStyle(sineStyle_);
    repaint();
}

void Ds1OpampAcPanel::beginRebuildBatch()
{
    ++rebuildBatchDepth_;
}

void Ds1OpampAcPanel::endRebuildBatch()
{
    if (rebuildBatchDepth_ <= 0)
        return;

    --rebuildBatchDepth_;
    if (rebuildBatchDepth_ > 0 || ! rebuildQueuedDuringBatch_)
        return;

    rebuildQueuedDuringBatch_ = false;
    cancelPendingUpdate();
    launchIsolatedStageJobs();
}

void Ds1OpampAcPanel::scheduleRebuild()
{
    rebuildGeneration_.fetch_add(1, std::memory_order_acq_rel);
    if (rebuildBatchDepth_ > 0)
    {
        rebuildQueuedDuringBatch_ = true;
        return;
    }

    if (! ds1_ac::circuitAcSweepIsCheap(circuitKind_))
        busy_ = true;
    triggerAsyncUpdate();
    repaint();
}

Ds1OpampAcPanel::RebuildParams Ds1OpampAcPanel::buildRebuildParams() const
{
    RebuildParams params;
    params.circuitKind = circuitKind_;
    params.model = model_;
    params.diodeModel = diodeModel_;
    params.bjtModel = bjtModel_;
    params.jfetModel = jfetModel_;
    params.gainControl = gainControl_;
    params.secondaryControl = secondaryControl_;
    params.tertiaryControl = tertiaryControl_;
    params.potTaper = potTaper_;
    params.plotKind = plotKind_;
    params.sweep = sweep_;
    params.previewFreqHz = previewFreqHz_;
    params.previewAmplitude = previewAmplitude_;
    params.schematicValues = schematicValues_;
    params.recomputeMagnitudeAxis = !hasFixedMagnitudeAxis_
                                 || !isValidMagnitudeAxis(fixedMagnitudeAxis_)
                                 || circuitKind_ != fixedMagnitudeAxisCircuit_
                                 || model_ != fixedMagnitudeAxisModel_
                                 || diodeModel_ != fixedMagnitudeAxisDiodeModel_
                                 || bjtModel_ != fixedMagnitudeAxisBjtModel_
                                 || jfetModel_ != fixedMagnitudeAxisJfetModel_
                                 || !juce::approximatelyEqual(sweep_.sampleRateHz, fixedMagnitudeAxisSampleRate_)
                                 || !juce::approximatelyEqual(secondaryControl_, fixedMagnitudeAxisSecondaryControl_)
                                 || !juce::approximatelyEqual(tertiaryControl_, fixedMagnitudeAxisTertiaryControl_)
                                 || potTaper_ != fixedMagnitudeAxisPotTaper_
                                 || schematicValues_ != fixedMagnitudeAxisSchematicValues_;
    params.magnitudeAxis = fixedMagnitudeAxis_;
    params.cachedResponse = lastResponse_;
    params.generation = rebuildGeneration_.load(std::memory_order_acquire);
    return params;
}

void Ds1OpampAcPanel::recomputePreviewNow()
{
    if (plotKind_ == ds1_ac::PlotKind::Magnitude)
        return;

    auto params = buildRebuildParams();
    params.previewOnly = true;

    if (previewEngine_ == nullptr)
        previewEngine_ = std::make_unique<ds1_ac::SineWavePreviewEngine>();

    applyRebuildResult(computeRebuild(params));
}

void Ds1OpampAcPanel::launchPreviewJob(RebuildParams params)
{
    if (params.plotKind == ds1_ac::PlotKind::Magnitude)
        return;

    params.previewOnly = true;
    outstandingWorkers_.fetch_add(1, std::memory_order_acq_rel);

    juce::Component::SafePointer<Ds1OpampAcPanel> safe(this);
    juce::Thread::launch([safe, params = std::move(params)]()
    {
        ds1_ac::SineWavePreviewEngine engine;
        RebuildResult result;
        result.generation = params.generation;
        result.plotKind = params.plotKind;
        result.previewOnly = true;
        result.response = params.cachedResponse;
        result.magnitudeAxis = params.magnitudeAxis;

        const auto* componentValues = params.schematicValues.empty() ? nullptr : &params.schematicValues;
        result.sinePreview = ds1_ac::computeSineWavePreview(params.circuitKind,
                                                            params.model,
                                                            params.diodeModel,
                                                            params.bjtModel,
                                                            params.jfetModel,
                                                            params.gainControl,
                                                            params.previewFreqHz,
                                                            params.previewAmplitude,
                                                            params.sweep,
                                                            params.secondaryControl,
                                                            params.tertiaryControl,
                                                            params.potTaper,
                                                            engine,
                                                            componentValues);

        if (safe != nullptr)
            safe->outstandingWorkers_.fetch_sub(1, std::memory_order_acq_rel);

        juce::MessageManager::callAsync([safe, result = std::move(result)]() mutable
        {
            if (safe == nullptr)
                return;
            if (result.generation != safe->rebuildGeneration_.load(std::memory_order_acquire))
                return;

            safe->applyRebuildResult(std::move(result));
        });
    });
}

void Ds1OpampAcPanel::launchAcJob(RebuildParams params)
{
    params.previewOnly = false;

    if (ds1_ac::circuitAcSweepIsCheap(params.circuitKind))
    {
        applyRebuildResult(computeRebuild(params));
        return;
    }

    outstandingWorkers_.fetch_add(1, std::memory_order_acq_rel);
    juce::Component::SafePointer<Ds1OpampAcPanel> safe(this);
    juce::Thread::launch([safe, params = std::move(params)]()
    {
        RebuildResult result{};
        if (safe != nullptr)
            result = safe->computeRebuild(params);

        if (safe != nullptr)
            safe->outstandingWorkers_.fetch_sub(1, std::memory_order_acq_rel);

        juce::MessageManager::callAsync([safe, result = std::move(result)]() mutable
        {
            if (safe == nullptr)
                return;
            if (result.generation != safe->rebuildGeneration_.load(std::memory_order_acquire))
                return;

            safe->applyRebuildResult(std::move(result));
        });
    });
}

void Ds1OpampAcPanel::launchIsolatedStageJobs()
{
    auto params = buildRebuildParams();
    if (! ds1_ac::circuitAcSweepIsCheap(params.circuitKind))
    {
        busy_ = true;
        repaint();
    }
    launchAcJob(params);
    launchPreviewJob(params);
}

void Ds1OpampAcPanel::handleAsyncUpdate()
{
    launchIsolatedStageJobs();
}

Ds1OpampAcPanel::RebuildResult Ds1OpampAcPanel::computeRebuild(const RebuildParams& params)
{
    RebuildResult result;
    result.generation = params.generation;
    result.plotKind = params.plotKind;
    result.previewOnly = params.previewOnly;

    const auto* componentValues = params.schematicValues.empty() ? nullptr : &params.schematicValues;

    if (params.previewOnly)
    {
        result.response = params.cachedResponse;
        result.magnitudeAxis = params.magnitudeAxis;
        result.magnitudeAxisRecomputed = false;
    }
    else
    {
        result.magnitudeAxis = params.recomputeMagnitudeAxis
            ? ds1_ac::computeMagnitudeAxisEnvelope(params.circuitKind,
                                                   params.model,
                                                   params.diodeModel,
                                                   params.bjtModel,
                                                   params.jfetModel,
                                                   params.sweep,
                                                   params.secondaryControl,
                                                   params.tertiaryControl,
                                                   params.potTaper,
                                                   componentValues)
            : params.magnitudeAxis;
        result.magnitudeAxisRecomputed = params.recomputeMagnitudeAxis;

        result.response = ds1_ac::computeAcResponse(params.circuitKind,
                                                      params.model,
                                                      params.diodeModel,
                                                      params.bjtModel,
                                                      params.jfetModel,
                                                      params.gainControl,
                                                      params.sweep,
                                                      result.magnitudeAxis,
                                                      params.secondaryControl,
                                                      params.tertiaryControl,
                                                      params.potTaper,
                                                      componentValues);

        if (params.recomputeMagnitudeAxis
            && !ds1_ac::circuitHasPrimaryControl(params.circuitKind)
            && !ds1_ac::circuitHasSecondaryControl(params.circuitKind))
        {
            result.magnitudeAxis = ds1_ac::magnitudeAxisFromCurve(result.response.magnitudeCurve, params.sweep);
            result.response.magnitudeAxis = result.magnitudeAxis;
        }
    }

    if (params.previewOnly)
    {
        if (previewEngine_ == nullptr)
            previewEngine_ = std::make_unique<ds1_ac::SineWavePreviewEngine>();

        result.sinePreview = ds1_ac::computeSineWavePreview(params.circuitKind,
                                                            params.model,
                                                            params.diodeModel,
                                                            params.bjtModel,
                                                            params.jfetModel,
                                                            params.gainControl,
                                                            params.previewFreqHz,
                                                            params.previewAmplitude,
                                                            params.sweep,
                                                            params.secondaryControl,
                                                            params.tertiaryControl,
                                                            params.potTaper,
                                                            *previewEngine_,
                                                            componentValues);
    }

    return result;
}

void Ds1OpampAcPanel::refreshCurveViews()
{
    if (hasFixedMagnitudeAxis_)
        lastResponse_.magnitudeAxis = fixedMagnitudeAxis_;

    const bool showBoth = plotKind_ == ds1_ac::PlotKind::Both;
    const bool showMagnitude = plotKind_ != ds1_ac::PlotKind::Phase;
    const bool showPhase = plotKind_ == ds1_ac::PlotKind::Phase || showBoth;
    const bool showSinePreview = showPhase;

    magnitudeCurve.setVisible(showMagnitude);
    phaseCurve.setVisible(showPhase);
    sineWaveCurve.setVisible(showSinePreview);

    if (showMagnitude && !lastResponse_.magnitudeCurve.empty())
        configureMagnitudeCurve(magnitudeCurve, lastResponse_, showBoth);

    if (showPhase && !lastResponse_.phaseCurve.empty())
        configurePhaseCurve(phaseCurve, lastResponse_);

    if (showSinePreview && !lastSinePreview_.outputCurve.empty())
        configureSineWaveCurve(sineWaveCurve, lastSinePreview_);
}

void Ds1OpampAcPanel::applyRebuildResult(RebuildResult&& result)
{
    busy_ = false;
    plotKind_ = result.plotKind;
    if (! result.previewOnly)
        lastResponse_ = std::move(result.response);

    if (result.previewOnly)
        lastSinePreview_ = std::move(result.sinePreview);

    if (result.magnitudeAxisRecomputed && isValidMagnitudeAxis(result.magnitudeAxis))
    {
        fixedMagnitudeAxis_ = result.magnitudeAxis;
        fixedMagnitudeAxisCircuit_ = circuitKind_;
        fixedMagnitudeAxisModel_ = model_;
        fixedMagnitudeAxisDiodeModel_ = diodeModel_;
        fixedMagnitudeAxisBjtModel_ = bjtModel_;
        fixedMagnitudeAxisJfetModel_ = jfetModel_;
        fixedMagnitudeAxisSampleRate_ = sweep_.sampleRateHz;
        fixedMagnitudeAxisSecondaryControl_ = secondaryControl_;
        fixedMagnitudeAxisTertiaryControl_ = tertiaryControl_;
        fixedMagnitudeAxisPotTaper_ = potTaper_;
        fixedMagnitudeAxisSchematicValues_ = schematicValues_;
        hasFixedMagnitudeAxis_ = true;
    }
    refreshCurveViews();
    resized();
    repaint();
}

void Ds1OpampAcPanel::paintBusyOverlay(juce::Graphics& g)
{
    if (!busy_)
        return;

    g.setColour(style_.colors.background.withAlpha(0.35f));
    g.fillRect(getLocalBounds());
    g.setFont(13.0f);
    g.setColour(style_.colors.label);
    g.drawText("Computing AC response...", getLocalBounds(), juce::Justification::centred);
}

void Ds1OpampAcPanel::paint(juce::Graphics& g)
{
    g.fillAll(style_.colors.background);
    paintBusyOverlay(g);
}

void Ds1OpampAcPanel::resized()
{
    auto area = getLocalBounds();

    if (plotKind_ == ds1_ac::PlotKind::Both)
    {
        magnitudeCurve.setBounds(area.removeFromTop(area.getHeight() * 3 / 5));
        area.removeFromTop(8);
        layoutPhaseRow(area, sineWaveCurve, phaseCurve);
        alignBottomRowWithMagnitude(magnitudeCurve, sineWaveCurve, phaseCurve);
        return;
    }

    if (plotKind_ == ds1_ac::PlotKind::Phase)
    {
        layoutPhaseRow(area, sineWaveCurve, phaseCurve);
        alignCurveControlFrames(sineWaveCurve, phaseCurve);
        return;
    }

    magnitudeCurve.setBounds(area);
}
