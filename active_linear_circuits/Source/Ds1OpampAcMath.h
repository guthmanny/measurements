#pragma once

#include <utility>
#include <vector>

#include <juce_core/juce_core.h>

#include "nudsp/linear_circuits/ac_booster_eq_f32.h"
#include "nudsp/linear_circuits/ds1_opamp_f32.h"
#include "nudsp/linear_circuits/ds1_tone_f32.h"
#include "nudsp/linear_circuits/ds_plus_opamp_f32.h"
#include "nudsp/linear_circuits/guvnor_level_f32.h"
#include "nudsp/linear_circuits/guvnor_opamp_f32.h"
#include "nudsp/linear_circuits/guvnor_postamp_f32.h"
#include "nudsp/linear_circuits/guvnor_preamp_f32.h"
#include "nudsp/linear_circuits/klon_centaur_tone_f32.h"
#include "nudsp/linear_circuits/rat_opamp_f32.h"
#include "nudsp/linear_circuits/ts9_tone_f32.h"
#include "nudsp/nonlinear_circuits/ac_booster_drive_f32.h"
#include "nudsp/nonlinear_circuits/diode_clipper_f32.h"
#include "nudsp/nonlinear_circuits/ds1_clipper_f32.h"
#include "nudsp/nonlinear_circuits/guvnor_clipper_f32.h"
#include "nudsp/nonlinear_circuits/klon_centaur_f32.h"
#include "nudsp/nonlinear_circuits/bjt_common_emitter_f32.h"
#include "nudsp/nonlinear_circuits/bjt_follower_f32.h"
#include "nudsp/nonlinear_circuits/bjt_follower_out_f32.h"
#include "nudsp/nonlinear_circuits/jfet_follower_f32.h"
#include "nudsp/nonlinear_circuits/rat_clipper_f32.h"
#include "nudsp/nonlinear_circuits/ts9_opamp_f32.h"

namespace ds1_ac
{

    enum class CircuitKind
    {
        Ds1Opamp,
        RatOpamp,
        GuvnorPreamp,
        GuvnorPostamp,
        GuvnorOpamp,
        GuvnorLevel,
        Ts9Tone,
        Ds1Tone,
        DsPlusOpamp,
        KlonCentaurTone,
        AcBoosterEq,
        Ds1Clipper,
        DiodeClipper,
        RatClipper,
        Ts9Opamp,
        AcBoosterDrive,
        KlonCentaur,
        GuvnorClipper,
        BjtFollower,
        BjtFollowerOut,
        BjtCommonEmitter,
        JfetFollower
    };

    enum class PlotKind
    {
        Magnitude,
        Phase,
        Both
    };

    struct AcSweepParams
    {
        double freqMinHz{20.0};
        double freqMaxHz{20000.0};
        int numPoints{256};
        double sampleRateHz{96000.0};

        void sanitise() noexcept;
    };

    struct AxisRange
    {
        float minX{0.0f};
        float maxX{1.0f};
        float minY{0.0f};
        float maxY{1.0f};
    };

    struct AcResponse
    {
        std::vector<std::pair<float, float>> magnitudeCurve;
        std::vector<std::pair<float, float>> phaseCurve;
        AxisRange magnitudeAxis;
        AxisRange phaseAxis;
        juce::String title;
    };

    struct SineWavePreview
    {
        std::vector<std::pair<float, float>> outputCurve;
        AxisRange axis;
        float vccHalf{4.5f};
        juce::String title;
    };

    constexpr double kDefaultFreqMinHz = 20.0;
    constexpr double kDefaultFreqMaxHz = 20000.0;
    constexpr double kDefaultSampleRateHz = 96000.0;
    constexpr double kSampleRate48kHz = 48000.0;
    constexpr double kSampleRate96kHz = 96000.0;
    constexpr double kSampleRate192kHz = 192000.0;
    constexpr double kSampleRate384kHz = 384000.0;
    constexpr double kPreviewFreqMinHz = 20.0;
    constexpr double kPreviewFreqMaxHz = 10000.0;
    constexpr double kDefaultPreviewFreqHz = 1000.0;

