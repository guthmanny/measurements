// Operator-menu coverage for fa1_buffer: defaults, process, ac_static vs Python reference.

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "nudsp/common/operator_spec_metadata.h"
#include "nudsp/linear_circuits/fa1_buffer_f32.h"

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
static constexpr double kPythonMagDbLowCut[kAcPoints] = {
    -22.115354, -13.560633, -6.310439, -2.394905, -1.408773, -1.252181, -1.229970, -1.226880,
};
static constexpr double kPythonMagDbFlat[kAcPoints] = {
    -4.945283, -1.944990, -1.333066, -1.241464, -1.228595, -1.226804, -1.226555, -1.226520,
};

static void assertAcMatches(const nx_fa1_buffer_config_t& config, const double* python_db, const char* label)
{
    double mag_db[kAcPoints] = {};
    double phase[kAcPoints] = {};
    nx_fa1_buffer_ac_static_f32(&config, 96000.0, kAcFreqs, mag_db, phase, kAcPoints);
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
    const char* hash = nx_fa1_buffer_spec_hash_hex();
    expect(hash != nullptr && std::strlen(hash) == 64U, "spec hash length");
    expect(std::strcmp(hash, nx_operator_spec_hash_hex("fa1_buffer")) == 0, "spec hash lookup");
    expect(nx_fa1_buffer_spec_revision() == 2U, "spec revision 2");

    nx_fa1_buffer_config_t config{};
    expect(nx_fa1_buffer_config_init(&config) == NX_SUCCESS, "config init");
    expect(config.sw1.position == 1, "SW1 default flat");
    expect(config.jfet == NX_JFET_J201, "J201 default");
    expect(std::fabs(config.R1 - 10000.0) < 1e-6, "R1 10k");
    expect(std::fabs(config.C1 - 1e-8) < 1e-18, "C1 10n");
    expect(std::fabs(config.R2 - 3.3e6) < 1e-3, "R2 3.3M");
    expect(std::fabs(config.R4 - 15000.0) < 1e-6, "R4 15k");
    expect(std::fabs(config.vg_bias - 3.0) < 1e-12, "vg_bias 3");

    const double fs = 96000.0;
    nx_fa1_buffer_f32_t* buffer = nx_fa1_buffer_create_f32(nullptr);
    expect(buffer != nullptr, "create");
    nx_fa1_buffer_prepare_f32(buffer, fs);
    expect(nx_fa1_buffer_set_jfet_model_f32(buffer, NX_JFET_J201) == NX_SUCCESS, "set J201");
    expect(nx_fa1_buffer_set_sw1_switch_f32(buffer, 1) == NX_SUCCESS, "set flat");
    nx_fa1_buffer_tick_f32(buffer, 64);
    nx_fa1_buffer_reset_f32(buffer);

    nx_fa1_buffer_config_t via_config{};
    expect(nx_fa1_buffer_get_config_f32(buffer, &via_config) == NX_SUCCESS, "get config");
    via_config.sw1.position = 0;
    expect(nx_fa1_buffer_set_config_f32(buffer, &via_config) == NX_SUCCESS, "set_config low_cut");
    expect(nx_fa1_buffer_get_config_f32(buffer, &via_config) == NX_SUCCESS, "get after set_config");
    expect(via_config.sw1.position == 0, "SW1 low_cut via set_config");
    double mag_low_cut = 0.0;
    double phase_scratch = 0.0;
    const double f400 = 400.0;
    nx_fa1_buffer_ac_static_f32(&via_config, fs, &f400, &mag_low_cut, &phase_scratch, 1);
    via_config.sw1.position = 1;
    expect(nx_fa1_buffer_set_config_f32(buffer, &via_config) == NX_SUCCESS, "set_config flat");
    double mag_flat = 0.0;
    nx_fa1_buffer_ac_static_f32(&via_config, fs, &f400, &mag_flat, &phase_scratch, 1);
    expect(std::fabs(mag_flat - mag_low_cut) > 0.8, "SW1 toggles 400 Hz AC by >0.8 dB");

    constexpr int n = 4096;
    float in[n];
    float out[n];
    const double freq = 400.0;
    const double amp = 1.0e-3;
    for (int i = 0; i < n; ++i)
        in[i] = static_cast<float>(amp * std::sin(2.0 * M_PI * freq * static_cast<double>(i) / fs));

    nx_fa1_buffer_process_f32(buffer, in, out, static_cast<size_t>(n));

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

    config.sw1.position = 0;
    assertAcMatches(config, kPythonMagDbLowCut, "low_cut");
    config.sw1.position = 1;
    assertAcMatches(config, kPythonMagDbFlat, "flat");

    const double freq400 = 400.0;
    double mag400 = 0.0;
    double phase400 = 0.0;
    nx_fa1_buffer_ac_static_f32(&config, fs, &freq400, &mag400, &phase400, 1);
    const double ac_err = std::fabs(process_db - mag400);
    std::printf("fa1_buffer process@400 %.2f dB  ac_static %.2f dB  |err| %.3f dB  python max err < 0.05 dB\n",
                process_db, mag400, ac_err);
    expect(ac_err < 0.25, "400 Hz process matches ac_static within 0.25 dB");

    expect(nx_fa1_buffer_destroy_f32(buffer, nullptr) == NX_SUCCESS, "destroy");
    std::printf("fa1_buffer operator test ok\n");
    return 0;
}
