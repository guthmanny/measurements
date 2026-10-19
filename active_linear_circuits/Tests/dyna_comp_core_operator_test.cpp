// Operator-menu coverage for dyna_comp_core (C API nx_ota_comp):
// 500kC Sensitivity, defaults, ac_static, ac_f32, small-signal process.

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "nudsp/common/operator_spec_metadata.h"
#include "nudsp/nonlinear_circuits/ota_comp.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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

// nx_ota_comp_ac_static_f32 @ 96 kHz, Sensitivity=0.5, CA3080 / 1N914 / 2N3904.
// LUT-slope residual (idle + delayed sidechain), not the analytical Iabc/(2 Vt) gm.
static constexpr double kAcMagDbMid[kAcPoints] = {
    9.166328, 9.232236, 6.366326, 0.124697, -6.040469, -9.137118, -11.608506, -18.192230,
};

static double dft_bin_mag_db(const float* x, size_t n, size_t bin)
{
    double re = 0.0;
    double im = 0.0;
    const double w = 2.0 * M_PI * static_cast<double>(bin) / static_cast<double>(n);
    for (size_t i = 0; i < n; ++i)
    {
        const double th = w * static_cast<double>(i);
        re += static_cast<double>(x[i]) * std::cos(th);
        im += static_cast<double>(x[i]) * -std::sin(th);
    }
    const double mag = 2.0 * std::sqrt(re * re + im * im) / static_cast<double>(n);
    return 20.0 * std::log10(mag + 1e-20);
}