    const char *circuitDisplayName(CircuitKind circuit) noexcept;
    const char *controlParameterName(CircuitKind circuit) noexcept;
    const char *secondaryControlParameterName(CircuitKind circuit) noexcept;
    const char *tertiaryControlParameterName(CircuitKind circuit) noexcept;
    const char *circuitProcessFunctionName(CircuitKind circuit) noexcept;
    bool circuitUsesOpampModel(CircuitKind circuit) noexcept;
    bool circuitUsesBjtModel(CircuitKind circuit) noexcept;
    bool circuitUsesJfetModel(CircuitKind circuit) noexcept;
    bool circuitHasPrimaryControl(CircuitKind circuit) noexcept;
    bool circuitUsesPotTaper(CircuitKind circuit) noexcept;
    bool circuitHasSecondaryControl(CircuitKind circuit) noexcept;
    bool circuitHasTertiaryControl(CircuitKind circuit) noexcept;
    nx_pot_taper_e defaultPotTaper(CircuitKind circuit) noexcept;
    const char *potTaperDisplayName(nx_pot_taper_e taper) noexcept;
    int potTaperComboId(nx_pot_taper_e taper) noexcept;
    nx_pot_taper_e potTaperFromComboId(int comboId) noexcept;
    const char *opampModelDisplayName(nx_opamp_model_e model) noexcept;
    const char *bjtModelDisplayName(nx_bjt_npn_model_e model) noexcept;
    const char *jfetModelDisplayName(nx_jfet_n_model_e model) noexcept;
    nx_bjt_npn_model_e defaultBjtModel(CircuitKind circuit) noexcept;
    nx_jfet_n_model_e defaultJfetModel(CircuitKind circuit) noexcept;
    int bjtModelComboId(nx_bjt_npn_model_e model) noexcept;
    nx_bjt_npn_model_e bjtModelFromComboId(int comboId) noexcept;
    int jfetModelComboId(nx_jfet_n_model_e model) noexcept;
    nx_jfet_n_model_e jfetModelFromComboId(int comboId) noexcept;
    juce::String formatFrequencyTick(float log10Hz) noexcept;
    juce::String formatMagnitudeTick(float magDb) noexcept;
    juce::String formatPhaseTick(float phaseDeg) noexcept;
    juce::String formatPeriodTick(float normalizedPeriod) noexcept;
    juce::String formatWaveformTick(float amplitude) noexcept;

    std::vector<double> buildLogFrequencySweep(const AcSweepParams &params);
    std::vector<float> buildLogFrequencyGridTicks(float logMin, float logMax, int targetCount = 6);

    /** Magnitude Y-axis envelope from primary control at 0 and 1 (fixed while control is swept). */
    AxisRange computeMagnitudeAxisEnvelope(CircuitKind circuit,
                                           nx_opamp_model_e model,
                                           nx_bjt_npn_model_e bjtModel,
                                           nx_jfet_n_model_e jfetModel,
                                           const AcSweepParams &params,
                                           double secondaryControl,
                                           double tertiaryControl,
                                           nx_pot_taper_e potTaper);

    AcResponse computeAcResponse(CircuitKind circuit,
                                 nx_opamp_model_e model,
                                 nx_bjt_npn_model_e bjtModel,
                                 nx_jfet_n_model_e jfetModel,
                                 double gainControl,
                                 const AcSweepParams &params,
                                 const AxisRange &magnitudeAxis,
                                 double secondaryControl,
                                 double tertiaryControl,
                                 nx_pot_taper_e potTaper);

    SineWavePreview computeSineWavePreview(CircuitKind circuit,
                                           nx_opamp_model_e model,
                                           nx_bjt_npn_model_e bjtModel,
                                           nx_jfet_n_model_e jfetModel,
                                           double gainControl,
                                           double freqHz,
                                           const AcSweepParams &params,
                                           double secondaryControl,
                                           double tertiaryControl,
                                           nx_pot_taper_e potTaper);

} // namespace ds1_ac
