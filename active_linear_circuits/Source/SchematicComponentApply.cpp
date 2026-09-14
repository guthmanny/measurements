#include "SchematicComponentApply.h"

#include <mudsp/schematic/svg_cell_id.hpp>

#include "nudsp/linear_circuits/ac_booster_eq_f32.h"
#include "nudsp/linear_circuits/ds1_opamp_f32.h"
#include "nudsp/linear_circuits/ds_plus_opamp_f32.h"
#include "nudsp/linear_circuits/guvnor_level_f32.h"
#include "nudsp/linear_circuits/guvnor_postamp_f32.h"
#include "nudsp/linear_circuits/guvnor_preamp_f32.h"
#include "nudsp/linear_circuits/klon_centaur_tone_f32.h"
#include "nudsp/linear_circuits/od1_post_f32.h"
#include "nudsp/linear_circuits/rat_opamp_f32.h"
#include "nudsp/linear_circuits/rc_level_f32.h"
#include "nudsp/linear_circuits/sansamp_classic_capture_f32.h"
#include "nudsp/linear_circuits/sansamp_classic_gain_ctrl_f32.h"
#include "nudsp/linear_circuits/sansamp_classic_output_f32.h"
#include "nudsp/linear_circuits/sansamp_classic_position_f32.h"
#include "nudsp/linear_circuits/sansamp_classic_postamp_f32.h"
#include "nudsp/linear_circuits/sansamp_classic_pre_eq_f32.h"
#include "nudsp/linear_circuits/sansamp_classic_preamp_f32.h"
#include "nudsp/linear_circuits/sansamp_classic_spk_f32.h"
#include "nudsp/linear_circuits/sd1_tone_f32.h"
#include "nudsp/linear_circuits/ts9_tone_f32.h"
#include "nudsp/nonlinear_circuits/ac_booster_drive_f32.h"
#include "nudsp/nonlinear_circuits/bjt_common_emitter_f32.h"
#include "nudsp/nonlinear_circuits/bjt_follower_f32.h"
#include "nudsp/nonlinear_circuits/bjt_follower_out_f32.h"
#include "nudsp/nonlinear_circuits/diode_clipper_f32.h"
#include "nudsp/nonlinear_circuits/ds1_clipper_f32.h"
#include "nudsp/nonlinear_circuits/guvnor_clipper_f32.h"
#include "nudsp/nonlinear_circuits/jfet_follower_f32.h"
#include "nudsp/nonlinear_circuits/klon_centaur_f32.h"
#include "nudsp/nonlinear_circuits/od1_drive_f32.h"
#include "nudsp/nonlinear_circuits/rat_clipper_f32.h"
#include "nudsp/nonlinear_circuits/rc_booster_drive1_f32.h"
#include "nudsp/nonlinear_circuits/ts9_opamp_f32.h"

