#include "Ds1OpampAcMath.h"

#include "SineWavePreviewEngine.h"
#include "SchematicComponentApply.h"

#include <atom/SvgView.h>

#include <cmath>
#include <limits>
#include <optional>

namespace ds1_ac
{

    void AcSweepParams::sanitise() noexcept
    {
        freqMinHz = juce::jmax(1.0, freqMinHz);
        freqMaxHz = juce::jmax(freqMinHz + 1.0, freqMaxHz);
        numPoints = juce::jlimit(32, 2048, numPoints);
        sampleRateHz = juce::jmax(1000.0, sampleRateHz);
    }

    namespace
    {
        struct StageInfo
        {
            CircuitKind kind;
            const char* topologyId;
            const char* operatorKey;
            const char* label;
            const char* processFn;
            const char* svgRel;
        };

        constexpr StageInfo kDs1Stages[] = {
            {CircuitKind::BjtFollower, "input", "bjt_follower", "Input BJT",
             "nx_bjt_follower_process_f32", "assets/schematics/core/nonlinear_circuits/bjt_follower.svg"},
            {CircuitKind::BjtCommonEmitter, "emitter", "bjt_common_emitter", "BJT Emitter",
             "nx_bjt_common_emitter_process_f32", "assets/schematics/core/nonlinear_circuits/bjt_common_emitter.svg"},
            {CircuitKind::Ds1Opamp, "opamp", "ds1_opamp", "Op Amp",
             "nx_ds1_opamp_process_f32", "assets/schematics/core/linear_circuits/ds1_opamp.svg"},
            {CircuitKind::Ds1Clipper, "clipper", "ds1_clipper", "Clipper / Tone / Level",
             "nx_ds1_clipper_process_f32", "assets/schematics/core/nonlinear_circuits/ds1_clipper.svg"},
            {CircuitKind::BjtFollowerOut, "output", "bjt_follower_out", "Output BJT",
             "nx_bjt_follower_out_process_f32", "assets/schematics/core/nonlinear_circuits/bjt_follower_out.svg"},
        };

        const StageInfo& stageInfo(CircuitKind circuit) noexcept
        {
            for (const auto& stage : kDs1Stages)
            {
                if (stage.kind == circuit)
                    return stage;
            }

            return kDs1Stages[2];
        }
    } // namespace

    const char* compositeDisplayName() noexcept
    {
        return "Boss DS-1";
    }

    const char* compositeSvgRelativePath() noexcept
    {
        return "assets/schematics/extensions/white_box/pedals/ds1.svg";
    }

    const char* circuitTopologyId(CircuitKind circuit) noexcept
    {
        return stageInfo(circuit).topologyId;
    }

    const char* circuitOperatorKey(CircuitKind circuit) noexcept
    {
        return stageInfo(circuit).operatorKey;
    }

    const char* circuitStageLabel(CircuitKind circuit) noexcept
    {
        return stageInfo(circuit).label;
    }

    const char* circuitDisplayName(CircuitKind circuit) noexcept
    {
        return circuitStageLabel(circuit);
    }

    juce::String circuitStageMenuLabel(CircuitKind circuit)
    {
        return juce::String(circuitTopologyId(circuit)) + " — " + circuitStageLabel(circuit);
    }

    const char* controlParameterName(CircuitKind circuit) noexcept
    {
        switch (circuit)
        {
        case CircuitKind::Ds1Opamp:
            return "Gain";
        case CircuitKind::Ds1Clipper:
            return "Tone";
        case CircuitKind::BjtFollower:
        case CircuitKind::BjtCommonEmitter:
        case CircuitKind::BjtFollowerOut:
            return "Control";
        }

        return "Control";
    }

    const char* secondaryControlParameterName(CircuitKind circuit) noexcept
    {
        if (circuit == CircuitKind::Ds1Clipper)
            return "Level";
        return "Control";
    }

    const char* tertiaryControlParameterName(CircuitKind) noexcept
    {
        return "Control";
    }

    const char* circuitProcessFunctionName(CircuitKind circuit) noexcept
    {
        return stageInfo(circuit).processFn;
    }

    const char* circuitSvgRelativePath(CircuitKind circuit) noexcept
    {
        return stageInfo(circuit).svgRel;
    }

    bool circuitUsesOpampModel(CircuitKind circuit) noexcept
    {
        return circuit == CircuitKind::Ds1Opamp;
    }

    bool circuitUsesDiodeModel(CircuitKind circuit) noexcept
    {
        return circuit == CircuitKind::Ds1Clipper;
    }

    bool circuitUsesBjtModel(CircuitKind circuit) noexcept
    {
        switch (circuit)
        {
        case CircuitKind::BjtFollower:
        case CircuitKind::BjtFollowerOut:
        case CircuitKind::BjtCommonEmitter:
            return true;
        case CircuitKind::Ds1Opamp:
        case CircuitKind::Ds1Clipper:
            return false;
        }

        return false;
    }

    bool circuitAcSweepIsCheap(CircuitKind circuit) noexcept
    {
        return circuitUsesBjtModel(circuit);
    }

    bool circuitUsesJfetModel(CircuitKind) noexcept
    {
        return false;
    }

    bool circuitHasPrimaryControl(CircuitKind circuit) noexcept
    {
        switch (circuit)
        {
        case CircuitKind::Ds1Opamp:
        case CircuitKind::Ds1Clipper:
            return true;
        case CircuitKind::BjtFollower:
        case CircuitKind::BjtCommonEmitter:
        case CircuitKind::BjtFollowerOut:
            return false;
        }

        return false;
    }

    bool circuitUsesPotTaper(CircuitKind circuit) noexcept
    {
        return circuitHasPrimaryControl(circuit);
    }

    bool circuitHasSecondaryControl(CircuitKind circuit) noexcept
    {
        return circuit == CircuitKind::Ds1Clipper;
    }

    bool circuitHasTertiaryControl(CircuitKind) noexcept
    {
        return false;
    }

    nx_pot_taper_e defaultPotTaper(CircuitKind) noexcept
    {
        return NX_POT_TAPER_LINEAR;
    }

    const char *potTaperDisplayName(nx_pot_taper_e taper) noexcept
    {
        switch (taper)
        {
        case NX_POT_TAPER_LINEAR:
            return "Linear";
        case NX_POT_TAPER_MULTIPLICATIVE:
            return "Multiplicative";
        case NX_POT_TAPER_A15:
            return "A15";
        case NX_POT_TAPER_A30:
            return "A30";
        case NX_POT_TAPER_A45:
            return "A45";
        case NX_POT_TAPER_G:
            return "G (4B)";
        case NX_POT_TAPER_C:
            return "C";
        case NX_POT_TAPER_3B:
            return "3B";
        default:
            return "Unknown";
        }
    }

