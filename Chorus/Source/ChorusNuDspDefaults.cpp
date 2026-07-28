#include "ChorusNuDspDefaults.h"

#include "extensions/CAMEL/chorus_f32.h"

ChorusNuDspLimits queryChorusNuDspLimits() noexcept
{
    ChorusNuDspLimits limits;
    nx_chorus_config_t cfg{};

    if (nx_chorus_config_init (&cfg) != NX_SUCCESS)
        return limits;

    limits.rateMin = static_cast<float> (cfg.rate.control_params.min_value);
    limits.rateMax = static_cast<float> (cfg.rate.control_params.max_value);
    limits.rateDefault = static_cast<float> (cfg.rate.control_params.value);
    limits.delayMin = static_cast<float> (cfg.delay.control_params.min_value);
    limits.delayMax = static_cast<float> (cfg.delay.control_params.max_value);
    limits.delayDefault = static_cast<float> (cfg.delay.control_params.value);
    limits.amountMin = static_cast<float> (cfg.amount.control_params.min_value);
    limits.amountMax = static_cast<float> (cfg.amount.control_params.max_value);
    limits.amountDefault = static_cast<float> (cfg.amount.control_params.value);
    return limits;
}
