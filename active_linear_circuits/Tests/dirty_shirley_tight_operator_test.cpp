// Operator-menu coverage for dirty_shirley_tight: defaults, process, ac_static vs Python.

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "nudsp/common/operator_spec_metadata.h"
#include "nudsp/linear_circuits/dirty_shirley_tight_f32.h"

static void expect(bool cond, const char* message)
{
    if (!cond)
    {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

// Log-spaced 20–20 kHz @ 96 kHz; Models/shirley/shirley_tight_solver.ac() NOR/TIGHT.
static constexpr int kAcPoints = 20;
static constexpr double kAcFreqs[kAcPoints] = {
    20.0,     28.769,   41.3828,  59.527,   85.6266,  123.1696, 177.1734, 254.855,
    366.5961, 527.3302, 758.538,  1091.119, 1569.52,  2257.676, 3247.554, 4671.443,
    6719.637, 9665.861, 13903.856, 20000.0,
};
static constexpr double kPythonMagDbNor[kAcPoints] = {
    -25.182541, -22.041718, -18.918936, -15.832806, -12.819742, -9.947020, -7.325719, -5.103893,
    -3.411384,  -2.275617,  -1.600737,  -1.238792,  -1.065575,  -1.007024,  -1.035968, -1.167789,
    -1.467664,  -2.071779,  -3.218104,  -5.296654,
};
static constexpr double kPythonMagDbTight[kAcPoints] = {
    -31.210933, -28.057326, -24.908288, -21.768664, -18.648331, -15.567143, -12.563801, -9.709279,
    -7.119103,  -4.943228,  -3.306201,  -2.224982,  -1.598617,  -1.286102,  -1.182217,  -1.246851,
    -1.512797,  -2.098939,  -3.234463,  -5.305450,
};

static void expectAcMatchesPython(const nx_dirty_shirley_tight_config_t& config, const double* expected,
                                  const char* label)
{
    double mag_db[kAcPoints] = {};
    double phase[kAcPoints] = {};
    nx_dirty_shirley_tight_ac_static_f32(&config, 96000.0, kAcFreqs, mag_db, phase, kAcPoints);
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
    const char* hash = nx_dirty_shirley_tight_spec_hash_hex();
    expect(hash != nullptr && std::strlen(hash) == 64U, "spec hash length");
    expect(std::strcmp(hash, nx_operator_spec_hash_hex("dirty_shirley_tight")) == 0, "spec hash lookup");
    expect(nx_dirty_shirley_tight_spec_revision() == 1U, "spec revision 1");

    nx_dirty_shirley_tight_config_t config{};
    expect(nx_dirty_shirley_tight_config_init(&config) == NX_SUCCESS, "config init");
    expect(std::fabs(config.C1 - 1e-08) < 1e-18, "C1 10n");
    expect(std::fabs(config.R1 - 10000.0) < 1e-3, "R1 10k");
    expect(std::fabs(config.C2 - 1e-09) < 1e-18, "C2 1n");
    expect(std::fabs(config.R2 - 18000.0) < 1e-3, "R2 18k");
    expect(std::fabs(config.R3 - 22000.0) < 1e-3, "R3 22k");
    expect(std::fabs(config.R4 - 3900.0) < 1e-3, "R4 3.9k");
    expect(config.sw_tight.position == 0, "SW_TIGHT default NOR");

    const double fs = 96000.0;
    nx_dirty_shirley_tight_f32_t* tight = nx_dirty_shirley_tight_create_f32(nullptr);
    expect(tight != nullptr, "create");
    nx_dirty_shirley_tight_prepare_f32(tight, fs);
    expect(nx_dirty_shirley_tight_set_sw_tight_switch_f32(tight, 0) == NX_SUCCESS, "set NOR");
    expect(nx_dirty_shirley_tight_get_sw_tight_switch_f32(tight) == 0, "get NOR");
    nx_dirty_shirley_tight_tick_f32(tight, 64);
    nx_dirty_shirley_tight_reset_f32(tight);

    constexpr int n = 4096;
    float in[n];
    float out[n];
    const double freq = 1000.0;
    const double amp = 1.0e-4;
    for (int i = 0; i < n; ++i)
        in[i] = static_cast<float>(amp * std::sin(2.0 * M_PI * freq * static_cast<double>(i) / fs));

    nx_dirty_shirley_tight_process_f32(tight, in, out, static_cast<size_t>(n));

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

    expectAcMatchesPython(config, kPythonMagDbNor, "NOR");
    config.sw_tight.position = 1;
    expectAcMatchesPython(config, kPythonMagDbTight, "TIGHT");

    double mag_1k = 0.0;
    double phase_1k = 0.0;
    config.sw_tight.position = 0;
    const double f_1k = 1000.0;
    nx_dirty_shirley_tight_ac_static_f32(&config, fs, &f_1k, &mag_1k, &phase_1k, 1);
    const double ac_err = std::fabs(process_db - mag_1k);
    std::printf("dirty_shirley_tight NOR process %.2f dB  ac_static@1kHz %.2f dB  |err| %.3f dB\n",
                process_db, mag_1k, ac_err);
    expect(ac_err < 0.25, "1 kHz process matches ac_static within 0.25 dB");

    expect(nx_dirty_shirley_tight_set_sw_tight_switch_f32(tight, 1) == NX_SUCCESS, "set TIGHT");
    expect(nx_dirty_shirley_tight_get_sw_tight_switch_f32(tight) == 1, "get TIGHT");

    expect(nx_dirty_shirley_tight_destroy_f32(tight, nullptr) == NX_SUCCESS, "destroy");
    std::printf("dirty_shirley_tight operator test ok\n");
    return 0;
}