    namespace
    {
        constexpr nx_pot_taper_e kSelectablePotTapers[] = {
            NX_POT_TAPER_LINEAR,
            NX_POT_TAPER_MULTIPLICATIVE,
            NX_POT_TAPER_A15,
            NX_POT_TAPER_A30,
            NX_POT_TAPER_A45,
            NX_POT_TAPER_G,
            NX_POT_TAPER_C,
            NX_POT_TAPER_3B,
        };
    } // namespace

    int potTaperComboId(nx_pot_taper_e taper) noexcept
    {
        for (size_t i = 0; i < std::size(kSelectablePotTapers); ++i)
        {
            if (kSelectablePotTapers[i] == taper)
                return static_cast<int>(i + 1);
        }

        return potTaperComboId(defaultPotTaper(CircuitKind::Ds1Opamp));
    }

    nx_pot_taper_e potTaperFromComboId(int comboId) noexcept
    {
        if (comboId >= 1 && comboId <= static_cast<int>(std::size(kSelectablePotTapers)))
            return kSelectablePotTapers[static_cast<size_t>(comboId - 1)];

        return NX_POT_TAPER_LINEAR;
    }

    const char *opampModelDisplayName(nx_opamp_model_e model) noexcept
    {
        switch (model)
        {
        case NX_OPAMP_IDEAL:
            return "Ideal";
        case NX_OPAMP_LM741:
            return "LM741";
        case NX_OPAMP_JRC4558:
            return "JRC4558";
        case NX_OPAMP_BA728:
            return "BA728";
        case NX_OPAMP_LM308:
            return "LM308";
        case NX_OPAMP_TL072:
            return "TL072";
        case NX_OPAMP_LM833:
            return "LM833";
        default:
            return "Unknown";
        }
    }

    namespace
    {
        constexpr nx_opamp_model_e kSelectableOpampModels[] = {
            NX_OPAMP_IDEAL,
            NX_OPAMP_LM741,
            NX_OPAMP_JRC4558,
            NX_OPAMP_BA728,
            NX_OPAMP_LM308,
            NX_OPAMP_TL072,
            NX_OPAMP_LM833,
        };

        uint32_t parseOverlayHexSuffix(const juce::String& suffix) noexcept
        {
            auto hex = suffix.trim();
            if (hex.startsWithIgnoreCase("0X"))
                hex = hex.substring(2);

            return static_cast<uint32_t>(hex.getHexValue32());
        }

        bool isSelectableOpampModel(nx_opamp_model_e model) noexcept
        {
            for (const auto candidate : kSelectableOpampModels)
            {
                if (candidate == model)
                    return true;
            }

            return false;
        }

        nx_opamp_model_e opampModelFromHexSuffix(const juce::String& suffix) noexcept
        {
            const auto value = static_cast<nx_opamp_model_e>(parseOverlayHexSuffix(suffix));
            return isSelectableOpampModel(value) ? value : NX_OPAMP_BA728;
        }

        nx_opamp_model_e opampModelFromOverlayLabel(const juce::String& label) noexcept
        {
            const auto trimmed = label.trim();
            if (trimmed.isEmpty())
                return NX_OPAMP_BA728;

            for (const auto model : kSelectableOpampModels)
            {
                const juce::String name = opampModelDisplayName(model);
                if (trimmed.equalsIgnoreCase(name))
                    return model;
            }

            const auto upper = trimmed.toUpperCase();
            for (const auto model : kSelectableOpampModels)
            {
                const juce::String name = opampModelDisplayName(model);
                const auto nameUpper = name.toUpperCase();
                if (nameUpper.contains(upper) || upper.contains(nameUpper))
                    return model;
                if (upper.length() >= 3 && nameUpper.endsWith(upper))
                    return model;
            }

            return NX_OPAMP_BA728;
        }
    } // namespace

    int opampModelComboId(nx_opamp_model_e model) noexcept
    {
        for (size_t i = 0; i < std::size(kSelectableOpampModels); ++i)
        {
            if (kSelectableOpampModels[i] == model)
                return static_cast<int>(i + 1);
        }

        return opampModelComboId(NX_OPAMP_BA728);
    }

    nx_opamp_model_e opampModelFromComboId(int comboId) noexcept
    {
        if (comboId >= 1 && comboId <= static_cast<int>(std::size(kSelectableOpampModels)))
            return kSelectableOpampModels[static_cast<size_t>(comboId - 1)];

        return NX_OPAMP_BA728;
    }

    nx_opamp_model_e opampModelFromOverlayKey(const juce::String& overlayKey) noexcept
    {
        const auto suffix = overlayKey.fromLastOccurrenceOf("_", false, false);
        if (suffix.startsWithIgnoreCase("0X"))
            return opampModelFromHexSuffix(suffix);

        return NX_OPAMP_BA728;
    }

    nx_opamp_model_e defaultOpampModelFromSvgView(const atom::SvgView& view)
    {
        const auto keys = view.findDocumentOverlayKeysWithPrefix("OPAMP_");
        for (const auto& key : keys)
        {
            if (view.getOverlayText(key).trim().isEmpty())
                continue;

            const auto suffix = key.fromLastOccurrenceOf("_", false, false);
            if (! suffix.startsWithIgnoreCase("0X"))
                continue;

            const auto value = static_cast<nx_opamp_model_e>(parseOverlayHexSuffix(suffix));
            if (isSelectableOpampModel(value))
                return value;
        }

        for (const auto& key : keys)
        {
            const auto label = view.getOverlayText(key).trim();
            if (label.isEmpty())
                continue;

            const auto model = opampModelFromOverlayLabel(label);
            if (model != NX_OPAMP_BA728 || label.equalsIgnoreCase("BA728"))
                return model;
        }

        return NX_OPAMP_BA728;
    }

    void populateOpampModelCombo(juce::ComboBox& combo)
    {
        combo.clear(juce::dontSendNotification);
        for (size_t i = 0; i < std::size(kSelectableOpampModels); ++i)
            combo.addItem(opampModelDisplayName(kSelectableOpampModels[i]), static_cast<int>(i + 1));
    }

