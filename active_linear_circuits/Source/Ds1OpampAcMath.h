#pragma once

#include <utility>
#include <vector>

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "SchematicComponentValues.h"

#include "nudsp/common/components.h"
#include "nudsp/linear_circuits/ds1_opamp_f32.h"
#include "nudsp/nonlinear_circuits/bjt_common_emitter_f32.h"
#include "nudsp/nonlinear_circuits/bjt_follower_f32.h"
#include "nudsp/nonlinear_circuits/bjt_follower_out_f32.h"
#include "nudsp/nonlinear_circuits/ds1_clipper_f32.h"

namespace atom
{
class SvgView;
}

namespace ds1_ac
{

    class SineWavePreviewEngine;

    enum class CircuitKind
    {
        BjtFollower,
        BjtCommonEmitter,
        Ds1Opamp,
        Ds1Clipper,
        BjtFollowerOut
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
        bool groundReferenced{false};
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
    constexpr double kPreviewAmpMin = 1.0e-6;
    constexpr double kPreviewAmpMax = 2.0;

    double defaultPreviewAmplitude(CircuitKind circuit) noexcept;

    const char *compositeDisplayName() noexcept;
    const char *compositeSvgRelativePath() noexcept;
    const char *circuitTopologyId(CircuitKind circuit) noexcept;
    const char *circuitOperatorKey(CircuitKind circuit) noexcept;
    const char *circuitStageLabel(CircuitKind circuit) noexcept;
    const char *circuitDisplayName(CircuitKind circuit) noexcept;
    juce::String circuitStageMenuLabel(CircuitKind circuit);
    const char *controlParameterName(CircuitKind circuit) noexcept;
    const char *secondaryControlParameterName(CircuitKind circuit) noexcept;
    const char *tertiaryControlParameterName(CircuitKind circuit) noexcept;
    const char *circuitProcessFunctionName(CircuitKind circuit) noexcept;
    const char *circuitSvgRelativePath(CircuitKind circuit) noexcept;
    bool circuitUsesOpampModel(CircuitKind circuit) noexcept;
    bool circuitUsesDiodeModel(CircuitKind circuit) noexcept;
    bool circuitUsesBjtModel(CircuitKind circuit) noexcept;
    bool circuitAcSweepIsCheap(CircuitKind circuit) noexcept;
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
    int opampModelComboId(nx_opamp_model_e model) noexcept;
    nx_opamp_model_e opampModelFromComboId(int comboId) noexcept;
    nx_opamp_model_e opampModelFromOverlayKey(const juce::String& overlayKey) noexcept;
    void populateOpampModelCombo(juce::ComboBox& combo);
    juce::String opampOverlayKeyForModel(nx_opamp_model_e model) noexcept;
    /** Default op-amp model encoded in schematic OPAMP overlays (hex id or label text). */
    nx_opamp_model_e defaultOpampModelFromSvgView(const atom::SvgView& view);
    const char *diodeModelDisplayName(nx_diode_model_t model) noexcept;
    nx_diode_model_t defaultDiodeModelForCircuit(CircuitKind circuit) noexcept;
    int diodeModelComboId(nx_diode_model_t model) noexcept;
    nx_diode_model_t diodeModelFromComboId(int comboId) noexcept;
    void populateDiodeModelCombo(juce::ComboBox& combo);
    /** Default diode model encoded in schematic DIODE overlays (hex id or label text). */
    nx_diode_model_t defaultDiodeModelFromSvgView(const atom::SvgView& view);
    const char *bjtModelDisplayName(nx_bjt_npn_model_e model) noexcept;
    const char *jfetModelDisplayName(nx_jfet_n_model_e model) noexcept;
    nx_bjt_npn_model_e defaultBjtModel(CircuitKind circuit) noexcept;
    nx_jfet_n_model_e defaultJfetModel(CircuitKind circuit) noexcept;
    int bjtModelComboId(nx_bjt_npn_model_e model) noexcept;
    nx_bjt_npn_model_e bjtModelFromComboId(int comboId) noexcept;
    void populateBjtModelCombo(juce::ComboBox& combo);
    /** Default BJT model encoded in schematic BJT_NPN overlays (hex id or label text). */
    nx_bjt_npn_model_e defaultBjtModelFromSvgView(const atom::SvgView& view);
    int jfetModelComboId(nx_jfet_n_model_e model) noexcept;
    nx_jfet_n_model_e jfetModelFromComboId(int comboId) noexcept;
    void populateJfetModelCombo(juce::ComboBox& combo);
    /** Default JFET model encoded in schematic JFET overlays (hex id or label text). */
    nx_jfet_n_model_e defaultJfetModelFromSvgView(const atom::SvgView& view);
    juce::String formatFrequencyTick(float log10Hz) noexcept;
    juce::String formatMagnitudeTick(float magDb) noexcept;
    juce::String formatPhaseTick(float phaseDeg) noexcept;
    juce::String formatPeriodTick(float normalizedPeriod) noexcept;
    juce::String formatWaveformTick(float amplitude) noexcept;

    std::vector<double> buildLogFrequencySweep(const AcSweepParams &params);
    std::vector<float> buildLogFrequencyGridTicks(float logMin, float logMax, int targetCount = 6);

    AxisRange magnitudeAxisFromCurve(const std::vector<std::pair<float, float>>& magnitudeCurve,
                                     const AcSweepParams& params);

    /** Magnitude Y-axis envelope from primary control at 0 and 1 (fixed while control is swept). */
    AxisRange computeMagnitudeAxisEnvelope(CircuitKind circuit,
                                           nx_opamp_model_e model,
                                           nx_diode_model_t diodeModel,
                                           nx_bjt_npn_model_e bjtModel,
                                           nx_jfet_n_model_e jfetModel,
                                           const AcSweepParams &params,
                                           double secondaryControl,
                                           double tertiaryControl,
                                           nx_pot_taper_e potTaper,
                                           const SchematicComponentValues* componentValues = nullptr);

    AcResponse computeAcResponse(CircuitKind circuit,
                                 nx_opamp_model_e model,
                                 nx_diode_model_t diodeModel,
                                 nx_bjt_npn_model_e bjtModel,
                                 nx_jfet_n_model_e jfetModel,
                                 double gainControl,
                                 const AcSweepParams &params,
                                 const AxisRange &magnitudeAxis,
                                 double secondaryControl,
                                 double tertiaryControl,
                                 nx_pot_taper_e potTaper,
                                 const SchematicComponentValues* componentValues = nullptr);

    SineWavePreview computeSineWavePreview(CircuitKind circuit,
                                           nx_opamp_model_e model,
                                           nx_diode_model_t diodeModel,
                                           nx_bjt_npn_model_e bjtModel,
                                           nx_jfet_n_model_e jfetModel,
                                           double gainControl,
                                           double freqHz,
                                           double amplitude,
                                           const AcSweepParams &params,
                                           double secondaryControl,
                                           double tertiaryControl,
                                           nx_pot_taper_e potTaper,
                                           SineWavePreviewEngine& engine,
                                           const SchematicComponentValues* componentValues = nullptr);

} // namespace ds1_ac