#include "extensions/white_box/pedals/ac_booster_f32.h"
#include "extensions/white_box/pedals/distortion_plus_f32.h"
#include "extensions/white_box/pedals/ds1_f32.h"
#include "extensions/white_box/pedals/guvnor_f32.h"
#include "extensions/white_box/pedals/klon_f32.h"
#include "extensions/white_box/pedals/od1_f32.h"
#include "extensions/white_box/pedals/rat_f32.h"
#include "extensions/white_box/pedals/rc_booster_f32.h"
#include "extensions/white_box/pedals/sansamp_classic_f32.h"
#include "extensions/white_box/pedals/sd1_f32.h"
#include "extensions/white_box/pedals/ts9_f32.h"

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
    if constexpr (requires { cfg.filter.pot_params.value; })
    {
        static constexpr const char* names[] = {"P1", "FILTER"};
        applyPotValue(cfg.filter, "FILTER", names, 2);
    }
    if constexpr (requires { cfg.p1.pot_params.value; })
    {
        static constexpr const char* names[] = {"P1"};
        applyPotValue(cfg.p1, "P1", names, 1);
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
    if constexpr (requires { cfg.Rsh; })
        set(&Config::Rsh, "RSH");
    if constexpr (requires { cfg.Rs; })
        set(&Config::Rs, "RS");
    if constexpr (requires { cfg.Rp1; })
        set(&Config::Rp1, "RP1");
    if constexpr (requires { cfg.Rgu; })
        set(&Config::Rgu, "RGU");
    if constexpr (requires { cfg.Rgd; })
        set(&Config::Rgd, "RGD");
    if constexpr (requires { cfg.Rp; })
        set(&Config::Rp, "RP");

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
    {
        set(&Config::C6, "C6");
        set(&Config::C6, "CF");
    }
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
    if constexpr (requires { cfg.vd; })
        set(&Config::vd, "VD");
    if constexpr (requires { cfg.Aol; })
        set(&Config::Aol, "AOL");
    if constexpr (requires { cfg.Fu; })
        set(&Config::Fu, "FU");

    auto applySwitch = [&](auto& sw, const char* name)
    {
        if (key.equalsIgnoreCase(name) || key.equalsIgnoreCase(juce::String("SWITCH_") + name))
            sw.position = static_cast<uint8_t>(juce::jlimit(0, 255, juce::roundToInt(value)));
    };

    if constexpr (requires { cfg.sw1.position; })
    {
        applySwitch(cfg.sw1, "SW1");
        if (mudsp::schematic::switchGangName(key.toStdString()) == "SW1")
            cfg.sw1.position = static_cast<uint8_t>(juce::jlimit(0, 255, juce::roundToInt(value)));
    }
    if constexpr (requires { cfg.sw_clean.position; })
        applySwitch(cfg.sw_clean, "SW_CLEAN");
    if constexpr (requires { cfg.sw_mb1.position; })
        applySwitch(cfg.sw_mb1, "SW_MB1");
    if constexpr (requires { cfg.sw_mb2.position; })
        applySwitch(cfg.sw_mb2, "SW_MB2");
    if constexpr (requires { cfg.sw_bright.position; })
        applySwitch(cfg.sw_bright, "SW_BRIGHT");
    if constexpr (requires { cfg.sw_low_drive.position; })
        applySwitch(cfg.sw_low_drive, "SW_LOW_DRIVE");
    if constexpr (requires { cfg.sw_vintage.position; })
        applySwitch(cfg.sw_vintage, "SW_VINTAGE");
    if constexpr (requires { cfg.sw_speaker_edge.position; })
        applySwitch(cfg.sw_speaker_edge, "SW_SPEAKER_EDGE");
    if constexpr (requires { cfg.sw_close_miking.position; })
        applySwitch(cfg.sw_close_miking, "SW_CLOSE_MIKING");
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
    if constexpr (requires { cfg.filter.pot_params.value; })
        putPot(cfg.filter, "P1");
    if constexpr (requires { cfg.p1.pot_params.value; })
        putPot(cfg.p1, "P1");
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
    if constexpr (requires { cfg.Rsh; })
        put("RSH", cfg.Rsh);
    if constexpr (requires { cfg.Rs; })
        put("RS", cfg.Rs);
    if constexpr (requires { cfg.Rp1; })
        put("RP1", cfg.Rp1);
    if constexpr (requires { cfg.Rgu; })
        put("RGU", cfg.Rgu);
    if constexpr (requires { cfg.Rgd; })
        put("RGD", cfg.Rgd);
    if constexpr (requires { cfg.Rp; })
        put("RP", cfg.Rp);

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
    {
        put("C6", cfg.C6);
        put("CF", cfg.C6);
    }
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
    if constexpr (requires { cfg.vd; })
        put("VD", cfg.vd);
    if constexpr (requires { cfg.Aol; })
        put("AOL", cfg.Aol);
    if constexpr (requires { cfg.Fu; })
        put("FU", cfg.Fu);
    if constexpr (requires { cfg.sw1.position; })
        put("SW1", static_cast<double>(cfg.sw1.position));
    if constexpr (requires { cfg.sw_clean.position; })
        put("SW_CLEAN", static_cast<double>(cfg.sw_clean.position));
    if constexpr (requires { cfg.sw_mb1.position; })
        put("SW_MB1", static_cast<double>(cfg.sw_mb1.position));
    if constexpr (requires { cfg.sw_mb2.position; })
        put("SW_MB2", static_cast<double>(cfg.sw_mb2.position));
    if constexpr (requires { cfg.sw_bright.position; })
        put("SW_BRIGHT", static_cast<double>(cfg.sw_bright.position));
    if constexpr (requires { cfg.sw_low_drive.position; })
        put("SW_LOW_DRIVE", static_cast<double>(cfg.sw_low_drive.position));
    if constexpr (requires { cfg.sw_vintage.position; })
        put("SW_VINTAGE", static_cast<double>(cfg.sw_vintage.position));
    if constexpr (requires { cfg.sw_speaker_edge.position; })
        put("SW_SPEAKER_EDGE", static_cast<double>(cfg.sw_speaker_edge.position));
    if constexpr (requires { cfg.sw_close_miking.position; })
        put("SW_CLOSE_MIKING", static_cast<double>(cfg.sw_close_miking.position));

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
    if constexpr (requires { cfg.jfet; })
    {
        models->hasJfet = true;
        models->jfet = cfg.jfet;
    }
}