    const char *diodeModelDisplayName(nx_diode_model_t model) noexcept
    {
        switch (model)
        {
        case NX_DIODE_1N4148:
            return "1N4148";
        case NX_DIODE_1N4001:
            return "1N4001";
        case NX_DIODE_1N34A:
            return "1N34A";
        case NX_DIODE_1N60P:
            return "1N60P";
        case NX_DIODE_1N914:
            return "1N914";
        case NX_DIODE_1N270:
            return "1N270";
        case NX_DIODE_1N4937:
            return "1N4937";
        case NX_DIODE_1N5399:
            return "1N5399";
        case NX_DIODE_DLED:
            return "Red LED";
        default:
            return "Unknown";
        }
    }

    namespace
    {
        constexpr nx_diode_model_t kSelectableDiodeModels[] = {
            NX_DIODE_1N4148,
            NX_DIODE_1N4001,
            NX_DIODE_1N34A,
            NX_DIODE_1N60P,
            NX_DIODE_1N914,
            NX_DIODE_1N270,
            NX_DIODE_1N4937,
            NX_DIODE_1N5399,
            NX_DIODE_DLED,
        };

        nx_diode_model_t diodeModelFromLabel(const juce::String& label) noexcept
        {
            const auto trimmed = label.trim();
            if (trimmed.isEmpty())
                return NX_DIODE_1N4148;

            if (trimmed.equalsIgnoreCase("RED"))
                return NX_DIODE_DLED;

            for (const auto model : kSelectableDiodeModels)
            {
                if (trimmed.equalsIgnoreCase(diodeModelDisplayName(model)))
                    return model;
            }

            const auto upper = trimmed.toUpperCase();
            for (const auto model : kSelectableDiodeModels)
            {
                const juce::String name = diodeModelDisplayName(model);
                if (name.toUpperCase().contains(upper) || upper.contains(name.toUpperCase()))
                    return model;
            }

            return NX_DIODE_1N4148;
        }
    } // namespace

    nx_diode_model_t defaultDiodeModelForCircuit(CircuitKind circuit) noexcept
    {
        juce::ignoreUnused(circuit);
        return NX_DIODE_1N4148;
    }

    int diodeModelComboId(nx_diode_model_t model) noexcept
    {
        for (size_t i = 0; i < std::size(kSelectableDiodeModels); ++i)
        {
            if (kSelectableDiodeModels[i] == model)
                return static_cast<int>(i + 1);
        }

        return diodeModelComboId(defaultDiodeModelForCircuit(CircuitKind::Ds1Clipper));
    }

    nx_diode_model_t diodeModelFromComboId(int comboId) noexcept
    {
        if (comboId >= 1 && comboId <= static_cast<int>(std::size(kSelectableDiodeModels)))
            return kSelectableDiodeModels[static_cast<size_t>(comboId - 1)];

        return NX_DIODE_1N4148;
    }

    nx_diode_model_t defaultDiodeModelFromSvgView(const atom::SvgView& view)
    {
        const auto keys = view.findDocumentOverlayKeysWithPrefix("DIODE_");
        for (const auto& key : keys)
        {
            if (view.getOverlayText(key).trim().isEmpty())
                continue;

            const auto suffix = key.fromLastOccurrenceOf("_", false, false);
            if (! suffix.startsWithIgnoreCase("0X"))
                continue;

            const auto value = static_cast<nx_diode_model_t>(suffix.getHexValue32());
            for (const auto model : kSelectableDiodeModels)
            {
                if (model == value)
                    return value;
            }
        }

        for (const auto& key : keys)
        {
            const auto label = view.getOverlayText(key).trim();
            if (label.isEmpty())
                continue;

            const auto fromLabel = diodeModelFromLabel(label);
            for (const auto model : kSelectableDiodeModels)
            {
                if (model == fromLabel)
                    return fromLabel;
            }
        }

        return NX_DIODE_1N4148;
    }

    void populateDiodeModelCombo(juce::ComboBox& combo)
    {
        combo.clear(juce::dontSendNotification);
        for (size_t i = 0; i < std::size(kSelectableDiodeModels); ++i)
            combo.addItem(diodeModelDisplayName(kSelectableDiodeModels[i]), static_cast<int>(i + 1));
    }

    juce::String opampOverlayKeyForModel(nx_opamp_model_e model) noexcept
    {
        return "OPAMP_0X" + juce::String::toHexString(static_cast<int>(model)).toUpperCase();
    }

    namespace
    {
        constexpr nx_bjt_npn_model_e kSelectableBjtModels[] = {
            NX_BJT_NPN,
            NX_BJT_2N3904,
            NX_BJT_2N2222,
        };

        constexpr nx_jfet_n_model_e kSelectableJfetModels[] = {
            NX_JFET_N,
            NX_JFET_J201,
            NX_JFET_2N5457,
        };
    } // namespace

    const char *bjtModelDisplayName(nx_bjt_npn_model_e model) noexcept
    {
        switch (model)
        {
        case NX_BJT_NPN:
            return "Generic NPN";
        case NX_BJT_2N3904:
            return "2N3904";
        case NX_BJT_2N2222:
            return "2N2222";
        default:
            return "Unknown";
        }
    }

    const char *jfetModelDisplayName(nx_jfet_n_model_e model) noexcept
    {
        switch (model)
        {
        case NX_JFET_N:
            return "Generic N-JFET";
        case NX_JFET_J201:
            return "J201";
        case NX_JFET_2N5457:
            return "2N5457";
        default:
            return "Unknown";
        }
    }

    nx_bjt_npn_model_e defaultBjtModel(CircuitKind circuit) noexcept
    {
        juce::ignoreUnused(circuit);
        return NX_BJT_2N3904;
    }

    nx_jfet_n_model_e defaultJfetModel(CircuitKind circuit) noexcept
    {
        juce::ignoreUnused(circuit);
        return NX_JFET_2N5457;
    }

    int bjtModelComboId(nx_bjt_npn_model_e model) noexcept
    {
        for (size_t i = 0; i < std::size(kSelectableBjtModels); ++i)
        {
            if (kSelectableBjtModels[i] == model)
                return static_cast<int>(i + 1);
        }

        return bjtModelComboId(defaultBjtModel(CircuitKind::BjtFollower));
    }

    nx_bjt_npn_model_e bjtModelFromComboId(int comboId) noexcept
    {
        if (comboId >= 1 && comboId <= static_cast<int>(std::size(kSelectableBjtModels)))
            return kSelectableBjtModels[static_cast<size_t>(comboId - 1)];

        return NX_BJT_2N3904;
    }

