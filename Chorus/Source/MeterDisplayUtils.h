#pragma once

#include <juce_atom_theme/juce_atom_theme.h>

#include <array>
#include <cmath>

namespace meter_display
{
inline constexpr float kDbMax = 0.0f;
inline constexpr float kLinearFloor = 1.0e-6f;
inline constexpr int kRefreshHz = 60;
inline constexpr std::array<int, 6> kDisplayRangeDbChoices { 20, 40, 60, 80, 100, 120 };

inline int displayRangeDbFromChoiceIndex (int choiceIndex) noexcept
{
    const int idx = juce::jlimit (0, (int) kDisplayRangeDbChoices.size() - 1, choiceIndex);
    return kDisplayRangeDbChoices[(size_t) idx];
}

inline float linearToDb (float linear) noexcept
{
    return 20.0f * std::log10 (juce::jmax (linear, kLinearFloor));
}

inline float linearToDbNormalized (float linear, float dbMin, float dbMax = kDbMax) noexcept
{
    const float db = juce::jlimit (dbMin, dbMax, linearToDb (linear));
    const float span = dbMax - dbMin;
    return span > 0.0f ? (db - dbMin) / span : 0.0f;
}

inline float attackRateFromMs (float attackMs, float dt) noexcept
{
    if (attackMs <= 0.0f)
        return 1.0f;
    return juce::jlimit (0.0f, 1.0f, 1.0f - std::exp (-dt / (attackMs * 0.001f)));
}

inline float releaseRateFromSec (float releaseSec, float dt) noexcept
{
    releaseSec = juce::jmax (1.0e-6f, releaseSec);
    return juce::jlimit (0.0f, 1.0f, 1.0f - std::exp (-dt / releaseSec));
}

inline float stepEnvelope (float current, float target, float attackMs, float releaseSec, float dt) noexcept
{
    const float attackRate = attackRateFromMs (attackMs, dt);
    const float releaseRate = releaseRateFromSec (releaseSec, dt);

    if (target >= current)
        return attackRate >= 0.999f ? target : current + (target - current) * attackRate;

    return current + (target - current) * releaseRate;
}

inline void configurePeakMeter (atom::MeterBar& meter, int barCount)
{
    meter.setBarCount (barCount);
    meter.setOrientation (atom::MeterBar::Orientation::Vertical);
    meter.setPeakHoldEnabled (true);
    meter.setSegmentCount (0);
    meter.setRefreshRateHz (kRefreshHz);
    meter.setPeakHoldTimeMs (0);
    meter.setPeakAttackTimeMs (0);
    meter.setPeakReleaseRate (1.0f);
    meter.setValueSuffix (" dB");
    meter.setValueDecimals (0);

    atom::MeterBarStyleOverride style;
    style.colors.peak = juce::Colours::white;
    style.metrics.peakThickness = 0.0f;
    style.metrics.roundness = 0.0f;
    style.metrics.outerPadding = 2.0f;
    style.metrics.barGap = barCount > 1 ? 1.0f : 0.0f;
    style.metrics.segmentGap = 0.0f;
    style.metrics.clipZoneThreshold = 0.95f;
    style.metrics.clipHoldTimeSec = 3.0f;
    meter.setStyleOverride (style);
}

inline void applyMeterValueRange (atom::MeterBar& meter, int displayRangeDbSpan)
{
    const float dbMin = -(float) juce::jmax (1, displayRangeDbSpan);
    meter.setValueRange ((double) dbMin, (double) kDbMax);
}

} // namespace meter_display
