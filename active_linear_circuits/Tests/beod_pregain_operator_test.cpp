// Operator-menu coverage for beod_pregain: defaults, process, ac_static vs Python, process vs ac_static.

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "nudsp/common/operator_spec_metadata.h"
#include "nudsp/nonlinear_circuits/beod_pregain_f32.h"

static void expect(bool cond, const char* message)
{
    if (!cond)
    {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

// Log-spaced 20–20 kHz @ 96 kHz; Models/beod/beod_pregain_solver.ac() gain=0.5 (ngspice embed ~0.3 dB RMS).
static constexpr int kAcPoints = 20;
static constexpr double kAcFreqs[kAcPoints] = {
    20.0,     28.769,   41.3828,  59.527,   85.6266,  123.1696, 177.1734, 254.855,
    366.5961, 527.3302, 758.538,  1091.119, 1569.52,  2257.676, 3247.554, 4671.443,
    6719.637, 9665.861, 13903.856, 20000.0,
};
static constexpr double kPythonMagDb[kAcPoints] = {
    5.969023,  8.452921,  11.135623, 13.827056, 16.337619, 18.476963, 20.100231, 21.181599,
    21.820872, 22.165389, 22.337306, 22.412270, 22.426789, 22.388078, 22.276222, 22.036456,
    21.563189, 20.686232, 19.180686, 16.788862,
};

static double measure_process_db(nx_beod_pregain_f32_t* pregain, double fs, double freq, double amp, int total_samples,
                                 int skip_samples)
{
    float* in = new float[static_cast<size_t>(total_samples)];
    float* out = new float[static_cast<size_t>(total_samples)];
    for (int i = 0; i < total_samples; ++i)
        in[i] = static_cast<float>(amp * std::sin(2.0 * M_PI * freq * static_cast<double>(i) / fs));

    nx_beod_pregain_process_f32(pregain, in, out, static_cast<size_t>(total_samples));

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
    const char* hash = nx_beod_pregain_spec_hash_hex();
    expect(hash != nullptr && std::strlen(hash) == 64U, "spec hash length");
    expect(std::strcmp(hash, nx_operator_spec_hash_hex("beod_pregain")) == 0, "spec hash lookup");
    expect(nx_beod_pregain_spec_revision() == 1U, "spec revision 1");

    nx_beod_pregain_config_t config{};
    expect(nx_beod_pregain_config_init(&config) == NX_SUCCESS, "config init");
    expect(config.opamp_model == NX_OPAMP_TL072, "default opamp TL072");
    expect(config.diode_model == NX_DIODE_DLED, "default diode DLED");
    expect(std::fabs(config.R3 - 22000.0) < 1e-6, "R3 22k");
    expect(std::fabs(config.C2 - 4.7e-08) < 1e-18, "C2 47n");
    expect(std::fabs(config.R2 - 22000.0) < 1e-6, "R2 22k");
    expect(std::fabs(config.C3 - 4.7e-11) < 1e-18, "C3 47p");
    expect(std::fabs(config.gain.pot_params.value - 1000000.0) < 1e-3, "gain 1M");
    expect(std::fabs(config.gain.pot_params.control - 0.5) < 1e-12, "gain control 0.5");
    expect(config.gain.pot_params.taper == NX_POT_TAPER_A30, "gain taper A30");

    // Schematic mis-edit (P1 shown as 0.5) must not stick after set_config sanitize.
    {
        nx_beod_pregain_config_t mis{};
        nx_beod_pregain_config_init(&mis);
        mis.gain.pot_params.value = 0.5;
        nx_beod_pregain_f32_t* tmp = nx_beod_pregain_create_f32(nullptr);
        expect(tmp != nullptr, "tmp create");
        expect(nx_beod_pregain_set_config_f32(tmp, &mis) == NX_SUCCESS, "set mis config");
        nx_beod_pregain_config_t snap{};
        nx_beod_pregain_get_config_f32(tmp, &snap);
        expect(std::fabs(snap.gain.pot_params.value - 1000000.0) < 1e-3, "sanitize restores 1M pot");
        nx_beod_pregain_destroy_f32(tmp, nullptr);
    }

    const double fs = 96000.0;
    nx_beod_pregain_f32_t* pregain = nx_beod_pregain_create_f32(nullptr);
    expect(pregain != nullptr, "create");
    nx_beod_pregain_prepare_f32(pregain, fs);
    nx_beod_pregain_set_opamp_model_f32(pregain, NX_OPAMP_TL072);
    nx_beod_pregain_set_diode_model_f32(pregain, NX_DIODE_DLED);
    nx_beod_pregain_set_gain_control_f32(pregain, 0.5);
    nx_beod_pregain_tick_f32(pregain, 64);
    nx_beod_pregain_reset_f32(pregain);

    constexpr int n = 4096;
    float in[n];
    float out[n];
    const double probe_freq = 1000.0;
    const double amp = 1.0e-4;
    for (int i = 0; i < n; ++i)
        in[i] = static_cast<float>(amp * std::sin(2.0 * M_PI * probe_freq * static_cast<double>(i) / fs));

    nx_beod_pregain_process_f32(pregain, in, out, static_cast<size_t>(n));

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
    nx_beod_pregain_ac_static_f32(&config, fs, kAcFreqs, mag_db, phase, kAcPoints);
    double max_ac_py_err = 0.0;
    for (int i = 0; i < kAcPoints; ++i)
    {
        expect(std::isfinite(mag_db[i]), "ac magnitude finite");
        expect(std::isfinite(phase[i]), "ac phase finite");
        const double err = std::fabs(mag_db[i] - kPythonMagDb[i]);
        if (err > max_ac_py_err)
            max_ac_py_err = err;
        if (err > 3.0)
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
    nx_beod_pregain_reset_f32(pregain);
    const double process_db = measure_process_db(pregain, fs, probe_freq, amp, k_long, k_warmup);
    const double ac_err = std::fabs(process_db - mag_db[idx_1k]);
    expect(ac_err < 0.5, "1 kHz process matches ac_static within 0.5 dB");

    double max_runtime_py_err = 0.0;
    for (int pt = 0; pt < kAcPoints; ++pt)
    {
        nx_beod_pregain_reset_f32(pregain);
        const double runtime_db = measure_process_db(pregain, fs, kAcFreqs[pt], amp, k_long, k_warmup);
        const double err = std::fabs(runtime_db - kPythonMagDb[pt]);
        if (err > max_runtime_py_err)
            max_runtime_py_err = err;
        if (err > 3.0)
        {
            std::fprintf(stderr, "FAIL: process vs python f=%.1f Hz  runtime=%.4f  py=%.4f  err=%.4f dB\n",
                         kAcFreqs[pt], runtime_db, kPythonMagDb[pt], err);
            std::exit(1);
        }
    }

    std::printf("beod_pregain ac_static vs Python: max |err| %.3f dB (< 3.0 dB)\n", max_ac_py_err);
    std::printf("beod_pregain process vs Python: max |err| %.3f dB (< 3.0 dB)\n", max_runtime_py_err);
    std::printf("beod_pregain process %.2f dB  ac_static@~1kHz %.2f dB  |err| %.3f dB\n", process_db,
                mag_db[idx_1k], ac_err);

    expect(nx_beod_pregain_destroy_f32(pregain, nullptr) == NX_SUCCESS, "destroy");
    std::printf("beod_pregain operator test ok\n");
    return 0;
}