    namespace
    {
        nx_bjt_npn_model_e bjtModelFromLabel(const juce::String& label) noexcept
        {
            const auto trimmed = label.trim();
            if (trimmed.isEmpty())
                return NX_BJT_2N3904;

            for (const auto model : kSelectableBjtModels)
            {
                if (trimmed.equalsIgnoreCase(bjtModelDisplayName(model)))
                    return model;
            }

            const auto upper = trimmed.toUpperCase();
            for (const auto model : kSelectableBjtModels)
            {
                const juce::String name = bjtModelDisplayName(model);
                if (name.toUpperCase().contains(upper) || upper.contains(name.toUpperCase()))
                    return model;
            }

            return NX_BJT_2N3904;
        }

        nx_jfet_n_model_e jfetModelFromLabel(const juce::String& label) noexcept
        {
            const auto trimmed = label.trim();
            if (trimmed.isEmpty())
                return NX_JFET_2N5457;

            for (const auto model : kSelectableJfetModels)
            {
                if (trimmed.equalsIgnoreCase(jfetModelDisplayName(model)))
                    return model;
            }

            const auto upper = trimmed.toUpperCase();
            for (const auto model : kSelectableJfetModels)
            {
                const juce::String name = jfetModelDisplayName(model);
                if (name.toUpperCase().contains(upper) || upper.contains(name.toUpperCase()))
                    return model;
            }

            return NX_JFET_2N5457;
        }

        bool isSelectableBjtModel(nx_bjt_npn_model_e model) noexcept
        {
            for (const auto item : kSelectableBjtModels)
            {
                if (item == model)
                    return true;
            }
            return false;
        }

        bool isSelectableJfetModel(nx_jfet_n_model_e model) noexcept
        {
            for (const auto item : kSelectableJfetModels)
            {
                if (item == model)
                    return true;
            }
            return false;
        }
    } // namespace

    void populateBjtModelCombo(juce::ComboBox& combo)
    {
        combo.clear(juce::dontSendNotification);
        for (size_t i = 0; i < std::size(kSelectableBjtModels); ++i)
            combo.addItem(bjtModelDisplayName(kSelectableBjtModels[i]), static_cast<int>(i + 1));
    }

    nx_bjt_npn_model_e defaultBjtModelFromSvgView(const atom::SvgView& view)
    {
        const auto keys = view.findDocumentOverlayKeysWithPrefix("BJT_NPN_");
        for (const auto& key : keys)
        {
            if (view.getOverlayText(key).trim().isEmpty())
                continue;

            const auto suffix = key.fromLastOccurrenceOf("_", false, false);
            if (! suffix.startsWithIgnoreCase("0X"))
                continue;

            const auto value = static_cast<nx_bjt_npn_model_e>(suffix.getHexValue32());
            if (isSelectableBjtModel(value))
                return value;
        }

        for (const auto& key : keys)
        {
            const auto label = view.getOverlayText(key).trim();
            if (label.isEmpty())
                continue;

            const auto fromLabel = bjtModelFromLabel(label);
            if (isSelectableBjtModel(fromLabel))
                return fromLabel;
        }

        return defaultBjtModel(CircuitKind::BjtFollower);
    }

    int jfetModelComboId(nx_jfet_n_model_e model) noexcept
    {
        for (size_t i = 0; i < std::size(kSelectableJfetModels); ++i)
        {
            if (kSelectableJfetModels[i] == model)
                return static_cast<int>(i + 1);
        }

        return jfetModelComboId(NX_JFET_2N5457);
    }

    nx_jfet_n_model_e jfetModelFromComboId(int comboId) noexcept
    {
        if (comboId >= 1 && comboId <= static_cast<int>(std::size(kSelectableJfetModels)))
            return kSelectableJfetModels[static_cast<size_t>(comboId - 1)];

        return NX_JFET_2N5457;
    }

    void populateJfetModelCombo(juce::ComboBox& combo)
    {
        combo.clear(juce::dontSendNotification);
        for (size_t i = 0; i < std::size(kSelectableJfetModels); ++i)
            combo.addItem(jfetModelDisplayName(kSelectableJfetModels[i]), static_cast<int>(i + 1));
    }

    nx_jfet_n_model_e defaultJfetModelFromSvgView(const atom::SvgView& view)
    {
        const auto keys = view.findDocumentOverlayKeysWithPrefix("JFET_");
        for (const auto& key : keys)
        {
            if (view.getOverlayText(key).trim().isEmpty())
                continue;

            const auto suffix = key.fromLastOccurrenceOf("_", false, false);
            if (! suffix.startsWithIgnoreCase("0X"))
                continue;

            const auto value = static_cast<nx_jfet_n_model_e>(suffix.getHexValue32());
            if (isSelectableJfetModel(value))
                return value;
        }

        for (const auto& key : keys)
        {
            const auto label = view.getOverlayText(key).trim();
            if (label.isEmpty())
                continue;

            const auto fromLabel = jfetModelFromLabel(label);
            if (isSelectableJfetModel(fromLabel))
                return fromLabel;
        }

        return NX_JFET_2N5457;
    }

    juce::String formatFrequencyTick(float log10Hz) noexcept
    {
        const double hz = std::pow(10.0, static_cast<double>(log10Hz));
        if (hz >= 1000.0)
            return juce::String(hz / 1000.0, hz >= 10000.0 ? 0 : 1) + " kHz";

        return juce::String(juce::roundToInt(hz)) + " Hz";
    }

    juce::String formatMagnitudeTick(float magDb) noexcept
    {
        return juce::String(magDb, std::abs(magDb) >= 10.0f ? 0 : 1) + " dB";
    }

    juce::String formatPhaseTick(float phaseDeg) noexcept
    {
        return juce::String(phaseDeg, 0) + juce::String(juce::CharPointer_UTF8("\xc2\xb0"));
    }

    juce::String formatPeriodTick(float normalizedPeriod) noexcept
    {
        const int degrees = juce::roundToInt(normalizedPeriod * 360.0f);
        return juce::String(degrees) + juce::String(juce::CharPointer_UTF8("\xc2\xb0"));
    }

    juce::String formatWaveformTick(float amplitude) noexcept
    {
        const juce::String sign = amplitude >= 0.0f ? "+" : "";
        return sign + juce::String(amplitude, 2);
    }

    std::vector<double> buildLogFrequencySweep(const AcSweepParams &params)
    {
        AcSweepParams safe = params;
        safe.sanitise();

        std::vector<double> freqs(static_cast<size_t>(safe.numPoints));
        if (safe.numPoints <= 1)
        {
            if (!freqs.empty())
                freqs[0] = safe.freqMinHz;
            return freqs;
        }

        const double ratio = safe.freqMaxHz / safe.freqMinHz;
        for (int i = 0; i < safe.numPoints; ++i)
        {
            const double t = static_cast<double>(i) / static_cast<double>(safe.numPoints - 1);
            freqs[static_cast<size_t>(i)] = safe.freqMinHz * std::pow(ratio, t);
        }

        return freqs;
    }

