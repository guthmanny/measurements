#include "SchematicComponentApply.h"

#include <mudsp/schematic/svg_cell_id.hpp>

#include "nudsp/linear_circuits/ac_booster_eq_f32.h"
#include "nudsp/linear_circuits/ds1_opamp_f32.h"
#include "nudsp/linear_circuits/ds_plus_opamp_f32.h"
#include "nudsp/linear_circuits/guvnor_level_f32.h"
#include "nudsp/linear_circuits/guvnor_postamp_f32.h"
#include "nudsp/linear_circuits/crunchbox_preamp_f32.h"
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
#include "nudsp/linear_circuits/beod_bass_f32.h"
#include "nudsp/linear_circuits/beod_preamp_f32.h"
#include "nudsp/linear_circuits/beod_tight_f32.h"
#include "nudsp/linear_circuits/beod_trim_f32.h"
#include "nudsp/linear_circuits/dirty_shirley_mid_f32.h"
#include "nudsp/linear_circuits/dirty_shirley_tight_f32.h"
#include "nudsp/linear_circuits/fa1_buffer_f32.h"
#include "nudsp/linear_circuits/fa1_volume_f32.h"
#include "nudsp/linear_circuits/fa1_eq_f32.h"
#include "nudsp/linear_circuits/bad_monkey_low_f32.h"
#include "nudsp/nonlinear_circuits/ota_comp_f32.h"
#include "nudsp/nonlinear_circuits/beod_clipper_f32.h"
#include "nudsp/nonlinear_circuits/beod_postgain_f32.h"
#include "nudsp/nonlinear_circuits/beod_pregain_f32.h"
#include "nudsp/linear_circuits/odr1_postamp_f32.h"
#include "nudsp/linear_circuits/op_buffer_f32.h"
#include "nudsp/linear_circuits/odr1_tone_f32.h"
#include "nudsp/linear_circuits/sd1_tone_f32.h"
#include "nudsp/linear_circuits/ts9_tone_f32.h"
#include "nudsp/nonlinear_circuits/ac_booster_drive_f32.h"
#include "nudsp/nonlinear_circuits/bjt_common_emitter_f32.h"
#include "nudsp/nonlinear_circuits/bjt_follower_f32.h"
#include "nudsp/nonlinear_circuits/bjt_follower_out_f32.h"
#include "nudsp/nonlinear_circuits/diode_clipper_f32.h"
#include "nudsp/nonlinear_circuits/ds1_clipper_f32.h"
#include "nudsp/nonlinear_circuits/crunchbox_clipper_f32.h"
#include "nudsp/nonlinear_circuits/crunchbox_clipper_presence_f32.h"
#include "nudsp/nonlinear_circuits/guvnor_clipper_f32.h"
#include "nudsp/nonlinear_circuits/jfet_follower_f32.h"
#include "nudsp/nonlinear_circuits/klon_centaur_f32.h"
#include "nudsp/nonlinear_circuits/od1_drive_f32.h"
#include "nudsp/nonlinear_circuits/odr1_clipper_f32.h"
#include "nudsp/nonlinear_circuits/odr1_drive_f32.h"
#include "nudsp/nonlinear_circuits/rat_clipper_f32.h"
#include "nudsp/nonlinear_circuits/rc_booster_drive1_f32.h"
#include "nudsp/nonlinear_circuits/ts9_opamp_f32.h"

