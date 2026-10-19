// Operator-menu coverage for odr1_tone: defaults, process, and ac_static.

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "nudsp/common/operator_spec_metadata.h"
#include "nudsp/linear_circuits/odr1_tone_f32.h"

static void expect(bool cond, const char* message)
{
    if (!cond)
    {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

int main()
{
    const char* hash = nx_odr1_tone_spec_hash_hex();
    expect(hash != nullptr && std::strlen(hash) == 64U, "spec hash length");
    expect(std::strcmp(hash, nx_operator_spec_hash_hex("odr1_tone")) == 0, "spec hash lookup");

    nx_odr1_tone_config_t config{};
    expect(nx_odr1_tone_config_init(&config) == NX_SUCCESS, "config init");
    expect(config.opamp_model == NX_OPAMP_JRC4558, "default opamp JRC4558");
    expect(std::fabs(config.tone.pot_params.control - 0.5) < 1e-9, "default tone");
    expect(std::fabs(config.tone.pot_params.value - 25000.0) < 1e-6, "25k pot");
    expect(std::fabs(config.R3 - 5100.0) < 1e-6, "R3 5.1k");
    expect(std::fabs(config.C1 - 2.2e-8) < 1e-15, "C1 22n");

    const double fs = 96000.0;
    nx_odr1_tone_f32_t* tone = nx_odr1_tone_create_f32(nullptr);
    expect(tone != nullptr, "create");
    nx_odr1_tone_prepare_f32(tone, fs);
    nx_odr1_tone_tick_f32(tone, 64);
    nx_odr1_tone_reset_f32(tone);

    constexpr int n = 4096;
    float in[n];
    float out[n];
    const double freq = 1000.0;
    const double amp = 1.0e-4;
    for (int i = 0; i < n; ++i)
        in[i] = static_cast<float>(amp * std::sin(2.0 * M_PI * freq * static_cast<double>(i) / fs));

    nx_odr1_tone_process_f32(tone, in, out, static_cast<size_t>(n));

    const int skip = static_cast<int>(0.01 * fs);
    double peak = 0.0;
    double sum_sq = 0.0;
    int count = 0;
    for (int i = skip; i < n; ++i)
    {
        expect(std::isfinite(out[i]), "process finite");
        const double a = std::fabs(out[i]);
        if (a > peak)
            peak = a;
        sum_sq += static_cast<double>(out[i]) * out[i];
        ++count;
    }
    expect(peak > 0.0, "process has energy");
    const double process_rms = std::sqrt(sum_sq / static_cast<double>(count));
    const double input_rms = amp / std::sqrt(2.0);
    const double process_db = 20.0 * std::log10(process_rms / input_rms);

    const double freqs[3] = {100.0, 1000.0, 10000.0};
    double mag_db[3] = {};
    double phase[3] = {};
    nx_odr1_tone_ac_static_f32(&config, fs, freqs, mag_db, phase, 3);
    for (int i = 0; i < 3; ++i)
    {
        expect(std::isfinite(mag_db[i]), "ac magnitude finite");
        expect(std::isfinite(phase[i]), "ac phase finite");
    }

    const double ac_err = std::fabs(process_db - mag_db[1]);
    std::printf("odr1_tone process %.2f dB  ac_static %.2f dB  |err| %.2f dB\n",
                process_db, mag_db[1], ac_err);
    expect(ac_err < 0.1, "1 kHz process matches ac_static within 0.1 dB");

    expect(nx_odr1_tone_destroy_f32(tone, nullptr) == NX_SUCCESS, "destroy");
    std::printf("odr1_tone operator test ok\n");
    return 0;
}