    std::vector<float> buildLogFrequencyGridTicks(float logMin, float logMax, int targetCount)
    {
        juce::ignoreUnused(targetCount);

        std::vector<float> ticks;
        const int decadeStart = static_cast<int>(std::ceil(static_cast<double>(logMin)));
        const int decadeEnd = static_cast<int>(std::floor(static_cast<double>(logMax)));

        for (int exp = decadeStart; exp <= decadeEnd; ++exp)
        {
            const float value = static_cast<float>(exp);
            if (value >= logMin && value <= logMax)
                ticks.push_back(value);
        }

        return ticks;
    }

    namespace
    {
        template<typename Inst>
        Inst* createCircuitInstance(Inst* (*createFn)(const nx_alloc_callbacks*),
                                    CircuitKind circuit,
                                    const SchematicComponentValues* componentValues)
        {
            Inst* inst = createFn(nullptr);
            if (inst != nullptr)
                applySchematicComponentValues(circuit, inst, componentValues);
            return inst;
        }

        double clampControl(double control) noexcept
        {
            return juce::jlimit(0.0, 1.0, control);
        }

        juce::String makeResponseTitle(CircuitKind circuit,
                                       nx_opamp_model_e model,
                                       nx_diode_model_t diodeModel,
                                       nx_bjt_npn_model_e bjtModel,
                                       nx_jfet_n_model_e jfetModel,
                                       double control,
                                       double secondaryControl,
                                       double tertiaryControl)
        {
            juce::ignoreUnused(jfetModel, tertiaryControl);

            juce::String title = juce::String(compositeDisplayName()) + "  |  "
                                 + circuitStageMenuLabel(circuit);

            if (circuitUsesOpampModel(circuit))
                title += "  |  " + juce::String(opampModelDisplayName(model));
            else if (circuitUsesDiodeModel(circuit))
                title += "  |  " + juce::String(diodeModelDisplayName(diodeModel));
            else if (circuitUsesBjtModel(circuit))
                title += "  |  " + juce::String(bjtModelDisplayName(bjtModel));

            if (circuitHasPrimaryControl(circuit))
                title += "  |  " + juce::String(controlParameterName(circuit)) + " " + juce::String(control, 2);
            if (circuitHasSecondaryControl(circuit))
                title += "  |  " + juce::String(secondaryControlParameterName(circuit)) + " "
                         + juce::String(secondaryControl, 2);

            return title;
        }

        void assignPotTaper(nx_smooth_pot_t& pot, nx_pot_taper_e taper) noexcept
        {
            pot.pot_params.taper = taper;
            pot.pot_params.table = nullptr;
            pot.pot_params.table_size = 0;
        }

        template<typename Instance>
        bool updatePotTaper(Instance* instance,
                            nx_result_t (*getPot)(const Instance*, nx_smooth_pot_t*),
                            nx_result_t (*setPot)(Instance*, const nx_smooth_pot_t*),
                            nx_pot_taper_e taper) noexcept
        {
            if (instance == nullptr || getPot == nullptr || setPot == nullptr)
                return false;

            nx_smooth_pot_t pot;
            if (getPot(instance, &pot) != NX_SUCCESS)
                return false;

            assignPotTaper(pot, taper);
            return setPot(instance, &pot) == NX_SUCCESS;
        }

        void applyDs1OpampPotTaper(nx_ds1_opamp_f32_t* opamp, nx_pot_taper_e taper) noexcept
        {
            updatePotTaper(opamp, nx_ds1_opamp_get_gain_pot_f32, nx_ds1_opamp_set_gain_pot_f32, taper);
        }

        void applyDs1ClipperPotTapers(nx_ds1_clipper_f32_t* clipper, nx_pot_taper_e taper) noexcept
        {
            updatePotTaper(clipper, nx_ds1_clipper_get_tone_pot_f32, nx_ds1_clipper_set_tone_pot_f32, taper);
            updatePotTaper(clipper, nx_ds1_clipper_get_level_pot_f32, nx_ds1_clipper_set_level_pot_f32, taper);
        }

        void runDs1AcSweep(nx_ds1_opamp_f32_t* opamp,
                           nx_opamp_model_e model,
                           nx_pot_taper_e potTaper,
                           double control,
                           double sampleRateHz,
                           const std::vector<double>& freqs,
                           std::vector<double>& magDb,
                           std::vector<double>& phaseDeg)
        {
            applyDs1OpampPotTaper(opamp, potTaper);
            nx_ds1_opamp_set_opamp_model_f32(opamp, model);
            nx_ds1_opamp_set_gain_control_f32(opamp, clampControl(control));
            nx_ds1_opamp_config_t cfg{};
            if (nx_ds1_opamp_get_config_f32(opamp, &cfg) != NX_SUCCESS)
                return;
            nx_ds1_opamp_ac_static_f32(&cfg, sampleRateHz, freqs.data(), magDb.data(), phaseDeg.data(), freqs.size());
        }

        void runDs1ClipperAcSweep(nx_ds1_clipper_f32_t* clipper,
                                  nx_diode_model_t diodeModel,
                                  nx_pot_taper_e potTaper,
                                  double toneControl,
                                  double levelControl,
                                  double sampleRateHz,
                                  const std::vector<double>& freqs,
                                  std::vector<double>& magDb,
                                  std::vector<double>& phaseDeg)
        {
            applyDs1ClipperPotTapers(clipper, potTaper);
            nx_ds1_clipper_set_diode_model_f32(clipper, diodeModel);
            nx_ds1_clipper_set_tone_control_f32(clipper, clampControl(toneControl));
            nx_ds1_clipper_set_level_control_f32(clipper, clampControl(levelControl));
            nx_ds1_clipper_config_t cfg{};
            if (nx_ds1_clipper_get_config_f32(clipper, &cfg) != NX_SUCCESS)
                return;
            nx_ds1_clipper_ac_static_f32(&cfg, sampleRateHz, freqs.data(), magDb.data(), phaseDeg.data(), freqs.size());
        }

        void runBjtFollowerAcSweep(nx_bjt_follower_f32_t* follower,
                                   nx_bjt_npn_model_e bjtModel,
                                   double sampleRateHz,
                                   const std::vector<double>& freqs,
                                   std::vector<double>& magDb,
                                   std::vector<double>& phaseDeg)
        {
            nx_bjt_follower_set_bjt_model_f32(follower, bjtModel);
            nx_bjt_follower_config_t cfg{};
            if (nx_bjt_follower_get_config_f32(follower, &cfg) != NX_SUCCESS)
                return;
            nx_bjt_follower_ac_static_f32(&cfg, sampleRateHz, freqs.data(), magDb.data(), phaseDeg.data(), freqs.size());
        }

