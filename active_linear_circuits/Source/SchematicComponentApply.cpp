#include "SchematicComponentApply.h"

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

    if constexpr (requires { cfg.gain.pot_params.value; })
    {
        if (key.equalsIgnoreCase("P1") || key.equalsIgnoreCase("GAIN"))
            cfg.gain.pot_params.value = value;
    }
    if constexpr (requires { cfg.tone.pot_params.value; })
    {
        if (key.equalsIgnoreCase("TONE"))
            cfg.tone.pot_params.control = juce::jlimit(0.0, 1.0, value);
        else if (key.equalsIgnoreCase("P1") || key.equalsIgnoreCase("RT") || key.equalsIgnoreCase("Rt"))
            cfg.tone.pot_params.value = value;
    }
    if constexpr (requires { cfg.level.pot_params.value; })
    {
        if (key.equalsIgnoreCase("LEVEL"))
            cfg.level.pot_params.control = juce::jlimit(0.0, 1.0, value);
        else if (key.equalsIgnoreCase("P1") || key.equalsIgnoreCase("RV") || key.equalsIgnoreCase("Rv"))
            cfg.level.pot_params.value = value;
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
        // BJT schematics label the base-bias rail POWER:VA (+4.5V), not VBIAS.
        if constexpr (!requires { cfg.va; })
            set(&Config::vbias, "VA");
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

#define APPLY_CONFIG(circuitKind, instPtr, instType, configType, getFn, setFn) \
    case circuitKind:                                                          \
        applyViaConfig(static_cast<instType*>(instPtr),                        \
                       values,                                                 \
                       getFn,                                                  \
                       setFn);                                                 \
        break

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
    default:
        break;
    }
}

#undef APPLY_CONFIG

} // namespace ds1_ac