#include "extensions/white_box/pedals/ac_booster_f32.h"
#include "extensions/white_box/pedals/angry_charlie_f32.h"
#include "extensions/white_box/pedals/crunch_box_f32.h"
#include "extensions/white_box/pedals/distortion_plus_f32.h"
#include "extensions/white_box/pedals/ds1_f32.h"
#include "extensions/white_box/pedals/guvnor_f32.h"
#include "extensions/white_box/pedals/klon_f32.h"
#include "extensions/white_box/pedals/od1_f32.h"
#include "extensions/white_box/pedals/beod_f32.h"
#include "extensions/white_box/pedals/dirty_shirley_f32.h"
#include "extensions/white_box/pedals/odr1_f32.h"
#include "extensions/white_box/pedals/rat_f32.h"
#include "extensions/white_box/pedals/rc_booster_f32.h"
#include "extensions/white_box/pedals/sansamp_classic_f32.h"
#include "extensions/white_box/pedals/sd1_f32.h"
#include "extensions/white_box/pedals/ts9_f32.h"
#include "extensions/white_box/pedals/bad_monkey_f32.h"
#include "extensions/white_box/pedals/dyna_comp_f32.h"

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
        {
            // Operator defaults duplicate pot resistance under GAIN/DRIVE/etc.; only 0..1 is a control.
            if (value >= 0.0 && value <= 1.0)
                pot.pot_params.control = value;
            else if (value > 1.0)
                pot.pot_params.value = value;
            return;
        }

        for (int i = 0; i < valueNameCount; ++i)
        {
            if (key.equalsIgnoreCase(valueNames[i]))
                pot.pot_params.value = value;
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
    if constexpr (requires { cfg.presence.pot_params.value; })
    {
        if constexpr (requires { cfg.treble.pot_params.value; } && ! requires { cfg.mid.pot_params.value; })
        {
            // BEOD clipper: P1=10kC presence, P2=100kB treble (not P2 presence like crunchbox).
            static constexpr const char* names[] = {"P1", "PRESENCE"};
            applyPotValue(cfg.presence, "PRESENCE", names, 2);
        }
        else
        {
            static constexpr const char* names[] = {"P2", "PRESENCE"};
            applyPotValue(cfg.presence, "PRESENCE", names, 2);
        }
    }
    if constexpr (requires { cfg.level.pot_params.value; })
    {
        if constexpr (requires { cfg.tone.pot_params.value; })
        {
            static constexpr const char* names[] = {"RV", "Rv", "LEVEL", "P3"};
            applyPotValue(cfg.level, "LEVEL", names, 4);
        }
        else
        {
            static constexpr const char* names[] = {"P1", "RV", "Rv", "LEVEL", "P3"};
            applyPotValue(cfg.level, "LEVEL", names, 5);
        }
    }
    if constexpr (requires { cfg.volume.pot_params.value; })
    {
        static constexpr const char* names[] = {"P1", "VOLUME"};
        applyPotValue(cfg.volume, "VOLUME", names, 2);
    }
    if constexpr (requires { cfg.low.pot_params.value; })
    {
        static constexpr const char* names[] = {"P1", "LOW"};
        applyPotValue(cfg.low, "LOW", names, 2);
    }
    if constexpr (requires { cfg.sensitivity.pot_params.value; })
    {
        static constexpr const char* names[] = {"P1", "SENSITIVITY", "RSENS"};
        applyPotValue(cfg.sensitivity, "SENSITIVITY", names, 3);
    }
    if constexpr (requires { cfg.attack.pot_params.value; })
    {
        static constexpr const char* names[] = {"P2", "ATTACK", "RATT"};
        applyPotValue(cfg.attack, "ATTACK", names, 3);
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
        else if constexpr (requires { cfg.presence.pot_params.value; })
        {
            static constexpr const char* names[] = {"P2", "TREBLE"};
            applyPotValue(cfg.treble, "TREBLE", names, 2);
        }
        else
        {
            static constexpr const char* names[] = {"P2", "TREBLE"};
            applyPotValue(cfg.treble, "TREBLE", names, 2);
        }
    }

    if constexpr (requires { cfg.R1; })
    {
        set(&Config::R1, "R1");
        set(&Config::R1, "RIN");
    }
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
    if constexpr (requires { cfg.R10; })
        set(&Config::R10, "R10");
    if constexpr (requires { cfg.R14; })
        set(&Config::R14, "R14");
    if constexpr (requires { cfg.R15; })
        set(&Config::R15, "R15");
    if constexpr (requires { cfg.Rota; })
        set(&Config::Rota, "ROTA");
    if constexpr (requires { cfg.Rc; })
        set(&Config::Rc, "RC");
    if constexpr (requires { cfg.Re; })
        set(&Config::Re, "RE");
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
    if constexpr (requires { cfg.CF; })
        set(&Config::CF, "CF");
    if constexpr (requires { cfg.C7; })
        set(&Config::C7, "C7");
    if constexpr (requires { cfg.C8; })
        set(&Config::C8, "C8");
    if constexpr (requires { cfg.C9; })
        set(&Config::C9, "C9");
    if constexpr (requires { cfg.C10; })
        set(&Config::C10, "C10");
    if constexpr (requires { cfg.Cota; })
        set(&Config::Cota, "COTA");

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
    if constexpr (requires { cfg.vg_bias; })
        set(&Config::vg_bias, "VA");
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
    if constexpr (requires { cfg.sw_tight.position; })
        applySwitch(cfg.sw_tight, "SW_TIGHT");
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
    {
        putPot(cfg.gain, "P1");
        putPot(cfg.gain, "GAIN");
    }
    if constexpr (requires { cfg.drive.pot_params.value; })
        putPot(cfg.drive, "P1");
    if constexpr (requires { cfg.distortion.pot_params.value; })
        putPot(cfg.distortion, "P1");
    if constexpr (requires { cfg.filter.pot_params.value; })
        putPot(cfg.filter, "P1");
    if constexpr (requires { cfg.p1.pot_params.value; })
        putPot(cfg.p1, "P1");
    if constexpr (requires { cfg.tone.pot_params.value; })
    {
        putPot(cfg.tone, "TONE");
        putPot(cfg.tone, "P1");
    }
    if constexpr (requires { cfg.presence.pot_params.value; })
    {
        putPot(cfg.presence, "PRESENCE");
        if constexpr (requires { cfg.treble.pot_params.value; } && ! requires { cfg.mid.pot_params.value; })
            putPot(cfg.presence, "P1");
        else
            putPot(cfg.presence, "P2");
    }
    if constexpr (requires { cfg.level.pot_params.value; })
    {
        putPot(cfg.level, "LEVEL");
        putPot(cfg.level, "P3");
        if constexpr (!requires { cfg.tone.pot_params.value; })
            putPot(cfg.level, "P1");
    }
    if constexpr (requires { cfg.volume.pot_params.value; })
    {
        putPot(cfg.volume, "P1");
        putPot(cfg.volume, "VOLUME");
    }
    if constexpr (requires { cfg.low.pot_params.value; })
    {
        putPot(cfg.low, "P1");
        putPot(cfg.low, "LOW");
    }
    if constexpr (requires { cfg.sensitivity.pot_params.value; })
    {
        putPot(cfg.sensitivity, "P1");
        putPot(cfg.sensitivity, "SENSITIVITY");
    }
    if constexpr (requires { cfg.attack.pot_params.value; })
    {
        putPot(cfg.attack, "P2");
        putPot(cfg.attack, "ATTACK");
    }
    if constexpr (requires { cfg.bass.pot_params.value; })
        putPot(cfg.bass, "P1");
    if constexpr (requires { cfg.mid.pot_params.value; })
        putPot(cfg.mid, "P2");
    if constexpr (requires { cfg.treble.pot_params.value; })
    {
        if constexpr (requires { cfg.mid.pot_params.value; })
            putPot(cfg.treble, "P3");
        else
        {
            putPot(cfg.treble, "TREBLE");
            putPot(cfg.treble, "P2");
        }
    }

    if constexpr (requires { cfg.R1; })
    {
        put("R1", cfg.R1);
        put("RIN", cfg.R1);
    }
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
    if constexpr (requires { cfg.R10; })
        put("R10", cfg.R10);
    if constexpr (requires { cfg.R14; })
        put("R14", cfg.R14);
    if constexpr (requires { cfg.R15; })
        put("R15", cfg.R15);
    if constexpr (requires { cfg.Rota; })
        put("ROTA", cfg.Rota);
    if constexpr (requires { cfg.Rc; })
        put("RC", cfg.Rc);
    if constexpr (requires { cfg.Re; })
        put("RE", cfg.Re);
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
    if constexpr (requires { cfg.CF; })
        put("CF", cfg.CF);
    if constexpr (requires { cfg.C7; })
        put("C7", cfg.C7);
    if constexpr (requires { cfg.C8; })
        put("C8", cfg.C8);
    if constexpr (requires { cfg.C9; })
        put("C9", cfg.C9);
    if constexpr (requires { cfg.C10; })
        put("C10", cfg.C10);
    if constexpr (requires { cfg.Cota; })
        put("COTA", cfg.Cota);

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
    if constexpr (requires { cfg.vg_bias; })
        put("VA", cfg.vg_bias);
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
    if constexpr (requires { cfg.sw_tight.position; })
        put("SW_TIGHT", static_cast<double>(cfg.sw_tight.position));

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
        READ_CONFIG(CircuitKind::Odr1Drive,
                    nx_odr1_drive_create_f32,
                    nx_odr1_drive_destroy_f32,
                    nx_odr1_drive_f32_t,
                    nx_odr1_drive_config_t,
                    nx_odr1_drive_get_config_f32);
        READ_CONFIG(CircuitKind::Odr1Clipper,
                    nx_odr1_clipper_create_f32,
                    nx_odr1_clipper_destroy_f32,
                    nx_odr1_clipper_f32_t,
                    nx_odr1_clipper_config_t,
                    nx_odr1_clipper_get_config_f32);
        READ_CONFIG(CircuitKind::Odr1Tone,
                    nx_odr1_tone_create_f32,
                    nx_odr1_tone_destroy_f32,
                    nx_odr1_tone_f32_t,
                    nx_odr1_tone_config_t,
                    nx_odr1_tone_get_config_f32);
        READ_CONFIG(CircuitKind::Odr1Postamp,
                    nx_odr1_postamp_create_f32,
                    nx_odr1_postamp_destroy_f32,
                    nx_odr1_postamp_f32_t,
                    nx_odr1_postamp_config_t,
                    nx_odr1_postamp_get_config_f32);
        READ_CONFIG(CircuitKind::BeodPreamp,
                    nx_beod_preamp_create_f32,
                    nx_beod_preamp_destroy_f32,
                    nx_beod_preamp_f32_t,
                    nx_beod_preamp_config_t,
                    nx_beod_preamp_get_config_f32);
        READ_CONFIG(CircuitKind::BeodTight,
                    nx_beod_tight_create_f32,
                    nx_beod_tight_destroy_f32,
                    nx_beod_tight_f32_t,
                    nx_beod_tight_config_t,
                    nx_beod_tight_get_config_f32);
        READ_CONFIG(CircuitKind::DirtyShirleyTight,
                    nx_dirty_shirley_tight_create_f32,
                    nx_dirty_shirley_tight_destroy_f32,
                    nx_dirty_shirley_tight_f32_t,
                    nx_dirty_shirley_tight_config_t,
                    nx_dirty_shirley_tight_get_config_f32);
        READ_CONFIG(CircuitKind::DirtyShirleyMid,
                    nx_dirty_shirley_mid_create_f32,
                    nx_dirty_shirley_mid_destroy_f32,
                    nx_dirty_shirley_mid_f32_t,
                    nx_dirty_shirley_mid_config_t,
                    nx_dirty_shirley_mid_get_config_f32);
        READ_CONFIG(CircuitKind::DirtyShirleyComposite,
                    nx_dirty_shirley_create_f32,
                    nx_dirty_shirley_destroy_f32,
                    nx_dirty_shirley_f32_t,
                    nx_dirty_shirley_config_t,
                    nx_dirty_shirley_get_config_f32);
        READ_CONFIG(CircuitKind::BeodPregain,
                    nx_beod_pregain_create_f32,
                    nx_beod_pregain_destroy_f32,
                    nx_beod_pregain_f32_t,
                    nx_beod_pregain_config_t,
                    nx_beod_pregain_get_config_f32);
        READ_CONFIG(CircuitKind::BeodPostgain,
                    nx_beod_postgain_create_f32,
                    nx_beod_postgain_destroy_f32,
                    nx_beod_postgain_f32_t,
                    nx_beod_postgain_config_t,
                    nx_beod_postgain_get_config_f32);
        READ_CONFIG(CircuitKind::BeodTrim,
                    nx_beod_trim_create_f32,
                    nx_beod_trim_destroy_f32,
                    nx_beod_trim_f32_t,
                    nx_beod_trim_config_t,
                    nx_beod_trim_get_config_f32);
        READ_CONFIG(CircuitKind::BeodClipper,
                    nx_beod_clipper_create_f32,
                    nx_beod_clipper_destroy_f32,
                    nx_beod_clipper_f32_t,
                    nx_beod_clipper_config_t,
                    nx_beod_clipper_get_config_f32);
        READ_CONFIG(CircuitKind::BeodBass,
                    nx_beod_bass_create_f32,
                    nx_beod_bass_destroy_f32,
                    nx_beod_bass_f32_t,
                    nx_beod_bass_config_t,
                    nx_beod_bass_get_config_f32);
        READ_CONFIG(CircuitKind::OpBuffer,
                    nx_op_buffer_create_f32,
                    nx_op_buffer_destroy_f32,
                    nx_op_buffer_f32_t,
                    nx_op_buffer_config_t,
                    nx_op_buffer_get_config_f32);
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
        READ_CONFIG(CircuitKind::CrunchboxPreamp,
                    nx_crunchbox_preamp_create_f32,
                    nx_crunchbox_preamp_destroy_f32,
                    nx_crunchbox_preamp_f32_t,
                    nx_crunchbox_preamp_config_t,
                    nx_crunchbox_preamp_get_config_f32);
        READ_CONFIG(CircuitKind::CrunchboxClipperPresence,
                    nx_crunchbox_clipper_presence_create_f32,
                    nx_crunchbox_clipper_presence_destroy_f32,
                    nx_crunchbox_clipper_presence_f32_t,
                    nx_crunchbox_clipper_presence_config_t,
                    nx_crunchbox_clipper_presence_get_config_f32);
        READ_CONFIG(CircuitKind::CrunchboxClipper,
                    nx_crunchbox_clipper_create_f32,
                    nx_crunchbox_clipper_destroy_f32,
                    nx_crunchbox_clipper_f32_t,
                    nx_crunchbox_clipper_config_t,
                    nx_crunchbox_clipper_get_config_f32);
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
        READ_CONFIG(CircuitKind::Fa1Buffer,
                    nx_fa1_buffer_create_f32,
                    nx_fa1_buffer_destroy_f32,
                    nx_fa1_buffer_f32_t,
                    nx_fa1_buffer_config_t,
                    nx_fa1_buffer_get_config_f32);
        READ_CONFIG(CircuitKind::Fa1Volume,
                    nx_fa1_volume_create_f32,
                    nx_fa1_volume_destroy_f32,
                    nx_fa1_volume_f32_t,
                    nx_fa1_volume_config_t,
                    nx_fa1_volume_get_config_f32);
        READ_CONFIG(CircuitKind::Fa1Eq,
                    nx_fa1_eq_create_f32,
                    nx_fa1_eq_destroy_f32,
                    nx_fa1_eq_f32_t,
                    nx_fa1_eq_config_t,
                    nx_fa1_eq_get_config_f32);
        READ_CONFIG(CircuitKind::BadMonkeyLow,
                    nx_bad_monkey_low_create_f32,
                    nx_bad_monkey_low_destroy_f32,
                    nx_bad_monkey_low_f32_t,
                    nx_bad_monkey_low_config_t,
                    nx_bad_monkey_low_get_config_f32);
        READ_CONFIG(CircuitKind::DynaCompCore,
                    nx_ota_comp_create_f32,
                    nx_ota_comp_destroy_f32,
                    nx_ota_comp_f32_t,
                    nx_ota_comp_config_t,
                    nx_ota_comp_get_config_f32);
    default:
        break;
    }
}

