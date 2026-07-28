#pragma once

/** Domain limits and defaults from nx_chorus_config_init (NuDSP chorus_f32). */
struct ChorusNuDspLimits
{
    float rateMin = 0.01f;
    float rateMax = 20.0f;
    float rateDefault = 1.0f;
    float delayMin = 1.0f;
    float delayMax = 100.0f;
    float delayDefault = 25.0f;
    float amountMin = 0.0f;
    float amountMax = 50.0f;
    float amountDefault = 10.0f;
};

ChorusNuDspLimits queryChorusNuDspLimits() noexcept;
