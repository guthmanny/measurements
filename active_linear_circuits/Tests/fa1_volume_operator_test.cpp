// Operator-menu coverage for fa1_volume: defaults, process, ac_static vs Python reference.

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "nudsp/common/operator_spec_metadata.h"
#include "nudsp/linear_circuits/fa1_volume_f32.h"

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
static constexpr double kPythonMagDbVol01[kAcPoints] = {
    -3.677492, -3.701078, -3.704369, -3.704858, -3.705150, -3.706812, -3.719168, -3.838655,
};
static constexpr double kPythonMagDbVol05[kAcPoints] = {
    20.731140, 20.731075, 20.730844, 20.729217, 20.717517, 20.633784, 20.053485, 16.573899,
};
static constexpr double kPythonMagDbVol10[kAcPoints] = {
    26.571589, 26.571477, 26.570680, 26.564947, 26.523880, 26.237753, 24.542976, 18.338693,
};

static void assertAcMatches(const nx_fa1_volume_config_t& config, const double* python_db, const char* label)
{
    double mag_db[kAcPoints] = {};
    double phase[kAcPoints] = {};
    nx_fa1_volume_ac_static_f32(&config, 96000.0, kAcFreqs, mag_db, phase, kAcPoints);
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
    const char* hash = nx_fa1_volume_spec_hash_hex();
    expect(hash != nullptr && std::strlen(hash) == 64U, "spec hash length");
    expect(std::strcmp(hash, nx_operator_spec_hash_hex("fa1_volume")) == 0, "spec hash lookup");
    expect(nx_fa1_volume_spec_revision() == 1U, "spec revision 1");

    nx_fa1_volume_config_t config{};
    expect(nx_fa1_volume_config_init(&config) == NX_SUCCESS, "config init");
    expect(config.opamp_model == NX_OPAMP_HA1457, "HA1457 default");
    expect(config.volume.pot_params.taper == NX_POT_TAPER_A30, "Volume A30");
    expect(std::fabs(config.volume.pot_params.control - 0.5) < 1e-12, "Volume default 0.5");
    expect(std::fabs(config.volume.pot_params.value - 1e6) < 1e-3, "Volume 1M");
    expect(std::fabs(config.R1 - 15000.0) < 1e-6, "R1 15k");
    expect(std::fabs(config.R2 - 470000.0) < 1e-3, "R2 470k");
    expect(std::fabs(config.C1 - 1e-5) < 1e-12, "C1 10u");
    expect(std::fabs(config.CF - 1e-10) < 1e-18, "CF 100p");

    const double fs = 96000.0;
    nx_fa1_volume_f32_t* volume = nx_fa1_volume_create_f32(nullptr);
    expect(volume != nullptr, "create");
    nx_fa1_volume_prepare_f32(volume, fs);
    expect(nx_fa1_volume_set_opamp_model_f32(volume, NX_OPAMP_HA1457) == NX_SUCCESS, "set HA1457");
    expect(nx_fa1_volume_set_volume_control_f32(volume, 0.5) == NX_SUCCESS, "set volume 0.5");
    nx_fa1_volume_tick_f32(volume, 64);
    nx_fa1_volume_reset_f32(volume);

    nx_fa1_volume_config_t via_config{};
    expect(nx_fa1_volume_get_config_f32(volume, &via_config) == NX_SUCCESS, "get config");
    via_config.volume.pot_params.control = 0.1;
    expect(nx_fa1_volume_set_config_f32(volume, &via_config) == NX_SUCCESS, "set_config vol 0.1");
    double mag_low = 0.0;
    double phase_scratch = 0.0;
    const double f400 = 400.0;
    nx_fa1_volume_ac_static_f32(&via_config, fs, &f400, &mag_low, &phase_scratch, 1);
    via_config.volume.pot_params.control = 1.0;
    expect(nx_fa1_volume_set_config_f32(volume, &via_config) == NX_SUCCESS, "set_config vol 1");
    double mag_high = 0.0;
    nx_fa1_volume_ac_static_f32(&via_config, fs, &f400, &mag_high, &phase_scratch, 1);
    expect(std::fabs(mag_high - mag_low) > 20.0, "volume 0.1 vs 1.0 moves 400 Hz AC by >20 dB");

    expect(nx_fa1_volume_set_volume_control_f32(volume, 0.5) == NX_SUCCESS, "restore volume 0.5");
    nx_fa1_volume_tick_f32(volume, 64);
    nx_fa1_volume_reset_f32(volume);

    constexpr int n = 4096;
    float in[n];
    float out[n];
    const double freq = 400.0;
    const double amp = 1.0e-3;
    for (int i = 0; i < n; ++i)
        in[i] = static_cast<float>(amp * std::sin(2.0 * M_PI * freq * static_cast<double>(i) / fs));

    nx_fa1_volume_process_f32(volume, in, out, static_cast<size_t>(n));

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

    config.volume.pot_params.control = 0.1;
    assertAcMatches(config, kPythonMagDbVol01, "volume 0.1");
    config.volume.pot_params.control = 0.5;
    assertAcMatches(config, kPythonMagDbVol05, "volume 0.5");
    config.volume.pot_params.control = 1.0;
    assertAcMatches(config, kPythonMagDbVol10, "volume 1.0");

    config.volume.pot_params.control = 0.5;
    const double freq400 = 400.0;
    double mag400 = 0.0;
    double phase400 = 0.0;
    nx_fa1_volume_ac_static_f32(&config, fs, &freq400, &mag400, &phase400, 1);
    const double ac_err = std::fabs(process_db - mag400);
    std::printf("fa1_volume process@400 %.2f dB  ac_static %.2f dB  |err| %.3f dB  python max err < 0.05 dB\n",
                process_db, mag400, ac_err);
    expect(ac_err < 0.25, "400 Hz process matches ac_static within 0.25 dB");

    config.volume.pot_params.control = 0.5;
    const double f20k = 20000.0;
    double mag_cf_100p = 0.0;
    double mag_cf_1n = 0.0;
    config.CF = 1e-10;
    nx_fa1_volume_ac_static_f32(&config, fs, &f20k, &mag_cf_100p, &phase400, 1);
    config.CF = 1e-9;
    nx_fa1_volume_ac_static_f32(&config, fs, &f20k, &mag_cf_1n, &phase400, 1);
    expect(std::fabs(mag_cf_1n - mag_cf_100p) > 1.0, "external CF moves 20 kHz AC");

    expect(nx_fa1_volume_destroy_f32(volume, nullptr) == NX_SUCCESS, "destroy");
    std::printf("fa1_volume operator test ok\n");
    return 0;
}
