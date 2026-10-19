// Operator-menu coverage for odr1_clipper: defaults, process, and ac_static.

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "nudsp/common/operator_spec_metadata.h"
#include "nudsp/nonlinear_circuits/odr1_clipper_f32.h"

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
    const char* hash = nx_odr1_clipper_spec_hash_hex();
    expect(hash != nullptr && std::strlen(hash) == 64U, "spec hash length");
    expect(std::strcmp(hash, nx_operator_spec_hash_hex("odr1_clipper")) == 0, "spec hash lookup");

    nx_odr1_clipper_config_t config{};
    expect(nx_odr1_clipper_config_init(&config) == NX_SUCCESS, "config init");
    expect(config.diode_model == NX_DIODE_1N4148, "default diode 1N4148");
    expect(std::fabs(config.C24 - 3.3e-9) < 1e-18, "schematic C24 = 3.3 nF");

    const double fs = 96000.0;
    nx_odr1_clipper_f32_t* clipper = nx_odr1_clipper_create_f32(nullptr);
    expect(clipper != nullptr, "create");
    nx_odr1_clipper_prepare_f32(clipper, fs);
    nx_odr1_clipper_tick_f32(clipper, 64);
    nx_odr1_clipper_reset_f32(clipper);

    constexpr int n = 4096;
    float in[n];
    float out[n];
    const double freq = 1000.0;
    const double amp = 1.0e-4;
    for (int i = 0; i < n; ++i)
        in[i] = static_cast<float>(amp * std::sin(2.0 * M_PI * freq * static_cast<double>(i) / fs));

    nx_odr1_clipper_process_f32(clipper, in, out, static_cast<size_t>(n));

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
    nx_odr1_clipper_ac_static_f32(&config, fs, freqs, mag_db, phase, 3);
    for (int i = 0; i < 3; ++i)
    {
        expect(std::isfinite(mag_db[i]), "ac magnitude finite");
        expect(std::isfinite(phase[i]), "ac phase finite");
    }

    const double ac_err = std::fabs(process_db - mag_db[1]);
    std::printf("odr1_clipper process %.2f dB  ac_static %.2f dB  |err| %.2f dB\n",
                process_db, mag_db[1], ac_err);
    expect(ac_err < 0.1, "1 kHz process matches ac_static within 0.1 dB");

    expect(nx_odr1_clipper_destroy_f32(clipper, nullptr) == NX_SUCCESS, "destroy");
    std::printf("odr1_clipper operator test ok\n");
    return 0;
}