template<typename Config>
void extractFromConfig(const Config& cfg, SchematicComponentValues* values, OperatorDeviceModels* models)
{
    if (values != nullptr)
        extractCommonFields(cfg, *values, models);
    else if (models != nullptr)
    {
        SchematicComponentValues unused;
        extractCommonFields(cfg, unused, models);
    }
}

void splitGuvnorOpamp(const nx_guvnor_opamp_config_t& opamp,
                      nx_guvnor_preamp_config_t& preamp,
                      nx_guvnor_postamp_config_t& postamp)
{
    nx_guvnor_preamp_config_init(&preamp);
    nx_guvnor_postamp_config_init(&postamp);
    preamp.gain = opamp.gain;
    preamp.R1 = opamp.R1;
    preamp.R2 = opamp.R2;
    preamp.R3 = opamp.R3;
    preamp.C1 = opamp.C1;
    preamp.C2 = opamp.C2;
    preamp.C3 = opamp.C3;
    preamp.vcc = opamp.vcc;
    preamp.va = opamp.va;
    preamp.custom_opamp = opamp.custom_opamp;
    preamp.opamp_model = opamp.opamp_model;
    postamp.gain = opamp.gain;
    postamp.R6 = opamp.R6;
    postamp.R7 = opamp.R7;
    postamp.C4 = opamp.C4;
    postamp.C5 = opamp.C5;
    postamp.C6 = opamp.C6;
    postamp.vcc = opamp.vcc;
    postamp.va = opamp.va;
    postamp.custom_opamp = opamp.custom_opamp;
    postamp.opamp_model = opamp.opamp_model;
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
        READ_CONFIG(CircuitKind::RatOpamp,
                    nx_rat_opamp_create_f32,
                    nx_rat_opamp_destroy_f32,
                    nx_rat_opamp_f32_t,
                    nx_rat_opamp_config_t,
                    nx_rat_opamp_get_config_f32);
        READ_CONFIG(CircuitKind::RatClipper,
                    nx_rat_clipper_create_f32,
                    nx_rat_clipper_destroy_f32,
                    nx_rat_clipper_f32_t,
                    nx_rat_clipper_config_t,
                    nx_rat_clipper_get_config_f32);
        READ_CONFIG(CircuitKind::JfetFollower,
                    nx_jfet_follower_create_f32,
                    nx_jfet_follower_destroy_f32,
                    nx_jfet_follower_f32_t,
                    nx_jfet_follower_config_t,
                    nx_jfet_follower_get_config_f32);
        READ_CONFIG(CircuitKind::SansampPreEq,
                    nx_sansamp_classic_pre_eq_create_f32,
                    nx_sansamp_classic_pre_eq_destroy_f32,
                    nx_sansamp_classic_pre_eq_f32_t,
                    nx_sansamp_classic_pre_eq_config_t,
                    nx_sansamp_classic_pre_eq_get_config_f32);
        READ_CONFIG(CircuitKind::SansampPreamp,
                    nx_sansamp_classic_preamp_create_f32,
                    nx_sansamp_classic_preamp_destroy_f32,
                    nx_sansamp_classic_preamp_f32_t,
                    nx_sansamp_classic_preamp_config_t,
                    nx_sansamp_classic_preamp_get_config_f32);
        READ_CONFIG(CircuitKind::SansampGainCtrl,
                    nx_sansamp_classic_gain_ctrl_create_f32,
                    nx_sansamp_classic_gain_ctrl_destroy_f32,
                    nx_sansamp_classic_gain_ctrl_f32_t,
                    nx_sansamp_classic_gain_ctrl_config_t,
                    nx_sansamp_classic_gain_ctrl_get_config_f32);
        READ_CONFIG(CircuitKind::SansampPostamp,
                    nx_sansamp_classic_postamp_create_f32,
                    nx_sansamp_classic_postamp_destroy_f32,
                    nx_sansamp_classic_postamp_f32_t,
                    nx_sansamp_classic_postamp_config_t,
                    nx_sansamp_classic_postamp_get_config_f32);
        READ_CONFIG(CircuitKind::SansampSpk,
                    nx_sansamp_classic_spk_create_f32,
                    nx_sansamp_classic_spk_destroy_f32,
                    nx_sansamp_classic_spk_f32_t,
                    nx_sansamp_classic_spk_config_t,
                    nx_sansamp_classic_spk_get_config_f32);
        READ_CONFIG(CircuitKind::SansampPosition,
                    nx_sansamp_classic_position_create_f32,
                    nx_sansamp_classic_position_destroy_f32,
                    nx_sansamp_classic_position_f32_t,
                    nx_sansamp_classic_position_config_t,
                    nx_sansamp_classic_position_get_config_f32);
        READ_CONFIG(CircuitKind::SansampCapture,
                    nx_sansamp_classic_capture_create_f32,
                    nx_sansamp_classic_capture_destroy_f32,
                    nx_sansamp_classic_capture_f32_t,
                    nx_sansamp_classic_capture_config_t,
                    nx_sansamp_classic_capture_get_config_f32);
        READ_CONFIG(CircuitKind::SansampOutput,
                    nx_sansamp_classic_output_create_f32,
                    nx_sansamp_classic_output_destroy_f32,
                    nx_sansamp_classic_output_f32_t,
                    nx_sansamp_classic_output_config_t,
                    nx_sansamp_classic_output_get_config_f32);
    default:
        break;
    }
}

