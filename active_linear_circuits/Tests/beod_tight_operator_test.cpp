// Operator-menu coverage for beod_tight: defaults, process, ac_static vs Python reference.

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "nudsp/common/operator_spec_metadata.h"
#include "nudsp/linear_circuits/beod_tight_f32.h"

static void expect(bool cond, const char* message)
{
    if (!cond)
    {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

// Log-spaced 20–20 kHz @ 96 kHz; Models/beod/beod_tight_solver.ac() tight=0.5, output vn3 (R2 10k / C3 1n).
static constexpr int kAcPoints = 20;
static constexpr double kAcFreqs[kAcPoints] = {
    20.0,     28.769,   41.3828,  59.527,   85.6266,  123.1696, 177.1734, 254.855,
    366.5961, 527.3302, 758.538,  1091.119, 1569.52,  2257.676, 3247.554, 4671.443,
    6719.637, 9665.861, 13903.856, 20000.0,
};
static constexpr double kPythonMagDb[kAcPoints] = {
    -28.568034, -25.417994, -22.276308, -19.151760, -16.062063, -13.041972, -10.156061, -7.512120,
    -5.256838,  -3.524588,  -2.352804,  -1.654135,  -1.283918,  -1.120004,  -1.095255,  -1.200254,
    -1.486614,  -2.083487,  -3.225322,  -5.300598,
};

int main()
{
    const char* hash = nx_beod_tight_spec_hash_hex();
    expect(hash != nullptr && std::strlen(hash) == 64U, "spec hash length");
    expect(std::strcmp(hash, nx_operator_spec_hash_hex("beod_tight")) == 0, "spec hash lookup");
    expect(nx_beod_tight_spec_revision() == 3U, "spec revision 3");

    nx_beod_tight_config_t config{};
    expect(nx_beod_tight_config_init(&config) == NX_SUCCESS, "config init");
    expect(std::fabs(config.C1 - 1e-08) < 1e-18, "C1 10n");
    expect(std::fabs(config.R1 - 4700.0) < 1e-3, "R1 4.7k");
    expect(std::fabs(config.R2 - 10000.0) < 1e-6, "R2 10k");
    expect(std::fabs(config.C3 - 1e-09) < 1e-18, "C3 1n");
    expect(std::fabs(config.tight.pot_params.value - 100000.0) < 1e-3, "tight 100k");
    expect(std::fabs(config.tight.pot_params.control - 0.5) < 1e-12, "tight control 0.5");
    expect(config.tight.pot_params.taper == NX_POT_TAPER_C, "tight taper C");

    const double fs = 96000.0;
    nx_beod_tight_f32_t* tight = nx_beod_tight_create_f32(nullptr);
    expect(tight != nullptr, "create");
    nx_beod_tight_prepare_f32(tight, fs);
    nx_beod_tight_set_tight_control_f32(tight, 0.5);
    nx_beod_tight_tick_f32(tight, 64);
    nx_beod_tight_reset_f32(tight);

    constexpr int n = 4096;
    float in[n];
    float out[n];
    const double freq = 1000.0;
    const double amp = 1.0e-4;
    for (int i = 0; i < n; ++i)
        in[i] = static_cast<float>(amp * std::sin(2.0 * M_PI * freq * static_cast<double>(i) / fs));

    nx_beod_tight_process_f32(tight, in, out, static_cast<size_t>(n));

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

    double mag_db[kAcPoints] = {};
    double phase[kAcPoints] = {};
    nx_beod_tight_ac_static_f32(&config, fs, kAcFreqs, mag_db, phase, kAcPoints);
    for (int i = 0; i < kAcPoints; ++i)
    {
        expect(std::isfinite(mag_db[i]), "ac magnitude finite");
        expect(std::isfinite(phase[i]), "ac phase finite");
        const double err = std::fabs(mag_db[i] - kPythonMagDb[i]);
        if (err > 0.05)
        {
            std::fprintf(stderr, "FAIL: python ac mismatch f=%.1f Hz  c=%.4f  py=%.4f  err=%.4f dB\n",
                         kAcFreqs[i], mag_db[i], kPythonMagDb[i], err);
            std::exit(1);
        }
    }

    int idx_1k = 11;
    for (int i = 0; i < kAcPoints; ++i)
    {
        if (std::fabs(kAcFreqs[i] - 1000.0) < 1.0)
            idx_1k = i;
    }
    const double ac_err = std::fabs(process_db - mag_db[idx_1k]);
    std::printf("beod_tight process %.2f dB  ac_static@1kHz %.2f dB  |err| %.3f dB  python max err < 0.05 dB\n",
                process_db, mag_db[idx_1k], ac_err);
    expect(ac_err < 0.25, "1 kHz process matches ac_static within 0.25 dB");

    expect(nx_beod_tight_destroy_f32(tight, nullptr) == NX_SUCCESS, "destroy");
    std::printf("beod_tight operator test ok\n");
    return 0;
}
