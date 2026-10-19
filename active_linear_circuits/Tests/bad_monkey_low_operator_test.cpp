// Operator-menu coverage for bad_monkey_low: defaults, process, ac_static vs Python reference.

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "nudsp/common/operator_spec_metadata.h"
#include "nudsp/linear_circuits/bad_monkey_low_f32.h"

static void expect(bool cond, const char* message)
{
    if (!cond)
    {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

static constexpr int kAcPoints = 8;
static constexpr double kAcFreqs[kAcPoints] = {
    20.0, 53.653916, 143.937135, 386.139546, 1035.894936, 2778.990989, 7455.187441, 20000.0,
};
static constexpr double kPythonMagDbFlat[kAcPoints] = {
    0.000053, -0.000090, -0.000440, -0.000634, -0.000714, -0.001034, -0.003385, -0.026000,
};
static constexpr double kPythonMagDbCut[kAcPoints] = {
    -6.692934, -7.922068, -3.944972, -2.962935, -2.811228, -2.789844, -2.787114, -2.789134,
};
static constexpr double kPythonMagDbBoost[kAcPoints] = {
    6.693225, 7.921615, 3.943413, 2.960961, 2.809087, 2.786980, 2.778931, 2.729669,
};

static void assertAcMatches(const nx_bad_monkey_low_config_t& config, const double* python_db, const char* label)
{
    double mag_db[kAcPoints] = {};
    double phase[kAcPoints] = {};
    nx_bad_monkey_low_ac_static_f32(&config, 96000.0, kAcFreqs, mag_db, phase, kAcPoints);
    for (int i = 0; i < kAcPoints; ++i)
    {
        expect(std::isfinite(mag_db[i]), "ac magnitude finite");
        expect(std::isfinite(phase[i]), "ac phase finite");
        const double err = std::fabs(mag_db[i] - python_db[i]);
        if (err > 0.05)
        {
            std::fprintf(stderr, "FAIL: %s python ac mismatch f=%.1f Hz  c=%.4f  py=%.4f  err=%.4f dB\n",
                         label, kAcFreqs[i], mag_db[i], python_db[i], err);
            std::exit(1);
        }
    }
}

int main()
{
    const char* hash = nx_bad_monkey_low_spec_hash_hex();
    expect(hash != nullptr && std::strlen(hash) == 64U, "spec hash length");
    expect(std::strcmp(hash, nx_operator_spec_hash_hex("bad_monkey_low")) == 0, "spec hash lookup");
    expect(nx_bad_monkey_low_spec_revision() == 1U, "spec revision 1");

    nx_bad_monkey_low_config_t config{};
    expect(nx_bad_monkey_low_config_init(&config) == NX_SUCCESS, "config init");
    expect(config.opamp_model == NX_OPAMP_JRC4558, "JRC4558 default");
    expect(config.bjt == NX_BJT_2N3904, "2N3904 default");
    expect(config.low.pot_params.taper == NX_POT_TAPER_LINEAR, "Low B");
    expect(std::fabs(config.low.pot_params.control - 0.5) < 1e-12, "Low default 0.5");
    expect(std::fabs(config.low.pot_params.value - 100000.0) < 1e-3, "Low 100k");
    expect(std::fabs(config.R1 - 22000.0) < 1e-6, "R1 22k");
    expect(std::fabs(config.R2 - 22000.0) < 1e-6, "R2 22k");
    expect(std::fabs(config.C1 - 4.7e-11) < 1e-18, "C1 47p");
    expect(std::fabs(config.C2 - 4.7e-7) < 1e-15, "C2 470n");
    expect(std::fabs(config.R3 - 100000.0) < 1e-6, "R3 100k");
    expect(std::fabs(config.R4 - 1000.0) < 1e-6, "R4 1k");
    expect(std::fabs(config.C3 - 4.7e-8) < 1e-16, "C3 47n");
    expect(std::fabs(config.R5 - 10000.0) < 1e-6, "R5 10k");
    expect(std::fabs(config.vcc - 9.0) < 1e-12, "vcc 9");
    expect(std::fabs(config.va - 4.5) < 1e-12, "va 4.5");

    const double fs = 96000.0;
    nx_bad_monkey_low_f32_t* low = nx_bad_monkey_low_create_f32(nullptr);
    expect(low != nullptr, "create");
    nx_bad_monkey_low_prepare_f32(low, fs);
    expect(nx_bad_monkey_low_set_opamp_model_f32(low, NX_OPAMP_JRC4558) == NX_SUCCESS, "set JRC4558");
    expect(nx_bad_monkey_low_set_bjt_model_f32(low, NX_BJT_2N3904) == NX_SUCCESS, "set 2N3904");
    expect(nx_bad_monkey_low_set_low_control_f32(low, 0.5) == NX_SUCCESS, "set low 0.5");
    nx_bad_monkey_low_tick_f32(low, 64);
    nx_bad_monkey_low_reset_f32(low);

    nx_bad_monkey_low_config_t via_config{};
    expect(nx_bad_monkey_low_get_config_f32(low, &via_config) == NX_SUCCESS, "get config");
    via_config.low.pot_params.control = 0.0;
    expect(nx_bad_monkey_low_set_config_f32(low, &via_config) == NX_SUCCESS, "set_config low 0");
    double mag_cut = 0.0;
    double phase_scratch = 0.0;
    const double f40 = 40.0;
    nx_bad_monkey_low_ac_static_f32(&via_config, fs, &f40, &mag_cut, &phase_scratch, 1);
    via_config.low.pot_params.control = 1.0;
    expect(nx_bad_monkey_low_set_config_f32(low, &via_config) == NX_SUCCESS, "set_config low 1");
    double mag_boost = 0.0;
    nx_bad_monkey_low_ac_static_f32(&via_config, fs, &f40, &mag_boost, &phase_scratch, 1);
    expect(mag_boost - mag_cut > 10.0, "low 0 vs 1 moves 40 Hz AC by >10 dB");

    expect(nx_bad_monkey_low_set_low_control_f32(low, 0.5) == NX_SUCCESS, "restore low 0.5");
    nx_bad_monkey_low_tick_f32(low, 64);
    nx_bad_monkey_low_reset_f32(low);

    constexpr int k_long = 4096 + 8192;
    constexpr int k_warmup = 4096;
    float in[k_long];
    float out[k_long];
    const double freq = 1000.0;
    const double amp = 1.0e-4;
    for (int i = 0; i < k_long; ++i)
        in[i] = static_cast<float>(amp * std::sin(2.0 * M_PI * freq * static_cast<double>(i) / fs));

    nx_bad_monkey_low_process_f32(low, in, out, static_cast<size_t>(k_long));

    double peak = 0.0;
    double sum_sq = 0.0;
    int count = 0;
    for (int i = k_warmup; i < k_long; ++i)
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

    config.low.pot_params.control = 0.5;
    assertAcMatches(config, kPythonMagDbFlat, "flat");
    config.low.pot_params.control = 0.0;
    assertAcMatches(config, kPythonMagDbCut, "cut");
    config.low.pot_params.control = 1.0;
    assertAcMatches(config, kPythonMagDbBoost, "boost");

    config.low.pot_params.control = 0.5;
    const double freq1k = 1000.0;
    double mag1k = 0.0;
    double phase1k = 0.0;
    nx_bad_monkey_low_ac_static_f32(&config, fs, &freq1k, &mag1k, &phase1k, 1);
    const double ac_err = std::fabs(process_db - mag1k);
    std::printf("bad_monkey_low process@1k %.2f dB  ac_static %.2f dB  |err| %.3f dB  python max err < 0.05 dB\n",
                process_db, mag1k, ac_err);
    expect(ac_err < 0.25, "1 kHz process matches ac_static within 0.25 dB");

    const double f20k = 20000.0;
    double mag_c1_47p = 0.0;
    double mag_c1_220p = 0.0;
    config.C1 = 4.7e-11;
    nx_bad_monkey_low_ac_static_f32(&config, fs, &f20k, &mag_c1_47p, &phase1k, 1);
    config.C1 = 2.2e-10;
    nx_bad_monkey_low_ac_static_f32(&config, fs, &f20k, &mag_c1_220p, &phase1k, 1);
    expect(std::fabs(mag_c1_220p - mag_c1_47p) > 0.01, "external C1 moves 20 kHz AC");

    expect(nx_bad_monkey_low_destroy_f32(low, nullptr) == NX_SUCCESS, "destroy");
    std::printf("bad_monkey_low operator test ok\n");
    return 0;
}
