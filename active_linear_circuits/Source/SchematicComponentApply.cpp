#include "SchematicComponentApply.h"

#include "nudsp/linear_circuits/ac_booster_eq_f32.h"
#include "nudsp/linear_circuits/ds1_opamp_f32.h"
#include "nudsp/linear_circuits/ds_plus_opamp_f32.h"
#include "nudsp/linear_circuits/guvnor_level_f32.h"
#include "nudsp/linear_circuits/guvnor_postamp_f32.h"
#include "nudsp/linear_circuits/guvnor_preamp_f32.h"
#include "nudsp/linear_circuits/klon_centaur_tone_f32.h"
#include "nudsp/linear_circuits/od1_post_f32.h"
#include "nudsp/linear_circuits/rc_level_f32.h"
#include "nudsp/linear_circuits/sd1_tone_f32.h"
#include "nudsp/linear_circuits/ts9_tone_f32.h"
#include "nudsp/nonlinear_circuits/ac_booster_drive_f32.h"
#include "nudsp/nonlinear_circuits/bjt_common_emitter_f32.h"
#include "nudsp/nonlinear_circuits/bjt_follower_f32.h"
#include "nudsp/nonlinear_circuits/bjt_follower_out_f32.h"
#include "nudsp/nonlinear_circuits/diode_clipper_f32.h"
#include "nudsp/nonlinear_circuits/ds1_clipper_f32.h"
#include "nudsp/nonlinear_circuits/guvnor_clipper_f32.h"
#include "nudsp/nonlinear_circuits/klon_centaur_f32.h"
#include "nudsp/nonlinear_circuits/od1_drive_f32.h"
#include "nudsp/nonlinear_circuits/rc_booster_drive1_f32.h"
#include "nudsp/nonlinear_circuits/ts9_opamp_f32.h"

#include <cmath>
#include <type_traits>

