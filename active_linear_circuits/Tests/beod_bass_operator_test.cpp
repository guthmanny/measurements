// Operator-menu coverage for beod_bass: draw.io defaults, process, ac vs Python (DS-1 white-box parity).

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "nudsp/common/operator_spec_metadata.h"
#include "nudsp/linear_circuits/beod_bass_f32.h"

static void expect(bool cond, const char* message)
{
    if (!cond)
    {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

static constexpr int kAcPoints = 20;
static constexpr double kAcFreqs[kAcPoints] = {
    20.0,     28.769,   41.3828,  59.527,   85.6266,  123.1696, 177.1734, 254.855,
    366.5961, 527.3302, 758.538,  1091.119, 1569.52,  2257.676, 3247.554, 4671.443,
    6719.637, 9665.861, 13903.856, 20000.0,
};

// Models/beod/beod_bass_solver.ac() @ 96 kHz, bass=0.5, TL072, R1/R2=33k, C1=22n, C2=220n, P1=100kC legacy.
static constexpr double kPythonMagDb[kAcPoints] = {
    4.389646, 5.612937, 6.199867, 5.876174, 4.797050, 3.411200, 2.145959, 1.221812, 0.648047, 0.328962,
    0.162897, 0.079573, 0.038555, 0.018551, 0.008839, 0.004132, 0.001850, 0.000736, 0.000171, -0.000173,
};

static double measure_process_db(nx_beod_bass_f32_t* bass, double fs, double freq, double amp, int total_samples,
                                 int skip_samples)
{
    float* in = new float[static_cast<size_t>(total_samples)];
    float* out = new float[static_cast<size_t>(total_samples)];
    for (int i = 0; i < total_samples; ++i)
        in[i] = static_cast<float>(amp * std::sin(2.0 * M_PI * freq * static_cast<double>(i) / fs));

    nx_beod_bass_process_f32(bass, in, out, static_cast<size_t>(total_samples));

    double sum_sq = 0.0;
    int count = 0;
    for (int i = skip_samples; i < total_samples; ++i)
    {
        expect(std::isfinite(out[i]), "process finite");
        sum_sq += static_cast<double>(out[i]) * out[i];
        ++count;
    }
    delete[] in;
    delete[] out;
    const double rms = std::sqrt(sum_sq / static_cast<double>(count));
    const double input_rms = amp / std::sqrt(2.0);
    return 20.0 * std::log10(rms / input_rms);
}

int main()
{
    const char* hash = nx_beod_bass_spec_hash_hex();
    expect(hash != nullptr && std::strlen(hash) == 64U, "spec hash length");
    expect(std::strcmp(hash, nx_operator_spec_hash_hex("beod_bass")) == 0, "spec hash lookup");
    expect(nx_beod_bass_spec_revision() == 2U, "spec revision 2");

    nx_beod_bass_config_t config{};
    expect(nx_beod_bass_config_init(&config) == NX_SUCCESS, "config init");
    expect(std::fabs(config.C1 - 2.2e-08) < 1e-18, "C1 22n");
    expect(std::fabs(config.C2 - 2.2e-07) < 1e-18, "C2 220n");
    expect(std::fabs(config.R1 - 33000.0) < 1e-3, "R1 33k");
    expect(std::fabs(config.R2 - 33000.0) < 1e-3, "R2 33k");
    expect(std::fabs(config.bass.pot_params.value - 100000.0) < 1e-3, "bass 100k");
    expect(std::fabs(config.bass.pot_params.control - 0.5) < 1e-12, "bass control 0.5");
    expect(config.bass.pot_params.taper == NX_POT_TAPER_C, "bass taper C");

    const double fs = 96000.0;
    nx_beod_bass_f32_t* bass = nx_beod_bass_create_f32(nullptr);
    expect(bass != nullptr, "create");
    nx_beod_bass_prepare_f32(bass, fs);
    nx_beod_bass_set_bass_control_f32(bass, 0.5);
    nx_beod_bass_tick_f32(bass, 64);
    nx_beod_bass_reset_f32(bass);

    double mag_db[kAcPoints] = {};
    double phase[kAcPoints] = {};
    nx_beod_bass_ac_static_f32(&config, fs, kAcFreqs, mag_db, phase, kAcPoints);

    double max_ac_py_err = 0.0;
    for (int i = 0; i < kAcPoints; ++i)
    {
        expect(std::isfinite(mag_db[i]), "ac magnitude finite");
        const double err = std::fabs(mag_db[i] - kPythonMagDb[i]);
        if (err > max_ac_py_err)
            max_ac_py_err = err;
        if (err > 0.05)
        {
            std::fprintf(stderr, "FAIL: ac_static vs python f=%.1f Hz  c=%.4f  py=%.4f  err=%.4f dB\n", kAcFreqs[i],
                         mag_db[i], kPythonMagDb[i], err);
            std::exit(1);
        }
    }

    int idx_1k = 11;
    for (int i = 0; i < kAcPoints; ++i)
    {
        if (std::fabs(kAcFreqs[i] - 1000.0) < 50.0)
            idx_1k = i;
    }

    constexpr int k_long = 4096 + 8192;
    constexpr int k_warmup = 4096;
    nx_beod_bass_reset_f32(bass);
    const double probe_freq = 1000.0;
    const double amp = 1.0e-4;
    const double process_db = measure_process_db(bass, fs, probe_freq, amp, k_long, k_warmup);
    const double ac_err = std::fabs(process_db - mag_db[idx_1k]);
    expect(ac_err < 0.25, "1 kHz process matches ac_static within 0.25 dB");

    double mag_ac_f32[kAcPoints] = {};
    double phase_ac_f32[kAcPoints] = {};
    nx_beod_bass_ac_f32(bass, kAcFreqs, mag_ac_f32, phase_ac_f32, kAcPoints);
    expect(std::fabs(mag_ac_f32[idx_1k] - mag_db[idx_1k]) < 0.05, "ac_f32 matches ac_static at ~1 kHz");

    std::printf("beod_bass ac_static vs Python: max |err| %.3f dB (< 0.05 dB)\n", max_ac_py_err);
    std::printf("beod_bass process %.2f dB  ac_static@~1kHz %.2f dB  |err| %.3f dB\n", process_db, mag_db[idx_1k],
                ac_err);

    expect(nx_beod_bass_destroy_f32(bass, nullptr) == NX_SUCCESS, "destroy");
    std::printf("beod_bass operator test ok\n");
    return 0;
}