        void runBjtFollowerOutAcSweep(nx_bjt_follower_out_f32_t* follower,
                                      nx_bjt_npn_model_e bjtModel,
                                      double sampleRateHz,
                                      const std::vector<double>& freqs,
                                      std::vector<double>& magDb,
                                      std::vector<double>& phaseDeg)
        {
            nx_bjt_follower_out_set_bjt_model_f32(follower, bjtModel);
            nx_bjt_follower_out_config_t cfg{};
            if (nx_bjt_follower_out_get_config_f32(follower, &cfg) != NX_SUCCESS)
                return;
            nx_bjt_follower_out_ac_static_f32(&cfg, sampleRateHz, freqs.data(), magDb.data(), phaseDeg.data(), freqs.size());
        }

        void runBjtCommonEmitterAcSweep(nx_bjt_common_emitter_f32_t* emitter,
                                        nx_bjt_npn_model_e bjtModel,
                                        double sampleRateHz,
                                        const std::vector<double>& freqs,
                                        std::vector<double>& magDb,
                                        std::vector<double>& phaseDeg)
        {
            nx_bjt_common_emitter_set_bjt_model_f32(emitter, bjtModel);
            nx_bjt_common_emitter_config_t cfg{};
            if (nx_bjt_common_emitter_get_config_f32(emitter, &cfg) != NX_SUCCESS)
                return;
            nx_bjt_common_emitter_ac_static_f32(&cfg, sampleRateHz, freqs.data(), magDb.data(), phaseDeg.data(), freqs.size());
        }

        AxisRange makeFrequencyAxis(float logMin, float logMax)
        {
            AxisRange range;
            range.minX = logMin;
            range.maxX = logMax;
            return range;
        }

        AxisRange paddedMagnitudeAxis(float minMag, float maxMag, float logMin, float logMax)
        {
            AxisRange range = makeFrequencyAxis(logMin, logMax);
            range.minY = -24.0f;
            range.maxY = 24.0f;

            if (!std::isfinite(minMag) || !std::isfinite(maxMag))
                return range;

            const float pad = juce::jmax(3.0f, 0.1f * (maxMag - minMag + 1.0f));
            range.minY = std::floor((minMag - pad) / 3.0f) * 3.0f;
            range.maxY = std::ceil((maxMag + pad) / 3.0f) * 3.0f;

            if (range.maxY <= range.minY)
                range.maxY = range.minY + 6.0f;

            return range;
        }

        void accumulateMagnitudeExtents(const std::vector<double>& magDb, float& minMag, float& maxMag)
        {
            for (const double value : magDb)
            {
                if (!std::isfinite(value))
                    continue;

                const float mag = static_cast<float>(value);
                minMag = juce::jmin(minMag, mag);
                maxMag = juce::jmax(maxMag, mag);
            }
        }

        AxisRange computePhaseAxis(const std::vector<std::pair<float, float>>& curve, float logMin, float logMax)
        {
            juce::ignoreUnused(curve);

            AxisRange range = makeFrequencyAxis(logMin, logMax);
            constexpr float kPhaseMinDeg = -180.0f;
            constexpr float kPhaseMaxDeg = 180.0f;
            constexpr float kEndpointPadDeg = 4.0f;
            range.minY = kPhaseMinDeg - kEndpointPadDeg;
            range.maxY = kPhaseMaxDeg + kEndpointPadDeg;
            return range;
        }

        void sweepPrimaryControlEnvelope(CircuitKind circuit,
                                         nx_opamp_model_e model,
                                         nx_diode_model_t diodeModel,
                                         nx_bjt_npn_model_e bjtModel,
                                         nx_jfet_n_model_e jfetModel,
                                         double secondaryControl,
                                         double tertiaryControl,
                                         nx_pot_taper_e potTaper,
                                         double sampleRateHz,
                                         const std::vector<double>& freqs,
                                         std::vector<double>& magDb,
                                         std::vector<double>& phaseDeg,
                                         float& minMag,
                                         float& maxMag,
                                         const SchematicComponentValues* componentValues)
        {
            juce::ignoreUnused(jfetModel, tertiaryControl);

            switch (circuit)
            {
            case CircuitKind::Ds1Clipper:
            {
                auto* clipper = createCircuitInstance<nx_ds1_clipper_f32_t>(nx_ds1_clipper_create_f32, circuit, componentValues);
                if (clipper == nullptr)
                    return;
                const double levelMid = clampControl(secondaryControl);
                runDs1ClipperAcSweep(clipper, diodeModel, potTaper, 0.0, levelMid, sampleRateHz, freqs, magDb, phaseDeg);
                accumulateMagnitudeExtents(magDb, minMag, maxMag);
                runDs1ClipperAcSweep(clipper, diodeModel, potTaper, 1.0, levelMid, sampleRateHz, freqs, magDb, phaseDeg);
                accumulateMagnitudeExtents(magDb, minMag, maxMag);
                runDs1ClipperAcSweep(clipper, diodeModel, potTaper, 0.5, 0.0, sampleRateHz, freqs, magDb, phaseDeg);
                accumulateMagnitudeExtents(magDb, minMag, maxMag);
                runDs1ClipperAcSweep(clipper, diodeModel, potTaper, 0.5, 1.0, sampleRateHz, freqs, magDb, phaseDeg);
                accumulateMagnitudeExtents(magDb, minMag, maxMag);
                nx_ds1_clipper_destroy_f32(clipper, nullptr);
                break;
            }
            case CircuitKind::BjtFollower:
            case CircuitKind::BjtFollowerOut:
            case CircuitKind::BjtCommonEmitter:
                juce::ignoreUnused(bjtModel, sampleRateHz, freqs, magDb, phaseDeg, componentValues);
                break;
            case CircuitKind::Ds1Opamp:
            default:
            {
                auto* opamp = createCircuitInstance<nx_ds1_opamp_f32_t>(nx_ds1_opamp_create_f32, CircuitKind::Ds1Opamp, componentValues);
                if (opamp == nullptr)
                    return;
                runDs1AcSweep(opamp, model, potTaper, 0.0, sampleRateHz, freqs, magDb, phaseDeg);
                accumulateMagnitudeExtents(magDb, minMag, maxMag);
                runDs1AcSweep(opamp, model, potTaper, 1.0, sampleRateHz, freqs, magDb, phaseDeg);
                accumulateMagnitudeExtents(magDb, minMag, maxMag);
                nx_ds1_opamp_destroy_f32(opamp, nullptr);
                break;
            }
            }
        }