int main()
{
    const char* hash = nx_ota_comp_spec_hash_hex();
    expect(hash != nullptr && std::strlen(hash) == 64U, "spec hash length");
    expect(std::strcmp(hash, nx_operator_spec_hash_hex("ota_comp")) == 0, "spec hash lookup");
    expect(nx_ota_comp_spec_revision() == 2U, "spec revision 2");
    expect(std::strcmp(hash, "dfe878fd00e997683e9db7d3571f8914b344cf9fb044af5e6c3861ddd299503e") == 0,
           "ota_comp spec hash");

    nx_ota_comp_config_t config{};
    expect(nx_ota_comp_config_init(&config) == NX_SUCCESS, "config init");
    expect(config.ota_model == NX_OTA_CA3080, "CA3080 default");
    expect(config.diode_model == NX_DIODE_1N914, "1N914 default");
    expect(config.bjt == NX_BJT_2N3904, "2N3904 default");
    expect(config.sensitivity.pot_params.taper == NX_POT_TAPER_C30, "Sensitivity 500kC");
    expect(std::fabs(config.sensitivity.pot_params.value - 500000.0) < 1e-3, "Sensitivity 500k");
    expect(std::fabs(config.sensitivity.pot_params.control - 0.5) < 1e-12, "Sensitivity default 0.5");
    expect(std::fabs(config.C1 - 10e-9) < 1e-18, "C1 10n");
    expect(std::fabs(config.C2 - 1e-6) < 1e-15, "C2 1u");
    expect(std::fabs(config.C3 - 10e-9) < 1e-18, "C3 10n");
    expect(std::fabs(config.C4 - 1e-6) < 1e-15, "C4 1u");
    expect(std::fabs(config.R1 - 1e6) < 1e-3, "R1 1M");
    expect(std::fabs(config.R2 - 1e3) < 1e-6, "R2 1k");
    expect(std::fabs(config.R3 - 1e3) < 1e-6, "R3 1k");
    expect(std::fabs(config.R4 - 470e3) < 1e-3, "R4 470k");
    expect(std::fabs(config.R5 - 1e6) < 1e-3, "R5 1M");
    expect(std::fabs(config.R6 - 15e3) < 1e-6, "R6 15k");
    expect(std::fabs(config.Rota - 150e3) < 1e-3, "Rota 150k");
    expect(std::fabs(config.Cota - 10e-9) < 1e-18, "Cota 10n");
    expect(std::fabs(config.Rc - 10e3) < 1e-6, "Rc 10k");
    expect(std::fabs(config.Re - 10e3) < 1e-6, "Re 10k");
    expect(std::fabs(config.R15 - 27000.0) < 1e-6, "R15 27k");
    expect(std::fabs(config.vcc - 9.0) < 1e-12, "vcc 9");
    expect(std::fabs(config.vbias - 2.54) < 1e-12, "vbias 2.54");

    const double fs = 96000.0;
    double mag_db[kAcPoints] = {};
    double phase[kAcPoints] = {};
    nx_ota_comp_ac_static_f32(&config, fs, kAcFreqs, mag_db, phase, kAcPoints);

    double max_ac_err = 0.0;
    for (int i = 0; i < kAcPoints; ++i)
    {
        expect(std::isfinite(mag_db[i]), "ac magnitude finite");
        expect(std::isfinite(phase[i]), "ac phase finite");
        const double err = std::fabs(mag_db[i] - kAcMagDbMid[i]);
        if (err > max_ac_err)
            max_ac_err = err;
        if (err > 0.05)
        {
            std::fprintf(stderr, "FAIL: ac_static gold f=%.1f Hz  c=%.4f  gold=%.4f  err=%.4f dB\n", kAcFreqs[i],
                         mag_db[i], kAcMagDbMid[i], err);
            std::exit(1);
        }
    }

    config.sensitivity.pot_params.control = 0.0;
    double mag_open = 0.0;
    double phase_scratch = 0.0;
    const double f1k = 1000.0;
    nx_ota_comp_ac_static_f32(&config, fs, &f1k, &mag_open, &phase_scratch, 1);
    config.sensitivity.pot_params.control = 1.0;
    double mag_tight = 0.0;
    nx_ota_comp_ac_static_f32(&config, fs, &f1k, &mag_tight, &phase_scratch, 1);
    expect(std::fabs(mag_open - mag_tight) > 0.05, "Sensitivity 500kC moves 1 kHz AC");
    config.sensitivity.pot_params.control = 0.5;

    nx_ota_comp_f32_t* core = nx_ota_comp_create_f32(nullptr);
    expect(core != nullptr, "create");
    nx_ota_comp_prepare_f32(core, fs);
    expect(nx_ota_comp_set_diode_model_f32(core, NX_DIODE_1N914) == NX_SUCCESS, "set 1N914");
    expect(nx_ota_comp_set_bjt_model_f32(core, NX_BJT_2N3904) == NX_SUCCESS, "set 2N3904");
    expect(nx_ota_comp_set_ota_model_f32(core, NX_OTA_CA3080) == NX_SUCCESS, "set CA3080");
    expect(nx_ota_comp_set_sensitivity_control_f32(core, 0.5) == NX_SUCCESS, "set sensitivity 0.5");
    nx_ota_comp_tick_f32(core, 64);
    nx_ota_comp_reset_f32(core);

    double mag_ac_f32[kAcPoints] = {};
    double phase_ac_f32[kAcPoints] = {};
    nx_ota_comp_ac_f32(core, kAcFreqs, mag_ac_f32, phase_ac_f32, kAcPoints);
    expect(std::fabs(mag_ac_f32[4] - mag_db[4]) < 0.05, "ac_f32 matches ac_static at ~1 kHz");

    constexpr int kSettle = 262144;
    constexpr int kN = 4096;
    const size_t total = static_cast<size_t>(kSettle + kN);
    std::vector<float> in(total);
    std::vector<float> out(total);
    const double amp = 1.0e-4;
    const size_t bin = static_cast<size_t>(1000.0 * static_cast<double>(kN) / fs + 0.5);
    const double freq = static_cast<double>(bin) * fs / static_cast<double>(kN);
    for (size_t i = 0; i < total; ++i)
        in[i] = static_cast<float>(amp * std::sin(2.0 * M_PI * freq * static_cast<double>(i) / fs));

    nx_ota_comp_reset_f32(core);
    nx_ota_comp_tick_f32(core, total);
    nx_ota_comp_process_f32(core, in.data(), out.data(), total);

    double mean = 0.0;
    double peak = 0.0;
    for (int i = 0; i < kN; ++i)
    {
        expect(std::isfinite(out[static_cast<size_t>(kSettle + i)]), "process finite");
        mean += static_cast<double>(out[static_cast<size_t>(kSettle + i)]);
        const double a = std::fabs(out[static_cast<size_t>(kSettle + i)]);
        if (a > peak)
            peak = a;
    }
    mean /= static_cast<double>(kN);
    expect(peak > 0.0, "process has energy");

    std::vector<float> ac(static_cast<size_t>(kN));
    for (int i = 0; i < kN; ++i)
        ac[static_cast<size_t>(i)] = static_cast<float>(static_cast<double>(out[static_cast<size_t>(kSettle + i)]) - mean);

    const double proc_db = dft_bin_mag_db(ac.data(), static_cast<size_t>(kN), bin) - 20.0 * std::log10(amp);
    double ac_1k = 0.0;
    nx_ota_comp_ac_static_f32(&config, fs, &freq, &ac_1k, &phase_scratch, 1);
    const double ac_err_f32 = std::fabs(proc_db - ac_1k);
    if (ac_err_f32 >= 0.1)
    {
        std::fprintf(stderr, "FAIL: 1 kHz f32 process %.4f dB  ac_static %.4f dB  |err| %.4f dB\n", proc_db, ac_1k,
                     ac_err_f32);
        std::exit(1);
    }

    nx_ota_comp_t* core64 = nx_ota_comp_create(nullptr);
    expect(core64 != nullptr, "create f64");
    nx_ota_comp_prepare(core64, fs);
    nx_ota_comp_reset(core64);
    nx_ota_comp_tick(core64, total);
    std::vector<double> in64(total);
    std::vector<double> out64(total);
    for (size_t i = 0; i < total; ++i)
        in64[i] = amp * std::sin(2.0 * M_PI * freq * static_cast<double>(i) / fs);
    nx_ota_comp_process(core64, in64.data(), out64.data(), total);
    double mean64 = 0.0;
    for (int i = 0; i < kN; ++i)
        mean64 += out64[static_cast<size_t>(kSettle + i)];
    mean64 /= static_cast<double>(kN);
    std::vector<double> ac64(static_cast<size_t>(kN));
    for (int i = 0; i < kN; ++i)
        ac64[static_cast<size_t>(i)] = out64[static_cast<size_t>(kSettle + i)] - mean64;
    double re = 0.0;
    double im = 0.0;
    const double w = 2.0 * M_PI * static_cast<double>(bin) / static_cast<double>(kN);
    for (int i = 0; i < kN; ++i)
    {
        const double th = w * static_cast<double>(i);
        re += ac64[static_cast<size_t>(i)] * std::cos(th);
        im += ac64[static_cast<size_t>(i)] * -std::sin(th);
    }
    const double proc64_db =
        20.0 * std::log10(2.0 * std::sqrt(re * re + im * im) / static_cast<double>(kN) + 1e-20) -
        20.0 * std::log10(amp);
    double ac64_1k = 0.0;
    nx_ota_comp_ac_static(&config, fs, &freq, &ac64_1k, &phase_scratch, 1);
    const double ac_err = std::fabs(proc64_db - ac64_1k);
    if (ac_err >= 0.1)
    {
        std::fprintf(stderr, "FAIL: 1 kHz f64 process %.4f dB  ac_static %.4f dB  |err| %.4f dB\n", proc64_db,
                     ac64_1k, ac_err);
        std::exit(1);
    }
    expect(nx_ota_comp_destroy(core64, nullptr) == NX_SUCCESS, "destroy f64");

    const double drive = 0.05;
    for (size_t i = 0; i < total; ++i)
        in[i] = static_cast<float>(drive * std::sin(2.0 * M_PI * freq * static_cast<double>(i) / fs));

    auto ac_peak_after = [&](double sensitivity) -> double {
        expect(nx_ota_comp_set_sensitivity_control_f32(core, sensitivity) == NX_SUCCESS, "set sensitivity");
        nx_ota_comp_tick_f32(core, 64);
        nx_ota_comp_reset_f32(core);
        nx_ota_comp_process_f32(core, in.data(), out.data(), total);
        double m = 0.0;
        for (int i = 0; i < kN; ++i)
        {
            expect(std::isfinite(out[static_cast<size_t>(kSettle + i)]), "process finite");
            m += static_cast<double>(out[static_cast<size_t>(kSettle + i)]);
        }
        m /= static_cast<double>(kN);
        double pk = 0.0;
        for (int i = 0; i < kN; ++i)
        {
            const double a = std::fabs(static_cast<double>(out[static_cast<size_t>(kSettle + i)]) - m);
            if (a > pk)
                pk = a;
        }
        return pk;
    };

    const double peak_mid_ac = ac_peak_after(0.5);
    const double peak_open = ac_peak_after(0.0);
    const double peak_tight = ac_peak_after(1.0);
    expect(std::fabs(peak_open - peak_tight) > 1e-4, "Sensitivity 500kC moves process AC peak");

    expect(nx_ota_comp_destroy_f32(core, nullptr) == NX_SUCCESS, "destroy");
    std::printf("dyna_comp_core ac_static vs gold: max |err| %.4f dB (< 0.05 dB)\n", max_ac_err);
    std::printf("dyna_comp_core f32 process@1k %.2f dB  ac_static %.2f dB  |err| %.3f dB\n", proc_db, ac_1k,
                ac_err_f32);
    std::printf("dyna_comp_core f64 process@1k %.2f dB  ac_static %.2f dB  |err| %.3f dB\n", proc64_db, ac64_1k,
                ac_err);
    std::printf("dyna_comp_core operator test ok  ac-peak@0.5=%.4f  open=%.4f  tight=%.4f\n", peak_mid_ac, peak_open,
                peak_tight);
    return 0;
}
