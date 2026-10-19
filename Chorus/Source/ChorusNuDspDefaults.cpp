#include "ChorusNuDspDefaults.h"

#include "extensions/CAMEL/chorus_f32.h"

ChorusNuDspLimits queryChorusNuDspLimits() noexcept
{
    static const ChorusNuDspLimits limits = []
    {
        ChorusNuDspLimits out;
        nx_chorus_config_t cfg{};

        if (nx_chorus_config_init(&cfg) != NX_SUCCESS)
            return out;

        out.rateMin = static_cast<float>(cfg.rate.control_params.min_value);
        out.rateMax = static_cast<float>(cfg.rate.control_params.max_value);
        out.rateDefault = static_cast<float>(cfg.rate.control_params.value);
        out.delayMin = static_cast<float>(cfg.delay.control_params.min_value);
        out.delayMax = static_cast<float>(cfg.delay.control_params.max_value);
        out.delayDefault = static_cast<float>(cfg.delay.control_params.value);
        out.amountMin = static_cast<float>(cfg.amount.control_params.min_value);
        out.amountMax = static_cast<float>(cfg.amount.control_params.max_value);
        out.amountDefault = static_cast<float>(cfg.amount.control_params.value);
        return out;
    }();
    return limits;
}