        bool runAcResponseSweep(CircuitKind circuit,
                                nx_opamp_model_e model,
                                nx_diode_model_t diodeModel,
                                nx_bjt_npn_model_e bjtModel,
                                nx_jfet_n_model_e jfetModel,
                                double gainControl,
                                double secondaryControl,
                                double tertiaryControl,
                                nx_pot_taper_e potTaper,
                                double sampleRateHz,
                                const std::vector<double>& freqs,
                                std::vector<double>& magDb,
                                std::vector<double>& phaseDeg,
                                const SchematicComponentValues* componentValues)
        {
            juce::ignoreUnused(jfetModel, tertiaryControl);

            switch (circuit)
            {
            case CircuitKind::Ds1Clipper:
            {
                auto* clipper = createCircuitInstance<nx_ds1_clipper_f32_t>(nx_ds1_clipper_create_f32, circuit, componentValues);
                if (clipper == nullptr)
                    return false;
                runDs1ClipperAcSweep(clipper,
                                       diodeModel,
                                       potTaper,
                                       gainControl,
                                       secondaryControl,
                                       sampleRateHz,
                                       freqs,
                                       magDb,
                                       phaseDeg);
                nx_ds1_clipper_destroy_f32(clipper, nullptr);
                return true;
            }
            case CircuitKind::BjtFollower:
            {
                auto* follower = createCircuitInstance<nx_bjt_follower_f32_t>(nx_bjt_follower_create_f32, circuit, componentValues);
                if (follower == nullptr)
                    return false;
                runBjtFollowerAcSweep(follower, bjtModel, sampleRateHz, freqs, magDb, phaseDeg);
                nx_bjt_follower_destroy_f32(follower, nullptr);
                return true;
            }
            case CircuitKind::BjtFollowerOut:
            {
                auto* follower = createCircuitInstance<nx_bjt_follower_out_f32_t>(nx_bjt_follower_out_create_f32, circuit, componentValues);
                if (follower == nullptr)
                    return false;
                runBjtFollowerOutAcSweep(follower, bjtModel, sampleRateHz, freqs, magDb, phaseDeg);
                nx_bjt_follower_out_destroy_f32(follower, nullptr);
                return true;
            }
            case CircuitKind::BjtCommonEmitter:
            {
                auto* emitter = createCircuitInstance<nx_bjt_common_emitter_f32_t>(nx_bjt_common_emitter_create_f32, circuit, componentValues);
                if (emitter == nullptr)
                    return false;
                runBjtCommonEmitterAcSweep(emitter, bjtModel, sampleRateHz, freqs, magDb, phaseDeg);
                nx_bjt_common_emitter_destroy_f32(emitter, nullptr);
                return true;
            }
            case CircuitKind::Ds1Opamp:
            default:
            {
                auto* opamp = createCircuitInstance<nx_ds1_opamp_f32_t>(nx_ds1_opamp_create_f32, CircuitKind::Ds1Opamp, componentValues);
                if (opamp == nullptr)
                    return false;
                runDs1AcSweep(opamp, model, potTaper, gainControl, sampleRateHz, freqs, magDb, phaseDeg);
                nx_ds1_opamp_destroy_f32(opamp, nullptr);
                return true;
            }
            }
        }

    } // namespace

    float lerpWaveSample(const std::vector<float>& samples, float index)
    {
        if (samples.empty())
            return 0.0f;

        if (samples.size() == 1)
            return samples.front();

        const float clamped = juce::jlimit(0.0f, static_cast<float>(samples.size() - 1), index);
        const int i0 = static_cast<int>(std::floor(clamped));
        const int i1 = juce::jmin(i0 + 1, static_cast<int>(samples.size() - 1));
        const float frac = clamped - static_cast<float>(i0);
        return samples[static_cast<size_t>(i0)] * (1.0f - frac) + samples[static_cast<size_t>(i1)] * frac;
    }

    AxisRange computeMagnitudeAxisEnvelope(CircuitKind circuit,
                                           nx_opamp_model_e model,
                                           nx_diode_model_t diodeModel,
                                           nx_bjt_npn_model_e bjtModel,
                                           nx_jfet_n_model_e jfetModel,
                                           const AcSweepParams& params,
                                           double secondaryControl,
                                           double tertiaryControl,
                                           nx_pot_taper_e potTaper,
                                           const SchematicComponentValues* componentValues)
    {
        juce::ignoreUnused(secondaryControl, tertiaryControl);

        AcSweepParams safe = params;
        safe.sanitise();

        const auto freqs = buildLogFrequencySweep(safe);
        const float logMin = static_cast<float>(std::log10(safe.freqMinHz));
        const float logMax = static_cast<float>(std::log10(safe.freqMaxHz));

        if (freqs.empty())
            return paddedMagnitudeAxis(0.0f, 0.0f, logMin, logMax);

        std::vector<double> magDb(freqs.size());
        std::vector<double> phaseDeg(freqs.size());

        float minMag = std::numeric_limits<float>::max();
        float maxMag = std::numeric_limits<float>::lowest();

        sweepPrimaryControlEnvelope(circuit,
                                    model,
                                    diodeModel,
                                    bjtModel,
                                    jfetModel,
                                    secondaryControl,
                                    tertiaryControl,
                                    potTaper,
                                    safe.sampleRateHz,
                                    freqs,
                                    magDb,
                                    phaseDeg,
                                    minMag,
                                    maxMag,
                                    componentValues);

        if (minMag > maxMag)
            return paddedMagnitudeAxis(0.0f, 0.0f, logMin, logMax);

        return paddedMagnitudeAxis(minMag, maxMag, logMin, logMax);
    }

    AxisRange magnitudeAxisFromCurve(const std::vector<std::pair<float, float>>& magnitudeCurve,
                                     const AcSweepParams& params)
    {
        AcSweepParams safe = params;
        safe.sanitise();
        const float logMin = static_cast<float>(std::log10(safe.freqMinHz));
        const float logMax = static_cast<float>(std::log10(safe.freqMaxHz));

        float minMag = std::numeric_limits<float>::max();
        float maxMag = std::numeric_limits<float>::lowest();
        for (const auto& point : magnitudeCurve)
        {
            if (!std::isfinite(point.second))
                continue;
            minMag = juce::jmin(minMag, point.second);
            maxMag = juce::jmax(maxMag, point.second);
        }

        if (minMag > maxMag)
            return paddedMagnitudeAxis(0.0f, 0.0f, logMin, logMax);
        return paddedMagnitudeAxis(minMag, maxMag, logMin, logMax);
    }