#undef READ_CONFIG

bool readCompositeStageState(PedalKind pedal,
                             CircuitKind circuit,
                             SchematicComponentValues* valuesOut,
                             OperatorDeviceModels* modelsOut)
{
    switch (pedal)
    {
    case PedalKind::Ds1:
    {
        nx_ds1_config_t cfg{};
        if (nx_ds1_config_init(&cfg) != NX_SUCCESS)
            return false;
        switch (circuit)
        {
        case CircuitKind::BjtFollower:
            extractFromConfig(cfg.input, valuesOut, modelsOut);
            return true;
        case CircuitKind::BjtCommonEmitter:
            extractFromConfig(cfg.emitter, valuesOut, modelsOut);
            return true;
        case CircuitKind::Ds1Opamp:
            extractFromConfig(cfg.opamp, valuesOut, modelsOut);
            return true;
        case CircuitKind::Ds1Clipper:
            extractFromConfig(cfg.clipper, valuesOut, modelsOut);
            return true;
        case CircuitKind::BjtFollowerOut:
            extractFromConfig(cfg.output, valuesOut, modelsOut);
            return true;
        default:
            return false;
        }
    }
    case PedalKind::Ts9:
    {
        nx_ts9_config_t cfg{};
        if (nx_ts9_config_init(&cfg) != NX_SUCCESS)
            return false;
        switch (circuit)
        {
        case CircuitKind::BjtFollower:
            extractFromConfig(cfg.input, valuesOut, modelsOut);
            return true;
        case CircuitKind::Ts9Opamp:
            extractFromConfig(cfg.opamp, valuesOut, modelsOut);
            return true;
        case CircuitKind::Ts9Tone:
            extractFromConfig(cfg.tone, valuesOut, modelsOut);
            return true;
        case CircuitKind::RcLevel:
            extractFromConfig(cfg.level, valuesOut, modelsOut);
            return true;
        case CircuitKind::BjtFollowerOut:
            extractFromConfig(cfg.output, valuesOut, modelsOut);
            return true;
        default:
            return false;
        }
    }
    case PedalKind::Sd1:
    {
        nx_sd1_config_t cfg{};
        if (nx_sd1_config_init(&cfg) != NX_SUCCESS)
            return false;
        switch (circuit)
        {
        case CircuitKind::BjtFollower:
            extractFromConfig(cfg.input, valuesOut, modelsOut);
            return true;
        case CircuitKind::Od1Drive:
            extractFromConfig(cfg.drive, valuesOut, modelsOut);
            return true;
        case CircuitKind::Sd1Tone:
            extractFromConfig(cfg.tone, valuesOut, modelsOut);
            return true;
        case CircuitKind::RcLevel:
            extractFromConfig(cfg.level, valuesOut, modelsOut);
            return true;
        case CircuitKind::BjtFollowerOut:
            extractFromConfig(cfg.output, valuesOut, modelsOut);
            return true;
        default:
            return false;
        }
    }
    case PedalKind::Od1:
    {
        nx_od1_config_t cfg{};
        if (nx_od1_config_init(&cfg) != NX_SUCCESS)
            return false;
        switch (circuit)
        {
        case CircuitKind::BjtFollower:
            extractFromConfig(cfg.input, valuesOut, modelsOut);
            return true;
        case CircuitKind::Od1Drive:
            extractFromConfig(cfg.drive, valuesOut, modelsOut);
            return true;
        case CircuitKind::Od1Post:
            extractFromConfig(cfg.post, valuesOut, modelsOut);
            return true;
        case CircuitKind::RcLevel:
            extractFromConfig(cfg.level, valuesOut, modelsOut);
            return true;
        case CircuitKind::BjtFollowerOut:
            extractFromConfig(cfg.output, valuesOut, modelsOut);
            return true;
        default:
            return false;
        }
    }
    case PedalKind::AcBooster:
    {
        nx_ac_booster_config_t cfg{};
        if (nx_ac_booster_config_init(&cfg) != NX_SUCCESS)
            return false;
        switch (circuit)
        {
        case CircuitKind::BjtFollower:
            extractFromConfig(cfg.input, valuesOut, modelsOut);
            return true;
        case CircuitKind::AcBoosterDrive:
            extractFromConfig(cfg.drive, valuesOut, modelsOut);
            return true;
        case CircuitKind::AcBoosterEq:
            extractFromConfig(cfg.eq, valuesOut, modelsOut);
            return true;
        case CircuitKind::RcLevel:
            extractFromConfig(cfg.level, valuesOut, modelsOut);
            return true;
        case CircuitKind::BjtFollowerOut:
            extractFromConfig(cfg.output, valuesOut, modelsOut);
            return true;
        default:
            return false;
        }
    }
    case PedalKind::RcBooster:
    {
        nx_rc_booster_config_t cfg{};
        if (nx_rc_booster_config_init(&cfg) != NX_SUCCESS)
            return false;
        switch (circuit)
        {
        case CircuitKind::BjtFollower:
            extractFromConfig(cfg.input, valuesOut, modelsOut);
            return true;
        case CircuitKind::RcBoosterDrive1:
            extractFromConfig(cfg.drive, valuesOut, modelsOut);
            return true;
        case CircuitKind::AcBoosterEq:
            extractFromConfig(cfg.eq, valuesOut, modelsOut);
            return true;
        case CircuitKind::RcLevel:
            extractFromConfig(cfg.level, valuesOut, modelsOut);
            return true;
        case CircuitKind::BjtFollowerOut:
            extractFromConfig(cfg.output, valuesOut, modelsOut);
            return true;
        default:
            return false;
        }
    }
    case PedalKind::Klon:
    {
        nx_klon_config_t cfg{};
        if (nx_klon_config_init(&cfg) != NX_SUCCESS)
            return false;
        switch (circuit)
        {
        case CircuitKind::KlonCentaur:
            extractFromConfig(cfg.centaur, valuesOut, modelsOut);
            return true;
        case CircuitKind::KlonCentaurTone:
            extractFromConfig(cfg.tone, valuesOut, modelsOut);
            return true;
        case CircuitKind::RcLevel:
            extractFromConfig(cfg.level, valuesOut, modelsOut);
            return true;
        default:
            return false;
        }
    }
    case PedalKind::DistortionPlus:
    {
        nx_distortion_plus_config_t cfg{};
        if (nx_distortion_plus_config_init(&cfg) != NX_SUCCESS)
            return false;
        switch (circuit)
        {
        case CircuitKind::DsPlusOpamp:
            extractFromConfig(cfg.opamp, valuesOut, modelsOut);
            return true;
        case CircuitKind::DiodeClipper:
            extractFromConfig(cfg.clipper, valuesOut, modelsOut);
            return true;
        default:
            return false;
        }
    }
    case PedalKind::Guvnor:
    {
        nx_guvnor_config_t cfg{};
        if (nx_guvnor_config_init(&cfg) != NX_SUCCESS)
            return false;
        switch (circuit)
        {
        case CircuitKind::GuvnorPreamp:
        case CircuitKind::GuvnorPostamp:
        {
            nx_guvnor_preamp_config_t preamp{};
            nx_guvnor_postamp_config_t postamp{};
            splitGuvnorOpamp(cfg.opamp, preamp, postamp);
            if (circuit == CircuitKind::GuvnorPreamp)
                extractFromConfig(preamp, valuesOut, modelsOut);
            else
                extractFromConfig(postamp, valuesOut, modelsOut);
            return true;
        }
        case CircuitKind::GuvnorClipper:
            extractFromConfig(cfg.clipper, valuesOut, modelsOut);
            return true;
        case CircuitKind::GuvnorLevel:
            extractFromConfig(cfg.level, valuesOut, modelsOut);
            return true;
        default:
            return false;
        }
    }
    case PedalKind::Rat:
    {
        nx_rat_config_t cfg{};
        if (nx_rat_config_init(&cfg) != NX_SUCCESS)
            return false;
        switch (circuit)
        {
        case CircuitKind::RatOpamp:
            extractFromConfig(cfg.opamp, valuesOut, modelsOut);
            return true;
        case CircuitKind::RatClipper:
            extractFromConfig(cfg.clipper, valuesOut, modelsOut);
            return true;
        case CircuitKind::JfetFollower:
            extractFromConfig(cfg.follower, valuesOut, modelsOut);
            return true;
        case CircuitKind::RcLevel:
            extractFromConfig(cfg.level, valuesOut, modelsOut);
            return true;
        default:
            return false;
        }
    }
    case PedalKind::SansampClassic:
    {
        nx_sansamp_classic_config_t cfg{};
        if (nx_sansamp_classic_config_init(&cfg) != NX_SUCCESS)
            return false;
        switch (circuit)
        {
        case CircuitKind::BjtFollower:
            extractFromConfig(cfg.bjt_follower, valuesOut, modelsOut);
            return true;
        case CircuitKind::SansampPreEq:
            extractFromConfig(cfg.pre_eq, valuesOut, modelsOut);
            return true;
        case CircuitKind::SansampPreamp:
            extractFromConfig(cfg.preamp, valuesOut, modelsOut);
            return true;
        case CircuitKind::SansampGainCtrl:
            extractFromConfig(cfg.gain_ctrl, valuesOut, modelsOut);
            return true;
        case CircuitKind::SansampPostamp:
            extractFromConfig(cfg.postamp, valuesOut, modelsOut);
            return true;
        case CircuitKind::SansampSpk:
            extractFromConfig(cfg.spk, valuesOut, modelsOut);
            return true;
        case CircuitKind::SansampPosition:
            extractFromConfig(cfg.position, valuesOut, modelsOut);
            return true;
        case CircuitKind::SansampCapture:
            extractFromConfig(cfg.capture, valuesOut, modelsOut);
            return true;
        case CircuitKind::SansampOutput:
            extractFromConfig(cfg.output, valuesOut, modelsOut);
            return true;
        default:
            return false;
        }
    }
    }

    return false;
}

