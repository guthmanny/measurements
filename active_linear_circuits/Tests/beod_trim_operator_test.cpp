// Operator-menu coverage for beod_trim: draw.io defaults, process, ac vs Python (DS-1 white-box parity).

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "nudsp/common/operator_spec_metadata.h"
#include "nudsp/linear_circuits/beod_trim_f32.h"

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

// Models/beod/beod_trim_solver.ac() @ 96 kHz, trim=0.5, TL072, R1/R2=10k, C1=100p, P1=100kB linear.
static constexpr double kPythonMagDb[kAcPoints] = {
    15.562962, 15.562960, 15.562956, 15.562948, 15.562931, 15.562895, 15.562822, 15.562670,
    15.562356, 15.561706, 15.560361, 15.557574, 15.551808, 15.539876, 15.515182, 15.463867,
    15.357007, 15.133258, 14.658520, 13.620664,
};

static double measure_process_db(nx_beod_trim_f32_t* trim, double fs, double freq, double amp, int total_samples,
                                 int skip_samples)
{
    float* in = new float[static_cast<size_t>(total_samples)];
    float* out = new float[static_cast<size_t>(total_samples)];
    for (int i = 0; i < total_samples; ++i)
        in[i] = static_cast<float>(amp * std::sin(2.0 * M_PI * freq * static_cast<double>(i) / fs));

    nx_beod_trim_process_f32(trim, in, out, static_cast<size_t>(total_samples));

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

    const double process_rms = std::sqrt(sum_sq / static_cast<double>(count));
    const double input_rms = amp / std::sqrt(2.0);
    return 20.0 * std::log10(process_rms / input_rms);
}

int main()
{
    const char* hash = nx_beod_trim_spec_hash_hex();
    expect(hash != nullptr && std::strlen(hash) == 64U, "spec hash length");
    expect(std::strcmp(hash, nx_operator_spec_hash_hex("beod_trim")) == 0, "spec hash lookup");
    expect(nx_beod_trim_spec_revision() == 2U, "spec revision 2");
    expect(std::strcmp(hash, "fefc425e2fdbbecb070adce7d8a06e9d833283faac8606ca43e328c12cf82b8a") == 0,
           "trim spec hash (draw.io aligned)");

    nx_beod_trim_config_t config{};
    expect(nx_beod_trim_config_init(&config) == NX_SUCCESS, "config init");
    expect(std::fabs(config.C1 - 1e-10) < 1e-18, "C1 100p");
    expect(std::fabs(config.R1 - 10000.0) < 1e-3, "R1 10k");
    expect(std::fabs(config.R2 - 10000.0) < 1e-3, "R2 10k");
    expect(std::fabs(config.trim.pot_params.value - 100000.0) < 1e-3, "trim 100k");
    expect(config.trim.pot_params.taper == NX_POT_TAPER_LINEAR, "trim taper B (linear)");
    expect(config.opamp_model == NX_OPAMP_TL072, "TL072 default");
    expect(std::fabs(config.va - 4.5) < 1e-6, "va 4.5 V");

    config.trim.pot_params.control = 0.5;
    const double fs = 96000.0;
    nx_beod_trim_f32_t* trim = nx_beod_trim_create_f32(nullptr);
    expect(trim != nullptr, "create");
    nx_beod_trim_prepare_f32(trim, fs);
    nx_beod_trim_set_opamp_model_f32(trim, NX_OPAMP_TL072);
    nx_beod_trim_set_trim_control_f32(trim, 0.5);
    nx_beod_trim_tick_f32(trim, 64);
    nx_beod_trim_reset_f32(trim);

    double mag_db[kAcPoints] = {};
    double phase[kAcPoints] = {};
    nx_beod_trim_ac_static_f32(&config, fs, kAcFreqs, mag_db, phase, kAcPoints);

    double max_ac_py_err = 0.0;
    for (int i = 0; i < kAcPoints; ++i)
    {
        expect(std::isfinite(mag_db[i]), "ac magnitude finite");
        expect(std::isfinite(phase[i]), "ac phase finite");
        const double err = std::fabs(mag_db[i] - kPythonMagDb[i]);
        if (err > max_ac_py_err)
            max_ac_py_err = err;
        if (err > 0.08)
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
    nx_beod_trim_reset_f32(trim);
    const double probe_freq = 1000.0;
    const double amp = 1.0e-4;
    const double process_db = measure_process_db(trim, fs, probe_freq, amp, k_long, k_warmup);
    const double ac_err = std::fabs(process_db - mag_db[idx_1k]);
    expect(ac_err < 0.25, "1 kHz process matches ac_static within 0.25 dB");

    double mag_ac_f32[kAcPoints] = {};
    double phase_ac_f32[kAcPoints] = {};
    nx_beod_trim_ac_f32(trim, kAcFreqs, mag_ac_f32, phase_ac_f32, kAcPoints);
    expect(std::fabs(mag_ac_f32[idx_1k] - mag_db[idx_1k]) < 0.05, "ac_f32 matches ac_static at ~1 kHz");

    expect(mag_db[idx_1k] > 14.0, "ac_static ~1 kHz gain > 14 dB (trim=0.5)");
    expect(mag_db[idx_1k] < 17.0, "ac_static ~1 kHz gain < 17 dB");

    std::printf("beod_trim ac_static vs Python: max |err| %.3f dB (< 0.08 dB)\n", max_ac_py_err);
    std::printf("beod_trim process %.2f dB  ac_static@~1kHz %.2f dB  |err| %.3f dB\n", process_db, mag_db[idx_1k],
                ac_err);

    expect(nx_beod_trim_destroy_f32(trim, nullptr) == NX_SUCCESS, "destroy");
    std::printf("beod_trim operator test ok\n");
    return 0;
}
