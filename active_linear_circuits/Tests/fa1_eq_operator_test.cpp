// Operator-menu coverage for fa1_eq: defaults, process, ac_static vs Python reference.

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "nudsp/common/operator_spec_metadata.h"
#include "nudsp/linear_circuits/fa1_eq_f32.h"

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
    -0.026381, -0.026771, -0.029491, -0.045582, -0.092798, -0.131567, -0.158443, -0.332072,
};
static constexpr double kPythonMagDbBass0[kAcPoints] = {
    -16.317075, -13.458990, -7.320640, -1.688803, -0.199887, -0.131651, -0.150692, -0.323733,
};
static constexpr double kPythonMagDbBass1[kAcPoints] = {
    16.200169, 13.352912, 7.218842, 1.551153, -0.020146, -0.139490, -0.167569, -0.340777,
};
static constexpr double kPythonMagDbTreble0[kAcPoints] = {
    -0.005504, 0.106959, 0.504693, 0.483264, -2.707490, -9.310311, -15.448825, -18.192734,
};
static constexpr double kPythonMagDbTreble1[kAcPoints] = {
    -0.047258, -0.160502, -0.563708, -0.572351, 2.577347, 9.312881, 15.895120, 16.003545,
};

static void assertAcMatches(const nx_fa1_eq_config_t& config, const double* python_db, const char* label)
{
    double mag_db[kAcPoints] = {};
    double phase[kAcPoints] = {};
    nx_fa1_eq_ac_static_f32(&config, 96000.0, kAcFreqs, mag_db, phase, kAcPoints);
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
    const char* hash = nx_fa1_eq_spec_hash_hex();
    expect(hash != nullptr && std::strlen(hash) == 64U, "spec hash length");
    expect(std::strcmp(hash, nx_operator_spec_hash_hex("fa1_eq")) == 0, "spec hash lookup");
    expect(nx_fa1_eq_spec_revision() == 1U, "spec revision 1");

    nx_fa1_eq_config_t config{};
    expect(nx_fa1_eq_config_init(&config) == NX_SUCCESS, "config init");
    expect(config.opamp_model == NX_OPAMP_HA1457, "HA1457 default");
    expect(config.bass.pot_params.taper == NX_POT_TAPER_LINEAR, "Bass B");
    expect(config.treble.pot_params.taper == NX_POT_TAPER_LINEAR, "Treble B");
    expect(std::fabs(config.bass.pot_params.control - 0.5) < 1e-12, "Bass default 0.5");
    expect(std::fabs(config.treble.pot_params.control - 0.5) < 1e-12, "Treble default 0.5");
    expect(std::fabs(config.bass.pot_params.value - 50000.0) < 1e-3, "Bass 50k");
    expect(std::fabs(config.treble.pot_params.value - 50000.0) < 1e-3, "Treble 50k");
    expect(std::fabs(config.RS - 100.0) < 1e-9, "RS 100");
    expect(std::fabs(config.R1 - 8200.0) < 1e-6, "R1 8.2k");
    expect(std::fabs(config.R2 - 8200.0) < 1e-6, "R2 8.2k");
    expect(std::fabs(config.R3 - 33000.0) < 1e-6, "R3 33k");
    expect(std::fabs(config.R4 - 10000.0) < 1e-6, "R4 10k");
    expect(std::fabs(config.C1 - 3.3e-8) < 1e-15, "C1 33n");
    expect(std::fabs(config.C2 - 3.3e-8) < 1e-15, "C2 33n");
    expect(std::fabs(config.C3 - 5.6e-9) < 1e-16, "C3 5.6n");
    expect(std::fabs(config.C4 - 5.6e-9) < 1e-16, "C4 5.6n");
    expect(std::fabs(config.CF - 1e-10) < 1e-18, "CF 100p");

    const double fs = 96000.0;
    nx_fa1_eq_f32_t* eq = nx_fa1_eq_create_f32(nullptr);
    expect(eq != nullptr, "create");
    nx_fa1_eq_prepare_f32(eq, fs);
    expect(nx_fa1_eq_set_opamp_model_f32(eq, NX_OPAMP_HA1457) == NX_SUCCESS, "set HA1457");
    expect(nx_fa1_eq_set_bass_control_f32(eq, 0.5) == NX_SUCCESS, "set bass 0.5");
    expect(nx_fa1_eq_set_treble_control_f32(eq, 0.5) == NX_SUCCESS, "set treble 0.5");
    nx_fa1_eq_tick_f32(eq, 64);
    nx_fa1_eq_reset_f32(eq);

    nx_fa1_eq_config_t via_config{};
    expect(nx_fa1_eq_get_config_f32(eq, &via_config) == NX_SUCCESS, "get config");
    via_config.bass.pot_params.control = 0.0;
    via_config.treble.pot_params.control = 0.5;
    expect(nx_fa1_eq_set_config_f32(eq, &via_config) == NX_SUCCESS, "set_config bass 0");
    double mag_low = 0.0;
    double phase_scratch = 0.0;
    const double f100 = 100.0;
    nx_fa1_eq_ac_static_f32(&via_config, fs, &f100, &mag_low, &phase_scratch, 1);
    via_config.bass.pot_params.control = 1.0;
    expect(nx_fa1_eq_set_config_f32(eq, &via_config) == NX_SUCCESS, "set_config bass 1");
    double mag_high = 0.0;
    nx_fa1_eq_ac_static_f32(&via_config, fs, &f100, &mag_high, &phase_scratch, 1);
    expect(mag_high - mag_low > 15.0, "bass 0 vs 1 moves 100 Hz AC by >15 dB");

    via_config.bass.pot_params.control = 0.5;
    via_config.treble.pot_params.control = 0.0;
    expect(nx_fa1_eq_set_config_f32(eq, &via_config) == NX_SUCCESS, "set_config treble 0");
    const double f8k = 8000.0;
    double mag_t0 = 0.0;
    nx_fa1_eq_ac_static_f32(&via_config, fs, &f8k, &mag_t0, &phase_scratch, 1);
    via_config.treble.pot_params.control = 1.0;
    expect(nx_fa1_eq_set_config_f32(eq, &via_config) == NX_SUCCESS, "set_config treble 1");
    double mag_t1 = 0.0;
    nx_fa1_eq_ac_static_f32(&via_config, fs, &f8k, &mag_t1, &phase_scratch, 1);
    expect(mag_t1 - mag_t0 > 25.0, "treble 0 vs 1 moves 8 kHz AC by >25 dB");

    expect(nx_fa1_eq_set_bass_control_f32(eq, 0.5) == NX_SUCCESS, "restore bass 0.5");
    expect(nx_fa1_eq_set_treble_control_f32(eq, 0.5) == NX_SUCCESS, "restore treble 0.5");
    nx_fa1_eq_tick_f32(eq, 64);
    nx_fa1_eq_reset_f32(eq);

    constexpr int n = 4096;
    float in[n];
    float out[n];
    const double freq = 400.0;
    const double amp = 1.0e-3;
    for (int i = 0; i < n; ++i)
        in[i] = static_cast<float>(amp * std::sin(2.0 * M_PI * freq * static_cast<double>(i) / fs));

    nx_fa1_eq_process_f32(eq, in, out, static_cast<size_t>(n));

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

    config.bass.pot_params.control = 0.5;
    config.treble.pot_params.control = 0.5;
    assertAcMatches(config, kPythonMagDbFlat, "flat");
    config.bass.pot_params.control = 0.0;
    assertAcMatches(config, kPythonMagDbBass0, "bass 0");
    config.bass.pot_params.control = 1.0;
    assertAcMatches(config, kPythonMagDbBass1, "bass 1");
    config.bass.pot_params.control = 0.5;
    config.treble.pot_params.control = 0.0;
    assertAcMatches(config, kPythonMagDbTreble0, "treble 0");
    config.treble.pot_params.control = 1.0;
    assertAcMatches(config, kPythonMagDbTreble1, "treble 1");

    config.bass.pot_params.control = 0.5;
    config.treble.pot_params.control = 0.5;
    const double freq400 = 400.0;
    double mag400 = 0.0;
    double phase400 = 0.0;
    nx_fa1_eq_ac_static_f32(&config, fs, &freq400, &mag400, &phase400, 1);
    const double ac_err = std::fabs(process_db - mag400);
    std::printf("fa1_eq process@400 %.2f dB  ac_static %.2f dB  |err| %.3f dB  python max err < 0.05 dB\n",
                process_db, mag400, ac_err);
    expect(ac_err < 0.25, "400 Hz process matches ac_static within 0.25 dB");

    const double f20k = 20000.0;
    double mag_cf_100p = 0.0;
    double mag_cf_1n = 0.0;
    config.CF = 1e-10;
    nx_fa1_eq_ac_static_f32(&config, fs, &f20k, &mag_cf_100p, &phase400, 1);
    config.CF = 1e-9;
    nx_fa1_eq_ac_static_f32(&config, fs, &f20k, &mag_cf_1n, &phase400, 1);
    expect(std::fabs(mag_cf_1n - mag_cf_100p) > 0.2, "external CF moves 20 kHz AC");

    expect(nx_fa1_eq_destroy_f32(eq, nullptr) == NX_SUCCESS, "destroy");
    std::printf("fa1_eq operator test ok\n");
    return 0;
}