void readStageState(PedalKind pedal,
                    CircuitKind circuit,
                    bool useCompositeConfig,
                    SchematicComponentValues* valuesOut,
                    OperatorDeviceModels* modelsOut)
{
    if (useCompositeConfig && readCompositeStageState(pedal, circuit, valuesOut, modelsOut))
        return;

    readOperatorState(circuit, valuesOut, modelsOut);
}

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
        APPLY_CONFIG(CircuitKind::RatOpamp,
                     instance,
                     nx_rat_opamp_f32_t,
                     nx_rat_opamp_config_t,
                     nx_rat_opamp_get_config_f32,
                     nx_rat_opamp_set_config_f32);
        APPLY_CONFIG(CircuitKind::RatClipper,
                     instance,
                     nx_rat_clipper_f32_t,
                     nx_rat_clipper_config_t,
                     nx_rat_clipper_get_config_f32,
                     nx_rat_clipper_set_config_f32);
        APPLY_CONFIG(CircuitKind::JfetFollower,
                     instance,
                     nx_jfet_follower_f32_t,
                     nx_jfet_follower_config_t,
                     nx_jfet_follower_get_config_f32,
                     nx_jfet_follower_set_config_f32);
        APPLY_CONFIG(CircuitKind::SansampPreEq,
                     instance,
                     nx_sansamp_classic_pre_eq_f32_t,
                     nx_sansamp_classic_pre_eq_config_t,
                     nx_sansamp_classic_pre_eq_get_config_f32,
                     nx_sansamp_classic_pre_eq_set_config_f32);
        APPLY_CONFIG(CircuitKind::SansampPreamp,
                     instance,
                     nx_sansamp_classic_preamp_f32_t,
                     nx_sansamp_classic_preamp_config_t,
                     nx_sansamp_classic_preamp_get_config_f32,
                     nx_sansamp_classic_preamp_set_config_f32);
        APPLY_CONFIG(CircuitKind::SansampGainCtrl,
                     instance,
                     nx_sansamp_classic_gain_ctrl_f32_t,
                     nx_sansamp_classic_gain_ctrl_config_t,
                     nx_sansamp_classic_gain_ctrl_get_config_f32,
                     nx_sansamp_classic_gain_ctrl_set_config_f32);
        APPLY_CONFIG(CircuitKind::SansampPostamp,
                     instance,
                     nx_sansamp_classic_postamp_f32_t,
                     nx_sansamp_classic_postamp_config_t,
                     nx_sansamp_classic_postamp_get_config_f32,
                     nx_sansamp_classic_postamp_set_config_f32);
        APPLY_CONFIG(CircuitKind::SansampSpk,
                     instance,
                     nx_sansamp_classic_spk_f32_t,
                     nx_sansamp_classic_spk_config_t,
                     nx_sansamp_classic_spk_get_config_f32,
                     nx_sansamp_classic_spk_set_config_f32);
        APPLY_CONFIG(CircuitKind::SansampPosition,
                     instance,
                     nx_sansamp_classic_position_f32_t,
                     nx_sansamp_classic_position_config_t,
                     nx_sansamp_classic_position_get_config_f32,
                     nx_sansamp_classic_position_set_config_f32);
        APPLY_CONFIG(CircuitKind::SansampCapture,
                     instance,
                     nx_sansamp_classic_capture_f32_t,
                     nx_sansamp_classic_capture_config_t,
                     nx_sansamp_classic_capture_get_config_f32,
                     nx_sansamp_classic_capture_set_config_f32);
        APPLY_CONFIG(CircuitKind::SansampOutput,
                     instance,
                     nx_sansamp_classic_output_f32_t,
                     nx_sansamp_classic_output_config_t,
                     nx_sansamp_classic_output_get_config_f32,
                     nx_sansamp_classic_output_set_config_f32);
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

SchematicComponentValues readStageComponentValues(PedalKind pedal, CircuitKind circuit, bool useCompositeConfig)
{
    SchematicComponentValues values;
    readStageState(pedal, circuit, useCompositeConfig, &values, nullptr);
    return values;
}

OperatorDeviceModels readStageDeviceModels(PedalKind pedal, CircuitKind circuit, bool useCompositeConfig)
{
    OperatorDeviceModels models;
    readStageState(pedal, circuit, useCompositeConfig, nullptr, &models);
    return models;
}

} // namespace ds1_ac
