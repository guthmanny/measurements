#pragma once

#include <utility>
#include <vector>

#include <juce_core/juce_core.h>

#include "nudsp/common/components.h"

namespace bjt_curve
{

enum class CircuitKind
{
    CommonEmitter,
    Follower,
    FollowerOut
};

enum class CurveKind
{
    IcVsVbe,
    IbVsVbe,
    IcVsVce
};

struct IbSweepParams
{
    // Defaults keep beta*Ib below NuDSP CE load-line ~0.9 mA (Bf~100 -> Ib≲8 uA).
    int count{5};
    float minAmps{1.0e-6f};
    float stepAmps{1.5e-6f};

    void sanitise() noexcept
    {
        count = juce::jlimit(1, 20, count);
        minAmps = juce::jmax(0.5e-6f, minAmps);
        stepAmps = juce::jmax(0.5e-6f, stepAmps);
    }
};

struct AxisRange
{
    float minX{0.0f};
    float maxX{1.0f};
    float minY{0.0f};
    float maxY{1.0f};
};

struct DcOperatingPoint
{
    bool valid{false};
    float vbe{0.0f};
    float vce{0.0f};
    float ib{0.0f};
    float ic{0.0f};
};

struct DcLoadLineOverlay
{
    std::vector<std::vector<std::pair<float, float>>> loadLines;
    // Single static bias point of the CE network (design_core DC), not one per Ib curve.
    DcOperatingPoint qPoint{};
};

// Fixed-bias DC operating point from the selected circuit's design_core.
DcOperatingPoint computeStaticOperatingPoint(nx_bjt_npn_model_e model, CircuitKind circuit);

const nx_bjt_t& modelParams(nx_bjt_npn_model_e model) noexcept;
const char* modelDisplayName(nx_bjt_npn_model_e model) noexcept;
const char* circuitDisplayName(CircuitKind circuit) noexcept;

juce::String formatModelSummary(const nx_bjt_t& bjt) noexcept;
juce::String formatIbSweepSummary(const IbSweepParams& params);

void curveAxisLabels(CurveKind kind, juce::String& xLabel, juce::String& yLabel);

enum class CurrentDisplayUnit
{
    Amps,
    Milliamps,
    Microamps,
    Nanoamps
};

CurrentDisplayUnit chooseCurrentDisplayUnit(float maxAbsAmps) noexcept;
juce::String currentAxisLabel(CurrentDisplayUnit unit, const juce::String& quantity = "Ic") noexcept;
juce::String formatCurrentTick(float amps, CurrentDisplayUnit unit) noexcept;
juce::String formatCurrentWithUnit(float amps);

std::vector<float> buildIbSweepValues(const IbSweepParams& params);

constexpr float kDefaultMinVce = 0.0f;
constexpr float kDefaultMaxVce = 5.0f;
constexpr float kDefaultMinVbe = 0.0f;
constexpr float kDefaultMaxVbe = 0.80f;
constexpr float kDefaultMaxIcAmps = 10.0e-3f;   // Ic vs Vbe Y-axis default
constexpr float kDefaultMaxIbAmps = 100.0e-6f;  // Ib vs Vbe Y-axis default

std::vector<std::pair<float, float>> buildCurve(nx_bjt_npn_model_e model,
                                                CircuitKind circuit,
                                                CurveKind kind,
                                                float ibAmps,
                                                int numPoints = 256,
                                                float vceMaxVolts = kDefaultMaxVce,
                                                float vbeMaxVolts = kDefaultMaxVbe);

std::vector<std::vector<std::pair<float, float>>> buildIcVsVceFamily(nx_bjt_npn_model_e model,
                                                                      CircuitKind circuit,
                                                                      const std::vector<float>& ibValues,
                                                                      int numPoints = 256,
                                                                      float vceMaxVolts = kDefaultMaxVce);

AxisRange computeAxisRange(CurveKind kind,
                           float ibAmps,
                           const std::vector<std::pair<float, float>>& samples);

// Force transfer-curve axes to the user-selected Vbe / current window (expands to include Q).
void applyTransferAxisLimits(AxisRange& range,
                             CurveKind kind,
                             float vbeMaxVolts,
                             float currentMaxAmps,
                             const DcOperatingPoint& qPoint = {}) noexcept;

juce::String formatOperatingPointLabel(const DcOperatingPoint& q, CurveKind kind);

AxisRange computeAxisRangeForFamily(const std::vector<std::vector<std::pair<float, float>>>& families);

DcLoadLineOverlay buildCommonEmitterDcLoadLineOverlay(nx_bjt_npn_model_e model,
                                                      const std::vector<float>& ibValues,
                                                      const std::vector<std::vector<std::pair<float, float>>>& families,
                                                      float vceMaxVolts = kDefaultMaxVce);

void expandAxisRangeForOverlay(AxisRange& range, const DcLoadLineOverlay& overlay, float vceMaxVolts);

// Clip a polyline segment to the axis rectangle (keeps slope; extends/trims to plot edges).
std::vector<std::pair<float, float>> clipSegmentToAxisRange(const std::vector<std::pair<float, float>>& segment,
                                                            const AxisRange& range);

void clipOverlayLoadLinesToAxis(DcLoadLineOverlay& overlay, const AxisRange& range);

std::vector<float> buildNiceTicks(float minVal, float maxVal, int targetCount = 5);

juce::String formatVoltageTick(float volts);
juce::String formatCurrentTick(float amps);

}  // namespace bjt_curve