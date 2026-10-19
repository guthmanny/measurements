// Operator-menu coverage for beod_clipper: draw.io defaults, ac_static vs Python, ac_f32 parity.

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "nudsp/common/operator_spec_metadata.h"
#include "nudsp/nonlinear_circuits/beod_clipper_f32.h"

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

// BeodClipperConfig @ 96 kHz, presence/treble=0.5, DLED, 2.2k / 10n / 4.7n (draw.io).
static constexpr double kPythonMagDb[kAcPoints] = {
    -0.001039, -0.002147, -0.004435, -0.009140, -0.018761, -0.038186, -0.076452, -0.148378,
    -0.272961, -0.464109, -0.721481, -1.049217, -1.501530, -2.205045, -3.328218, -5.001938,
    -7.244551, -9.982961, -13.166760, -16.919778,
};

int main()
{
    const char* hash = nx_beod_clipper_spec_hash_hex();
    expect(hash != nullptr && std::strlen(hash) == 64U, "spec hash length");
    expect(std::strcmp(hash, nx_operator_spec_hash_hex("beod_clipper")) == 0, "spec hash lookup");
    expect(nx_beod_clipper_spec_revision() == 3U, "spec revision 3");
    expect(std::strcmp(hash, "76769616e4f0aaad52f3de2ce44e7a51bd3b1f4bb4d07646cfe3faf7a9f1981c") == 0,
           "clipper spec hash (draw.io aligned)");

    nx_beod_clipper_config_t config{};
    expect(nx_beod_clipper_config_init(&config) == NX_SUCCESS, "config init");
    expect(std::fabs(config.R1 - 2200.0) < 1e-6, "R1 2.2k");
    expect(std::fabs(config.C1 - 1e-8) < 1e-18, "C1 10n");
    expect(std::fabs(config.C2 - 4.7e-9) < 1e-18, "C2 4.7n");
    expect(std::fabs(config.presence.pot_params.value - 10000.0) < 1e-3, "presence 10k");
    expect(std::fabs(config.treble.pot_params.value - 100000.0) < 1e-3, "treble 100k");
    expect(config.presence.pot_params.taper == NX_POT_TAPER_C, "presence taper C");
    expect(config.treble.pot_params.taper == NX_POT_TAPER_LINEAR, "treble taper B (linear)");
    expect(config.diode_model == NX_DIODE_DLED, "default diode LED");

    config.presence.pot_params.control = 0.5;
    config.treble.pot_params.control = 0.5;
    const double fs = 96000.0;

    double mag_db[kAcPoints] = {};
    double phase[kAcPoints] = {};
    nx_beod_clipper_ac_static_f32(&config, fs, kAcFreqs, mag_db, phase, kAcPoints);

    double max_ac_py_err = 0.0;
    for (int i = 0; i < kAcPoints; ++i)
    {
        expect(std::isfinite(mag_db[i]), "ac magnitude finite");
        expect(std::isfinite(phase[i]), "ac phase finite");
        const double err = std::fabs(mag_db[i] - kPythonMagDb[i]);
        if (err > max_ac_py_err)
            max_ac_py_err = err;
        if (err > 0.03)
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

    nx_beod_clipper_f32_t* clipper = nx_beod_clipper_create_f32(nullptr);
    expect(clipper != nullptr, "create");
    nx_beod_clipper_set_diode_model_f32(clipper, NX_DIODE_DLED);
    nx_beod_clipper_set_presence_control_f32(clipper, 0.5);
    nx_beod_clipper_set_treble_control_f32(clipper, 0.5);
    nx_beod_clipper_prepare_f32(clipper, fs);
    nx_beod_clipper_tick_f32(clipper, 64);
    nx_beod_clipper_reset_f32(clipper);

    double mag_ac_f32[kAcPoints] = {};
    double phase_ac_f32[kAcPoints] = {};
    nx_beod_clipper_ac_f32(clipper, kAcFreqs, mag_ac_f32, phase_ac_f32, kAcPoints);
    expect(std::fabs(mag_ac_f32[idx_1k] - mag_db[idx_1k]) < 0.05, "ac_f32 matches ac_static at ~1 kHz");

    std::printf("beod_clipper ac_static vs Python: max |err| %.4f dB (< 0.03 dB)\n", max_ac_py_err);
    std::printf("beod_clipper ac_static@~1kHz %.3f dB  ac_f32 %.3f dB\n", mag_db[idx_1k], mag_ac_f32[idx_1k]);

    expect(nx_beod_clipper_destroy_f32(clipper, nullptr) == NX_SUCCESS, "destroy");
    std::printf("beod_clipper operator test ok\n");
    return 0;
}
