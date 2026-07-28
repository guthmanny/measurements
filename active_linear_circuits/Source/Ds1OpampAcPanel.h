#pragma once

#include <atomic>
#include <cstdint>

#include <juce_atom_theme/juce_atom_theme.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "Ds1OpampAcMath.h"

class Ds1OpampAcPanel final : public juce::Component,
                              private juce::AsyncUpdater
{
public:
    Ds1OpampAcPanel();
    ~Ds1OpampAcPanel() override;

    void setCircuitKind(ds1_ac::CircuitKind circuitKind);
    void setOpampModel(nx_opamp_model_e model);
    void setBjtModel(nx_bjt_npn_model_e bjtModel);
    void setJfetModel(nx_jfet_n_model_e jfetModel);
    void setGainControl(double gainControl);
    void setSecondaryControl(double secondaryControl);
    void setTertiaryControl(double tertiaryControl);
    void setPotTaper(nx_pot_taper_e potTaper);
    void setPlotKind(ds1_ac::PlotKind plotKind);
    void setPreviewFrequencyHz(double freqHz);
    void setSampleRateHz(double sampleRateHz);
    void applyTheme(const atom::ThemeColors& themeColors);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    struct RebuildParams
    {
        ds1_ac::CircuitKind circuitKind{ds1_ac::CircuitKind::Ds1Opamp};
        nx_opamp_model_e model{NX_OPAMP_BA728};
        nx_bjt_npn_model_e bjtModel{NX_BJT_2N3904};
        nx_jfet_n_model_e jfetModel{NX_JFET_2N5457};
        double gainControl{0.5};
        double secondaryControl{0.5};
        double tertiaryControl{0.5};
        nx_pot_taper_e potTaper{NX_POT_TAPER_LINEAR};
        ds1_ac::PlotKind plotKind{ds1_ac::PlotKind::Magnitude};
        ds1_ac::AcSweepParams sweep{};
        ds1_ac::AxisRange magnitudeAxis{};
        double previewFreqHz{ds1_ac::kDefaultPreviewFreqHz};
        bool recomputeMagnitudeAxis{false};
        uint32_t generation{0};
    };

    struct RebuildResult
    {
        uint32_t generation{0};
        ds1_ac::PlotKind plotKind{ds1_ac::PlotKind::Magnitude};
        ds1_ac::AcResponse response;
        ds1_ac::SineWavePreview sinePreview;
        ds1_ac::AxisRange magnitudeAxis{};
        bool magnitudeAxisRecomputed{false};
    };

    void scheduleRebuild();
    void handleAsyncUpdate() override;
    static RebuildResult computeRebuild(const RebuildParams& params);
    void applyRebuildResult(RebuildResult&& result);
    void refreshCurveViews();
    void paintBusyOverlay(juce::Graphics& g);

    ds1_ac::AcResponse lastResponse_;
    ds1_ac::SineWavePreview lastSinePreview_;
    ds1_ac::AxisRange fixedMagnitudeAxis_;
    ds1_ac::CircuitKind fixedMagnitudeAxisCircuit_{ds1_ac::CircuitKind::Ds1Opamp};
    nx_opamp_model_e fixedMagnitudeAxisModel_{NX_OPAMP_BA728};
    nx_bjt_npn_model_e fixedMagnitudeAxisBjtModel_{NX_BJT_2N3904};
    nx_jfet_n_model_e fixedMagnitudeAxisJfetModel_{NX_JFET_2N5457};
    double fixedMagnitudeAxisSampleRate_{ds1_ac::kDefaultSampleRateHz};
    double fixedMagnitudeAxisSecondaryControl_{0.5};
    double fixedMagnitudeAxisTertiaryControl_{0.5};
    nx_pot_taper_e fixedMagnitudeAxisPotTaper_{NX_POT_TAPER_LINEAR};
    bool hasFixedMagnitudeAxis_{false};

    ds1_ac::CircuitKind circuitKind_{ds1_ac::CircuitKind::Ds1Opamp};
    nx_opamp_model_e model_{NX_OPAMP_BA728};
    nx_bjt_npn_model_e bjtModel_{NX_BJT_2N3904};
    nx_jfet_n_model_e jfetModel_{NX_JFET_2N5457};
    double gainControl_{0.5};
    double secondaryControl_{0.5};
    double tertiaryControl_{0.5};
    nx_pot_taper_e potTaper_{NX_POT_TAPER_LINEAR};
    double previewFreqHz_{ds1_ac::kDefaultPreviewFreqHz};
    ds1_ac::PlotKind plotKind_{ds1_ac::PlotKind::Magnitude};
    ds1_ac::AcSweepParams sweep_;

    atom::CurveControl::Style style_;
    atom::CurveControl::Style sineStyle_;
    atom::CurveControl magnitudeCurve{atom::CurveControl::Direction::Speedup};
    atom::CurveControl phaseCurve{atom::CurveControl::Direction::Speedup};
    atom::CurveControl sineWaveCurve{atom::CurveControl::Direction::Speedup};

    std::atomic<uint32_t> rebuildGeneration_{0};
    std::atomic<bool> rebuildInFlight_{false};
    bool busy_{false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Ds1OpampAcPanel)
};
