#include "Ds1OpampAcMath.h"

#include "SineWavePreviewEngine.h"
#include "SchematicComponentApply.h"

#include <atom/SvgView.h>

#include <cmath>
#include <cstddef>
#include <iterator>
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
        enum DeviceFlag : unsigned
        {
            DevNone = 0,
            DevOpamp = 1u << 0,
            DevDiode = 1u << 1,
            DevBjt = 1u << 2,
        };

        struct StageInfo
        {
            CircuitKind kind;
            const char* topologyId;
            const char* operatorKey;
            const char* label;
            const char* processFn;
            const char* svgRel;
            const char* primaryControl;
            const char* secondaryControl;
            const char* tertiaryControl;
            unsigned devices;
            bool cheapAc;
            bool groundReferenced;
            double previewAmp;
        };

        constexpr StageInfo kCircuits[] = {
            {CircuitKind::BjtFollower, "input", "bjt_follower", "Input BJT",
             "nx_bjt_follower_process_f32", "assets/schematics/core/nonlinear_circuits/bjt_follower.svg",
             nullptr, nullptr, nullptr, DevBjt, true, false, 0.01},
            {CircuitKind::BjtCommonEmitter, "emitter", "bjt_common_emitter", "BJT Emitter",
             "nx_bjt_common_emitter_process_f32", "assets/schematics/core/nonlinear_circuits/bjt_common_emitter.svg",
             nullptr, nullptr, nullptr, DevBjt, true, true, 0.2},
            {CircuitKind::Ds1Opamp, "opamp", "ds1_opamp", "Op Amp",
             "nx_ds1_opamp_process_f32", "assets/schematics/core/linear_circuits/ds1_opamp.svg",
             "Gain", nullptr, nullptr, DevOpamp, true, false, 1.0},
            {CircuitKind::Ds1Clipper, "clipper", "ds1_clipper", "Clipper / Tone / Level",
             "nx_ds1_clipper_process_f32", "assets/schematics/core/nonlinear_circuits/ds1_clipper.svg",
             "Tone", "Level", nullptr, DevDiode, false, false, 0.05},
            {CircuitKind::BjtFollowerOut, "output", "bjt_follower_out", "Output BJT",
             "nx_bjt_follower_out_process_f32", "assets/schematics/core/nonlinear_circuits/bjt_follower_out.svg",
             nullptr, nullptr, nullptr, DevBjt, true, false, 0.01},
            {CircuitKind::Od1Drive, "drive", "od1_drive", "OD-1 Drive",
             "nx_od1_drive_process_f32", "assets/schematics/core/nonlinear_circuits/od1_drive.svg",
             "Drive", nullptr, nullptr, DevOpamp | DevDiode, false, false, 0.05},
            {CircuitKind::Sd1Tone, "tone", "sd1_tone", "SD-1 Tone",
             "nx_sd1_tone_process_f32", "assets/schematics/core/linear_circuits/sd1_tone.svg",
             "Tone", nullptr, nullptr, DevOpamp, true, false, 1.0},
            {CircuitKind::RcLevel, "level", "rc_level", "Level",
             "nx_rc_level_process_f32", "assets/schematics/core/linear_circuits/rc_level.svg",
             "Level", nullptr, nullptr, DevNone, true, false, 1.0},
            {CircuitKind::Od1Post, "post", "od1_post", "OD-1 Post",
             "nx_od1_post_process_f32", "assets/schematics/core/linear_circuits/od1_post.svg",
             nullptr, nullptr, nullptr, DevOpamp, true, false, 1.0},
            {CircuitKind::AcBoosterDrive, "drive", "ac_booster_drive", "AC Booster Drive",
             "nx_ac_booster_drive_process_f32", "assets/schematics/core/nonlinear_circuits/ac_booster_drive.svg",
             "Gain", nullptr, nullptr, DevOpamp | DevDiode, false, false, 0.05},
            {CircuitKind::AcBoosterEq, "eq", "ac_booster_eq", "AC Booster EQ",
             "nx_ac_booster_eq_process_f32", "assets/schematics/core/linear_circuits/ac_booster_eq.svg",
             "Bass", "Treble", nullptr, DevOpamp, true, false, 1.0},
            {CircuitKind::RcBoosterDrive1, "drive", "rc_booster_drive1", "RC Booster Drive",
             "nx_rc_booster_drive1_process_f32", "assets/schematics/core/nonlinear_circuits/rc_booster_drive1.svg",
             "Gain", nullptr, nullptr, DevOpamp | DevDiode, false, false, 0.05},
            {CircuitKind::DsPlusOpamp, "opamp", "ds_plus_opamp", "Distortion+ Opamp",
             "nx_ds_plus_opamp_process_f32", "assets/schematics/core/linear_circuits/ds_plus_opamp.svg",
             "Distortion", nullptr, nullptr, DevOpamp, true, false, 1.0},
            {CircuitKind::Ts9Opamp, "opamp", "ts9_opamp", "TS9 Opamp",
             "nx_ts9_opamp_process_f32", "assets/schematics/core/nonlinear_circuits/ts9_opamp.svg",
             "Drive", nullptr, nullptr, DevOpamp | DevDiode, false, false, 0.05},
            {CircuitKind::Ts9Tone, "tone", "ts9_tone", "TS9 Tone",
             "nx_ts9_tone_process_f32", "assets/schematics/core/linear_circuits/ts9_tone.svg",
             "Tone", nullptr, nullptr, DevOpamp, true, false, 1.0},
            {CircuitKind::KlonCentaur, "drive", "klon_centaur", "Klon Drive",
             "nx_klon_centaur_process_f32", "assets/schematics/core/nonlinear_circuits/klon_centaur.svg",
             "Gain", nullptr, nullptr, DevOpamp | DevDiode, false, false, 0.05},
            {CircuitKind::KlonCentaurTone, "tone", "klon_centaur_tone", "Klon Tone",
             "nx_klon_centaur_tone_process_f32", "assets/schematics/core/linear_circuits/klon_centaur_tone.svg",
             "Treble", nullptr, nullptr, DevOpamp, true, false, 1.0},
            {CircuitKind::GuvnorPreamp, "preamp", "guvnor_preamp", "Preamp",
             "nx_guvnor_preamp_process_f32", "assets/schematics/core/linear_circuits/guvnor_preamp.svg",
             "Gain", nullptr, nullptr, DevOpamp, true, false, 1.0},
            {CircuitKind::GuvnorPostamp, "postamp", "guvnor_postamp", "Postamp",
             "nx_guvnor_postamp_process_f32", "assets/schematics/core/linear_circuits/guvnor_postamp.svg",
             "Gain", nullptr, nullptr, DevOpamp, true, false, 1.0},
            {CircuitKind::GuvnorClipper, "clipper", "guvnor_clipper", "Clipper",
             "nx_guvnor_clipper_process_f32", "assets/schematics/core/nonlinear_circuits/guvnor_clipper.svg",
             "Bass", "Mid", "Treble", DevDiode, false, false, 0.05},
            {CircuitKind::GuvnorLevel, "level", "guvnor_level", "Level",
             "nx_guvnor_level_process_f32", "assets/schematics/core/linear_circuits/guvnor_level.svg",
             "Level", nullptr, nullptr, DevNone, true, false, 1.0},
            {CircuitKind::DiodeClipper, "clipper", "diode_clipper", "Diode Clipper",
             "nx_diode_clipper_process_f32", "assets/schematics/core/nonlinear_circuits/diode_clipper.svg",
             "Level", nullptr, nullptr, DevDiode, false, false, 0.05},
        };

        struct PedalStage
        {
            CircuitKind kind;
            const char* topologyId;
            const char* label;
        };

        struct PedalInfo
        {
            PedalKind kind;
            const char* catalogKey;
            const char* displayName;
            const char* svgRel;
            const PedalStage* stages;
            int stageCount;
        };

        constexpr PedalStage kDs1PedalStages[] = {
            {CircuitKind::BjtFollower, "input", "Input BJT"},
            {CircuitKind::BjtCommonEmitter, "emitter", "BJT Emitter"},
            {CircuitKind::Ds1Opamp, "opamp", "Op Amp"},
            {CircuitKind::Ds1Clipper, "clipper", "Clipper / Tone / Level"},
            {CircuitKind::BjtFollowerOut, "output", "Output BJT"},
        };
        constexpr PedalStage kSd1PedalStages[] = {
            {CircuitKind::BjtFollower, "input", "Input Follower"},
            {CircuitKind::Od1Drive, "drive", "OD-1 Drive"},
            {CircuitKind::Sd1Tone, "tone", "Tone"},
            {CircuitKind::RcLevel, "level", "Level"},
            {CircuitKind::BjtFollowerOut, "output", "Output Follower"},
        };
        constexpr PedalStage kOd1PedalStages[] = {
            {CircuitKind::BjtFollower, "input", "Input Follower"},
            {CircuitKind::Od1Drive, "drive", "Drive"},
            {CircuitKind::Od1Post, "post", "Post"},
            {CircuitKind::RcLevel, "level", "Level"},
            {CircuitKind::BjtFollowerOut, "output", "Output Follower"},
        };
        constexpr PedalStage kAcBoosterPedalStages[] = {
            {CircuitKind::BjtFollower, "input", "Input Follower"},
            {CircuitKind::AcBoosterDrive, "drive", "Drive"},
            {CircuitKind::AcBoosterEq, "eq", "EQ"},
            {CircuitKind::RcLevel, "level", "Level"},
            {CircuitKind::BjtFollowerOut, "output", "Output Follower"},
        };
        constexpr PedalStage kRcBoosterPedalStages[] = {
            {CircuitKind::BjtFollower, "input", "Input Follower"},
            {CircuitKind::RcBoosterDrive1, "drive", "Drive"},
            {CircuitKind::AcBoosterEq, "eq", "EQ"},
            {CircuitKind::RcLevel, "level", "Level"},
            {CircuitKind::BjtFollowerOut, "output", "Output Follower"},
        };
        constexpr PedalStage kDistortionPlusPedalStages[] = {
            {CircuitKind::DsPlusOpamp, "opamp", "Opamp"},
            {CircuitKind::DiodeClipper, "clipper", "Diode Clipper"},
        };
        constexpr PedalStage kTs9PedalStages[] = {
            {CircuitKind::BjtFollower, "input", "Input Follower"},
            {CircuitKind::Ts9Opamp, "opamp", "TS9 Opamp"},
            {CircuitKind::Ts9Tone, "tone", "Tone"},
            {CircuitKind::RcLevel, "level", "Level"},
            {CircuitKind::BjtFollowerOut, "output", "Output Follower"},
        };
        constexpr PedalStage kKlonPedalStages[] = {
            {CircuitKind::KlonCentaur, "drive", "Drive"},
            {CircuitKind::KlonCentaurTone, "tone", "Tone"},
            {CircuitKind::RcLevel, "level", "Level"},
        };
        constexpr PedalStage kGuvnorPedalStages[] = {
            {CircuitKind::GuvnorPreamp, "preamp", "Preamp"},
            {CircuitKind::GuvnorPostamp, "postamp", "Postamp"},
            {CircuitKind::GuvnorClipper, "clipper", "Clipper"},
            {CircuitKind::GuvnorLevel, "level", "Level"},
        };

        constexpr PedalInfo kPedals[] = {
            {PedalKind::Ds1, "ds1", "Boss DS-1",
             "assets/schematics/extensions/white_box/pedals/ds1.svg",
             kDs1PedalStages, static_cast<int>(std::size(kDs1PedalStages))},
            {PedalKind::Sd1, "sd1", "Boss SD-1",
             "assets/schematics/extensions/white_box/pedals/sd1.svg",
             kSd1PedalStages, static_cast<int>(std::size(kSd1PedalStages))},
            {PedalKind::Od1, "od1", "Boss OD-1",
             "assets/schematics/extensions/white_box/pedals/od1.svg",
             kOd1PedalStages, static_cast<int>(std::size(kOd1PedalStages))},
            {PedalKind::AcBooster, "ac_booster", "AC Booster",
             "assets/schematics/extensions/white_box/pedals/ac_booster.svg",
             kAcBoosterPedalStages, static_cast<int>(std::size(kAcBoosterPedalStages))},
            {PedalKind::RcBooster, "rc_booster", "RC Booster",
             "assets/schematics/extensions/white_box/pedals/rc_booster.svg",
             kRcBoosterPedalStages, static_cast<int>(std::size(kRcBoosterPedalStages))},
            {PedalKind::DistortionPlus, "distortion_plus", "Distortion+",
             "assets/schematics/extensions/white_box/pedals/distortion_plus.svg",
             kDistortionPlusPedalStages, static_cast<int>(std::size(kDistortionPlusPedalStages))},
            {PedalKind::Ts9, "ts9", "Boss TS-9",
             "assets/schematics/extensions/white_box/pedals/ts9.svg",
             kTs9PedalStages, static_cast<int>(std::size(kTs9PedalStages))},
            {PedalKind::Klon, "klon", "Klon Centaur",
             "assets/schematics/extensions/white_box/pedals/klon.svg",
             kKlonPedalStages, static_cast<int>(std::size(kKlonPedalStages))},
            {PedalKind::Guvnor, "guvnor", "Guv'nor",
             "assets/schematics/extensions/white_box/pedals/guvnor.svg",
             kGuvnorPedalStages, static_cast<int>(std::size(kGuvnorPedalStages))},
        };

        const StageInfo& stageInfo(CircuitKind circuit) noexcept
        {
            for (const auto& stage : kCircuits)
            {
                if (stage.kind == circuit)
                    return stage;
            }

            return kCircuits[0];
        }

        const PedalInfo& pedalInfo(PedalKind pedal) noexcept
        {
            for (const auto& info : kPedals)
            {
                if (info.kind == pedal)
                    return info;
            }

            return kPedals[0];
        }

        const PedalStage* findPedalStage(PedalKind pedal, CircuitKind circuit) noexcept
        {
            const auto& info = pedalInfo(pedal);
            for (int i = 0; i < info.stageCount; ++i)
            {
                if (info.stages[i].kind == circuit)
                    return &info.stages[i];
            }

            return nullptr;
        }
    } // namespace

    const char* compositeDisplayName() noexcept
    {
        return compositeDisplayName(PedalKind::Ds1);
    }

    const char* compositeDisplayName(PedalKind pedal) noexcept
    {
        return pedalInfo(pedal).displayName;
    }

    const char* compositeSvgRelativePath() noexcept
    {
        return compositeSvgRelativePath(PedalKind::Ds1);
    }

    const char* compositeSvgRelativePath(PedalKind pedal) noexcept
    {
        return pedalInfo(pedal).svgRel;
    }

    const char* pedalCatalogKey(PedalKind pedal) noexcept
    {
        return pedalInfo(pedal).catalogKey;
    }

    int pedalStageCount(PedalKind pedal) noexcept
    {
        return pedalInfo(pedal).stageCount;
    }

    CircuitKind pedalStageCircuit(PedalKind pedal, int index) noexcept
    {
        const auto& info = pedalInfo(pedal);
        if (index < 0 || index >= info.stageCount)
            return info.stages[0].kind;
        return info.stages[index].kind;
    }

    const char* pedalStageTopologyId(PedalKind pedal, int index) noexcept
    {
        const auto& info = pedalInfo(pedal);
        if (index < 0 || index >= info.stageCount)
            return info.stages[0].topologyId;
        return info.stages[index].topologyId;
    }

    const char* pedalStageLabel(PedalKind pedal, int index) noexcept
    {
        const auto& info = pedalInfo(pedal);
        if (index < 0 || index >= info.stageCount)
            return info.stages[0].label;
        return info.stages[index].label;
    }

    juce::String pedalStageMenuLabel(PedalKind pedal, int index)
    {
        return juce::String(pedalStageTopologyId(pedal, index)) + " — " + pedalStageLabel(pedal, index);
    }

    const char* circuitTopologyId(CircuitKind circuit) noexcept
    {
        return stageInfo(circuit).topologyId;
    }

    const char* circuitTopologyId(PedalKind pedal, CircuitKind circuit) noexcept
    {
        if (const auto* stage = findPedalStage(pedal, circuit))
            return stage->topologyId;
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
        const auto* name = stageInfo(circuit).primaryControl;
        return name != nullptr ? name : "Control";
    }

    const char* secondaryControlParameterName(CircuitKind circuit) noexcept
    {
        const auto* name = stageInfo(circuit).secondaryControl;
        return name != nullptr ? name : "Control";
    }

    const char* tertiaryControlParameterName(CircuitKind circuit) noexcept
    {
        const auto* name = stageInfo(circuit).tertiaryControl;
        return name != nullptr ? name : "Control";
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
        return (stageInfo(circuit).devices & DevOpamp) != 0;
    }

    bool circuitUsesDiodeModel(CircuitKind circuit) noexcept
    {
        return (stageInfo(circuit).devices & DevDiode) != 0;
    }

    bool circuitUsesBjtModel(CircuitKind circuit) noexcept
    {
        return (stageInfo(circuit).devices & DevBjt) != 0;
    }

    bool circuitAcSweepIsCheap(CircuitKind circuit) noexcept
    {
        return stageInfo(circuit).cheapAc;
    }

    bool circuitUsesJfetModel(CircuitKind) noexcept
    {
        return false;
    }

    bool circuitHasPrimaryControl(CircuitKind circuit) noexcept
    {
        return stageInfo(circuit).primaryControl != nullptr;
    }

    bool circuitUsesPotTaper(CircuitKind circuit) noexcept
    {
        return circuitHasPrimaryControl(circuit);
    }

    bool circuitHasSecondaryControl(CircuitKind circuit) noexcept
    {
        return stageInfo(circuit).secondaryControl != nullptr;
    }

    bool circuitHasTertiaryControl(CircuitKind circuit) noexcept
    {
        return stageInfo(circuit).tertiaryControl != nullptr;
    }

    nx_pot_taper_e defaultPotTaper(CircuitKind circuit) noexcept
    {
        switch (circuit)
        {
        case CircuitKind::Sd1Tone:
        case CircuitKind::Ts9Tone:
            return NX_POT_TAPER_G;
        case CircuitKind::DsPlusOpamp:
            return NX_POT_TAPER_C;
        case CircuitKind::BjtFollower:
        case CircuitKind::BjtCommonEmitter:
        case CircuitKind::Ds1Opamp:
        case CircuitKind::Ds1Clipper:
        case CircuitKind::BjtFollowerOut:
        case CircuitKind::Od1Drive:
        case CircuitKind::RcLevel:
        case CircuitKind::Od1Post:
        case CircuitKind::AcBoosterDrive:
        case CircuitKind::AcBoosterEq:
        case CircuitKind::RcBoosterDrive1:
        case CircuitKind::Ts9Opamp:
        case CircuitKind::KlonCentaur:
        case CircuitKind::KlonCentaurTone:
        case CircuitKind::GuvnorPreamp:
        case CircuitKind::GuvnorPostamp:
        case CircuitKind::GuvnorClipper:
        case CircuitKind::GuvnorLevel:
        case CircuitKind::DiodeClipper:
            return NX_POT_TAPER_LINEAR;
        }

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
                                       double tertiaryControl,
                                       nx_pot_taper_e potTaper)
        {
            juce::ignoreUnused(jfetModel);

            juce::String title = circuitStageMenuLabel(circuit);

            if (circuitUsesOpampModel(circuit))
                title += "  |  " + juce::String(opampModelDisplayName(model));
            else if (circuitUsesDiodeModel(circuit))
                title += "  |  " + juce::String(diodeModelDisplayName(diodeModel));
            else if (circuitUsesBjtModel(circuit))
                title += "  |  " + juce::String(bjtModelDisplayName(bjtModel));

            if (circuitHasPrimaryControl(circuit))
                title += "  |  " + juce::String(controlParameterName(circuit)) + " " + juce::String(control, 2)
                         + "  |  " + juce::String(potTaperDisplayName(potTaper));
            if (circuitHasSecondaryControl(circuit))
                title += "  |  " + juce::String(secondaryControlParameterName(circuit)) + " "
                         + juce::String(secondaryControl, 2);
            if (circuitHasTertiaryControl(circuit))
                title += "  |  " + juce::String(tertiaryControlParameterName(circuit)) + " "
                         + juce::String(tertiaryControl, 2);

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

        template<typename Inst, typename Config>
        void runConfigAcSweep(Inst* inst,
                              nx_result_t (*getConfig)(const Inst*, Config*),
                              void (*acStatic)(const Config*, double, const double*, double*, double*, size_t),
                              double sampleRateHz,
                              const std::vector<double>& freqs,
                              std::vector<double>& magDb,
                              std::vector<double>& phaseDeg)
        {
            if (inst == nullptr || getConfig == nullptr || acStatic == nullptr)
                return;

            Config cfg{};
            if (getConfig(inst, &cfg) != NX_SUCCESS)
                return;
            acStatic(&cfg, sampleRateHz, freqs.data(), magDb.data(), phaseDeg.data(), freqs.size());
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

        void runOd1DriveAcSweep(nx_od1_drive_f32_t* drive,
                                nx_opamp_model_e model,
                                nx_diode_model_t diodeModel,
                                nx_pot_taper_e potTaper,
                                double control,
                                double sampleRateHz,
                                const std::vector<double>& freqs,
                                std::vector<double>& magDb,
                                std::vector<double>& phaseDeg)
        {
            updatePotTaper(drive, nx_od1_drive_get_drive_pot_f32, nx_od1_drive_set_drive_pot_f32, potTaper);
            nx_od1_drive_set_opamp_model_f32(drive, model);
            nx_od1_drive_set_diode_model_f32(drive, diodeModel);
            nx_od1_drive_set_drive_control_f32(drive, clampControl(control));
            runConfigAcSweep(drive, nx_od1_drive_get_config_f32, nx_od1_drive_ac_static_f32,
                             sampleRateHz, freqs, magDb, phaseDeg);
        }

        void runSd1ToneAcSweep(nx_sd1_tone_f32_t* tone,
                               nx_opamp_model_e model,
                               nx_pot_taper_e potTaper,
                               double control,
                               double sampleRateHz,
                               const std::vector<double>& freqs,
                               std::vector<double>& magDb,
                               std::vector<double>& phaseDeg)
        {
            updatePotTaper(tone, nx_sd1_tone_get_tone_pot_f32, nx_sd1_tone_set_tone_pot_f32, potTaper);
            nx_sd1_tone_set_opamp_model_f32(tone, model);
            nx_sd1_tone_set_tone_control_f32(tone, clampControl(control));
            runConfigAcSweep(tone, nx_sd1_tone_get_config_f32, nx_sd1_tone_ac_static_f32,
                             sampleRateHz, freqs, magDb, phaseDeg);
        }

        void runRcLevelAcSweep(nx_rc_level_f32_t* level,
                               nx_pot_taper_e potTaper,
                               double control,
                               double sampleRateHz,
                               const std::vector<double>& freqs,
                               std::vector<double>& magDb,
                               std::vector<double>& phaseDeg)
        {
            updatePotTaper(level, nx_rc_level_get_level_pot_f32, nx_rc_level_set_level_pot_f32, potTaper);
            nx_rc_level_set_level_control_f32(level, clampControl(control));
            runConfigAcSweep(level, nx_rc_level_get_config_f32, nx_rc_level_ac_static_f32,
                             sampleRateHz, freqs, magDb, phaseDeg);
        }

        void runOd1PostAcSweep(nx_od1_post_f32_t* post,
                               nx_opamp_model_e model,
                               double sampleRateHz,
                               const std::vector<double>& freqs,
                               std::vector<double>& magDb,
                               std::vector<double>& phaseDeg)
        {
            nx_od1_post_set_opamp_model_f32(post, model);
            runConfigAcSweep(post, nx_od1_post_get_config_f32, nx_od1_post_ac_static_f32,
                             sampleRateHz, freqs, magDb, phaseDeg);
        }

        void runAcBoosterDriveAcSweep(nx_ac_booster_drive_f32_t* drive,
                                      nx_opamp_model_e model,
                                      nx_diode_model_t diodeModel,
                                      nx_pot_taper_e potTaper,
                                      double control,
                                      double sampleRateHz,
                                      const std::vector<double>& freqs,
                                      std::vector<double>& magDb,
                                      std::vector<double>& phaseDeg)
        {
            updatePotTaper(drive, nx_ac_booster_drive_get_gain_pot_f32, nx_ac_booster_drive_set_gain_pot_f32, potTaper);
            nx_ac_booster_drive_set_opamp_model_f32(drive, model);
            nx_ac_booster_drive_set_diode_model_f32(drive, diodeModel);
            nx_ac_booster_drive_set_gain_control_f32(drive, clampControl(control));
            runConfigAcSweep(drive, nx_ac_booster_drive_get_config_f32, nx_ac_booster_drive_ac_static_f32,
                             sampleRateHz, freqs, magDb, phaseDeg);
        }

        void runAcBoosterEqAcSweep(nx_ac_booster_eq_f32_t* eq,
                                   nx_opamp_model_e model,
                                   nx_pot_taper_e potTaper,
                                   double bassControl,
                                   double trebleControl,
                                   double sampleRateHz,
                                   const std::vector<double>& freqs,
                                   std::vector<double>& magDb,
                                   std::vector<double>& phaseDeg)
        {
            updatePotTaper(eq, nx_ac_booster_eq_get_bass_pot_f32, nx_ac_booster_eq_set_bass_pot_f32, potTaper);
            updatePotTaper(eq, nx_ac_booster_eq_get_treble_pot_f32, nx_ac_booster_eq_set_treble_pot_f32, potTaper);
            nx_ac_booster_eq_set_opamp_model_f32(eq, model);
            nx_ac_booster_eq_set_bass_control_f32(eq, clampControl(bassControl));
            nx_ac_booster_eq_set_treble_control_f32(eq, clampControl(trebleControl));
            runConfigAcSweep(eq, nx_ac_booster_eq_get_config_f32, nx_ac_booster_eq_ac_static_f32,
                             sampleRateHz, freqs, magDb, phaseDeg);
        }

        void runRcBoosterDrive1AcSweep(nx_rc_booster_drive1_f32_t* drive,
                                       nx_opamp_model_e model,
                                       nx_diode_model_t diodeModel,
                                       nx_pot_taper_e potTaper,
                                       double control,
                                       double sampleRateHz,
                                       const std::vector<double>& freqs,
                                       std::vector<double>& magDb,
                                       std::vector<double>& phaseDeg)
        {
            updatePotTaper(drive, nx_rc_booster_drive1_get_gain_pot_f32, nx_rc_booster_drive1_set_gain_pot_f32, potTaper);
            nx_rc_booster_drive1_set_opamp_model_f32(drive, model);
            nx_rc_booster_drive1_set_diode_model_f32(drive, diodeModel);
            nx_rc_booster_drive1_set_gain_control_f32(drive, clampControl(control));
            runConfigAcSweep(drive, nx_rc_booster_drive1_get_config_f32, nx_rc_booster_drive1_ac_static_f32,
                             sampleRateHz, freqs, magDb, phaseDeg);
        }

        void runDsPlusOpampAcSweep(nx_ds_plus_opamp_f32_t* opamp,
                                   nx_opamp_model_e model,
                                   nx_pot_taper_e potTaper,
                                   double control,
                                   double sampleRateHz,
                                   const std::vector<double>& freqs,
                                   std::vector<double>& magDb,
                                   std::vector<double>& phaseDeg)
        {
            updatePotTaper(opamp, nx_ds_plus_opamp_get_distortion_pot_f32, nx_ds_plus_opamp_set_distortion_pot_f32, potTaper);
            nx_ds_plus_opamp_set_opamp_model_f32(opamp, model);
            nx_ds_plus_opamp_set_distortion_control_f32(opamp, clampControl(control));
            runConfigAcSweep(opamp, nx_ds_plus_opamp_get_config_f32, nx_ds_plus_opamp_ac_static_f32,
                             sampleRateHz, freqs, magDb, phaseDeg);
        }

        void runDiodeClipperAcSweep(nx_diode_clipper_f32_t* clipper,
                                    nx_diode_model_t diodeModel,
                                    nx_pot_taper_e potTaper,
                                    double control,
                                    double sampleRateHz,
                                    const std::vector<double>& freqs,
                                    std::vector<double>& magDb,
                                    std::vector<double>& phaseDeg)
        {
            updatePotTaper(clipper, nx_diode_clipper_get_level_pot_f32, nx_diode_clipper_set_level_pot_f32, potTaper);
            nx_diode_clipper_set_diode_model_f32(clipper, diodeModel);
            nx_diode_clipper_set_level_control_f32(clipper, clampControl(control));
            runConfigAcSweep(clipper, nx_diode_clipper_get_config_f32, nx_diode_clipper_ac_static_f32,
                             sampleRateHz, freqs, magDb, phaseDeg);
        }

        void runTs9OpampAcSweep(nx_ts9_opamp_f32_t* opamp,
                                nx_opamp_model_e model,
                                nx_diode_model_t diodeModel,
                                nx_pot_taper_e potTaper,
                                double control,
                                double sampleRateHz,
                                const std::vector<double>& freqs,
                                std::vector<double>& magDb,
                                std::vector<double>& phaseDeg)
        {
            updatePotTaper(opamp, nx_ts9_opamp_get_drive_pot_f32, nx_ts9_opamp_set_drive_pot_f32, potTaper);
            nx_ts9_opamp_set_opamp_model_f32(opamp, model);
            nx_ts9_opamp_set_diode_model_f32(opamp, diodeModel);
            nx_ts9_opamp_set_drive_control_f32(opamp, clampControl(control));
            runConfigAcSweep(opamp, nx_ts9_opamp_get_config_f32, nx_ts9_opamp_ac_static_f32,
                             sampleRateHz, freqs, magDb, phaseDeg);
        }

        void runTs9ToneAcSweep(nx_ts9_tone_f32_t* tone,
                               nx_opamp_model_e model,
                               nx_pot_taper_e potTaper,
                               double control,
                               double sampleRateHz,
                               const std::vector<double>& freqs,
                               std::vector<double>& magDb,
                               std::vector<double>& phaseDeg)
        {
            updatePotTaper(tone, nx_ts9_tone_get_tone_pot_f32, nx_ts9_tone_set_tone_pot_f32, potTaper);
            nx_ts9_tone_set_opamp_model_f32(tone, model);
            nx_ts9_tone_set_tone_control_f32(tone, clampControl(control));
            runConfigAcSweep(tone, nx_ts9_tone_get_config_f32, nx_ts9_tone_ac_static_f32,
                             sampleRateHz, freqs, magDb, phaseDeg);
        }

        void runKlonCentaurAcSweep(nx_klon_centaur_f32_t* drive,
                                   nx_opamp_model_e model,
                                   nx_diode_model_t diodeModel,
                                   nx_pot_taper_e potTaper,
                                   double control,
                                   double sampleRateHz,
                                   const std::vector<double>& freqs,
                                   std::vector<double>& magDb,
                                   std::vector<double>& phaseDeg)
        {
            updatePotTaper(drive, nx_klon_centaur_get_gain_pot_f32, nx_klon_centaur_set_gain_pot_f32, potTaper);
            nx_klon_centaur_set_opamp_model_f32(drive, model);
            nx_klon_centaur_set_diode_model_f32(drive, diodeModel);
            nx_klon_centaur_set_gain_control_f32(drive, clampControl(control));
            runConfigAcSweep(drive, nx_klon_centaur_get_config_f32, nx_klon_centaur_ac_static_f32,
                             sampleRateHz, freqs, magDb, phaseDeg);
        }

        void runKlonCentaurToneAcSweep(nx_klon_centaur_tone_f32_t* tone,
                                       nx_opamp_model_e model,
                                       nx_pot_taper_e potTaper,
                                       double control,
                                       double sampleRateHz,
                                       const std::vector<double>& freqs,
                                       std::vector<double>& magDb,
                                       std::vector<double>& phaseDeg)
        {
            updatePotTaper(tone, nx_klon_centaur_tone_get_treble_pot_f32, nx_klon_centaur_tone_set_treble_pot_f32, potTaper);
            nx_klon_centaur_tone_set_opamp_model_f32(tone, model);
            nx_klon_centaur_tone_set_treble_control_f32(tone, clampControl(control));
            runConfigAcSweep(tone, nx_klon_centaur_tone_get_config_f32, nx_klon_centaur_tone_ac_static_f32,
                             sampleRateHz, freqs, magDb, phaseDeg);
        }

        void runGuvnorPreampAcSweep(nx_guvnor_preamp_f32_t* preamp,
                                    nx_opamp_model_e model,
                                    nx_pot_taper_e potTaper,
                                    double control,
                                    double sampleRateHz,
                                    const std::vector<double>& freqs,
                                    std::vector<double>& magDb,
                                    std::vector<double>& phaseDeg)
        {
            updatePotTaper(preamp, nx_guvnor_preamp_get_gain_pot_f32, nx_guvnor_preamp_set_gain_pot_f32, potTaper);
            nx_guvnor_preamp_set_opamp_model_f32(preamp, model);
            nx_guvnor_preamp_set_gain_control_f32(preamp, clampControl(control));
            runConfigAcSweep(preamp, nx_guvnor_preamp_get_config_f32, nx_guvnor_preamp_ac_static_f32,
                             sampleRateHz, freqs, magDb, phaseDeg);
        }

        void runGuvnorPostampAcSweep(nx_guvnor_postamp_f32_t* postamp,
                                     nx_opamp_model_e model,
                                     nx_pot_taper_e potTaper,
                                     double control,
                                     double sampleRateHz,
                                     const std::vector<double>& freqs,
                                     std::vector<double>& magDb,
                                     std::vector<double>& phaseDeg)
        {
            updatePotTaper(postamp, nx_guvnor_postamp_get_gain_pot_f32, nx_guvnor_postamp_set_gain_pot_f32, potTaper);
            nx_guvnor_postamp_set_opamp_model_f32(postamp, model);
            nx_guvnor_postamp_set_gain_control_f32(postamp, clampControl(control));
            runConfigAcSweep(postamp, nx_guvnor_postamp_get_config_f32, nx_guvnor_postamp_ac_static_f32,
                             sampleRateHz, freqs, magDb, phaseDeg);
        }

        void runGuvnorClipperAcSweep(nx_guvnor_clipper_f32_t* clipper,
                                     nx_diode_model_t diodeModel,
                                     nx_pot_taper_e potTaper,
                                     double bassControl,
                                     double midControl,
                                     double trebleControl,
                                     double sampleRateHz,
                                     const std::vector<double>& freqs,
                                     std::vector<double>& magDb,
                                     std::vector<double>& phaseDeg)
        {
            updatePotTaper(clipper, nx_guvnor_clipper_get_bass_pot_f32, nx_guvnor_clipper_set_bass_pot_f32, potTaper);
            updatePotTaper(clipper, nx_guvnor_clipper_get_mid_pot_f32, nx_guvnor_clipper_set_mid_pot_f32, potTaper);
            updatePotTaper(clipper, nx_guvnor_clipper_get_treble_pot_f32, nx_guvnor_clipper_set_treble_pot_f32, potTaper);
            nx_guvnor_clipper_set_diode_model_f32(clipper, diodeModel);
            nx_guvnor_clipper_set_bass_control_f32(clipper, clampControl(bassControl));
            nx_guvnor_clipper_set_mid_control_f32(clipper, clampControl(midControl));
            nx_guvnor_clipper_set_treble_control_f32(clipper, clampControl(trebleControl));
            runConfigAcSweep(clipper, nx_guvnor_clipper_get_config_f32, nx_guvnor_clipper_ac_static_f32,
                             sampleRateHz, freqs, magDb, phaseDeg);
        }

        void runGuvnorLevelAcSweep(nx_guvnor_level_f32_t* level,
                                   nx_pot_taper_e potTaper,
                                   double control,
                                   double sampleRateHz,
                                   const std::vector<double>& freqs,
                                   std::vector<double>& magDb,
                                   std::vector<double>& phaseDeg)
        {
            updatePotTaper(level, nx_guvnor_level_get_level_pot_f32, nx_guvnor_level_set_level_pot_f32, potTaper);
            nx_guvnor_level_set_level_control_f32(level, clampControl(control));
            runConfigAcSweep(level, nx_guvnor_level_get_config_f32, nx_guvnor_level_ac_static_f32,
                             sampleRateHz, freqs, magDb, phaseDeg);
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
            juce::ignoreUnused(jfetModel);

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
            case CircuitKind::Od1Post:
                juce::ignoreUnused(bjtModel, sampleRateHz, freqs, magDb, phaseDeg, componentValues);
                break;
            case CircuitKind::Od1Drive:
            {
                auto* drive = createCircuitInstance<nx_od1_drive_f32_t>(nx_od1_drive_create_f32, circuit, componentValues);
                if (drive == nullptr)
                    return;
                runOd1DriveAcSweep(drive, model, diodeModel, potTaper, 0.0, sampleRateHz, freqs, magDb, phaseDeg);
                accumulateMagnitudeExtents(magDb, minMag, maxMag);
                runOd1DriveAcSweep(drive, model, diodeModel, potTaper, 1.0, sampleRateHz, freqs, magDb, phaseDeg);
                accumulateMagnitudeExtents(magDb, minMag, maxMag);
                nx_od1_drive_destroy_f32(drive, nullptr);
                break;
            }
            case CircuitKind::Sd1Tone:
            {
                auto* tone = createCircuitInstance<nx_sd1_tone_f32_t>(nx_sd1_tone_create_f32, circuit, componentValues);
                if (tone == nullptr)
                    return;
                runSd1ToneAcSweep(tone, model, potTaper, 0.0, sampleRateHz, freqs, magDb, phaseDeg);
                accumulateMagnitudeExtents(magDb, minMag, maxMag);
                runSd1ToneAcSweep(tone, model, potTaper, 1.0, sampleRateHz, freqs, magDb, phaseDeg);
                accumulateMagnitudeExtents(magDb, minMag, maxMag);
                nx_sd1_tone_destroy_f32(tone, nullptr);
                break;
            }
            case CircuitKind::RcLevel:
            {
                auto* level = createCircuitInstance<nx_rc_level_f32_t>(nx_rc_level_create_f32, circuit, componentValues);
                if (level == nullptr)
                    return;
                runRcLevelAcSweep(level, potTaper, 0.0, sampleRateHz, freqs, magDb, phaseDeg);
                accumulateMagnitudeExtents(magDb, minMag, maxMag);
                runRcLevelAcSweep(level, potTaper, 1.0, sampleRateHz, freqs, magDb, phaseDeg);
                accumulateMagnitudeExtents(magDb, minMag, maxMag);
                nx_rc_level_destroy_f32(level, nullptr);
                break;
            }
            case CircuitKind::AcBoosterDrive:
            {
                auto* drive = createCircuitInstance<nx_ac_booster_drive_f32_t>(nx_ac_booster_drive_create_f32, circuit, componentValues);
                if (drive == nullptr)
                    return;
                runAcBoosterDriveAcSweep(drive, model, diodeModel, potTaper, 0.0, sampleRateHz, freqs, magDb, phaseDeg);
                accumulateMagnitudeExtents(magDb, minMag, maxMag);
                runAcBoosterDriveAcSweep(drive, model, diodeModel, potTaper, 1.0, sampleRateHz, freqs, magDb, phaseDeg);
                accumulateMagnitudeExtents(magDb, minMag, maxMag);
                nx_ac_booster_drive_destroy_f32(drive, nullptr);
                break;
            }
            case CircuitKind::AcBoosterEq:
            {
                auto* eq = createCircuitInstance<nx_ac_booster_eq_f32_t>(nx_ac_booster_eq_create_f32, circuit, componentValues);
                if (eq == nullptr)
                    return;
                const double trebleMid = clampControl(secondaryControl);
                runAcBoosterEqAcSweep(eq, model, potTaper, 0.0, trebleMid, sampleRateHz, freqs, magDb, phaseDeg);
                accumulateMagnitudeExtents(magDb, minMag, maxMag);
                runAcBoosterEqAcSweep(eq, model, potTaper, 1.0, trebleMid, sampleRateHz, freqs, magDb, phaseDeg);
                accumulateMagnitudeExtents(magDb, minMag, maxMag);
                runAcBoosterEqAcSweep(eq, model, potTaper, 0.5, 0.0, sampleRateHz, freqs, magDb, phaseDeg);
                accumulateMagnitudeExtents(magDb, minMag, maxMag);
                runAcBoosterEqAcSweep(eq, model, potTaper, 0.5, 1.0, sampleRateHz, freqs, magDb, phaseDeg);
                accumulateMagnitudeExtents(magDb, minMag, maxMag);
                nx_ac_booster_eq_destroy_f32(eq, nullptr);
                break;
            }
            case CircuitKind::RcBoosterDrive1:
            {
                auto* drive = createCircuitInstance<nx_rc_booster_drive1_f32_t>(nx_rc_booster_drive1_create_f32, circuit, componentValues);
                if (drive == nullptr)
                    return;
                runRcBoosterDrive1AcSweep(drive, model, diodeModel, potTaper, 0.0, sampleRateHz, freqs, magDb, phaseDeg);
                accumulateMagnitudeExtents(magDb, minMag, maxMag);
                runRcBoosterDrive1AcSweep(drive, model, diodeModel, potTaper, 1.0, sampleRateHz, freqs, magDb, phaseDeg);
                accumulateMagnitudeExtents(magDb, minMag, maxMag);
                nx_rc_booster_drive1_destroy_f32(drive, nullptr);
                break;
            }
            case CircuitKind::DsPlusOpamp:
            {
                auto* opamp = createCircuitInstance<nx_ds_plus_opamp_f32_t>(nx_ds_plus_opamp_create_f32, circuit, componentValues);
                if (opamp == nullptr)
                    return;
                runDsPlusOpampAcSweep(opamp, model, potTaper, 0.0, sampleRateHz, freqs, magDb, phaseDeg);
                accumulateMagnitudeExtents(magDb, minMag, maxMag);
                runDsPlusOpampAcSweep(opamp, model, potTaper, 1.0, sampleRateHz, freqs, magDb, phaseDeg);
                accumulateMagnitudeExtents(magDb, minMag, maxMag);
                nx_ds_plus_opamp_destroy_f32(opamp, nullptr);
                break;
            }
            case CircuitKind::Ts9Opamp:
            {
                auto* opamp = createCircuitInstance<nx_ts9_opamp_f32_t>(nx_ts9_opamp_create_f32, circuit, componentValues);
                if (opamp == nullptr)
                    return;
                runTs9OpampAcSweep(opamp, model, diodeModel, potTaper, 0.0, sampleRateHz, freqs, magDb, phaseDeg);
                accumulateMagnitudeExtents(magDb, minMag, maxMag);
                runTs9OpampAcSweep(opamp, model, diodeModel, potTaper, 1.0, sampleRateHz, freqs, magDb, phaseDeg);
                accumulateMagnitudeExtents(magDb, minMag, maxMag);
                nx_ts9_opamp_destroy_f32(opamp, nullptr);
                break;
            }
            case CircuitKind::Ts9Tone:
            {
                auto* tone = createCircuitInstance<nx_ts9_tone_f32_t>(nx_ts9_tone_create_f32, circuit, componentValues);
                if (tone == nullptr)
                    return;
                runTs9ToneAcSweep(tone, model, potTaper, 0.0, sampleRateHz, freqs, magDb, phaseDeg);
                accumulateMagnitudeExtents(magDb, minMag, maxMag);
                runTs9ToneAcSweep(tone, model, potTaper, 1.0, sampleRateHz, freqs, magDb, phaseDeg);
                accumulateMagnitudeExtents(magDb, minMag, maxMag);
                nx_ts9_tone_destroy_f32(tone, nullptr);
                break;
            }
            case CircuitKind::KlonCentaur:
            {
                auto* drive = createCircuitInstance<nx_klon_centaur_f32_t>(nx_klon_centaur_create_f32, circuit, componentValues);
                if (drive == nullptr)
                    return;
                runKlonCentaurAcSweep(drive, model, diodeModel, potTaper, 0.0, sampleRateHz, freqs, magDb, phaseDeg);
                accumulateMagnitudeExtents(magDb, minMag, maxMag);
                runKlonCentaurAcSweep(drive, model, diodeModel, potTaper, 1.0, sampleRateHz, freqs, magDb, phaseDeg);
                accumulateMagnitudeExtents(magDb, minMag, maxMag);
                nx_klon_centaur_destroy_f32(drive, nullptr);
                break;
            }
            case CircuitKind::KlonCentaurTone:
            {
                auto* tone = createCircuitInstance<nx_klon_centaur_tone_f32_t>(nx_klon_centaur_tone_create_f32, circuit, componentValues);
                if (tone == nullptr)
                    return;
                runKlonCentaurToneAcSweep(tone, model, potTaper, 0.0, sampleRateHz, freqs, magDb, phaseDeg);
                accumulateMagnitudeExtents(magDb, minMag, maxMag);
                runKlonCentaurToneAcSweep(tone, model, potTaper, 1.0, sampleRateHz, freqs, magDb, phaseDeg);
                accumulateMagnitudeExtents(magDb, minMag, maxMag);
                nx_klon_centaur_tone_destroy_f32(tone, nullptr);
                break;
            }
            case CircuitKind::GuvnorPreamp:
            {
                auto* preamp = createCircuitInstance<nx_guvnor_preamp_f32_t>(nx_guvnor_preamp_create_f32, circuit, componentValues);
                if (preamp == nullptr)
                    return;
                runGuvnorPreampAcSweep(preamp, model, potTaper, 0.0, sampleRateHz, freqs, magDb, phaseDeg);
                accumulateMagnitudeExtents(magDb, minMag, maxMag);
                runGuvnorPreampAcSweep(preamp, model, potTaper, 1.0, sampleRateHz, freqs, magDb, phaseDeg);
                accumulateMagnitudeExtents(magDb, minMag, maxMag);
                nx_guvnor_preamp_destroy_f32(preamp, nullptr);
                break;
            }
            case CircuitKind::GuvnorPostamp:
            {
                auto* postamp = createCircuitInstance<nx_guvnor_postamp_f32_t>(nx_guvnor_postamp_create_f32, circuit, componentValues);
                if (postamp == nullptr)
                    return;
                runGuvnorPostampAcSweep(postamp, model, potTaper, 0.0, sampleRateHz, freqs, magDb, phaseDeg);
                accumulateMagnitudeExtents(magDb, minMag, maxMag);
                runGuvnorPostampAcSweep(postamp, model, potTaper, 1.0, sampleRateHz, freqs, magDb, phaseDeg);
                accumulateMagnitudeExtents(magDb, minMag, maxMag);
                nx_guvnor_postamp_destroy_f32(postamp, nullptr);
                break;
            }
            case CircuitKind::GuvnorClipper:
            {
                auto* clipper = createCircuitInstance<nx_guvnor_clipper_f32_t>(nx_guvnor_clipper_create_f32, circuit, componentValues);
                if (clipper == nullptr)
                    return;
                const double midMid = clampControl(secondaryControl);
                const double trebleMid = clampControl(tertiaryControl);
                runGuvnorClipperAcSweep(clipper, diodeModel, potTaper, 0.0, midMid, trebleMid, sampleRateHz, freqs, magDb, phaseDeg);
                accumulateMagnitudeExtents(magDb, minMag, maxMag);
                runGuvnorClipperAcSweep(clipper, diodeModel, potTaper, 1.0, midMid, trebleMid, sampleRateHz, freqs, magDb, phaseDeg);
                accumulateMagnitudeExtents(magDb, minMag, maxMag);
                runGuvnorClipperAcSweep(clipper, diodeModel, potTaper, 0.5, 0.0, trebleMid, sampleRateHz, freqs, magDb, phaseDeg);
                accumulateMagnitudeExtents(magDb, minMag, maxMag);
                runGuvnorClipperAcSweep(clipper, diodeModel, potTaper, 0.5, 1.0, trebleMid, sampleRateHz, freqs, magDb, phaseDeg);
                accumulateMagnitudeExtents(magDb, minMag, maxMag);
                runGuvnorClipperAcSweep(clipper, diodeModel, potTaper, 0.5, midMid, 0.0, sampleRateHz, freqs, magDb, phaseDeg);
                accumulateMagnitudeExtents(magDb, minMag, maxMag);
                runGuvnorClipperAcSweep(clipper, diodeModel, potTaper, 0.5, midMid, 1.0, sampleRateHz, freqs, magDb, phaseDeg);
                accumulateMagnitudeExtents(magDb, minMag, maxMag);
                nx_guvnor_clipper_destroy_f32(clipper, nullptr);
                break;
            }
            case CircuitKind::GuvnorLevel:
            {
                auto* level = createCircuitInstance<nx_guvnor_level_f32_t>(nx_guvnor_level_create_f32, circuit, componentValues);
                if (level == nullptr)
                    return;
                runGuvnorLevelAcSweep(level, potTaper, 0.0, sampleRateHz, freqs, magDb, phaseDeg);
                accumulateMagnitudeExtents(magDb, minMag, maxMag);
                runGuvnorLevelAcSweep(level, potTaper, 1.0, sampleRateHz, freqs, magDb, phaseDeg);
                accumulateMagnitudeExtents(magDb, minMag, maxMag);
                nx_guvnor_level_destroy_f32(level, nullptr);
                break;
            }
            case CircuitKind::DiodeClipper:
            {
                auto* clipper = createCircuitInstance<nx_diode_clipper_f32_t>(nx_diode_clipper_create_f32, circuit, componentValues);
                if (clipper == nullptr)
                    return;
                runDiodeClipperAcSweep(clipper, diodeModel, potTaper, 0.0, sampleRateHz, freqs, magDb, phaseDeg);
                accumulateMagnitudeExtents(magDb, minMag, maxMag);
                runDiodeClipperAcSweep(clipper, diodeModel, potTaper, 1.0, sampleRateHz, freqs, magDb, phaseDeg);
                accumulateMagnitudeExtents(magDb, minMag, maxMag);
                nx_diode_clipper_destroy_f32(clipper, nullptr);
                break;
            }
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
            juce::ignoreUnused(jfetModel);

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
            case CircuitKind::Od1Drive:
            {
                auto* drive = createCircuitInstance<nx_od1_drive_f32_t>(nx_od1_drive_create_f32, circuit, componentValues);
                if (drive == nullptr)
                    return false;
                runOd1DriveAcSweep(drive, model, diodeModel, potTaper, gainControl, sampleRateHz, freqs, magDb, phaseDeg);
                nx_od1_drive_destroy_f32(drive, nullptr);
                return true;
            }
            case CircuitKind::Sd1Tone:
            {
                auto* tone = createCircuitInstance<nx_sd1_tone_f32_t>(nx_sd1_tone_create_f32, circuit, componentValues);
                if (tone == nullptr)
                    return false;
                runSd1ToneAcSweep(tone, model, potTaper, gainControl, sampleRateHz, freqs, magDb, phaseDeg);
                nx_sd1_tone_destroy_f32(tone, nullptr);
                return true;
            }
            case CircuitKind::RcLevel:
            {
                auto* level = createCircuitInstance<nx_rc_level_f32_t>(nx_rc_level_create_f32, circuit, componentValues);
                if (level == nullptr)
                    return false;
                runRcLevelAcSweep(level, potTaper, gainControl, sampleRateHz, freqs, magDb, phaseDeg);
                nx_rc_level_destroy_f32(level, nullptr);
                return true;
            }
            case CircuitKind::Od1Post:
            {
                auto* post = createCircuitInstance<nx_od1_post_f32_t>(nx_od1_post_create_f32, circuit, componentValues);
                if (post == nullptr)
                    return false;
                runOd1PostAcSweep(post, model, sampleRateHz, freqs, magDb, phaseDeg);
                nx_od1_post_destroy_f32(post, nullptr);
                return true;
            }
            case CircuitKind::AcBoosterDrive:
            {
                auto* drive = createCircuitInstance<nx_ac_booster_drive_f32_t>(nx_ac_booster_drive_create_f32, circuit, componentValues);
                if (drive == nullptr)
                    return false;
                runAcBoosterDriveAcSweep(drive, model, diodeModel, potTaper, gainControl, sampleRateHz, freqs, magDb, phaseDeg);
                nx_ac_booster_drive_destroy_f32(drive, nullptr);
                return true;
            }
            case CircuitKind::AcBoosterEq:
            {
                auto* eq = createCircuitInstance<nx_ac_booster_eq_f32_t>(nx_ac_booster_eq_create_f32, circuit, componentValues);
                if (eq == nullptr)
                    return false;
                runAcBoosterEqAcSweep(eq, model, potTaper, gainControl, secondaryControl, sampleRateHz, freqs, magDb, phaseDeg);
                nx_ac_booster_eq_destroy_f32(eq, nullptr);
                return true;
            }
            case CircuitKind::RcBoosterDrive1:
            {
                auto* drive = createCircuitInstance<nx_rc_booster_drive1_f32_t>(nx_rc_booster_drive1_create_f32, circuit, componentValues);
                if (drive == nullptr)
                    return false;
                runRcBoosterDrive1AcSweep(drive, model, diodeModel, potTaper, gainControl, sampleRateHz, freqs, magDb, phaseDeg);
                nx_rc_booster_drive1_destroy_f32(drive, nullptr);
                return true;
            }
            case CircuitKind::DsPlusOpamp:
            {
                auto* opamp = createCircuitInstance<nx_ds_plus_opamp_f32_t>(nx_ds_plus_opamp_create_f32, circuit, componentValues);
                if (opamp == nullptr)
                    return false;
                runDsPlusOpampAcSweep(opamp, model, potTaper, gainControl, sampleRateHz, freqs, magDb, phaseDeg);
                nx_ds_plus_opamp_destroy_f32(opamp, nullptr);
                return true;
            }
            case CircuitKind::Ts9Opamp:
            {
                auto* opamp = createCircuitInstance<nx_ts9_opamp_f32_t>(nx_ts9_opamp_create_f32, circuit, componentValues);
                if (opamp == nullptr)
                    return false;
                runTs9OpampAcSweep(opamp, model, diodeModel, potTaper, gainControl, sampleRateHz, freqs, magDb, phaseDeg);
                nx_ts9_opamp_destroy_f32(opamp, nullptr);
                return true;
            }
            case CircuitKind::Ts9Tone:
            {
                auto* tone = createCircuitInstance<nx_ts9_tone_f32_t>(nx_ts9_tone_create_f32, circuit, componentValues);
                if (tone == nullptr)
                    return false;
                runTs9ToneAcSweep(tone, model, potTaper, gainControl, sampleRateHz, freqs, magDb, phaseDeg);
                nx_ts9_tone_destroy_f32(tone, nullptr);
                return true;
            }
            case CircuitKind::KlonCentaur:
            {
                auto* drive = createCircuitInstance<nx_klon_centaur_f32_t>(nx_klon_centaur_create_f32, circuit, componentValues);
                if (drive == nullptr)
                    return false;
                runKlonCentaurAcSweep(drive, model, diodeModel, potTaper, gainControl, sampleRateHz, freqs, magDb, phaseDeg);
                nx_klon_centaur_destroy_f32(drive, nullptr);
                return true;
            }
            case CircuitKind::KlonCentaurTone:
            {
                auto* tone = createCircuitInstance<nx_klon_centaur_tone_f32_t>(nx_klon_centaur_tone_create_f32, circuit, componentValues);
                if (tone == nullptr)
                    return false;
                runKlonCentaurToneAcSweep(tone, model, potTaper, gainControl, sampleRateHz, freqs, magDb, phaseDeg);
                nx_klon_centaur_tone_destroy_f32(tone, nullptr);
                return true;
            }
            case CircuitKind::GuvnorPreamp:
            {
                auto* preamp = createCircuitInstance<nx_guvnor_preamp_f32_t>(nx_guvnor_preamp_create_f32, circuit, componentValues);
                if (preamp == nullptr)
                    return false;
                runGuvnorPreampAcSweep(preamp, model, potTaper, gainControl, sampleRateHz, freqs, magDb, phaseDeg);
                nx_guvnor_preamp_destroy_f32(preamp, nullptr);
                return true;
            }
            case CircuitKind::GuvnorPostamp:
            {
                auto* postamp = createCircuitInstance<nx_guvnor_postamp_f32_t>(nx_guvnor_postamp_create_f32, circuit, componentValues);
                if (postamp == nullptr)
                    return false;
                runGuvnorPostampAcSweep(postamp, model, potTaper, gainControl, sampleRateHz, freqs, magDb, phaseDeg);
                nx_guvnor_postamp_destroy_f32(postamp, nullptr);
                return true;
            }
            case CircuitKind::GuvnorClipper:
            {
                auto* clipper = createCircuitInstance<nx_guvnor_clipper_f32_t>(nx_guvnor_clipper_create_f32, circuit, componentValues);
                if (clipper == nullptr)
                    return false;
                runGuvnorClipperAcSweep(clipper,
                                        diodeModel,
                                        potTaper,
                                        gainControl,
                                        secondaryControl,
                                        tertiaryControl,
                                        sampleRateHz,
                                        freqs,
                                        magDb,
                                        phaseDeg);
                nx_guvnor_clipper_destroy_f32(clipper, nullptr);
                return true;
            }
            case CircuitKind::GuvnorLevel:
            {
                auto* level = createCircuitInstance<nx_guvnor_level_f32_t>(nx_guvnor_level_create_f32, circuit, componentValues);
                if (level == nullptr)
                    return false;
                runGuvnorLevelAcSweep(level, potTaper, gainControl, sampleRateHz, freqs, magDb, phaseDeg);
                nx_guvnor_level_destroy_f32(level, nullptr);
                return true;
            }
            case CircuitKind::DiodeClipper:
            {
                auto* clipper = createCircuitInstance<nx_diode_clipper_f32_t>(nx_diode_clipper_create_f32, circuit, componentValues);
                if (clipper == nullptr)
                    return false;
                runDiodeClipperAcSweep(clipper, diodeModel, potTaper, gainControl, sampleRateHz, freqs, magDb, phaseDeg);
                nx_diode_clipper_destroy_f32(clipper, nullptr);
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
        response.title = makeResponseTitle(circuit, model, diodeModel, bjtModel, jfetModel, gainControl, secondaryControl, tertiaryControl, potTaper);

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
        preview.groundReferenced = stageInfo(circuit).groundReferenced;
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
        return stageInfo(circuit).previewAmp;
    }

} // namespace ds1_ac