namespace ds1_ac
{
namespace
{

juce::String canonicalComponentKey(juce::String key)
{
    const int colon = key.indexOfChar(':');
    if (colon >= 0)
        key = key.substring(colon + 1);
    return key.trim();
}

template<typename Config>
void applyCommonFields(Config& cfg, const juce::String& rawKey, double value)
{
    const juce::String key = canonicalComponentKey(rawKey);

    auto set = [&](auto member, const char* name)
    {
        if (key.equalsIgnoreCase(name))
            cfg.*member = value;
    };

    auto applyPotValue = [&](auto& pot, const char* controlName, const char* const* valueNames, int valueNameCount)
    {
        if (key.equalsIgnoreCase(controlName))
            pot.pot_params.control = juce::jlimit(0.0, 1.0, value);
        else
        {
            for (int i = 0; i < valueNameCount; ++i)
            {
                if (key.equalsIgnoreCase(valueNames[i]))
                    pot.pot_params.value = value;
            }
        }
    };

    if constexpr (requires { cfg.gain.pot_params.value; })
    {
        static constexpr const char* names[] = {"P1", "GAIN"};
        applyPotValue(cfg.gain, "GAIN", names, 2);
    }
    if constexpr (requires { cfg.drive.pot_params.value; })
    {
        static constexpr const char* names[] = {"P1", "DRIVE"};
        applyPotValue(cfg.drive, "DRIVE", names, 2);
    }
    if constexpr (requires { cfg.distortion.pot_params.value; })
    {
        static constexpr const char* names[] = {"P1", "DISTORTION"};
        applyPotValue(cfg.distortion, "DISTORTION", names, 2);
    }
    if constexpr (requires { cfg.tone.pot_params.value; })
    {
        static constexpr const char* names[] = {"P1", "RT", "Rt", "TONE"};
        applyPotValue(cfg.tone, "TONE", names, 4);
    }
    if constexpr (requires { cfg.level.pot_params.value; })
    {
        static constexpr const char* names[] = {"P1", "RV", "Rv", "LEVEL"};
        applyPotValue(cfg.level, "LEVEL", names, 4);
    }
    if constexpr (requires { cfg.bass.pot_params.value; })
    {
        static constexpr const char* names[] = {"P1", "BASS"};
        applyPotValue(cfg.bass, "BASS", names, 2);
    }
    if constexpr (requires { cfg.mid.pot_params.value; })
    {
        static constexpr const char* names[] = {"P2", "MID"};
        applyPotValue(cfg.mid, "MID", names, 2);
    }
    if constexpr (requires { cfg.treble.pot_params.value; })
    {
        if constexpr (requires { cfg.mid.pot_params.value; })
        {
            static constexpr const char* names[] = {"P3", "TREBLE"};
            applyPotValue(cfg.treble, "TREBLE", names, 2);
        }
        else
        {
            static constexpr const char* names[] = {"P2", "TREBLE"};
            applyPotValue(cfg.treble, "TREBLE", names, 2);
        }
    }

    if constexpr (requires { cfg.R1; })
        set(&Config::R1, "R1");
    if constexpr (requires { cfg.R2; })
        set(&Config::R2, "R2");
    if constexpr (requires { cfg.R3; })
        set(&Config::R3, "R3");
    if constexpr (requires { cfg.R4; })
        set(&Config::R4, "R4");
    if constexpr (requires { cfg.R5; })
        set(&Config::R5, "R5");
    if constexpr (requires { cfg.R6; })
        set(&Config::R6, "R6");
    if constexpr (requires { cfg.R7; })
        set(&Config::R7, "R7");
    if constexpr (requires { cfg.R8; })
        set(&Config::R8, "R8");
    if constexpr (requires { cfg.R9; })
        set(&Config::R9, "R9");

    if constexpr (requires { cfg.C1; })
        set(&Config::C1, "C1");
    if constexpr (requires { cfg.C2; })
        set(&Config::C2, "C2");
    if constexpr (requires { cfg.C3; })
        set(&Config::C3, "C3");
    if constexpr (requires { cfg.C4; })
        set(&Config::C4, "C4");
    if constexpr (requires { cfg.C5; })
        set(&Config::C5, "C5");
    if constexpr (requires { cfg.C6; })
        set(&Config::C6, "C6");
    if constexpr (requires { cfg.C7; })
        set(&Config::C7, "C7");
    if constexpr (requires { cfg.C8; })
        set(&Config::C8, "C8");
    if constexpr (requires { cfg.C9; })
        set(&Config::C9, "C9");

    if constexpr (requires { cfg.va; })
        set(&Config::va, "VA");
    if constexpr (requires { cfg.vcc; })
        set(&Config::vcc, "VCC");
    if constexpr (requires { cfg.vbias; })
    {
        set(&Config::vbias, "VBIAS");
        if constexpr (! requires { cfg.va; })
            set(&Config::vbias, "VA");
    }
    if constexpr (requires { cfg.vce_sat; })
        set(&Config::vce_sat, "VCE_SAT");
}

template<typename Config>
void extractCommonFields(const Config& cfg, SchematicComponentValues& values, OperatorDeviceModels* models)
{
    auto put = [&](const char* name, double value)
    {
        if (std::isfinite(value))
            values.values[name] = value;
    };

    auto putPot = [&](const auto& pot, const char* valueName)
    {
        put(valueName, pot.pot_params.value);
        values.potTapers[valueName] = pot.pot_params.taper;
    };

    if constexpr (requires { cfg.gain.pot_params.value; })
        putPot(cfg.gain, "P1");
    if constexpr (requires { cfg.drive.pot_params.value; })
        putPot(cfg.drive, "P1");
    if constexpr (requires { cfg.distortion.pot_params.value; })
        putPot(cfg.distortion, "P1");
    if constexpr (requires { cfg.tone.pot_params.value; })
        putPot(cfg.tone, "P1");
    if constexpr (requires { cfg.level.pot_params.value; })
        putPot(cfg.level, "P1");
    if constexpr (requires { cfg.bass.pot_params.value; })
        putPot(cfg.bass, "P1");
    if constexpr (requires { cfg.mid.pot_params.value; })
        putPot(cfg.mid, "P2");
    if constexpr (requires { cfg.treble.pot_params.value; })
    {
        if constexpr (requires { cfg.mid.pot_params.value; })
            putPot(cfg.treble, "P3");
        else
            putPot(cfg.treble, "P2");
    }

    if constexpr (requires { cfg.R1; })
        put("R1", cfg.R1);
    if constexpr (requires { cfg.R2; })
        put("R2", cfg.R2);
    if constexpr (requires { cfg.R3; })
        put("R3", cfg.R3);
    if constexpr (requires { cfg.R4; })
        put("R4", cfg.R4);
    if constexpr (requires { cfg.R5; })
        put("R5", cfg.R5);
    if constexpr (requires { cfg.R6; })
        put("R6", cfg.R6);
    if constexpr (requires { cfg.R7; })
        put("R7", cfg.R7);
    if constexpr (requires { cfg.R8; })
        put("R8", cfg.R8);
    if constexpr (requires { cfg.R9; })
        put("R9", cfg.R9);

    if constexpr (requires { cfg.C1; })
        put("C1", cfg.C1);
    if constexpr (requires { cfg.C2; })
        put("C2", cfg.C2);
    if constexpr (requires { cfg.C3; })
        put("C3", cfg.C3);
    if constexpr (requires { cfg.C4; })
        put("C4", cfg.C4);
    if constexpr (requires { cfg.C5; })
        put("C5", cfg.C5);
    if constexpr (requires { cfg.C6; })
        put("C6", cfg.C6);
    if constexpr (requires { cfg.C7; })
        put("C7", cfg.C7);
    if constexpr (requires { cfg.C8; })
        put("C8", cfg.C8);
    if constexpr (requires { cfg.C9; })
        put("C9", cfg.C9);

    if constexpr (requires { cfg.va; })
        put("VA", cfg.va);
    if constexpr (requires { cfg.vcc; })
        put("VCC", cfg.vcc);
    if constexpr (requires { cfg.vbias; })
    {
        put("VBIAS", cfg.vbias);
        if constexpr (! requires { cfg.va; })
            put("VA", cfg.vbias);
    }
    if constexpr (requires { cfg.vce_sat; })
        put("VCE_SAT", cfg.vce_sat);

    if (models == nullptr)
        return;

    if constexpr (requires { cfg.opamp_model; })
    {
        models->hasOpamp = true;
        models->opamp = cfg.opamp_model;
    }
    if constexpr (requires { cfg.diode_model; })
    {
        models->hasDiode = true;
        models->diode = cfg.diode_model;
    }
    if constexpr (requires { cfg.bjt; })
    {
        models->hasBjt = true;
        models->bjt = cfg.bjt;
    }
}

template<typename Inst, typename Config>
void applyViaConfig(Inst* inst,
                    const SchematicComponentValues* values,
                    nx_result_t (*getConfig)(const Inst*, Config*),
                    nx_result_t (*setConfig)(Inst*, const Config*))
{
    if (inst == nullptr || values == nullptr || values->empty())
        return;

    Config cfg{};
    if (getConfig(inst, &cfg) != NX_SUCCESS)
        return;

    for (const auto& entry : values->values)
        applyCommonFields(cfg, entry.first, entry.second);

    setConfig(inst, &cfg);
}

template<typename Inst, typename Config>
bool readViaConfig(Inst* inst,
                   SchematicComponentValues* values,
                   OperatorDeviceModels* models,
                   nx_result_t (*getConfig)(const Inst*, Config*))
{
    if (inst == nullptr || getConfig == nullptr)
        return false;

    Config cfg{};
    if (getConfig(inst, &cfg) != NX_SUCCESS)
        return false;

    if (values != nullptr)
        extractCommonFields(cfg, *values, models);
    else if (models != nullptr)
    {
        SchematicComponentValues unused;
        extractCommonFields(cfg, unused, models);
    }

    return true;
}

#define APPLY_CONFIG(circuitKind, instPtr, instType, configType, getFn, setFn) \
    case circuitKind:                                                          \
        applyViaConfig(static_cast<instType*>(instPtr),                        \
                       values,                                                 \
                       getFn,                                                  \
                       setFn);                                                 \
        break

#define READ_CONFIG(circuitKind, createFn, destroyFn, instType, configType, getFn) \
    case circuitKind:                                                              \
    {                                                                              \
        auto* inst = createFn(nullptr);                                            \
        if (inst == nullptr)                                                       \
            break;                                                                 \
        readViaConfig(inst, valuesOut, modelsOut, getFn);                          \
        destroyFn(inst, nullptr);                                                  \
        break;                                                                     \
    }

void readOperatorState(CircuitKind circuit,
                       SchematicComponentValues* valuesOut,
                       OperatorDeviceModels* modelsOut)
{
    switch (circuit)
    {
        READ_CONFIG(CircuitKind::Ds1Opamp,
                    nx_ds1_opamp_create_f32,
                    nx_ds1_opamp_destroy_f32,
                    nx_ds1_opamp_f32_t,
                    nx_ds1_opamp_config_t,
                    nx_ds1_opamp_get_config_f32);
        READ_CONFIG(CircuitKind::Ds1Clipper,
                    nx_ds1_clipper_create_f32,
                    nx_ds1_clipper_destroy_f32,
                    nx_ds1_clipper_f32_t,
                    nx_ds1_clipper_config_t,
                    nx_ds1_clipper_get_config_f32);
        READ_CONFIG(CircuitKind::BjtFollower,
                    nx_bjt_follower_create_f32,
                    nx_bjt_follower_destroy_f32,
                    nx_bjt_follower_f32_t,
                    nx_bjt_follower_config_t,
                    nx_bjt_follower_get_config_f32);
        READ_CONFIG(CircuitKind::BjtFollowerOut,
                    nx_bjt_follower_out_create_f32,
                    nx_bjt_follower_out_destroy_f32,
                    nx_bjt_follower_out_f32_t,
                    nx_bjt_follower_out_config_t,
                    nx_bjt_follower_out_get_config_f32);
        READ_CONFIG(CircuitKind::BjtCommonEmitter,
                    nx_bjt_common_emitter_create_f32,
                    nx_bjt_common_emitter_destroy_f32,
                    nx_bjt_common_emitter_f32_t,
                    nx_bjt_common_emitter_config_t,
                    nx_bjt_common_emitter_get_config_f32);
        READ_CONFIG(CircuitKind::Od1Drive,
                    nx_od1_drive_create_f32,
                    nx_od1_drive_destroy_f32,
                    nx_od1_drive_f32_t,
                    nx_od1_drive_config_t,
                    nx_od1_drive_get_config_f32);
        READ_CONFIG(CircuitKind::Sd1Tone,
                    nx_sd1_tone_create_f32,
                    nx_sd1_tone_destroy_f32,
                    nx_sd1_tone_f32_t,
                    nx_sd1_tone_config_t,
                    nx_sd1_tone_get_config_f32);
        READ_CONFIG(CircuitKind::RcLevel,
                    nx_rc_level_create_f32,
                    nx_rc_level_destroy_f32,
                    nx_rc_level_f32_t,
                    nx_rc_level_config_t,
                    nx_rc_level_get_config_f32);
        READ_CONFIG(CircuitKind::Od1Post,
                    nx_od1_post_create_f32,
                    nx_od1_post_destroy_f32,
                    nx_od1_post_f32_t,
                    nx_od1_post_config_t,
                    nx_od1_post_get_config_f32);
        READ_CONFIG(CircuitKind::AcBoosterDrive,
                    nx_ac_booster_drive_create_f32,
                    nx_ac_booster_drive_destroy_f32,
                    nx_ac_booster_drive_f32_t,
                    nx_ac_booster_drive_config_t,
                    nx_ac_booster_drive_get_config_f32);
        READ_CONFIG(CircuitKind::AcBoosterEq,
                    nx_ac_booster_eq_create_f32,
                    nx_ac_booster_eq_destroy_f32,
                    nx_ac_booster_eq_f32_t,
                    nx_ac_booster_eq_config_t,
                    nx_ac_booster_eq_get_config_f32);
        READ_CONFIG(CircuitKind::RcBoosterDrive1,
                    nx_rc_booster_drive1_create_f32,
                    nx_rc_booster_drive1_destroy_f32,
                    nx_rc_booster_drive1_f32_t,
                    nx_rc_booster_drive1_config_t,
                    nx_rc_booster_drive1_get_config_f32);
        READ_CONFIG(CircuitKind::DsPlusOpamp,
                    nx_ds_plus_opamp_create_f32,
                    nx_ds_plus_opamp_destroy_f32,
                    nx_ds_plus_opamp_f32_t,
                    nx_ds_plus_opamp_config_t,
                    nx_ds_plus_opamp_get_config_f32);
        READ_CONFIG(CircuitKind::DiodeClipper,
                    nx_diode_clipper_create_f32,
                    nx_diode_clipper_destroy_f32,
                    nx_diode_clipper_f32_t,
                    nx_diode_clipper_config_t,
                    nx_diode_clipper_get_config_f32);
        READ_CONFIG(CircuitKind::Ts9Opamp,
                    nx_ts9_opamp_create_f32,
                    nx_ts9_opamp_destroy_f32,
                    nx_ts9_opamp_f32_t,
                    nx_ts9_opamp_config_t,
                    nx_ts9_opamp_get_config_f32);
        READ_CONFIG(CircuitKind::Ts9Tone,
                    nx_ts9_tone_create_f32,
                    nx_ts9_tone_destroy_f32,
                    nx_ts9_tone_f32_t,
                    nx_ts9_tone_config_t,
                    nx_ts9_tone_get_config_f32);
        READ_CONFIG(CircuitKind::KlonCentaur,
                    nx_klon_centaur_create_f32,
                    nx_klon_centaur_destroy_f32,
                    nx_klon_centaur_f32_t,
                    nx_klon_centaur_config_t,
                    nx_klon_centaur_get_config_f32);
        READ_CONFIG(CircuitKind::KlonCentaurTone,
                    nx_klon_centaur_tone_create_f32,
                    nx_klon_centaur_tone_destroy_f32,
                    nx_klon_centaur_tone_f32_t,
                    nx_klon_centaur_tone_config_t,
                    nx_klon_centaur_tone_get_config_f32);
        READ_CONFIG(CircuitKind::GuvnorPreamp,
                    nx_guvnor_preamp_create_f32,
                    nx_guvnor_preamp_destroy_f32,
                    nx_guvnor_preamp_f32_t,
                    nx_guvnor_preamp_config_t,
                    nx_guvnor_preamp_get_config_f32);
        READ_CONFIG(CircuitKind::GuvnorPostamp,
                    nx_guvnor_postamp_create_f32,
                    nx_guvnor_postamp_destroy_f32,
                    nx_guvnor_postamp_f32_t,
                    nx_guvnor_postamp_config_t,
                    nx_guvnor_postamp_get_config_f32);
        READ_CONFIG(CircuitKind::GuvnorClipper,
                    nx_guvnor_clipper_create_f32,
                    nx_guvnor_clipper_destroy_f32,
                    nx_guvnor_clipper_f32_t,
                    nx_guvnor_clipper_config_t,
                    nx_guvnor_clipper_get_config_f32);
        READ_CONFIG(CircuitKind::GuvnorLevel,
                    nx_guvnor_level_create_f32,
                    nx_guvnor_level_destroy_f32,
                    nx_guvnor_level_f32_t,
                    nx_guvnor_level_config_t,
                    nx_guvnor_level_get_config_f32);
    default:
        break;
    }
}

#undef READ_CONFIG

} // namespace

void applySchematicComponentValues(CircuitKind circuit,
                                   void* instance,
                                   const SchematicComponentValues* values)
{
    if (instance == nullptr || values == nullptr || values->empty())
        return;

    switch (circuit)
    {
        APPLY_CONFIG(CircuitKind::Ds1Opamp,
                     instance,
                     nx_ds1_opamp_f32_t,
                     nx_ds1_opamp_config_t,
                     nx_ds1_opamp_get_config_f32,
                     nx_ds1_opamp_set_config_f32);
        APPLY_CONFIG(CircuitKind::Ds1Clipper,
                     instance,
                     nx_ds1_clipper_f32_t,
                     nx_ds1_clipper_config_t,
                     nx_ds1_clipper_get_config_f32,
                     nx_ds1_clipper_set_config_f32);
        APPLY_CONFIG(CircuitKind::BjtFollower,
                     instance,
                     nx_bjt_follower_f32_t,
                     nx_bjt_follower_config_t,
                     nx_bjt_follower_get_config_f32,
                     nx_bjt_follower_set_config_f32);
        APPLY_CONFIG(CircuitKind::BjtFollowerOut,
                     instance,
                     nx_bjt_follower_out_f32_t,
                     nx_bjt_follower_out_config_t,
                     nx_bjt_follower_out_get_config_f32,
                     nx_bjt_follower_out_set_config_f32);
        APPLY_CONFIG(CircuitKind::BjtCommonEmitter,
                     instance,
                     nx_bjt_common_emitter_f32_t,
                     nx_bjt_common_emitter_config_t,
                     nx_bjt_common_emitter_get_config_f32,
                     nx_bjt_common_emitter_set_config_f32);
        APPLY_CONFIG(CircuitKind::Od1Drive,
                     instance,
                     nx_od1_drive_f32_t,
                     nx_od1_drive_config_t,
                     nx_od1_drive_get_config_f32,
                     nx_od1_drive_set_config_f32);
        APPLY_CONFIG(CircuitKind::Sd1Tone,
                     instance,
                     nx_sd1_tone_f32_t,
                     nx_sd1_tone_config_t,
                     nx_sd1_tone_get_config_f32,
                     nx_sd1_tone_set_config_f32);
        APPLY_CONFIG(CircuitKind::RcLevel,
                     instance,
                     nx_rc_level_f32_t,
                     nx_rc_level_config_t,
                     nx_rc_level_get_config_f32,
                     nx_rc_level_set_config_f32);
        APPLY_CONFIG(CircuitKind::Od1Post,
                     instance,
                     nx_od1_post_f32_t,
                     nx_od1_post_config_t,
                     nx_od1_post_get_config_f32,
                     nx_od1_post_set_config_f32);
        APPLY_CONFIG(CircuitKind::AcBoosterDrive,
                     instance,
                     nx_ac_booster_drive_f32_t,
                     nx_ac_booster_drive_config_t,
                     nx_ac_booster_drive_get_config_f32,
                     nx_ac_booster_drive_set_config_f32);
        APPLY_CONFIG(CircuitKind::AcBoosterEq,
                     instance,
                     nx_ac_booster_eq_f32_t,
                     nx_ac_booster_eq_config_t,
                     nx_ac_booster_eq_get_config_f32,
                     nx_ac_booster_eq_set_config_f32);
        APPLY_CONFIG(CircuitKind::RcBoosterDrive1,
                     instance,
                     nx_rc_booster_drive1_f32_t,
                     nx_rc_booster_drive1_config_t,
                     nx_rc_booster_drive1_get_config_f32,
                     nx_rc_booster_drive1_set_config_f32);
        APPLY_CONFIG(CircuitKind::DsPlusOpamp,
                     instance,
                     nx_ds_plus_opamp_f32_t,
                     nx_ds_plus_opamp_config_t,
                     nx_ds_plus_opamp_get_config_f32,
                     nx_ds_plus_opamp_set_config_f32);
        APPLY_CONFIG(CircuitKind::DiodeClipper,
                     instance,
                     nx_diode_clipper_f32_t,
                     nx_diode_clipper_config_t,
                     nx_diode_clipper_get_config_f32,
                     nx_diode_clipper_set_config_f32);
        APPLY_CONFIG(CircuitKind::Ts9Opamp,
                     instance,
                     nx_ts9_opamp_f32_t,
                     nx_ts9_opamp_config_t,
                     nx_ts9_opamp_get_config_f32,
                     nx_ts9_opamp_set_config_f32);
        APPLY_CONFIG(CircuitKind::Ts9Tone,
                     instance,
                     nx_ts9_tone_f32_t,
                     nx_ts9_tone_config_t,
                     nx_ts9_tone_get_config_f32,
                     nx_ts9_tone_set_config_f32);
        APPLY_CONFIG(CircuitKind::KlonCentaur,
                     instance,
                     nx_klon_centaur_f32_t,
                     nx_klon_centaur_config_t,
                     nx_klon_centaur_get_config_f32,
                     nx_klon_centaur_set_config_f32);
        APPLY_CONFIG(CircuitKind::KlonCentaurTone,
                     instance,
                     nx_klon_centaur_tone_f32_t,
                     nx_klon_centaur_tone_config_t,
                     nx_klon_centaur_tone_get_config_f32,
                     nx_klon_centaur_tone_set_config_f32);
        APPLY_CONFIG(CircuitKind::GuvnorPreamp,
                     instance,
                     nx_guvnor_preamp_f32_t,
                     nx_guvnor_preamp_config_t,
                     nx_guvnor_preamp_get_config_f32,
                     nx_guvnor_preamp_set_config_f32);
        APPLY_CONFIG(CircuitKind::GuvnorPostamp,
                     instance,
                     nx_guvnor_postamp_f32_t,
                     nx_guvnor_postamp_config_t,
                     nx_guvnor_postamp_get_config_f32,
                     nx_guvnor_postamp_set_config_f32);
        APPLY_CONFIG(CircuitKind::GuvnorClipper,
                     instance,
                     nx_guvnor_clipper_f32_t,
                     nx_guvnor_clipper_config_t,
                     nx_guvnor_clipper_get_config_f32,
                     nx_guvnor_clipper_set_config_f32);
        APPLY_CONFIG(CircuitKind::GuvnorLevel,
                     instance,
                     nx_guvnor_level_f32_t,
                     nx_guvnor_level_config_t,
                     nx_guvnor_level_get_config_f32,
                     nx_guvnor_level_set_config_f32);
    default:
        break;
    }
}

#undef APPLY_CONFIG

SchematicComponentValues readOperatorComponentValues(CircuitKind circuit)
{
    SchematicComponentValues values;
    readOperatorState(circuit, &values, nullptr);
    return values;
}

OperatorDeviceModels readOperatorDeviceModels(CircuitKind circuit)
{
    OperatorDeviceModels models;
    readOperatorState(circuit, nullptr, &models);
    return models;
}

} // namespace ds1_ac