#undef READ_CONFIG

void fillBeodStageConfigsFromComposite(const nx_beod_config_t& parent,
                                       nx_beod_preamp_config_t& preamp,
                                       nx_beod_tight_config_t& tight,
                                       nx_beod_pregain_config_t& pregain,
                                       nx_beod_postgain_config_t& postgain,
                                       nx_beod_trim_config_t& trim,
                                       nx_beod_clipper_config_t& clipper,
                                       nx_beod_bass_config_t& bass,
                                       nx_rc_level_config_t& level)
{
    nx_beod_preamp_config_init(&preamp);
    preamp.vcc = parent.vcc;
    preamp.va = parent.va;
    preamp.custom_opamp = parent.custom_opamp;
    preamp.opamp_model = parent.opamp_model;

    nx_beod_tight_config_init(&tight);
    tight.tight = parent.tight;

    nx_beod_pregain_config_init(&pregain);
    pregain.gain = parent.gain;
    pregain.vcc = parent.vcc;
    pregain.va = parent.va;
    pregain.custom_opamp = parent.custom_opamp;
    pregain.opamp_model = parent.opamp_model;

    nx_beod_postgain_config_init(&postgain);
    postgain.vcc = parent.vcc;
    postgain.va = parent.va;
    postgain.custom_opamp = parent.custom_opamp;
    postgain.opamp_model = parent.opamp_model;

    nx_beod_trim_config_init(&trim);
    trim.trim = parent.trim;
    trim.vcc = parent.vcc;
    trim.va = parent.va;
    trim.custom_opamp = parent.custom_opamp;
    trim.opamp_model = parent.opamp_model;

    nx_beod_clipper_config_init(&clipper);
    clipper.presence = parent.presence;
    clipper.treble = parent.treble;

    nx_beod_bass_config_init(&bass);
    bass.bass = parent.bass;
    bass.vcc = parent.vcc;
    bass.va = parent.va;
    bass.custom_opamp = parent.custom_opamp;
    bass.opamp_model = parent.opamp_model;

    nx_rc_level_config_init(&level);
    level.level = parent.level;
    level.C1 = parent.C1;
    level.R1 = parent.R1;
}

