// Operator-menu coverage for beod_preamp: defaults, process, ac_static vs Python reference.

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "nudsp/common/operator_spec_metadata.h"
#include "nudsp/linear_circuits/beod_preamp_f32.h"

static void expect(bool cond, const char* message)
{
    if (!cond)
    {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

// Log-spaced 20–20 kHz @ 96 kHz; from Models/beod/beod_preamp_solver.ac() (LTI, spec R1=470k).
static constexpr int kAcPoints = 20;
static constexpr double kAcFreqs[kAcPoints] = {
    20.0,     28.769,   41.3828,  59.527,   85.6266,  123.1696, 177.1734, 254.855,
    366.5961, 527.3302, 758.538,  1091.119, 1569.52,  2257.676, 3247.554, 4671.443,
    6719.637, 9665.861, 13903.856, 20000.0,
};
static constexpr double kPythonMagDb[kAcPoints] = {
    6.826229,  7.750438,  8.279096,  8.559665,  8.702046,  8.772568,  8.807064,  8.823834,
    8.83196,   8.835889,  8.837779,  8.838675,  8.839068,  8.839178,  8.839062,  8.838653,
    8.837703,  8.835591,  8.830688,  8.817744,
};

int main()
{
    const char* hash = nx_beod_preamp_spec_hash_hex();
    expect(hash != nullptr && std::strlen(hash) == 64U, "spec hash length");
    expect(std::strcmp(hash, nx_operator_spec_hash_hex("beod_preamp")) == 0, "spec hash lookup");

    nx_beod_preamp_config_t config{};
    expect(nx_beod_preamp_config_init(&config) == NX_SUCCESS, "config init");
    expect(config.opamp_model == NX_OPAMP_TL072, "default opamp TL072");
    expect(std::fabs(config.R1 - 470000.0) < 1e-3, "R1 470k");
    expect(std::fabs(config.C1 - 2.2e-08) < 1e-18, "C1 22n");
    expect(std::fabs(config.R2 - 10000.0) < 1e-6, "R2 10k");
    expect(std::fabs(config.C2 - 4.7e-11) < 1e-18, "C2 47p");
    expect(std::fabs(config.R4 - 22000.0) < 1e-6, "R4 22k");
    expect(std::fabs(config.R3 - 39000.0) < 1e-6, "R3 39k");

    const double fs = 96000.0;
    nx_beod_preamp_f32_t* preamp = nx_beod_preamp_create_f32(nullptr);
    expect(preamp != nullptr, "create");
    nx_beod_preamp_prepare_f32(preamp, fs);
    nx_beod_preamp_tick_f32(preamp, 64);
    nx_beod_preamp_reset_f32(preamp);

    constexpr int n = 4096;
    float in[n];
    float out[n];
    const double freq = 1000.0;
    const double amp = 1.0e-4;
    for (int i = 0; i < n; ++i)
        in[i] = static_cast<float>(amp * std::sin(2.0 * M_PI * freq * static_cast<double>(i) / fs));

    nx_beod_preamp_process_f32(preamp, in, out, static_cast<size_t>(n));

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
    nx_beod_preamp_ac_static_f32(&config, fs, kAcFreqs, mag_db, phase, kAcPoints);
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

    const double ac_err = std::fabs(process_db - mag_db[11]);
    std::printf("beod_preamp process %.2f dB  ac_static@1kHz %.2f dB  |err| %.3f dB  python max err < 0.05 dB\n",
                process_db, mag_db[11], ac_err);
    expect(ac_err < 0.15, "1 kHz process matches ac_static within 0.15 dB");

    expect(nx_beod_preamp_destroy_f32(preamp, nullptr) == NX_SUCCESS, "destroy");
    std::printf("beod_preamp operator test ok\n");
    return 0;
}
