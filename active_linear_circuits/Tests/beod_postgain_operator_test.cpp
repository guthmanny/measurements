// Operator-menu coverage for beod_postgain: defaults, process, ac_static vs Python, process vs ac_static.

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "nudsp/common/operator_spec_metadata.h"
#include "nudsp/nonlinear_circuits/beod_postgain_f32.h"

static void expect(bool cond, const char* message)
{
    if (!cond)
    {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

// Log-spaced 20–20 kHz @ 96 kHz; Models/beod/beod_postgain_solver.ac() (ngspice embed ~0.01 dB RMS).
static constexpr int kAcPoints = 20;
static constexpr double kAcFreqs[kAcPoints] = {
    20.0,     28.769,   41.3828,  59.527,   85.6266,  123.1696, 177.1734, 254.855,
    366.5961, 527.3302, 758.538,  1091.119, 1569.52,  2257.676, 3247.554, 4671.443,
    6719.637, 9665.861, 13903.856, 20000.0,
};
static constexpr double kPythonMagDb[kAcPoints] = {
    2.093446,  5.174926,  8.178854,  11.034516, 13.626798, 15.806332, 17.449444, 18.541028,
    19.186478, 19.536528, 19.716383, 19.806050, 19.850058, 19.871487, 19.881882, 19.886915,
    19.889349, 19.890525, 19.891094, 19.891367,
};

static double measure_process_db(nx_beod_postgain_f32_t* postgain, double fs, double freq, double amp,
                                 int total_samples, int skip_samples)
{
    float* in = new float[static_cast<size_t>(total_samples)];
    float* out = new float[static_cast<size_t>(total_samples)];
    for (int i = 0; i < total_samples; ++i)
        in[i] = static_cast<float>(amp * std::sin(2.0 * M_PI * freq * static_cast<double>(i) / fs));

    nx_beod_postgain_process_f32(postgain, in, out, static_cast<size_t>(total_samples));

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
    const char* hash = nx_beod_postgain_spec_hash_hex();
    expect(hash != nullptr && std::strlen(hash) == 64U, "spec hash length");
    expect(std::strcmp(hash, nx_operator_spec_hash_hex("beod_postgain")) == 0, "spec hash lookup");
    expect(nx_beod_postgain_spec_revision() == 1U, "spec revision 1");

    nx_beod_postgain_config_t config{};
    expect(nx_beod_postgain_config_init(&config) == NX_SUCCESS, "config init");
    expect(config.opamp_model == NX_OPAMP_TL072, "default opamp TL072");
    expect(config.diode_model == NX_DIODE_1N4148, "default diode 1N4148");
    expect(std::fabs(config.C1 - 4.7e-08) < 1e-18, "C1 47n");
    expect(std::fabs(config.R2 - 22000.0) < 1e-6, "R2 22k");
    expect(std::fabs(config.R4 - 22000.0) < 1e-6, "R4 22k");
    expect(std::fabs(config.R3 - 220000.0) < 1e-6, "R3 220k");
    expect(config.Rbias >= 1e11, "Rbias open (internal)");
    expect(std::fabs(config.va - 4.5) < 1e-6, "va 4.5 V");
    expect(std::fabs(config.vcc - 9.0) < 1e-6, "vcc 9 V");

    const double fs = 96000.0;
    nx_beod_postgain_f32_t* postgain = nx_beod_postgain_create_f32(nullptr);
    expect(postgain != nullptr, "create");
    nx_beod_postgain_prepare_f32(postgain, fs);
    nx_beod_postgain_set_opamp_model_f32(postgain, NX_OPAMP_TL072);
    nx_beod_postgain_set_diode_model_f32(postgain, NX_DIODE_1N4148);
    nx_beod_postgain_tick_f32(postgain, 64);
    nx_beod_postgain_reset_f32(postgain);

    constexpr int n = 4096;
    float in[n];
    float out[n];
    const double probe_freq = 1000.0;
    const double amp = 1.0e-4;
    for (int i = 0; i < n; ++i)
        in[i] = static_cast<float>(amp * std::sin(2.0 * M_PI * probe_freq * static_cast<double>(i) / fs));

    nx_beod_postgain_process_f32(postgain, in, out, static_cast<size_t>(n));

    const int skip = static_cast<int>(0.01 * fs);
    double peak = 0.0;
    for (int i = skip; i < n; ++i)
    {
        const double a = std::fabs(out[i]);
        if (a > peak)
            peak = a;
    }
    expect(peak > 0.0, "process has energy");

    double mag_db[kAcPoints] = {};
    double phase[kAcPoints] = {};
    nx_beod_postgain_ac_static_f32(&config, fs, kAcFreqs, mag_db, phase, kAcPoints);
    double max_ac_py_err = 0.0;
    for (int i = 0; i < kAcPoints; ++i)
    {
        expect(std::isfinite(mag_db[i]), "ac magnitude finite");
        expect(std::isfinite(phase[i]), "ac phase finite");
        const double err = std::fabs(mag_db[i] - kPythonMagDb[i]);
        if (err > max_ac_py_err)
            max_ac_py_err = err;
        if (err > 0.5)
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
    nx_beod_postgain_reset_f32(postgain);
    const double process_db = measure_process_db(postgain, fs, probe_freq, amp, k_long, k_warmup);
    const double ac_err = std::fabs(process_db - mag_db[idx_1k]);
    expect(ac_err < 0.5, "1 kHz process matches ac_static within 0.5 dB");

    double max_runtime_py_err = 0.0;
    for (int pt = 0; pt < kAcPoints; ++pt)
    {
        nx_beod_postgain_reset_f32(postgain);
        const double runtime_db = measure_process_db(postgain, fs, kAcFreqs[pt], amp, k_long, k_warmup);
        const double err = std::fabs(runtime_db - kPythonMagDb[pt]);
        if (err > max_runtime_py_err)
            max_runtime_py_err = err;
        if (err > 0.5)
        {
            std::fprintf(stderr, "FAIL: process vs python f=%.1f Hz  runtime=%.4f  py=%.4f  err=%.4f dB\n",
                         kAcFreqs[pt], runtime_db, kPythonMagDb[pt], err);
            std::exit(1);
        }
    }

    std::printf("beod_postgain ac_static vs Python: max |err| %.3f dB (< 0.5 dB)\n", max_ac_py_err);
    std::printf("beod_postgain process vs Python: max |err| %.3f dB (< 0.5 dB)\n", max_runtime_py_err);
    std::printf("beod_postgain process %.2f dB  ac_static@~1kHz %.2f dB  |err| %.3f dB\n", process_db,
                mag_db[idx_1k], ac_err);

    expect(mag_db[idx_1k] > 10.0, "ac_static ~1 kHz gain > 10 dB (schematic 47n/22k defaults)");
    expect(mag_db[idx_1k] < 30.0, "ac_static ~1 kHz gain < 30 dB");

    double mag_ac_f32[kAcPoints] = {};
    double phase_ac_f32[kAcPoints] = {};
    nx_beod_postgain_ac_f32(postgain, kAcFreqs, mag_ac_f32, phase_ac_f32, kAcPoints);
    expect(std::fabs(mag_ac_f32[idx_1k] - mag_db[idx_1k]) < 0.05, "ac_f32 matches ac_static at ~1 kHz");

    expect(nx_beod_postgain_destroy_f32(postgain, nullptr) == NX_SUCCESS, "destroy");
    std::printf("beod_postgain operator test ok\n");
    return 0;
}
