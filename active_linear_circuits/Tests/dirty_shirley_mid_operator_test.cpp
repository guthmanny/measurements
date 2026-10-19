// Operator-menu coverage for dirty_shirley_mid: defaults, process, ac_static vs Python.

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "nudsp/common/operator_spec_metadata.h"
#include "nudsp/linear_circuits/dirty_shirley_mid_f32.h"

static void expect(bool cond, const char* message)
{
    if (!cond)
    {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

// Log-spaced 20–20 kHz @ 96 kHz; Models/shirley/shirley_mid_solver.ac().
static constexpr int kAcPoints = 20;
static constexpr double kAcFreqs[kAcPoints] = {
    20.0,     28.769,   41.3828,  59.527,   85.6266,  123.1696, 177.1734, 254.855,
    366.5961, 527.3302, 758.538,  1091.119, 1569.52,  2257.676, 3247.554, 4671.443,
    6719.637, 9665.861, 13903.856, 20000.0,
};
static constexpr double kPythonMagDbLevel50Mid50[kAcPoints] = {
    -12.486377, -12.379952, -12.170262, -11.775877, -11.092275, -10.049916, -8.717264, -7.350349,
    -6.301525,  -5.856565,  -6.132856,  -7.058737,  -8.383977,  -9.757874,  -10.887243, -11.657332,
    -12.113716, -12.360298, -12.486450, -12.548958,
};
static constexpr double kPythonMagDbLevel25Mid75[kAcPoints] = {
    -24.388741, -24.262657, -24.014289, -23.546994, -22.734364, -21.479505, -19.818177, -17.979752,
    -16.352991, -15.386745, -15.407253, -16.407771, -18.053981, -19.897231, -21.553078, -22.797948,
    -23.601102, -24.061651, -24.305787, -24.429038,
};

static void expectAcMatchesPython(const nx_dirty_shirley_mid_config_t& config, const double* expected,
                                  const char* label)
{
    double mag_db[kAcPoints] = {};
    double phase[kAcPoints] = {};
    nx_dirty_shirley_mid_ac_static_f32(&config, 96000.0, kAcFreqs, mag_db, phase, kAcPoints);
    for (int i = 0; i < kAcPoints; ++i)
    {
        expect(std::isfinite(mag_db[i]), "ac magnitude finite");
        expect(std::isfinite(phase[i]), "ac phase finite");
        const double err = std::fabs(mag_db[i] - expected[i]);
        if (err > 0.05)
        {
            std::fprintf(stderr, "FAIL: %s python ac mismatch f=%.1f Hz  c=%.4f  py=%.4f  err=%.4f dB\n",
                         label, kAcFreqs[i], mag_db[i], expected[i], err);
            std::exit(1);
        }
    }
}

int main()
{
    const char* hash = nx_dirty_shirley_mid_spec_hash_hex();
    expect(hash != nullptr && std::strlen(hash) == 64U, "spec hash length");
    expect(std::strcmp(hash, nx_operator_spec_hash_hex("dirty_shirley_mid")) == 0, "spec hash lookup");
    expect(nx_dirty_shirley_mid_spec_revision() == 1U, "spec revision 1");

    nx_dirty_shirley_mid_config_t config{};
    expect(nx_dirty_shirley_mid_config_init(&config) == NX_SUCCESS, "config init");
    expect(std::fabs(config.R3 - 2200.0) < 1e-3, "R3 2.2k");
    expect(std::fabs(config.R4 - 2200.0) < 1e-3, "R4 2.2k");
    expect(std::fabs(config.R5 - 470000.0) < 1e-3, "R5 470k");
    expect(std::fabs(config.C1 - 2.2e-09) < 1e-18, "C1 2.2n");
    expect(std::fabs(config.R1 - 2200.0) < 1e-3, "R1 2.2k");
    expect(std::fabs(config.R2 - 22000.0) < 1e-3, "R2 22k");
    expect(std::fabs(config.C2 - 1e-08) < 1e-18, "C2 10n");
    expect(std::fabs(config.level.pot_params.control - 0.5) < 1e-12, "LEVEL default 0.5");
    expect(std::fabs(config.level.pot_params.value - 50000.0) < 1e-3, "LEVEL 50k");
    expect(std::fabs(config.mid.pot_params.control - 0.5) < 1e-12, "MID default 0.5");
    expect(std::fabs(config.mid.pot_params.value - 100000.0) < 1e-3, "MID 100k");
    expect(config.opamp_model == NX_OPAMP_TL072, "TL072");

    expectAcMatchesPython(config, kPythonMagDbLevel50Mid50, "level=0.5 mid=0.5");
    config.level.pot_params.control = 0.25;
    config.mid.pot_params.control = 0.75;
    expectAcMatchesPython(config, kPythonMagDbLevel25Mid75, "level=0.25 mid=0.75");

    const double fs = 96000.0;
    nx_dirty_shirley_mid_f32_t* mid = nx_dirty_shirley_mid_create_f32(nullptr);
    expect(mid != nullptr, "create");
    config.level.pot_params.control = 0.5;
    config.mid.pot_params.control = 0.5;
    expect(nx_dirty_shirley_mid_set_config_f32(mid, &config) == NX_SUCCESS, "set config defaults");
    nx_dirty_shirley_mid_prepare_f32(mid, fs);
    nx_dirty_shirley_mid_tick_f32(mid, 64);
    nx_dirty_shirley_mid_reset_f32(mid);

    constexpr int n = 4096;
    float in[n];
    float out[n];
    const double freq = 1000.0;
    const double amp = 1.0e-4;
    for (int i = 0; i < n; ++i)
        in[i] = static_cast<float>(amp * std::sin(2.0 * M_PI * freq * static_cast<double>(i) / fs));

    nx_dirty_shirley_mid_process_f32(mid, in, out, static_cast<size_t>(n));

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

    double mag_1k = 0.0;
    double phase_1k = 0.0;
    const double f_1k = 1000.0;
    nx_dirty_shirley_mid_ac_static_f32(&config, fs, &f_1k, &mag_1k, &phase_1k, 1);
    const double ac_err = std::fabs(process_db - mag_1k);
    std::printf("dirty_shirley_mid process %.2f dB  ac_static@1kHz %.2f dB  |err| %.3f dB\n",
                process_db, mag_1k, ac_err);
    expect(ac_err < 0.25, "1 kHz process matches ac_static within 0.25 dB");

    expect(nx_dirty_shirley_mid_set_level_control_f32(mid, 0.25) == NX_SUCCESS, "set level");
    expect(std::fabs(nx_dirty_shirley_mid_get_level_control_f32(mid) - 0.25) < 1e-12, "get level");
    expect(nx_dirty_shirley_mid_set_mid_control_f32(mid, 0.75) == NX_SUCCESS, "set mid");
    expect(std::fabs(nx_dirty_shirley_mid_get_mid_control_f32(mid) - 0.75) < 1e-12, "get mid");

    expect(nx_dirty_shirley_mid_destroy_f32(mid, nullptr) == NX_SUCCESS, "destroy");
    std::printf("dirty_shirley_mid operator test ok\n");
    return 0;
}