void fillDirtyShirleyStageConfigsFromComposite(const nx_dirty_shirley_config_t& parent,
                                               nx_beod_preamp_config_t& preamp,
                                               nx_dirty_shirley_tight_config_t& tight,
                                               nx_beod_pregain_config_t& pregain,
                                               nx_beod_postgain_config_t& postgain,
                                               nx_beod_trim_config_t& trim,
                                               nx_beod_clipper_config_t& clipper,
                                               nx_beod_bass_config_t& bass,
                                               nx_dirty_shirley_mid_config_t& mid)
{
    nx_beod_preamp_config_init(&preamp);
    preamp.vcc = parent.vcc;
    preamp.va = parent.va;
    preamp.custom_opamp = parent.custom_opamp;
    preamp.opamp_model = parent.opamp_model;

    nx_dirty_shirley_tight_config_init(&tight);
    tight.sw_tight = parent.sw_tight;

    nx_beod_pregain_config_init(&pregain);
    pregain.gain = parent.gain;
    pregain.vcc = parent.vcc;
    pregain.va = parent.va;
    pregain.custom_opamp = parent.custom_opamp;
    pregain.opamp_model = parent.opamp_model;

    nx_beod_postgain_config_init(&postgain);
    postgain.vcc = parent.vcc;
    postgain.va = parent.va;
    postgain.custom_opamp = parent.custom_opamp;
    postgain.opamp_model = parent.opamp_model;

    nx_beod_trim_config_init(&trim);
    trim.trim = parent.trim;
    trim.vcc = parent.vcc;
    trim.va = parent.va;
    trim.custom_opamp = parent.custom_opamp;
    trim.opamp_model = parent.opamp_model;

    nx_beod_clipper_config_init(&clipper);
    clipper.presence = parent.presence;
    clipper.treble = parent.treble;

    nx_beod_bass_config_init(&bass);
    bass.bass = parent.bass;
    bass.vcc = parent.vcc;
    bass.va = parent.va;
    bass.custom_opamp = parent.custom_opamp;
    bass.opamp_model = parent.opamp_model;

    nx_dirty_shirley_mid_config_init(&mid);
    mid.level = parent.level;
    mid.mid = parent.mid;
    mid.vcc = parent.vcc;
    mid.va = parent.va;
    mid.custom_opamp = parent.custom_opamp;
    mid.opamp_model = parent.opamp_model;
}

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
    case PedalKind::AngryCharlie:
    {
        nx_angry_charlie_config_t cfg{};
        if (nx_angry_charlie_config_init(&cfg) != NX_SUCCESS)
            return false;
        switch (circuit)
        {
        case CircuitKind::CrunchboxPreamp:
            extractFromConfig(cfg.preamp, valuesOut, modelsOut);
            return true;
        case CircuitKind::CrunchboxClipperPresence:
            extractFromConfig(cfg.clipper_presence, valuesOut, modelsOut);
            return true;
        default:
            return false;
        }
    }
    case PedalKind::CrunchBox:
    {
        nx_crunch_box_config_t cfg{};
        if (nx_crunch_box_config_init(&cfg) != NX_SUCCESS)
            return false;
        switch (circuit)
        {
        case CircuitKind::CrunchboxPreamp:
            extractFromConfig(cfg.preamp, valuesOut, modelsOut);
            return true;
        case CircuitKind::CrunchboxClipper:
            extractFromConfig(cfg.clipper, valuesOut, modelsOut);
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
    case PedalKind::Odr1:
    {
        nx_odr1_config_t cfg{};
        if (nx_odr1_config_init(&cfg) != NX_SUCCESS)
            return false;
        switch (circuit)
        {
        case CircuitKind::JfetFollower:
            extractFromConfig(cfg.input, valuesOut, modelsOut);
            return true;
        case CircuitKind::Odr1Drive:
            extractFromConfig(cfg.drive, valuesOut, modelsOut);
            return true;
        case CircuitKind::Odr1Clipper:
            extractFromConfig(cfg.clipper, valuesOut, modelsOut);
            return true;
        case CircuitKind::Odr1Tone:
            extractFromConfig(cfg.tone, valuesOut, modelsOut);
            return true;
        case CircuitKind::Odr1Postamp:
            extractFromConfig(cfg.postamp, valuesOut, modelsOut);
            return true;
        case CircuitKind::RcLevel:
            extractFromConfig(cfg.level, valuesOut, modelsOut);
            return true;
        case CircuitKind::OpBuffer:
            extractFromConfig(cfg.output, valuesOut, modelsOut);
            return true;
        default:
            return false;
        }
    }
    case PedalKind::Beod:
    {
        nx_beod_config_t cfg{};
        if (nx_beod_config_init(&cfg) != NX_SUCCESS)
            return false;
        nx_beod_preamp_config_t preamp{};
        nx_beod_tight_config_t tight{};
        nx_beod_pregain_config_t pregain{};
        nx_beod_postgain_config_t postgain{};
        nx_beod_trim_config_t trim{};
        nx_beod_clipper_config_t clipper{};
        nx_beod_bass_config_t bass{};
        nx_rc_level_config_t level{};
        fillBeodStageConfigsFromComposite(cfg, preamp, tight, pregain, postgain, trim, clipper, bass, level);
        switch (circuit)
        {
        case CircuitKind::BeodPreamp:
            extractFromConfig(preamp, valuesOut, modelsOut);
            return true;
        case CircuitKind::BeodTight:
            extractFromConfig(tight, valuesOut, modelsOut);
            return true;
        case CircuitKind::BeodPregain:
            extractFromConfig(pregain, valuesOut, modelsOut);
            return true;
        case CircuitKind::BeodPostgain:
            extractFromConfig(postgain, valuesOut, modelsOut);
            return true;
        case CircuitKind::BeodTrim:
            extractFromConfig(trim, valuesOut, modelsOut);
            return true;
        case CircuitKind::BeodClipper:
            extractFromConfig(clipper, valuesOut, modelsOut);
            return true;
        case CircuitKind::BeodBass:
            extractFromConfig(bass, valuesOut, modelsOut);
            return true;
        case CircuitKind::RcLevel:
            extractFromConfig(level, valuesOut, modelsOut);
            return true;
        default:
            return false;
        }
    }
    case PedalKind::BadMonkey:
    {
        nx_bad_monkey_config_t cfg{};
        if (nx_bad_monkey_config_init(&cfg) != NX_SUCCESS)
            return false;
        switch (circuit)
        {
        case CircuitKind::JfetFollower:
            extractFromConfig(cfg.input, valuesOut, modelsOut);
            return true;
        case CircuitKind::Ts9Opamp:
            extractFromConfig(cfg.opamp, valuesOut, modelsOut);
            return true;
        case CircuitKind::Ts9Tone:
            extractFromConfig(cfg.tone, valuesOut, modelsOut);
            return true;
        case CircuitKind::BadMonkeyLow:
            extractFromConfig(cfg.low, valuesOut, modelsOut);
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
    case PedalKind::DynaComp:
    {
        nx_dyna_comp_config_t cfg{};
        if (nx_dyna_comp_config_init(&cfg) != NX_SUCCESS)
            return false;
        switch (circuit)
        {
        case CircuitKind::BjtFollowerOut:
            extractFromConfig(cfg.input, valuesOut, modelsOut);
            return true;
        case CircuitKind::DynaCompCore:
            extractFromConfig(cfg.compressor, valuesOut, modelsOut);
            return true;
        case CircuitKind::RcLevel:
            extractFromConfig(cfg.output, valuesOut, modelsOut);
            return true;
        default:
            return false;
        }
    }
    case PedalKind::DirtyShirley:
    {
        nx_dirty_shirley_config_t cfg{};
        if (nx_dirty_shirley_config_init(&cfg) != NX_SUCCESS)
            return false;
        nx_beod_preamp_config_t preamp{};
        nx_dirty_shirley_tight_config_t tight{};
        nx_beod_pregain_config_t pregain{};
        nx_beod_postgain_config_t postgain{};
        nx_beod_trim_config_t trim{};
        nx_beod_clipper_config_t clipper{};
        nx_beod_bass_config_t bass{};
        nx_dirty_shirley_mid_config_t mid{};
        fillDirtyShirleyStageConfigsFromComposite(cfg, preamp, tight, pregain, postgain, trim, clipper, bass, mid);
        switch (circuit)
        {
        case CircuitKind::BeodPreamp:
            extractFromConfig(preamp, valuesOut, modelsOut);
            return true;
        case CircuitKind::DirtyShirleyTight:
            extractFromConfig(tight, valuesOut, modelsOut);
            return true;
        case CircuitKind::BeodPregain:
            extractFromConfig(pregain, valuesOut, modelsOut);
            return true;
        case CircuitKind::BeodPostgain:
            extractFromConfig(postgain, valuesOut, modelsOut);
            return true;
        case CircuitKind::BeodTrim:
            extractFromConfig(trim, valuesOut, modelsOut);
            return true;
        case CircuitKind::BeodClipper:
            extractFromConfig(clipper, valuesOut, modelsOut);
            return true;
        case CircuitKind::BeodBass:
            extractFromConfig(bass, valuesOut, modelsOut);
            return true;
        case CircuitKind::DirtyShirleyMid:
            extractFromConfig(mid, valuesOut, modelsOut);
            return true;
        case CircuitKind::DirtyShirleyComposite:
            extractFromConfig(cfg, valuesOut, modelsOut);
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
        APPLY_CONFIG(CircuitKind::Odr1Drive,
                     instance,
                     nx_odr1_drive_f32_t,
                     nx_odr1_drive_config_t,
                     nx_odr1_drive_get_config_f32,
                     nx_odr1_drive_set_config_f32);
        APPLY_CONFIG(CircuitKind::Odr1Clipper,
                     instance,
                     nx_odr1_clipper_f32_t,
                     nx_odr1_clipper_config_t,
                     nx_odr1_clipper_get_config_f32,
                     nx_odr1_clipper_set_config_f32);
        APPLY_CONFIG(CircuitKind::Odr1Tone,
                     instance,
                     nx_odr1_tone_f32_t,
                     nx_odr1_tone_config_t,
                     nx_odr1_tone_get_config_f32,
                     nx_odr1_tone_set_config_f32);
        APPLY_CONFIG(CircuitKind::Odr1Postamp,
                     instance,
                     nx_odr1_postamp_f32_t,
                     nx_odr1_postamp_config_t,
                     nx_odr1_postamp_get_config_f32,
                     nx_odr1_postamp_set_config_f32);
        APPLY_CONFIG(CircuitKind::BeodPreamp,
                     instance,
                     nx_beod_preamp_f32_t,
                     nx_beod_preamp_config_t,
                     nx_beod_preamp_get_config_f32,
                     nx_beod_preamp_set_config_f32);
        APPLY_CONFIG(CircuitKind::BeodTight,
                     instance,
                     nx_beod_tight_f32_t,
                     nx_beod_tight_config_t,
                     nx_beod_tight_get_config_f32,
                     nx_beod_tight_set_config_f32);
        APPLY_CONFIG(CircuitKind::DirtyShirleyTight,
                     instance,
                     nx_dirty_shirley_tight_f32_t,
                     nx_dirty_shirley_tight_config_t,
                     nx_dirty_shirley_tight_get_config_f32,
                     nx_dirty_shirley_tight_set_config_f32);
        APPLY_CONFIG(CircuitKind::DirtyShirleyMid,
                     instance,
                     nx_dirty_shirley_mid_f32_t,
                     nx_dirty_shirley_mid_config_t,
                     nx_dirty_shirley_mid_get_config_f32,
                     nx_dirty_shirley_mid_set_config_f32);
        APPLY_CONFIG(CircuitKind::DirtyShirleyComposite,
                     instance,
                     nx_dirty_shirley_f32_t,
                     nx_dirty_shirley_config_t,
                     nx_dirty_shirley_get_config_f32,
                     nx_dirty_shirley_set_config_f32);
        APPLY_CONFIG(CircuitKind::BeodPregain,
                     instance,
                     nx_beod_pregain_f32_t,
                     nx_beod_pregain_config_t,
                     nx_beod_pregain_get_config_f32,
                     nx_beod_pregain_set_config_f32);
        APPLY_CONFIG(CircuitKind::BeodPostgain,
                     instance,
                     nx_beod_postgain_f32_t,
                     nx_beod_postgain_config_t,
                     nx_beod_postgain_get_config_f32,
                     nx_beod_postgain_set_config_f32);
        APPLY_CONFIG(CircuitKind::BeodTrim,
                     instance,
                     nx_beod_trim_f32_t,
                     nx_beod_trim_config_t,
                     nx_beod_trim_get_config_f32,
                     nx_beod_trim_set_config_f32);
        APPLY_CONFIG(CircuitKind::BeodClipper,
                     instance,
                     nx_beod_clipper_f32_t,
                     nx_beod_clipper_config_t,
                     nx_beod_clipper_get_config_f32,
                     nx_beod_clipper_set_config_f32);
        APPLY_CONFIG(CircuitKind::BeodBass,
                     instance,
                     nx_beod_bass_f32_t,
                     nx_beod_bass_config_t,
                     nx_beod_bass_get_config_f32,
                     nx_beod_bass_set_config_f32);
        APPLY_CONFIG(CircuitKind::OpBuffer,
                     instance,
                     nx_op_buffer_f32_t,
                     nx_op_buffer_config_t,
                     nx_op_buffer_get_config_f32,
                     nx_op_buffer_set_config_f32);
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
        APPLY_CONFIG(CircuitKind::CrunchboxPreamp,
                     instance,
                     nx_crunchbox_preamp_f32_t,
                     nx_crunchbox_preamp_config_t,
                     nx_crunchbox_preamp_get_config_f32,
                     nx_crunchbox_preamp_set_config_f32);
        APPLY_CONFIG(CircuitKind::CrunchboxClipperPresence,
                     instance,
                     nx_crunchbox_clipper_presence_f32_t,
                     nx_crunchbox_clipper_presence_config_t,
                     nx_crunchbox_clipper_presence_get_config_f32,
                     nx_crunchbox_clipper_presence_set_config_f32);
        APPLY_CONFIG(CircuitKind::CrunchboxClipper,
                     instance,
                     nx_crunchbox_clipper_f32_t,
                     nx_crunchbox_clipper_config_t,
                     nx_crunchbox_clipper_get_config_f32,
                     nx_crunchbox_clipper_set_config_f32);
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
        APPLY_CONFIG(CircuitKind::Fa1Buffer,
                     instance,
                     nx_fa1_buffer_f32_t,
                     nx_fa1_buffer_config_t,
                     nx_fa1_buffer_get_config_f32,
                     nx_fa1_buffer_set_config_f32);
        APPLY_CONFIG(CircuitKind::Fa1Volume,
                     instance,
                     nx_fa1_volume_f32_t,
                     nx_fa1_volume_config_t,
                     nx_fa1_volume_get_config_f32,
                     nx_fa1_volume_set_config_f32);
        APPLY_CONFIG(CircuitKind::Fa1Eq,
                     instance,
                     nx_fa1_eq_f32_t,
                     nx_fa1_eq_config_t,
                     nx_fa1_eq_get_config_f32,
                     nx_fa1_eq_set_config_f32);
        APPLY_CONFIG(CircuitKind::BadMonkeyLow,
                     instance,
                     nx_bad_monkey_low_f32_t,
                     nx_bad_monkey_low_config_t,
                     nx_bad_monkey_low_get_config_f32,
                     nx_bad_monkey_low_set_config_f32);
        APPLY_CONFIG(CircuitKind::DynaCompCore,
                     instance,
                     nx_ota_comp_f32_t,
                     nx_ota_comp_config_t,
                     nx_ota_comp_get_config_f32,
                     nx_ota_comp_set_config_f32);
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