    AcResponse computeAcResponse(CircuitKind circuit,
                                 nx_opamp_model_e model,
                                 nx_diode_model_t diodeModel,
                                 nx_bjt_npn_model_e bjtModel,
                                 nx_jfet_n_model_e jfetModel,
                                 double gainControl,
                                 const AcSweepParams& params,
                                 const AxisRange& magnitudeAxis,
                                 double secondaryControl,
                                 double tertiaryControl,
                                 nx_pot_taper_e potTaper,
                                 const SchematicComponentValues* componentValues)
    {
        AcSweepParams safe = params;
        safe.sanitise();

        AcResponse response;
        response.title = makeResponseTitle(circuit, model, diodeModel, bjtModel, jfetModel, gainControl, secondaryControl, tertiaryControl);

        const auto freqs = buildLogFrequencySweep(safe);
        if (freqs.empty())
            return response;

        std::vector<double> magDb(freqs.size());
        std::vector<double> phaseDeg(freqs.size());

        if (!runAcResponseSweep(circuit,
                                model,
                                diodeModel,
                                bjtModel,
                                jfetModel,
                                gainControl,
                                secondaryControl,
                                tertiaryControl,
                                potTaper,
                                safe.sampleRateHz,
                                freqs,
                                magDb,
                                phaseDeg,
                                componentValues))
        {
            return response;
        }

        response.magnitudeCurve.reserve(freqs.size());
        response.phaseCurve.reserve(freqs.size());

        const float logMin = static_cast<float>(std::log10(safe.freqMinHz));
        const float logMax = static_cast<float>(std::log10(safe.freqMaxHz));

        for (size_t i = 0; i < freqs.size(); ++i)
        {
            const float logFreq = static_cast<float>(std::log10(freqs[i]));
            response.magnitudeCurve.emplace_back(logFreq, static_cast<float>(magDb[i]));
            response.phaseCurve.emplace_back(logFreq, static_cast<float>(phaseDeg[i]));
        }

        response.magnitudeAxis = magnitudeAxis;
        response.magnitudeAxis.minX = logMin;
        response.magnitudeAxis.maxX = logMax;
        response.phaseAxis = computePhaseAxis(response.phaseCurve, logMin, logMax);

        return response;
    }

    SineWavePreview computeSineWavePreview(CircuitKind circuit,
                                           nx_opamp_model_e model,
                                           nx_diode_model_t diodeModel,
                                           nx_bjt_npn_model_e bjtModel,
                                           nx_jfet_n_model_e jfetModel,
                                           double gainControl,
                                           double freqHz,
                                           double amplitude,
                                           const AcSweepParams& params,
                                           double secondaryControl,
                                           double tertiaryControl,
                                           nx_pot_taper_e potTaper,
                                           SineWavePreviewEngine& engine,
                                           const SchematicComponentValues* componentValues)
    {
        AcSweepParams safe = params;
        safe.sanitise();

        constexpr int kPreviewDisplayPoints = 256;

        SineWavePreview preview;
        const double clampedFreq = juce::jlimit(kPreviewFreqMinHz, kPreviewFreqMaxHz, freqHz);
        const double clampedAmp = juce::jlimit(kPreviewAmpMin, kPreviewAmpMax, amplitude);
        preview.title = juce::String(clampedFreq, clampedFreq >= 1000.0 ? 1 : 0) + " Hz @ "
                        + juce::String(clampedAmp, clampedAmp >= 0.01 ? 3 : 4);

        const float inputScale = static_cast<float>(clampedAmp);
        juce::ignoreUnused(gainControl);
        const int periodSamples =
            juce::jmax(8, static_cast<int>(std::llround(safe.sampleRateHz / clampedFreq)));
        std::vector<float> input(static_cast<size_t>(periodSamples));
        std::vector<float> output(static_cast<size_t>(periodSamples));

        for (int i = 0; i < periodSamples; ++i)
        {
            const double phase = juce::MathConstants<double>::twoPi * static_cast<double>(i)
                                 / static_cast<double>(periodSamples);
            input[static_cast<size_t>(i)] = inputScale * static_cast<float>(std::sin(phase));
        }

        SinePreviewSetupParams setup;
        setup.circuit = circuit;
        setup.model = model;
        setup.diodeModel = diodeModel;
        setup.bjtModel = bjtModel;
        setup.jfetModel = jfetModel;
        setup.gainControl = gainControl;
        setup.secondaryControl = secondaryControl;
        setup.tertiaryControl = tertiaryControl;
        setup.potTaper = potTaper;
        setup.sampleRateHz = safe.sampleRateHz;
        setup.componentValues = componentValues;

        double vcc = 9.0;
        if (! engine.runProcess(setup,
                                input.data(),
                                output.data(),
                                static_cast<size_t>(periodSamples),
                                vcc))
        {
            return preview;
        }

        preview.vccHalf = static_cast<float>(vcc * 0.5);
        preview.groundReferenced = (circuit == CircuitKind::BjtCommonEmitter);
        constexpr float kEndpointPadFraction = 0.02f;
        const float endpointPad = preview.vccHalf * kEndpointPadFraction;
        if (preview.groundReferenced)
        {
            preview.axis.minY = -endpointPad;
            preview.axis.maxY = static_cast<float>(vcc) + endpointPad;
        }
        else
        {
            preview.axis.minY = -preview.vccHalf - endpointPad;
            preview.axis.maxY = preview.vccHalf + endpointPad;
        }

        preview.outputCurve.reserve(static_cast<size_t>(kPreviewDisplayPoints));

        for (int i = 0; i < kPreviewDisplayPoints; ++i)
        {
            const float t = static_cast<float>(i) / static_cast<float>(kPreviewDisplayPoints - 1);
            const float srcIndex = t * static_cast<float>(periodSamples - 1);
            preview.outputCurve.emplace_back(t, lerpWaveSample(output, srcIndex));
        }

        preview.axis.minX = 0.0f;
        preview.axis.maxX = 1.0f;

        return preview;
    }

    double defaultPreviewAmplitude(CircuitKind circuit) noexcept
    {
        switch (circuit)
        {
        case CircuitKind::Ds1Clipper:
            return 0.05;
        case CircuitKind::BjtCommonEmitter:
            return 0.2;
        case CircuitKind::BjtFollower:
        case CircuitKind::BjtFollowerOut:
            return 0.01;
        case CircuitKind::Ds1Opamp:
            return 1.0;
        }

        return 1.0;
    }

} // namespace ds1_ac
