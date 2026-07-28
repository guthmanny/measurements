#include "Ds1OpampAcMath.h"

#include <cmath>
#include <limits>

namespace ds1_ac
{

void AcSweepParams::sanitise() noexcept
{
    freqMinHz = juce::jmax(1.0, freqMinHz);
    freqMaxHz = juce::jmax(freqMinHz + 1.0, freqMaxHz);
    numPoints = juce::jlimit(32, 2048, numPoints);
    sampleRateHz = juce::jmax(1000.0, sampleRateHz);
}

const char* circuitDisplayName(CircuitKind circuit) noexcept
{
    switch (circuit)
    {
        case CircuitKind::Ds1Opamp:
            return "DS-1";
        case CircuitKind::RatOpamp:
            return "RAT";
        case CircuitKind::GuvnorPreamp:
            return "Guvnor Preamp";
        case CircuitKind::GuvnorPostamp:
            return "Guvnor Postamp";
        case CircuitKind::GuvnorOpamp:
            return "Guvnor OpAmp";
        case CircuitKind::Ts9Tone:
            return "TS-9 Tone";
        case CircuitKind::Ds1Tone:
            return "DS-1 Tone";
        case CircuitKind::DsPlusOpamp:
            return "DS+";
        case CircuitKind::KlonCentaurTone:
            return "Klon Centaur Tone";
        case CircuitKind::AcBoosterEq:
            return "AC Booster EQ";
        case CircuitKind::Ds1Clipper:
            return "DS-1 Clipper";
        case CircuitKind::DiodeClipper:
            return "Diode Clipper";
        case CircuitKind::RatClipper:
            return "RAT Clipper";
        case CircuitKind::Ts9Opamp:
            return "TS-9 OpAmp";
        case CircuitKind::AcBoosterDrive:
            return "AC Booster Drive";
        case CircuitKind::KlonCentaur:
            return "Klon Centaur";
        case CircuitKind::GuvnorClipper:
            return "Guvnor Clipper";
        case CircuitKind::BjtFollower:
            return "BJT Follower";
        case CircuitKind::BjtFollowerOut:
            return "BJT Follower Out";
        case CircuitKind::BjtCommonEmitter:
            return "BJT Common Emitter";
        case CircuitKind::JfetFollower:
            return "JFET Follower";
    }

    return "Unknown";
}

const char* controlParameterName(CircuitKind circuit) noexcept
{
    switch (circuit)
    {
        case CircuitKind::Ds1Opamp:
        case CircuitKind::GuvnorPreamp:
        case CircuitKind::GuvnorPostamp:
        case CircuitKind::GuvnorOpamp:
            return "Gain";
        case CircuitKind::RatOpamp:
        case CircuitKind::DsPlusOpamp:
            return "Distortion";
        case CircuitKind::Ts9Tone:
        case CircuitKind::Ds1Tone:
            return "Tone";
        case CircuitKind::KlonCentaurTone:
            return "Treble";
        case CircuitKind::AcBoosterEq:
            return "Bass";
        case CircuitKind::RatClipper:
            return "Filter";
        case CircuitKind::Ts9Opamp:
            return "Drive";
        case CircuitKind::AcBoosterDrive:
            return "Gain";
        case CircuitKind::KlonCentaur:
            return "Gain";
        case CircuitKind::GuvnorClipper:
            return "Bass";
        case CircuitKind::Ds1Clipper:
        case CircuitKind::DiodeClipper:
            return "Control";
    }

    return "Control";
}

const char* secondaryControlParameterName(CircuitKind circuit) noexcept
{
    switch (circuit)
    {
        case CircuitKind::AcBoosterEq:
            return "Treble";
        case CircuitKind::GuvnorClipper:
            return "Mid";
        default:
            return "Control";
    }
}

const char* tertiaryControlParameterName(CircuitKind circuit) noexcept
{
    switch (circuit)
    {
        case CircuitKind::GuvnorClipper:
            return "Treble";
        default:
            return "Control";
    }
}

const char* circuitProcessFunctionName(CircuitKind circuit) noexcept
{
    switch (circuit)
    {
        case CircuitKind::Ds1Opamp:
            return "nx_ds1_opamp_process_f32";
        case CircuitKind::RatOpamp:
            return "nx_rat_opamp_process_f32";
        case CircuitKind::GuvnorPreamp:
            return "nx_guvnor_preamp_process_f32";
        case CircuitKind::GuvnorPostamp:
            return "nx_guvnor_postamp_process_f32";
        case CircuitKind::GuvnorOpamp:
            return "nx_guvnor_opamp_process_f32";
        case CircuitKind::Ts9Tone:
            return "nx_ts9_tone_process_f32";
        case CircuitKind::Ds1Tone:
            return "nx_ds1_tone_process_f32";
        case CircuitKind::DsPlusOpamp:
            return "nx_ds_plus_opamp_process_f32";
        case CircuitKind::KlonCentaurTone:
            return "nx_klon_centaur_tone_process_f32";
        case CircuitKind::AcBoosterEq:
            return "nx_ac_booster_eq_process_f32";
        case CircuitKind::Ds1Clipper:
            return "nx_ds1_clipper_process_f32";
        case CircuitKind::DiodeClipper:
            return "nx_diode_clipper_process_f32";
        case CircuitKind::RatClipper:
            return "nx_rat_clipper_process_f32";
        case CircuitKind::Ts9Opamp:
            return "nx_ts9_opamp_process_f32";
        case CircuitKind::AcBoosterDrive:
            return "nx_ac_booster_drive_process_f32";
        case CircuitKind::KlonCentaur:
            return "nx_klon_centaur_process_f32";
        case CircuitKind::GuvnorClipper:
            return "nx_guvnor_clipper_process_f32";
        case CircuitKind::BjtFollower:
            return "nx_bjt_follower_process_f32";
        case CircuitKind::BjtFollowerOut:
            return "nx_bjt_follower_out_process_f32";
        case CircuitKind::BjtCommonEmitter:
            return "nx_bjt_common_emitter_process_f32";
        case CircuitKind::JfetFollower:
            return "nx_jfet_follower_process_f32";
    }

    return "nx_opamp_process_f32";
}

bool circuitUsesOpampModel(CircuitKind circuit) noexcept
{
    switch (circuit)
    {
        case CircuitKind::Ds1Tone:
        case CircuitKind::Ds1Clipper:
        case CircuitKind::DiodeClipper:
        case CircuitKind::RatClipper:
        case CircuitKind::KlonCentaur:
        case CircuitKind::GuvnorClipper:
            return false;
        case CircuitKind::BjtFollower:
        case CircuitKind::BjtFollowerOut:
        case CircuitKind::BjtCommonEmitter:
        case CircuitKind::JfetFollower:
            return false;
        default:
            return true;
    }
}

bool circuitUsesBjtModel(CircuitKind circuit) noexcept
{
    switch (circuit)
    {
        case CircuitKind::BjtFollower:
        case CircuitKind::BjtFollowerOut:
        case CircuitKind::BjtCommonEmitter:
            return true;
        default:
            return false;
    }
}

bool circuitUsesJfetModel(CircuitKind circuit) noexcept
{
    return circuit == CircuitKind::JfetFollower;
}

bool circuitHasPrimaryControl(CircuitKind circuit) noexcept
{
    switch (circuit)
    {
        case CircuitKind::Ds1Clipper:
        case CircuitKind::DiodeClipper:
        case CircuitKind::BjtFollower:
        case CircuitKind::BjtFollowerOut:
        case CircuitKind::BjtCommonEmitter:
        case CircuitKind::JfetFollower:
            return false;
        default:
            return true;
    }
}

bool circuitUsesPotTaper(CircuitKind circuit) noexcept
{
    switch (circuit)
    {
        case CircuitKind::Ds1Clipper:
        case CircuitKind::DiodeClipper:
        case CircuitKind::BjtFollower:
        case CircuitKind::BjtFollowerOut:
        case CircuitKind::BjtCommonEmitter:
        case CircuitKind::JfetFollower:
            return false;
        default:
            return true;
    }
}

bool circuitHasSecondaryControl(CircuitKind circuit) noexcept
{
    return circuit == CircuitKind::AcBoosterEq || circuit == CircuitKind::GuvnorClipper;
}

bool circuitHasTertiaryControl(CircuitKind circuit) noexcept
{
    return circuit == CircuitKind::GuvnorClipper;
}

nx_pot_taper_e defaultPotTaper(CircuitKind circuit) noexcept
{
    switch (circuit)
    {
        case CircuitKind::RatOpamp:
            return NX_POT_TAPER_A30;
        case CircuitKind::GuvnorPreamp:
        case CircuitKind::GuvnorPostamp:
        case CircuitKind::GuvnorOpamp:
        case CircuitKind::Ts9Tone:
        case CircuitKind::KlonCentaurTone:
        case CircuitKind::AcBoosterEq:
            return NX_POT_TAPER_G;
        case CircuitKind::Ds1Tone:
        case CircuitKind::DsPlusOpamp:
        case CircuitKind::Ds1Opamp:
        case CircuitKind::RatClipper:
        case CircuitKind::KlonCentaur:
        case CircuitKind::GuvnorClipper:
            return NX_POT_TAPER_LINEAR;
        case CircuitKind::Ts9Opamp:
        case CircuitKind::AcBoosterDrive:
            return NX_POT_TAPER_G;
        case CircuitKind::Ds1Clipper:
        case CircuitKind::DiodeClipper:
            return NX_POT_TAPER_LINEAR;
        default:
            return NX_POT_TAPER_LINEAR;
    }
}

const char* potTaperDisplayName(nx_pot_taper_e taper) noexcept
{
    switch (taper)
    {
        case NX_POT_TAPER_LINEAR:
            return "Linear";
        case NX_POT_TAPER_MULTIPLICATIVE:
            return "Multiplicative";
        case NX_POT_TAPER_A15:
            return "A15";
        case NX_POT_TAPER_A30:
            return "A30";
        case NX_POT_TAPER_A45:
            return "A45";
        case NX_POT_TAPER_G:
            return "G (4B)";
        case NX_POT_TAPER_C:
            return "C";
        case NX_POT_TAPER_3B:
            return "3B";
        default:
            return "Unknown";
    }
}

namespace
{
constexpr nx_pot_taper_e kSelectablePotTapers[] = {
    NX_POT_TAPER_LINEAR,
    NX_POT_TAPER_MULTIPLICATIVE,
    NX_POT_TAPER_A15,
    NX_POT_TAPER_A30,
    NX_POT_TAPER_A45,
    NX_POT_TAPER_G,
    NX_POT_TAPER_C,
    NX_POT_TAPER_3B,
};
}  // namespace

int potTaperComboId(nx_pot_taper_e taper) noexcept
{
    for (size_t i = 0; i < std::size(kSelectablePotTapers); ++i)
    {
        if (kSelectablePotTapers[i] == taper)
            return static_cast<int>(i + 1);
    }

    return potTaperComboId(defaultPotTaper(CircuitKind::Ds1Opamp));
}

nx_pot_taper_e potTaperFromComboId(int comboId) noexcept
{
    if (comboId >= 1 && comboId <= static_cast<int>(std::size(kSelectablePotTapers)))
        return kSelectablePotTapers[static_cast<size_t>(comboId - 1)];

    return NX_POT_TAPER_LINEAR;
}

const char* opampModelDisplayName(nx_opamp_model_e model) noexcept
{
    switch (model)
    {
        case NX_OPAMP_IDEAL:
            return "Ideal";
        case NX_OPAMP_LM741:
            return "LM741";
        case NX_OPAMP_JRC4558:
            return "JRC4558";
        case NX_OPAMP_BA728:
            return "BA728";
        case NX_OPAMP_LM308:
            return "LM308";
        case NX_OPAMP_TL072:
            return "TL072";
        default:
            return "Unknown";
    }
}

namespace
{
constexpr nx_bjt_npn_model_e kSelectableBjtModels[] = {
    NX_BJT_NPN,
    NX_BJT_2N3904,
    NX_BJT_2N2222,
};

constexpr nx_jfet_n_model_e kSelectableJfetModels[] = {
    NX_JFET_N,
    NX_JFET_J201,
    NX_JFET_2N5457,
};
}  // namespace

const char* bjtModelDisplayName(nx_bjt_npn_model_e model) noexcept
{
    switch (model)
    {
        case NX_BJT_NPN:
            return "Generic NPN";
        case NX_BJT_2N3904:
            return "2N3904";
        case NX_BJT_2N2222:
            return "2N2222";
        default:
            return "Unknown";
    }
}

const char* jfetModelDisplayName(nx_jfet_n_model_e model) noexcept
{
    switch (model)
    {
        case NX_JFET_N:
            return "Generic N-JFET";
        case NX_JFET_J201:
            return "J201";
        case NX_JFET_2N5457:
            return "2N5457";
        default:
            return "Unknown";
    }
}

nx_bjt_npn_model_e defaultBjtModel(CircuitKind circuit) noexcept
{
    juce::ignoreUnused(circuit);
    return NX_BJT_2N3904;
}

nx_jfet_n_model_e defaultJfetModel(CircuitKind circuit) noexcept
{
    juce::ignoreUnused(circuit);
    return NX_JFET_2N5457;
}

int bjtModelComboId(nx_bjt_npn_model_e model) noexcept
{
    for (size_t i = 0; i < std::size(kSelectableBjtModels); ++i)
    {
        if (kSelectableBjtModels[i] == model)
            return static_cast<int>(i + 1);
    }

    return bjtModelComboId(defaultBjtModel(CircuitKind::BjtFollower));
}

nx_bjt_npn_model_e bjtModelFromComboId(int comboId) noexcept
{
    if (comboId >= 1 && comboId <= static_cast<int>(std::size(kSelectableBjtModels)))
        return kSelectableBjtModels[static_cast<size_t>(comboId - 1)];

    return NX_BJT_2N3904;
}

int jfetModelComboId(nx_jfet_n_model_e model) noexcept
{
    for (size_t i = 0; i < std::size(kSelectableJfetModels); ++i)
    {
        if (kSelectableJfetModels[i] == model)
            return static_cast<int>(i + 1);
    }

    return jfetModelComboId(defaultJfetModel(CircuitKind::JfetFollower));
}

nx_jfet_n_model_e jfetModelFromComboId(int comboId) noexcept
{
    if (comboId >= 1 && comboId <= static_cast<int>(std::size(kSelectableJfetModels)))
        return kSelectableJfetModels[static_cast<size_t>(comboId - 1)];

    return NX_JFET_2N5457;
}

juce::String formatFrequencyTick(float log10Hz) noexcept
{
    const double hz = std::pow(10.0, static_cast<double>(log10Hz));
    if (hz >= 1000.0)
        return juce::String(hz / 1000.0, hz >= 10000.0 ? 0 : 1) + " kHz";

    return juce::String(juce::roundToInt(hz)) + " Hz";
}

juce::String formatMagnitudeTick(float magDb) noexcept
{
    return juce::String(magDb, std::abs(magDb) >= 10.0f ? 0 : 1) + " dB";
}

juce::String formatPhaseTick(float phaseDeg) noexcept
{
    return juce::String(phaseDeg, 0) + juce::String(juce::CharPointer_UTF8("\xc2\xb0"));
}

juce::String formatPeriodTick(float normalizedPeriod) noexcept
{
    const int degrees = juce::roundToInt(normalizedPeriod * 360.0f);
    return juce::String(degrees) + juce::String(juce::CharPointer_UTF8("\xc2\xb0"));
}

juce::String formatWaveformTick(float amplitude) noexcept
{
    const juce::String sign = amplitude >= 0.0f ? "+" : "";
    return sign + juce::String(amplitude, 2);
}

std::vector<double> buildLogFrequencySweep(const AcSweepParams& params)
{
    AcSweepParams safe = params;
    safe.sanitise();

    std::vector<double> freqs(static_cast<size_t>(safe.numPoints));
    if (safe.numPoints <= 1)
    {
        if (!freqs.empty())
            freqs[0] = safe.freqMinHz;
        return freqs;
    }

    const double ratio = safe.freqMaxHz / safe.freqMinHz;
    for (int i = 0; i < safe.numPoints; ++i)
    {
        const double t = static_cast<double>(i) / static_cast<double>(safe.numPoints - 1);
        freqs[static_cast<size_t>(i)] = safe.freqMinHz * std::pow(ratio, t);
    }

    return freqs;
}

std::vector<float> buildLogFrequencyGridTicks(float logMin, float logMax, int targetCount)
{
    juce::ignoreUnused(targetCount);

    std::vector<float> ticks;
    const int decadeStart = static_cast<int>(std::ceil(static_cast<double>(logMin)));
    const int decadeEnd = static_cast<int>(std::floor(static_cast<double>(logMax)));

    for (int exp = decadeStart; exp <= decadeEnd; ++exp)
    {
        const float value = static_cast<float>(exp);
        if (value >= logMin && value <= logMax)
            ticks.push_back(value);
    }

    return ticks;
}

namespace
{
double clampControl(double control) noexcept
{
    return juce::jlimit(0.0, 1.0, control);
}

juce::String makeResponseTitle(CircuitKind circuit,
                               nx_opamp_model_e model,
                               nx_bjt_npn_model_e bjtModel,
                               nx_jfet_n_model_e jfetModel,
                               double control,
                               double secondaryControl,
                               double tertiaryControl)
{
    juce::String title = juce::String(circuitDisplayName(circuit));

    if (circuitUsesOpampModel(circuit))
        title += " OpAmp AC  |  " + juce::String(opampModelDisplayName(model));
    else if (circuitUsesBjtModel(circuit))
        title += " AC  |  " + juce::String(bjtModelDisplayName(bjtModel));
    else if (circuitUsesJfetModel(circuit))
        title += " AC  |  " + juce::String(jfetModelDisplayName(jfetModel));
    else
        title += " AC";

    if (circuitHasPrimaryControl(circuit))
    {
        title += "  |  "
               + juce::String(controlParameterName(circuit))
               + " "
               + juce::String(control, 2);
    }

    if (circuitHasSecondaryControl(circuit))
    {
        title += "  |  "
               + juce::String(secondaryControlParameterName(circuit))
               + " "
               + juce::String(secondaryControl, 2);
    }

    if (circuitHasTertiaryControl(circuit))
    {
        title += "  |  "
               + juce::String(tertiaryControlParameterName(circuit))
               + " "
               + juce::String(tertiaryControl, 2);
    }

    return title;
}

void assignPotTaper(nx_smooth_pot_t& pot, nx_pot_taper_e taper) noexcept
{
    pot.pot_params.taper = taper;
    pot.pot_params.table = nullptr;
    pot.pot_params.table_size = 0;
}

template<typename Instance>
bool updatePotTaper(Instance* instance,
                    nx_result_t (*getPot)(const Instance*, nx_smooth_pot_t*),
                    nx_result_t (*setPot)(Instance*, const nx_smooth_pot_t*),
                    nx_pot_taper_e taper) noexcept
{
    if (instance == nullptr || getPot == nullptr || setPot == nullptr)
        return false;

    nx_smooth_pot_t pot;
    if (getPot(instance, &pot) != NX_SUCCESS)
        return false;

    assignPotTaper(pot, taper);
    return setPot(instance, &pot) == NX_SUCCESS;
}

void applyDs1OpampPotTaper(nx_ds1_opamp_f32_t* opamp, nx_pot_taper_e taper) noexcept
{
    updatePotTaper(opamp, nx_ds1_opamp_get_gain_pot_f32, nx_ds1_opamp_set_gain_pot_f32, taper);
}

void applyRatOpampPotTaper(nx_rat_opamp_f32_t* opamp, nx_pot_taper_e taper) noexcept
{
    updatePotTaper(opamp, nx_rat_opamp_get_distortion_pot_f32, nx_rat_opamp_set_distortion_pot_f32, taper);
}

void applyGuvnorPreampPotTaper(nx_guvnor_preamp_f32_t* opamp, nx_pot_taper_e taper) noexcept
{
    updatePotTaper(opamp, nx_guvnor_preamp_get_gain_pot_f32, nx_guvnor_preamp_set_gain_pot_f32, taper);
}

void applyGuvnorPostampPotTaper(nx_guvnor_postamp_f32_t* opamp, nx_pot_taper_e taper) noexcept
{
    updatePotTaper(opamp, nx_guvnor_postamp_get_gain_pot_f32, nx_guvnor_postamp_set_gain_pot_f32, taper);
}

void applyGuvnorOpampPotTaper(nx_guvnor_opamp_f32_t* opamp, nx_pot_taper_e taper) noexcept
{
    updatePotTaper(opamp, nx_guvnor_opamp_get_gain_pot_f32, nx_guvnor_opamp_set_gain_pot_f32, taper);
}

void applyTs9TonePotTaper(nx_ts9_tone_f32_t* tone, nx_pot_taper_e taper) noexcept
{
    updatePotTaper(tone, nx_ts9_tone_get_tone_pot_f32, nx_ts9_tone_set_tone_pot_f32, taper);
}

void applyDs1TonePotTaper(nx_ds1_tone_f32_t* tone, nx_pot_taper_e taper) noexcept
{
    updatePotTaper(tone, nx_ds1_tone_get_tone_pot_f32, nx_ds1_tone_set_tone_pot_f32, taper);
}

void applyDsPlusOpampPotTaper(nx_ds_plus_opamp_f32_t* opamp, nx_pot_taper_e taper) noexcept
{
    updatePotTaper(opamp, nx_ds_plus_opamp_get_distortion_pot_f32, nx_ds_plus_opamp_set_distortion_pot_f32, taper);
}

void applyKlonCentaurTonePotTaper(nx_klon_centaur_tone_f32_t* tone, nx_pot_taper_e taper) noexcept
{
    updatePotTaper(tone, nx_klon_centaur_tone_get_treble_pot_f32, nx_klon_centaur_tone_set_treble_pot_f32, taper);
}

void applyAcBoosterEqPotTapers(nx_ac_booster_eq_f32_t* eq, nx_pot_taper_e taper) noexcept
{
    updatePotTaper(eq, nx_ac_booster_eq_get_bass_pot_f32, nx_ac_booster_eq_set_bass_pot_f32, taper);
    updatePotTaper(eq, nx_ac_booster_eq_get_treble_pot_f32, nx_ac_booster_eq_set_treble_pot_f32, taper);
}

void applyRatClipperPotTaper(nx_rat_clipper_f32_t* clipper, nx_pot_taper_e taper) noexcept
{
    updatePotTaper(clipper, nx_rat_clipper_get_filter_pot_f32, nx_rat_clipper_set_filter_pot_f32, taper);
}

void applyTs9OpampPotTaper(nx_ts9_opamp_f32_t* opamp, nx_pot_taper_e taper) noexcept
{
    updatePotTaper(opamp, nx_ts9_opamp_get_drive_pot_f32, nx_ts9_opamp_set_drive_pot_f32, taper);
}

void applyAcBoosterDrivePotTaper(nx_ac_booster_drive_f32_t* drive, nx_pot_taper_e taper) noexcept
{
    updatePotTaper(drive, nx_ac_booster_drive_get_gain_pot_f32, nx_ac_booster_drive_set_gain_pot_f32, taper);
}

void applyKlonCentaurPotTaper(nx_klon_centaur_f32_t* centaur, nx_pot_taper_e taper) noexcept
{
    updatePotTaper(centaur, nx_klon_centaur_get_gain_pot_f32, nx_klon_centaur_set_gain_pot_f32, taper);
}

void applyGuvnorClipperPotTapers(nx_guvnor_clipper_f32_t* clipper, nx_pot_taper_e taper) noexcept
{
    updatePotTaper(clipper, nx_guvnor_clipper_get_bass_pot_f32, nx_guvnor_clipper_set_bass_pot_f32, taper);
    updatePotTaper(clipper, nx_guvnor_clipper_get_mid_pot_f32, nx_guvnor_clipper_set_mid_pot_f32, taper);
    updatePotTaper(clipper, nx_guvnor_clipper_get_treble_pot_f32, nx_guvnor_clipper_set_treble_pot_f32, taper);
}

void runDs1AcSweep(nx_ds1_opamp_f32_t* opamp,
                   nx_opamp_model_e model,
                   nx_pot_taper_e potTaper,
                   double control,
                   double sampleRateHz,
                   const std::vector<double>& freqs,
                   std::vector<double>& magDb,
                   std::vector<double>& phaseDeg)
{
    applyDs1OpampPotTaper(opamp, potTaper);
    nx_ds1_opamp_set_opamp_model_f32(opamp, model);
    nx_ds1_opamp_set_gain_control_f32(opamp, clampControl(control));
    nx_ds1_opamp_prepare_f32(opamp, sampleRateHz);
    nx_ds1_opamp_ac_f32(opamp, freqs.data(), magDb.data(), phaseDeg.data(), freqs.size());
}

void runRatAcSweep(nx_rat_opamp_f32_t* opamp,
                   nx_opamp_model_e model,
                   nx_pot_taper_e potTaper,
                   double control,
                   double sampleRateHz,
                   const std::vector<double>& freqs,
                   std::vector<double>& magDb,
                   std::vector<double>& phaseDeg)
{
    applyRatOpampPotTaper(opamp, potTaper);
    nx_rat_opamp_set_opamp_model_f32(opamp, model);
    nx_rat_opamp_set_distortion_control_f32(opamp, clampControl(control));
    nx_rat_opamp_prepare_f32(opamp, sampleRateHz);
    nx_rat_opamp_ac_f32(opamp, freqs.data(), magDb.data(), phaseDeg.data(), freqs.size());
}

void runGuvnorPreampAcSweep(nx_guvnor_preamp_f32_t* opamp,
                            nx_opamp_model_e model,
                            nx_pot_taper_e potTaper,
                            double control,
                            double sampleRateHz,
                            const std::vector<double>& freqs,
                            std::vector<double>& magDb,
                            std::vector<double>& phaseDeg)
{
    applyGuvnorPreampPotTaper(opamp, potTaper);
    nx_guvnor_preamp_set_opamp_model_f32(opamp, model);
    nx_guvnor_preamp_set_gain_control_f32(opamp, clampControl(control));
    nx_guvnor_preamp_prepare_f32(opamp, sampleRateHz);
    nx_guvnor_preamp_ac_f32(opamp, freqs.data(), magDb.data(), phaseDeg.data(), freqs.size());
}

void runGuvnorPostampAcSweep(nx_guvnor_postamp_f32_t* opamp,
                             nx_opamp_model_e model,
                             nx_pot_taper_e potTaper,
                             double control,
                             double sampleRateHz,
                             const std::vector<double>& freqs,
                             std::vector<double>& magDb,
                             std::vector<double>& phaseDeg)
{
    applyGuvnorPostampPotTaper(opamp, potTaper);
    nx_guvnor_postamp_set_opamp_model_f32(opamp, model);
    nx_guvnor_postamp_set_gain_control_f32(opamp, clampControl(control));
    nx_guvnor_postamp_prepare_f32(opamp, sampleRateHz);
    nx_guvnor_postamp_ac_f32(opamp, freqs.data(), magDb.data(), phaseDeg.data(), freqs.size());
}

void runGuvnorOpampAcSweep(nx_guvnor_opamp_f32_t* opamp,
                           nx_opamp_model_e model,
                           nx_pot_taper_e potTaper,
                           double control,
                           double sampleRateHz,
                           const std::vector<double>& freqs,
                           std::vector<double>& magDb,
                           std::vector<double>& phaseDeg)
{
    applyGuvnorOpampPotTaper(opamp, potTaper);
    nx_guvnor_opamp_set_opamp_model_f32(opamp, model);
    nx_guvnor_opamp_set_gain_control_f32(opamp, clampControl(control));
    nx_guvnor_opamp_prepare_f32(opamp, sampleRateHz);
    nx_guvnor_opamp_ac_f32(opamp, freqs.data(), magDb.data(), phaseDeg.data(), freqs.size());
}

void runTs9ToneAcSweep(nx_ts9_tone_f32_t* tone,
                       nx_opamp_model_e model,
                       nx_pot_taper_e potTaper,
                       double control,
                       double sampleRateHz,
                       const std::vector<double>& freqs,
                       std::vector<double>& magDb,
                       std::vector<double>& phaseDeg)
{
    applyTs9TonePotTaper(tone, potTaper);
    nx_ts9_tone_set_opamp_model_f32(tone, model);
    nx_ts9_tone_set_tone_control_f32(tone, clampControl(control));
    nx_ts9_tone_prepare_f32(tone, sampleRateHz);
    nx_ts9_tone_ac_f32(tone, freqs.data(), magDb.data(), phaseDeg.data(), freqs.size());
}

void runDs1ToneAcSweep(nx_ds1_tone_f32_t* tone,
                       nx_pot_taper_e potTaper,
                       double control,
                       double sampleRateHz,
                       const std::vector<double>& freqs,
                       std::vector<double>& magDb,
                       std::vector<double>& phaseDeg)
{
    applyDs1TonePotTaper(tone, potTaper);
    nx_ds1_tone_set_tone_control_f32(tone, clampControl(control));
    nx_ds1_tone_prepare_f32(tone, sampleRateHz);
    nx_ds1_tone_ac_f32(tone, freqs.data(), magDb.data(), phaseDeg.data(), freqs.size());
}

void runDsPlusOpampAcSweep(nx_ds_plus_opamp_f32_t* opamp,
                           nx_opamp_model_e model,
                           nx_pot_taper_e potTaper,
                           double control,
                           double sampleRateHz,
                           const std::vector<double>& freqs,
                           std::vector<double>& magDb,
                           std::vector<double>& phaseDeg)
{
    applyDsPlusOpampPotTaper(opamp, potTaper);
    nx_ds_plus_opamp_set_opamp_model_f32(opamp, model);
    nx_ds_plus_opamp_set_distortion_control_f32(opamp, clampControl(control));
    nx_ds_plus_opamp_prepare_f32(opamp, sampleRateHz);
    nx_ds_plus_opamp_ac_f32(opamp, freqs.data(), magDb.data(), phaseDeg.data(), freqs.size());
}

void runKlonCentaurToneAcSweep(nx_klon_centaur_tone_f32_t* tone,
                               nx_opamp_model_e model,
                               nx_pot_taper_e potTaper,
                               double control,
                               double sampleRateHz,
                               const std::vector<double>& freqs,
                               std::vector<double>& magDb,
                               std::vector<double>& phaseDeg)
{
    applyKlonCentaurTonePotTaper(tone, potTaper);
    nx_klon_centaur_tone_set_opamp_model_f32(tone, model);
    nx_klon_centaur_tone_set_treble_control_f32(tone, clampControl(control));
    nx_klon_centaur_tone_prepare_f32(tone, sampleRateHz);
    nx_klon_centaur_tone_ac_f32(tone, freqs.data(), magDb.data(), phaseDeg.data(), freqs.size());
}

void runAcBoosterEqAcSweep(nx_ac_booster_eq_f32_t* eq,
                           nx_opamp_model_e model,
                           nx_pot_taper_e potTaper,
                           double bassControl,
                           double trebleControl,
                           double sampleRateHz,
                           const std::vector<double>& freqs,
                           std::vector<double>& magDb,
                           std::vector<double>& phaseDeg)
{
    applyAcBoosterEqPotTapers(eq, potTaper);
    nx_ac_booster_eq_set_opamp_model_f32(eq, model);
    nx_ac_booster_eq_set_bass_control_f32(eq, clampControl(bassControl));
    nx_ac_booster_eq_set_treble_control_f32(eq, clampControl(trebleControl));
    nx_ac_booster_eq_prepare_f32(eq, sampleRateHz);
    nx_ac_booster_eq_ac_f32(eq, freqs.data(), magDb.data(), phaseDeg.data(), freqs.size());
}

void runDs1ClipperAcSweep(nx_ds1_clipper_f32_t* clipper,
                          double sampleRateHz,
                          const std::vector<double>& freqs,
                          std::vector<double>& magDb,
                          std::vector<double>& phaseDeg)
{
    nx_ds1_clipper_prepare_f32(clipper, sampleRateHz);
    nx_ds1_clipper_ac_f32(clipper, freqs.data(), magDb.data(), phaseDeg.data(), freqs.size());
}

void runDiodeClipperAcSweep(nx_diode_clipper_f32_t* clipper,
                            double sampleRateHz,
                            const std::vector<double>& freqs,
                            std::vector<double>& magDb,
                            std::vector<double>& phaseDeg)
{
    nx_diode_clipper_prepare_f32(clipper, sampleRateHz);
    nx_diode_clipper_ac_f32(clipper, freqs.data(), magDb.data(), phaseDeg.data(), freqs.size());
}

void runRatClipperAcSweep(nx_rat_clipper_f32_t* clipper,
                          nx_pot_taper_e potTaper,
                          double control,
                          double sampleRateHz,
                          const std::vector<double>& freqs,
                          std::vector<double>& magDb,
                          std::vector<double>& phaseDeg)
{
    applyRatClipperPotTaper(clipper, potTaper);
    nx_rat_clipper_set_filter_control_f32(clipper, clampControl(control));
    nx_rat_clipper_prepare_f32(clipper, sampleRateHz);
    nx_rat_clipper_ac_f32(clipper, freqs.data(), magDb.data(), phaseDeg.data(), freqs.size());
}

void runTs9OpampAcSweep(nx_ts9_opamp_f32_t* opamp,
                        nx_opamp_model_e model,
                        nx_pot_taper_e potTaper,
                        double control,
                        double sampleRateHz,
                        const std::vector<double>& freqs,
                        std::vector<double>& magDb,
                        std::vector<double>& phaseDeg)
{
    applyTs9OpampPotTaper(opamp, potTaper);
    nx_ts9_opamp_set_opamp_model_f32(opamp, model);
    nx_ts9_opamp_set_drive_control_f32(opamp, clampControl(control));
    nx_ts9_opamp_prepare_f32(opamp, sampleRateHz);
    nx_ts9_opamp_ac_f32(opamp, freqs.data(), magDb.data(), phaseDeg.data(), freqs.size());
}

void runAcBoosterDriveAcSweep(nx_ac_booster_drive_f32_t* drive,
                              nx_opamp_model_e model,
                              nx_pot_taper_e potTaper,
                              double control,
                              double sampleRateHz,
                              const std::vector<double>& freqs,
                              std::vector<double>& magDb,
                              std::vector<double>& phaseDeg)
{
    applyAcBoosterDrivePotTaper(drive, potTaper);
    nx_ac_booster_drive_set_opamp_model_f32(drive, model);
    nx_ac_booster_drive_set_gain_control_f32(drive, clampControl(control));
    nx_ac_booster_drive_prepare_f32(drive, sampleRateHz);
    nx_ac_booster_drive_ac_f32(drive, freqs.data(), magDb.data(), phaseDeg.data(), freqs.size());
}

void runKlonCentaurAcSweep(nx_klon_centaur_f32_t* centaur,
                           nx_pot_taper_e potTaper,
                           double control,
                           double sampleRateHz,
                           const std::vector<double>& freqs,
                           std::vector<double>& magDb,
                           std::vector<double>& phaseDeg)
{
    applyKlonCentaurPotTaper(centaur, potTaper);
    nx_klon_centaur_set_gain_control_f32(centaur, clampControl(control));
    nx_klon_centaur_prepare_f32(centaur, sampleRateHz);
    nx_klon_centaur_ac_f32(centaur, freqs.data(), magDb.data(), phaseDeg.data(), freqs.size());
}

void runGuvnorClipperAcSweep(nx_guvnor_clipper_f32_t* clipper,
                             nx_pot_taper_e potTaper,
                             double bassControl,
                             double midControl,
                             double trebleControl,
                             double sampleRateHz,
                             const std::vector<double>& freqs,
                             std::vector<double>& magDb,
                             std::vector<double>& phaseDeg)
{
    applyGuvnorClipperPotTapers(clipper, potTaper);
    nx_guvnor_clipper_set_bass_control_f32(clipper, clampControl(bassControl));
    nx_guvnor_clipper_set_mid_control_f32(clipper, clampControl(midControl));
    nx_guvnor_clipper_set_treble_control_f32(clipper, clampControl(trebleControl));
    nx_guvnor_clipper_prepare_f32(clipper, sampleRateHz);
    nx_guvnor_clipper_ac_f32(clipper, freqs.data(), magDb.data(), phaseDeg.data(), freqs.size());
}

void runBjtFollowerAcSweep(nx_bjt_follower_f32_t* follower,
                            nx_bjt_npn_model_e bjtModel,
                            double sampleRateHz,
                            const std::vector<double>& freqs,
                            std::vector<double>& magDb,
                            std::vector<double>& phaseDeg)
{
    nx_bjt_follower_set_bjt_model_f32(follower, bjtModel);
    nx_bjt_follower_prepare_f32(follower, sampleRateHz);
    nx_bjt_follower_ac_f32(follower, freqs.data(), magDb.data(), phaseDeg.data(), freqs.size());
}

void runBjtFollowerOutAcSweep(nx_bjt_follower_out_f32_t* follower,
                              nx_bjt_npn_model_e bjtModel,
                              double sampleRateHz,
                              const std::vector<double>& freqs,
                              std::vector<double>& magDb,
                              std::vector<double>& phaseDeg)
{
    nx_bjt_follower_out_set_bjt_model_f32(follower, bjtModel);
    nx_bjt_follower_out_prepare_f32(follower, sampleRateHz);
    nx_bjt_follower_out_ac_f32(follower, freqs.data(), magDb.data(), phaseDeg.data(), freqs.size());
}

void runBjtCommonEmitterAcSweep(nx_bjt_common_emitter_f32_t* emitter,
                                nx_bjt_npn_model_e bjtModel,
                                double sampleRateHz,
                                const std::vector<double>& freqs,
                                std::vector<double>& magDb,
                                std::vector<double>& phaseDeg)
{
    nx_bjt_common_emitter_set_bjt_model_f32(emitter, bjtModel);
    nx_bjt_common_emitter_prepare_f32(emitter, sampleRateHz);
    nx_bjt_common_emitter_ac_f32(emitter, freqs.data(), magDb.data(), phaseDeg.data(), freqs.size());
}

void runJfetFollowerAcSweep(nx_jfet_follower_f32_t* follower,
                            nx_jfet_n_model_e jfetModel,
                            double sampleRateHz,
                            const std::vector<double>& freqs,
                            std::vector<double>& magDb,
                            std::vector<double>& phaseDeg)
{
    nx_jfet_follower_set_jfet_model_f32(follower, jfetModel);
    nx_jfet_follower_prepare_f32(follower, sampleRateHz);
    nx_jfet_follower_ac_f32(follower, freqs.data(), magDb.data(), phaseDeg.data(), freqs.size());
}

AxisRange makeFrequencyAxis(float logMin, float logMax)
{
    AxisRange range;
    range.minX = logMin;
    range.maxX = logMax;
    return range;
}

AxisRange paddedMagnitudeAxis(float minMag, float maxMag, float logMin, float logMax)
{
    AxisRange range = makeFrequencyAxis(logMin, logMax);
    range.minY = -24.0f;
    range.maxY = 24.0f;

    if (!std::isfinite(minMag) || !std::isfinite(maxMag))
        return range;

    const float pad = juce::jmax(3.0f, 0.1f * (maxMag - minMag + 1.0f));
    range.minY = std::floor((minMag - pad) / 3.0f) * 3.0f;
    range.maxY = std::ceil((maxMag + pad) / 3.0f) * 3.0f;

    if (range.maxY <= range.minY)
        range.maxY = range.minY + 6.0f;

    return range;
}

void accumulateMagnitudeExtents(const std::vector<double>& magDb, float& minMag, float& maxMag)
{
    for (const double value : magDb)
    {
        if (!std::isfinite(value))
            continue;

        const float mag = static_cast<float>(value);
        minMag = juce::jmin(minMag, mag);
        maxMag = juce::jmax(maxMag, mag);
    }
}

AxisRange computePhaseAxis(const std::vector<std::pair<float, float>>& curve, float logMin, float logMax)
{
    juce::ignoreUnused(curve);

    AxisRange range = makeFrequencyAxis(logMin, logMax);
    constexpr float kPhaseMinDeg = -180.0f;
    constexpr float kPhaseMaxDeg = 180.0f;
    constexpr float kEndpointPadDeg = 4.0f;
    range.minY = kPhaseMinDeg - kEndpointPadDeg;
    range.maxY = kPhaseMaxDeg + kEndpointPadDeg;
    return range;
}

void sweepPrimaryControlEnvelope(CircuitKind circuit,
                                 nx_opamp_model_e model,
                                 nx_bjt_npn_model_e bjtModel,
                                 nx_jfet_n_model_e jfetModel,
                                 double secondaryControl,
                                 double tertiaryControl,
                                 nx_pot_taper_e potTaper,
                                 double sampleRateHz,
                                 const std::vector<double>& freqs,
                                 std::vector<double>& magDb,
                                 std::vector<double>& phaseDeg,
                                 float& minMag,
                                 float& maxMag)
{
    juce::ignoreUnused(bjtModel, jfetModel);
    if (circuit == CircuitKind::RatOpamp)
    {
        nx_rat_opamp_f32_t* opamp = nx_rat_opamp_create_f32(nullptr);
        if (opamp == nullptr)
            return;

        runRatAcSweep(opamp, model, potTaper, 0.0, sampleRateHz, freqs, magDb, phaseDeg);
        accumulateMagnitudeExtents(magDb, minMag, maxMag);

        runRatAcSweep(opamp, model, potTaper, 1.0, sampleRateHz, freqs, magDb, phaseDeg);
        accumulateMagnitudeExtents(magDb, minMag, maxMag);

        nx_rat_opamp_destroy_f32(opamp, nullptr);
    }
    else if (circuit == CircuitKind::GuvnorPreamp)
    {
        nx_guvnor_preamp_f32_t* opamp = nx_guvnor_preamp_create_f32(nullptr);
        if (opamp == nullptr)
            return;

        runGuvnorPreampAcSweep(opamp, model, potTaper, 0.0, sampleRateHz, freqs, magDb, phaseDeg);
        accumulateMagnitudeExtents(magDb, minMag, maxMag);

        runGuvnorPreampAcSweep(opamp, model, potTaper, 1.0, sampleRateHz, freqs, magDb, phaseDeg);
        accumulateMagnitudeExtents(magDb, minMag, maxMag);

        nx_guvnor_preamp_destroy_f32(opamp, nullptr);
    }
    else if (circuit == CircuitKind::GuvnorPostamp)
    {
        nx_guvnor_postamp_f32_t* opamp = nx_guvnor_postamp_create_f32(nullptr);
        if (opamp == nullptr)
            return;

        runGuvnorPostampAcSweep(opamp, model, potTaper, 0.0, sampleRateHz, freqs, magDb, phaseDeg);
        accumulateMagnitudeExtents(magDb, minMag, maxMag);

        runGuvnorPostampAcSweep(opamp, model, potTaper, 1.0, sampleRateHz, freqs, magDb, phaseDeg);
        accumulateMagnitudeExtents(magDb, minMag, maxMag);

        nx_guvnor_postamp_destroy_f32(opamp, nullptr);
    }
    else if (circuit == CircuitKind::GuvnorOpamp)
    {
        nx_guvnor_opamp_f32_t* opamp = nx_guvnor_opamp_create_f32(nullptr);
        if (opamp == nullptr)
            return;

        runGuvnorOpampAcSweep(opamp, model, potTaper, 0.0, sampleRateHz, freqs, magDb, phaseDeg);
        accumulateMagnitudeExtents(magDb, minMag, maxMag);

        runGuvnorOpampAcSweep(opamp, model, potTaper, 1.0, sampleRateHz, freqs, magDb, phaseDeg);
        accumulateMagnitudeExtents(magDb, minMag, maxMag);

        nx_guvnor_opamp_destroy_f32(opamp, nullptr);
    }
    else if (circuit == CircuitKind::Ts9Tone)
    {
        nx_ts9_tone_f32_t* tone = nx_ts9_tone_create_f32(nullptr);
        if (tone == nullptr)
            return;

        runTs9ToneAcSweep(tone, model, potTaper, 0.0, sampleRateHz, freqs, magDb, phaseDeg);
        accumulateMagnitudeExtents(magDb, minMag, maxMag);

        runTs9ToneAcSweep(tone, model, potTaper, 1.0, sampleRateHz, freqs, magDb, phaseDeg);
        accumulateMagnitudeExtents(magDb, minMag, maxMag);

        nx_ts9_tone_destroy_f32(tone, nullptr);
    }
    else if (circuit == CircuitKind::Ds1Tone)
    {
        nx_ds1_tone_f32_t* tone = nx_ds1_tone_create_f32(nullptr);
        if (tone == nullptr)
            return;

        runDs1ToneAcSweep(tone, potTaper, 0.0, sampleRateHz, freqs, magDb, phaseDeg);
        accumulateMagnitudeExtents(magDb, minMag, maxMag);

        runDs1ToneAcSweep(tone, potTaper, 1.0, sampleRateHz, freqs, magDb, phaseDeg);
        accumulateMagnitudeExtents(magDb, minMag, maxMag);

        nx_ds1_tone_destroy_f32(tone, nullptr);
    }
    else if (circuit == CircuitKind::DsPlusOpamp)
    {
        nx_ds_plus_opamp_f32_t* opamp = nx_ds_plus_opamp_create_f32(nullptr);
        if (opamp == nullptr)
            return;

        runDsPlusOpampAcSweep(opamp, model, potTaper, 0.0, sampleRateHz, freqs, magDb, phaseDeg);
        accumulateMagnitudeExtents(magDb, minMag, maxMag);

        runDsPlusOpampAcSweep(opamp, model, potTaper, 1.0, sampleRateHz, freqs, magDb, phaseDeg);
        accumulateMagnitudeExtents(magDb, minMag, maxMag);

        nx_ds_plus_opamp_destroy_f32(opamp, nullptr);
    }
    else if (circuit == CircuitKind::KlonCentaurTone)
    {
        nx_klon_centaur_tone_f32_t* tone = nx_klon_centaur_tone_create_f32(nullptr);
        if (tone == nullptr)
            return;

        runKlonCentaurToneAcSweep(tone, model, potTaper, 0.0, sampleRateHz, freqs, magDb, phaseDeg);
        accumulateMagnitudeExtents(magDb, minMag, maxMag);

        runKlonCentaurToneAcSweep(tone, model, potTaper, 1.0, sampleRateHz, freqs, magDb, phaseDeg);
        accumulateMagnitudeExtents(magDb, minMag, maxMag);

        nx_klon_centaur_tone_destroy_f32(tone, nullptr);
    }
    else if (circuit == CircuitKind::AcBoosterEq)
    {
        juce::ignoreUnused(secondaryControl);

        nx_ac_booster_eq_f32_t* eq = nx_ac_booster_eq_create_f32(nullptr);
        if (eq == nullptr)
            return;

        constexpr double cornerControls[4][2] = {
            { 0.0, 0.0 },
            { 0.0, 1.0 },
            { 1.0, 0.0 },
            { 1.0, 1.0 },
        };

        for (const auto& corner : cornerControls)
        {
            runAcBoosterEqAcSweep(eq, model, potTaper, corner[0], corner[1], sampleRateHz, freqs, magDb, phaseDeg);
            accumulateMagnitudeExtents(magDb, minMag, maxMag);
        }

        nx_ac_booster_eq_destroy_f32(eq, nullptr);
    }
    else if (circuit == CircuitKind::Ds1Clipper)
    {
        nx_ds1_clipper_f32_t* clipper = nx_ds1_clipper_create_f32(nullptr);
        if (clipper == nullptr)
            return;

        runDs1ClipperAcSweep(clipper, sampleRateHz, freqs, magDb, phaseDeg);
        accumulateMagnitudeExtents(magDb, minMag, maxMag);

        nx_ds1_clipper_destroy_f32(clipper, nullptr);
    }
    else if (circuit == CircuitKind::DiodeClipper)
    {
        nx_diode_clipper_f32_t* clipper = nx_diode_clipper_create_f32(nullptr);
        if (clipper == nullptr)
            return;

        runDiodeClipperAcSweep(clipper, sampleRateHz, freqs, magDb, phaseDeg);
        accumulateMagnitudeExtents(magDb, minMag, maxMag);

        nx_diode_clipper_destroy_f32(clipper, nullptr);
    }
    else if (circuit == CircuitKind::RatClipper)
    {
        nx_rat_clipper_f32_t* clipper = nx_rat_clipper_create_f32(nullptr);
        if (clipper == nullptr)
            return;

        runRatClipperAcSweep(clipper, potTaper, 0.0, sampleRateHz, freqs, magDb, phaseDeg);
        accumulateMagnitudeExtents(magDb, minMag, maxMag);

        runRatClipperAcSweep(clipper, potTaper, 1.0, sampleRateHz, freqs, magDb, phaseDeg);
        accumulateMagnitudeExtents(magDb, minMag, maxMag);

        nx_rat_clipper_destroy_f32(clipper, nullptr);
    }
    else if (circuit == CircuitKind::Ts9Opamp)
    {
        nx_ts9_opamp_f32_t* opamp = nx_ts9_opamp_create_f32(nullptr);
        if (opamp == nullptr)
            return;

        runTs9OpampAcSweep(opamp, model, potTaper, 0.0, sampleRateHz, freqs, magDb, phaseDeg);
        accumulateMagnitudeExtents(magDb, minMag, maxMag);

        runTs9OpampAcSweep(opamp, model, potTaper, 1.0, sampleRateHz, freqs, magDb, phaseDeg);
        accumulateMagnitudeExtents(magDb, minMag, maxMag);

        nx_ts9_opamp_destroy_f32(opamp, nullptr);
    }
    else if (circuit == CircuitKind::AcBoosterDrive)
    {
        nx_ac_booster_drive_f32_t* drive = nx_ac_booster_drive_create_f32(nullptr);
        if (drive == nullptr)
            return;

        runAcBoosterDriveAcSweep(drive, model, potTaper, 0.0, sampleRateHz, freqs, magDb, phaseDeg);
        accumulateMagnitudeExtents(magDb, minMag, maxMag);

        runAcBoosterDriveAcSweep(drive, model, potTaper, 1.0, sampleRateHz, freqs, magDb, phaseDeg);
        accumulateMagnitudeExtents(magDb, minMag, maxMag);

        nx_ac_booster_drive_destroy_f32(drive, nullptr);
    }
    else if (circuit == CircuitKind::KlonCentaur)
    {
        nx_klon_centaur_f32_t* centaur = nx_klon_centaur_create_f32(nullptr);
        if (centaur == nullptr)
            return;

        runKlonCentaurAcSweep(centaur, potTaper, 0.0, sampleRateHz, freqs, magDb, phaseDeg);
        accumulateMagnitudeExtents(magDb, minMag, maxMag);

        runKlonCentaurAcSweep(centaur, potTaper, 1.0, sampleRateHz, freqs, magDb, phaseDeg);
        accumulateMagnitudeExtents(magDb, minMag, maxMag);

        nx_klon_centaur_destroy_f32(centaur, nullptr);
    }
    else if (circuit == CircuitKind::GuvnorClipper)
    {
        nx_guvnor_clipper_f32_t* clipper = nx_guvnor_clipper_create_f32(nullptr);
        if (clipper == nullptr)
            return;

        constexpr double cornerControls[8][3] = {
            { 0.0, 0.0, 0.0 },
            { 0.0, 0.0, 1.0 },
            { 0.0, 1.0, 0.0 },
            { 0.0, 1.0, 1.0 },
            { 1.0, 0.0, 0.0 },
            { 1.0, 0.0, 1.0 },
            { 1.0, 1.0, 0.0 },
            { 1.0, 1.0, 1.0 },
        };

        for (const auto& corner : cornerControls)
        {
            runGuvnorClipperAcSweep(clipper,
                                    potTaper,
                                    corner[0],
                                    corner[1],
                                    corner[2],
                                    sampleRateHz,
                                    freqs,
                                    magDb,
                                    phaseDeg);
            accumulateMagnitudeExtents(magDb, minMag, maxMag);
        }

        nx_guvnor_clipper_destroy_f32(clipper, nullptr);
    }
    else if (circuit == CircuitKind::BjtFollower)
    {
        nx_bjt_follower_f32_t* follower = nx_bjt_follower_create_f32(nullptr);
        if (follower == nullptr)
            return;

        runBjtFollowerAcSweep(follower, bjtModel, sampleRateHz, freqs, magDb, phaseDeg);
        accumulateMagnitudeExtents(magDb, minMag, maxMag);

        nx_bjt_follower_destroy_f32(follower, nullptr);
    }
    else if (circuit == CircuitKind::BjtFollowerOut)
    {
        nx_bjt_follower_out_f32_t* follower = nx_bjt_follower_out_create_f32(nullptr);
        if (follower == nullptr)
            return;

        runBjtFollowerOutAcSweep(follower, bjtModel, sampleRateHz, freqs, magDb, phaseDeg);
        accumulateMagnitudeExtents(magDb, minMag, maxMag);

        nx_bjt_follower_out_destroy_f32(follower, nullptr);
    }
    else if (circuit == CircuitKind::BjtCommonEmitter)
    {
        nx_bjt_common_emitter_f32_t* emitter = nx_bjt_common_emitter_create_f32(nullptr);
        if (emitter == nullptr)
            return;

        runBjtCommonEmitterAcSweep(emitter, bjtModel, sampleRateHz, freqs, magDb, phaseDeg);
        accumulateMagnitudeExtents(magDb, minMag, maxMag);

        nx_bjt_common_emitter_destroy_f32(emitter, nullptr);
    }
    else if (circuit == CircuitKind::JfetFollower)
    {
        nx_jfet_follower_f32_t* follower = nx_jfet_follower_create_f32(nullptr);
        if (follower == nullptr)
            return;

        runJfetFollowerAcSweep(follower, jfetModel, sampleRateHz, freqs, magDb, phaseDeg);
        accumulateMagnitudeExtents(magDb, minMag, maxMag);

        nx_jfet_follower_destroy_f32(follower, nullptr);
    }
    else
    {
        nx_ds1_opamp_f32_t* opamp = nx_ds1_opamp_create_f32(nullptr);
        if (opamp == nullptr)
            return;

        runDs1AcSweep(opamp, model, potTaper, 0.0, sampleRateHz, freqs, magDb, phaseDeg);
        accumulateMagnitudeExtents(magDb, minMag, maxMag);

        runDs1AcSweep(opamp, model, potTaper, 1.0, sampleRateHz, freqs, magDb, phaseDeg);
        accumulateMagnitudeExtents(magDb, minMag, maxMag);

        nx_ds1_opamp_destroy_f32(opamp, nullptr);
    }
}

bool runAcResponseSweep(CircuitKind circuit,
                        nx_opamp_model_e model,
                        nx_bjt_npn_model_e bjtModel,
                        nx_jfet_n_model_e jfetModel,
                        double gainControl,
                        double secondaryControl,
                        double tertiaryControl,
                        nx_pot_taper_e potTaper,
                        double sampleRateHz,
                        const std::vector<double>& freqs,
                        std::vector<double>& magDb,
                        std::vector<double>& phaseDeg)
{
    if (circuit == CircuitKind::RatOpamp)
    {
        nx_rat_opamp_f32_t* opamp = nx_rat_opamp_create_f32(nullptr);
        if (opamp == nullptr)
            return false;

        runRatAcSweep(opamp, model, potTaper, gainControl, sampleRateHz, freqs, magDb, phaseDeg);
        nx_rat_opamp_destroy_f32(opamp, nullptr);
        return true;
    }

    if (circuit == CircuitKind::GuvnorPreamp)
    {
        nx_guvnor_preamp_f32_t* opamp = nx_guvnor_preamp_create_f32(nullptr);
        if (opamp == nullptr)
            return false;

        runGuvnorPreampAcSweep(opamp, model, potTaper, gainControl, sampleRateHz, freqs, magDb, phaseDeg);
        nx_guvnor_preamp_destroy_f32(opamp, nullptr);
        return true;
    }

    if (circuit == CircuitKind::GuvnorPostamp)
    {
        nx_guvnor_postamp_f32_t* opamp = nx_guvnor_postamp_create_f32(nullptr);
        if (opamp == nullptr)
            return false;

        runGuvnorPostampAcSweep(opamp, model, potTaper, gainControl, sampleRateHz, freqs, magDb, phaseDeg);
        nx_guvnor_postamp_destroy_f32(opamp, nullptr);
        return true;
    }

    if (circuit == CircuitKind::GuvnorOpamp)
    {
        nx_guvnor_opamp_f32_t* opamp = nx_guvnor_opamp_create_f32(nullptr);
        if (opamp == nullptr)
            return false;

        runGuvnorOpampAcSweep(opamp, model, potTaper, gainControl, sampleRateHz, freqs, magDb, phaseDeg);
        nx_guvnor_opamp_destroy_f32(opamp, nullptr);
        return true;
    }

    if (circuit == CircuitKind::Ts9Tone)
    {
        nx_ts9_tone_f32_t* tone = nx_ts9_tone_create_f32(nullptr);
        if (tone == nullptr)
            return false;

        runTs9ToneAcSweep(tone, model, potTaper, gainControl, sampleRateHz, freqs, magDb, phaseDeg);
        nx_ts9_tone_destroy_f32(tone, nullptr);
        return true;
    }

    if (circuit == CircuitKind::Ds1Tone)
    {
        nx_ds1_tone_f32_t* tone = nx_ds1_tone_create_f32(nullptr);
        if (tone == nullptr)
            return false;

        runDs1ToneAcSweep(tone, potTaper, gainControl, sampleRateHz, freqs, magDb, phaseDeg);
        nx_ds1_tone_destroy_f32(tone, nullptr);
        return true;
    }

    if (circuit == CircuitKind::DsPlusOpamp)
    {
        nx_ds_plus_opamp_f32_t* opamp = nx_ds_plus_opamp_create_f32(nullptr);
        if (opamp == nullptr)
            return false;

        runDsPlusOpampAcSweep(opamp, model, potTaper, gainControl, sampleRateHz, freqs, magDb, phaseDeg);
        nx_ds_plus_opamp_destroy_f32(opamp, nullptr);
        return true;
    }

    if (circuit == CircuitKind::KlonCentaurTone)
    {
        nx_klon_centaur_tone_f32_t* tone = nx_klon_centaur_tone_create_f32(nullptr);
        if (tone == nullptr)
            return false;

        runKlonCentaurToneAcSweep(tone, model, potTaper, gainControl, sampleRateHz, freqs, magDb, phaseDeg);
        nx_klon_centaur_tone_destroy_f32(tone, nullptr);
        return true;
    }

    if (circuit == CircuitKind::AcBoosterEq)
    {
        nx_ac_booster_eq_f32_t* eq = nx_ac_booster_eq_create_f32(nullptr);
        if (eq == nullptr)
            return false;

        runAcBoosterEqAcSweep(eq, model, potTaper, gainControl, secondaryControl, sampleRateHz, freqs, magDb, phaseDeg);
        nx_ac_booster_eq_destroy_f32(eq, nullptr);
        return true;
    }

    if (circuit == CircuitKind::Ds1Clipper)
    {
        nx_ds1_clipper_f32_t* clipper = nx_ds1_clipper_create_f32(nullptr);
        if (clipper == nullptr)
            return false;

        runDs1ClipperAcSweep(clipper, sampleRateHz, freqs, magDb, phaseDeg);
        nx_ds1_clipper_destroy_f32(clipper, nullptr);
        return true;
    }

    if (circuit == CircuitKind::DiodeClipper)
    {
        nx_diode_clipper_f32_t* clipper = nx_diode_clipper_create_f32(nullptr);
        if (clipper == nullptr)
            return false;

        runDiodeClipperAcSweep(clipper, sampleRateHz, freqs, magDb, phaseDeg);
        nx_diode_clipper_destroy_f32(clipper, nullptr);
        return true;
    }

    if (circuit == CircuitKind::RatClipper)
    {
        nx_rat_clipper_f32_t* clipper = nx_rat_clipper_create_f32(nullptr);
        if (clipper == nullptr)
            return false;

        runRatClipperAcSweep(clipper, potTaper, gainControl, sampleRateHz, freqs, magDb, phaseDeg);
        nx_rat_clipper_destroy_f32(clipper, nullptr);
        return true;
    }

    if (circuit == CircuitKind::Ts9Opamp)
    {
        nx_ts9_opamp_f32_t* opamp = nx_ts9_opamp_create_f32(nullptr);
        if (opamp == nullptr)
            return false;

        runTs9OpampAcSweep(opamp, model, potTaper, gainControl, sampleRateHz, freqs, magDb, phaseDeg);
        nx_ts9_opamp_destroy_f32(opamp, nullptr);
        return true;
    }

    if (circuit == CircuitKind::AcBoosterDrive)
    {
        nx_ac_booster_drive_f32_t* drive = nx_ac_booster_drive_create_f32(nullptr);
        if (drive == nullptr)
            return false;

        runAcBoosterDriveAcSweep(drive, model, potTaper, gainControl, sampleRateHz, freqs, magDb, phaseDeg);
        nx_ac_booster_drive_destroy_f32(drive, nullptr);
        return true;
    }

    if (circuit == CircuitKind::KlonCentaur)
    {
        nx_klon_centaur_f32_t* centaur = nx_klon_centaur_create_f32(nullptr);
        if (centaur == nullptr)
            return false;

        runKlonCentaurAcSweep(centaur, potTaper, gainControl, sampleRateHz, freqs, magDb, phaseDeg);
        nx_klon_centaur_destroy_f32(centaur, nullptr);
        return true;
    }

    if (circuit == CircuitKind::GuvnorClipper)
    {
        nx_guvnor_clipper_f32_t* clipper = nx_guvnor_clipper_create_f32(nullptr);
        if (clipper == nullptr)
            return false;

        runGuvnorClipperAcSweep(clipper,
                                potTaper,
                                gainControl,
                                secondaryControl,
                                tertiaryControl,
                                sampleRateHz,
                                freqs,
                                magDb,
                                phaseDeg);
        nx_guvnor_clipper_destroy_f32(clipper, nullptr);
        return true;
    }

    if (circuit == CircuitKind::BjtFollower)
    {
        nx_bjt_follower_f32_t* follower = nx_bjt_follower_create_f32(nullptr);
        if (follower == nullptr)
            return false;

        runBjtFollowerAcSweep(follower, bjtModel, sampleRateHz, freqs, magDb, phaseDeg);
        nx_bjt_follower_destroy_f32(follower, nullptr);
        return true;
    }

    if (circuit == CircuitKind::BjtFollowerOut)
    {
        nx_bjt_follower_out_f32_t* follower = nx_bjt_follower_out_create_f32(nullptr);
        if (follower == nullptr)
            return false;

        runBjtFollowerOutAcSweep(follower, bjtModel, sampleRateHz, freqs, magDb, phaseDeg);
        nx_bjt_follower_out_destroy_f32(follower, nullptr);
        return true;
    }

    if (circuit == CircuitKind::BjtCommonEmitter)
    {
        nx_bjt_common_emitter_f32_t* emitter = nx_bjt_common_emitter_create_f32(nullptr);
        if (emitter == nullptr)
            return false;

        runBjtCommonEmitterAcSweep(emitter, bjtModel, sampleRateHz, freqs, magDb, phaseDeg);
        nx_bjt_common_emitter_destroy_f32(emitter, nullptr);
        return true;
    }

    if (circuit == CircuitKind::JfetFollower)
    {
        nx_jfet_follower_f32_t* follower = nx_jfet_follower_create_f32(nullptr);
        if (follower == nullptr)
            return false;

        runJfetFollowerAcSweep(follower, jfetModel, sampleRateHz, freqs, magDb, phaseDeg);
        nx_jfet_follower_destroy_f32(follower, nullptr);
        return true;
    }

    nx_ds1_opamp_f32_t* opamp = nx_ds1_opamp_create_f32(nullptr);
    if (opamp == nullptr)
        return false;

    runDs1AcSweep(opamp, model, potTaper, gainControl, sampleRateHz, freqs, magDb, phaseDeg);
    nx_ds1_opamp_destroy_f32(opamp, nullptr);
    return true;
}

constexpr int kPreviewDisplayPoints = 256;
constexpr int kWarmupPeriods = 8;
constexpr float kRatPreviewInputMinScale = 0.1f;
constexpr float kTs9PreviewInputScale = 0.05f;
constexpr float kDsPlusPreviewInputScale = 0.01f;
constexpr float kDs1TonePreviewInputScale = 0.5f;
constexpr float kKlonPreviewInputScale = 0.5f;
constexpr float kClipperPreviewInputScale = 0.05f;
constexpr float kAcBoosterDrivePreviewInputScale = 0.05f;
constexpr float kTransistorPreviewInputScale = 0.01f;
constexpr double kDs1TonePreviewVcc = 2.0;
constexpr double kClipperPreviewVcc = 9.0;

float ratPreviewInputScale(double distortionControl) noexcept
{
    const float control = static_cast<float>(juce::jlimit(0.0, 1.0, distortionControl));
    return juce::jmin(1.0f, kRatPreviewInputMinScale + control * (1.0f - kRatPreviewInputMinScale));
}

float previewInputScale(CircuitKind circuit, double control) noexcept
{
    switch (circuit)
    {
        case CircuitKind::RatOpamp:
            return ratPreviewInputScale(control);
        case CircuitKind::Ts9Tone:
            return kTs9PreviewInputScale;
        case CircuitKind::DsPlusOpamp:
            return kDsPlusPreviewInputScale;
        case CircuitKind::Ds1Tone:
            return kDs1TonePreviewInputScale;
        case CircuitKind::KlonCentaurTone:
            return kKlonPreviewInputScale;
        case CircuitKind::Ds1Clipper:
        case CircuitKind::DiodeClipper:
        case CircuitKind::RatClipper:
        case CircuitKind::Ts9Opamp:
        case CircuitKind::GuvnorClipper:
            return kClipperPreviewInputScale;
        case CircuitKind::AcBoosterDrive:
            return kAcBoosterDrivePreviewInputScale;
        case CircuitKind::KlonCentaur:
            return kKlonPreviewInputScale;
        case CircuitKind::BjtFollower:
        case CircuitKind::BjtFollowerOut:
        case CircuitKind::BjtCommonEmitter:
        case CircuitKind::JfetFollower:
            return kTransistorPreviewInputScale;
        default:
            return 1.0f;
    }
}

float lerpWaveSample(const std::vector<float>& samples, float index)
{
    if (samples.empty())
        return 0.0f;

    if (samples.size() == 1)
        return samples.front();

    const float clamped = juce::jlimit(0.0f, static_cast<float>(samples.size() - 1), index);
    const int i0 = static_cast<int>(std::floor(clamped));
    const int i1 = juce::jmin(i0 + 1, static_cast<int>(samples.size()) - 1);
    const float frac = clamped - static_cast<float>(i0);
    return samples[static_cast<size_t>(i0)] * (1.0f - frac) + samples[static_cast<size_t>(i1)] * frac;
}

bool runSineWavePreviewProcess(CircuitKind circuit,
                               nx_opamp_model_e model,
                               nx_bjt_npn_model_e bjtModel,
                               nx_jfet_n_model_e jfetModel,
                               double gainControl,
                               double secondaryControl,
                               double tertiaryControl,
                               nx_pot_taper_e potTaper,
                               double sampleRateHz,
                               const std::vector<float>& input,
                               std::vector<float>& output,
                               size_t totalSamples,
                               double& vccOut)
{
    if (circuit == CircuitKind::RatOpamp)
    {
        nx_rat_opamp_f32_t* opamp = nx_rat_opamp_create_f32(nullptr);
        if (opamp == nullptr)
            return false;

        applyRatOpampPotTaper(opamp, potTaper);
        nx_rat_opamp_set_opamp_model_f32(opamp, model);
        nx_rat_opamp_set_distortion_control_f32(opamp, clampControl(gainControl));
        nx_rat_opamp_prepare_f32(opamp, sampleRateHz);
        nx_rat_opamp_reset_f32(opamp);

        for (int i = 0; i < 500; ++i)
            nx_rat_opamp_tick_f32(opamp, 128);

        nx_rat_opamp_process_f32(opamp, input.data(), output.data(), totalSamples);
        nx_rat_opamp_tick_f32(opamp, totalSamples);

        vccOut = nx_rat_opamp_get_vcc_f32(opamp);
        nx_rat_opamp_destroy_f32(opamp, nullptr);
        return true;
    }

    if (circuit == CircuitKind::GuvnorPreamp)
    {
        nx_guvnor_preamp_f32_t* opamp = nx_guvnor_preamp_create_f32(nullptr);
        if (opamp == nullptr)
            return false;

        applyGuvnorPreampPotTaper(opamp, potTaper);
        nx_guvnor_preamp_set_opamp_model_f32(opamp, model);
        nx_guvnor_preamp_set_gain_control_f32(opamp, clampControl(gainControl));
        nx_guvnor_preamp_prepare_f32(opamp, sampleRateHz);
        nx_guvnor_preamp_reset_f32(opamp);

        for (int i = 0; i < 500; ++i)
            nx_guvnor_preamp_tick_f32(opamp, 128);

        nx_guvnor_preamp_process_f32(opamp, input.data(), output.data(), totalSamples);
        nx_guvnor_preamp_tick_f32(opamp, totalSamples);

        vccOut = nx_guvnor_preamp_get_vcc_f32(opamp);
        nx_guvnor_preamp_destroy_f32(opamp, nullptr);
        return true;
    }

    if (circuit == CircuitKind::GuvnorPostamp)
    {
        nx_guvnor_postamp_f32_t* opamp = nx_guvnor_postamp_create_f32(nullptr);
        if (opamp == nullptr)
            return false;

        applyGuvnorPostampPotTaper(opamp, potTaper);
        nx_guvnor_postamp_set_opamp_model_f32(opamp, model);
        nx_guvnor_postamp_set_gain_control_f32(opamp, clampControl(gainControl));
        nx_guvnor_postamp_prepare_f32(opamp, sampleRateHz);
        nx_guvnor_postamp_reset_f32(opamp);

        for (int i = 0; i < 500; ++i)
            nx_guvnor_postamp_tick_f32(opamp, 128);

        nx_guvnor_postamp_process_f32(opamp, input.data(), output.data(), totalSamples);
        nx_guvnor_postamp_tick_f32(opamp, totalSamples);

        vccOut = nx_guvnor_postamp_get_vcc_f32(opamp);
        nx_guvnor_postamp_destroy_f32(opamp, nullptr);
        return true;
    }

    if (circuit == CircuitKind::GuvnorOpamp)
    {
        nx_guvnor_opamp_f32_t* opamp = nx_guvnor_opamp_create_f32(nullptr);
        if (opamp == nullptr)
            return false;

        applyGuvnorOpampPotTaper(opamp, potTaper);
        nx_guvnor_opamp_set_opamp_model_f32(opamp, model);
        nx_guvnor_opamp_set_gain_control_f32(opamp, clampControl(gainControl));
        nx_guvnor_opamp_prepare_f32(opamp, sampleRateHz);
        nx_guvnor_opamp_reset_f32(opamp);

        for (int i = 0; i < 500; ++i)
            nx_guvnor_opamp_tick_f32(opamp, 128);

        nx_guvnor_opamp_process_f32(opamp, input.data(), output.data(), totalSamples);
        nx_guvnor_opamp_tick_f32(opamp, totalSamples);

        vccOut = nx_guvnor_opamp_get_vcc_f32(opamp);
        nx_guvnor_opamp_destroy_f32(opamp, nullptr);
        return true;
    }

    if (circuit == CircuitKind::Ts9Tone)
    {
        nx_ts9_tone_f32_t* tone = nx_ts9_tone_create_f32(nullptr);
        if (tone == nullptr)
            return false;

        applyTs9TonePotTaper(tone, potTaper);
        nx_ts9_tone_set_opamp_model_f32(tone, model);
        nx_ts9_tone_set_tone_control_f32(tone, clampControl(gainControl));
        nx_ts9_tone_prepare_f32(tone, sampleRateHz);
        nx_ts9_tone_reset_f32(tone);

        for (int i = 0; i < 500; ++i)
            nx_ts9_tone_tick_f32(tone, 128);

        nx_ts9_tone_process_f32(tone, input.data(), output.data(), totalSamples);
        nx_ts9_tone_tick_f32(tone, totalSamples);

        vccOut = nx_ts9_tone_get_vcc_f32(tone);
        nx_ts9_tone_destroy_f32(tone, nullptr);
        return true;
    }

    if (circuit == CircuitKind::Ds1Tone)
    {
        nx_ds1_tone_f32_t* tone = nx_ds1_tone_create_f32(nullptr);
        if (tone == nullptr)
            return false;

        applyDs1TonePotTaper(tone, potTaper);
        nx_ds1_tone_set_tone_control_f32(tone, clampControl(gainControl));
        nx_ds1_tone_prepare_f32(tone, sampleRateHz);
        nx_ds1_tone_reset_f32(tone);

        for (int i = 0; i < 500; ++i)
            nx_ds1_tone_tick_f32(tone, 128);

        nx_ds1_tone_process_f32(tone, input.data(), output.data(), totalSamples);
        nx_ds1_tone_tick_f32(tone, totalSamples);

        vccOut = kDs1TonePreviewVcc;
        nx_ds1_tone_destroy_f32(tone, nullptr);
        return true;
    }

    if (circuit == CircuitKind::DsPlusOpamp)
    {
        nx_ds_plus_opamp_f32_t* opamp = nx_ds_plus_opamp_create_f32(nullptr);
        if (opamp == nullptr)
            return false;

        applyDsPlusOpampPotTaper(opamp, potTaper);
        nx_ds_plus_opamp_set_opamp_model_f32(opamp, model);
        nx_ds_plus_opamp_set_distortion_control_f32(opamp, clampControl(gainControl));
        nx_ds_plus_opamp_prepare_f32(opamp, sampleRateHz);
        nx_ds_plus_opamp_reset_f32(opamp);

        for (int i = 0; i < 500; ++i)
            nx_ds_plus_opamp_tick_f32(opamp, 128);

        nx_ds_plus_opamp_process_f32(opamp, input.data(), output.data(), totalSamples);
        nx_ds_plus_opamp_tick_f32(opamp, totalSamples);

        vccOut = nx_ds_plus_opamp_get_vcc_f32(opamp);
        nx_ds_plus_opamp_destroy_f32(opamp, nullptr);
        return true;
    }

    if (circuit == CircuitKind::KlonCentaurTone)
    {
        nx_klon_centaur_tone_f32_t* tone = nx_klon_centaur_tone_create_f32(nullptr);
        if (tone == nullptr)
            return false;

        applyKlonCentaurTonePotTaper(tone, potTaper);
        nx_klon_centaur_tone_set_opamp_model_f32(tone, model);
        nx_klon_centaur_tone_set_treble_control_f32(tone, clampControl(gainControl));
        nx_klon_centaur_tone_prepare_f32(tone, sampleRateHz);
        nx_klon_centaur_tone_reset_f32(tone);

        for (int i = 0; i < 500; ++i)
            nx_klon_centaur_tone_tick_f32(tone, 128);

        nx_klon_centaur_tone_process_f32(tone, input.data(), output.data(), totalSamples);
        nx_klon_centaur_tone_tick_f32(tone, totalSamples);

        vccOut = nx_klon_centaur_tone_get_vcc_f32(tone);
        nx_klon_centaur_tone_destroy_f32(tone, nullptr);
        return true;
    }

    if (circuit == CircuitKind::AcBoosterEq)
    {
        nx_ac_booster_eq_f32_t* eq = nx_ac_booster_eq_create_f32(nullptr);
        if (eq == nullptr)
            return false;

        applyAcBoosterEqPotTapers(eq, potTaper);
        nx_ac_booster_eq_set_opamp_model_f32(eq, model);
        nx_ac_booster_eq_set_bass_control_f32(eq, clampControl(gainControl));
        nx_ac_booster_eq_set_treble_control_f32(eq, clampControl(secondaryControl));
        nx_ac_booster_eq_prepare_f32(eq, sampleRateHz);
        nx_ac_booster_eq_reset_f32(eq);

        for (int i = 0; i < 500; ++i)
            nx_ac_booster_eq_tick_f32(eq, 128);

        nx_ac_booster_eq_process_f32(eq, input.data(), output.data(), totalSamples);
        nx_ac_booster_eq_tick_f32(eq, totalSamples);

        vccOut = nx_ac_booster_eq_get_vcc_f32(eq);
        nx_ac_booster_eq_destroy_f32(eq, nullptr);
        return true;
    }

    if (circuit == CircuitKind::Ds1Clipper)
    {
        nx_ds1_clipper_f32_t* clipper = nx_ds1_clipper_create_f32(nullptr);
        if (clipper == nullptr)
            return false;

        nx_ds1_clipper_prepare_f32(clipper, sampleRateHz);
        nx_ds1_clipper_reset_f32(clipper);

        for (int i = 0; i < 500; ++i)
            nx_ds1_clipper_tick_f32(clipper, 128);

        nx_ds1_clipper_process_f32(clipper, input.data(), output.data(), totalSamples);
        nx_ds1_clipper_tick_f32(clipper, totalSamples);

        vccOut = kClipperPreviewVcc;
        nx_ds1_clipper_destroy_f32(clipper, nullptr);
        return true;
    }

    if (circuit == CircuitKind::DiodeClipper)
    {
        nx_diode_clipper_f32_t* clipper = nx_diode_clipper_create_f32(nullptr);
        if (clipper == nullptr)
            return false;

        nx_diode_clipper_prepare_f32(clipper, sampleRateHz);
        nx_diode_clipper_reset_f32(clipper);

        for (int i = 0; i < 500; ++i)
            nx_diode_clipper_tick_f32(clipper, 128);

        nx_diode_clipper_process_f32(clipper, input.data(), output.data(), totalSamples);
        nx_diode_clipper_tick_f32(clipper, totalSamples);

        vccOut = kClipperPreviewVcc;
        nx_diode_clipper_destroy_f32(clipper, nullptr);
        return true;
    }

    if (circuit == CircuitKind::RatClipper)
    {
        nx_rat_clipper_f32_t* clipper = nx_rat_clipper_create_f32(nullptr);
        if (clipper == nullptr)
            return false;

        applyRatClipperPotTaper(clipper, potTaper);
        nx_rat_clipper_set_filter_control_f32(clipper, clampControl(gainControl));
        nx_rat_clipper_prepare_f32(clipper, sampleRateHz);
        nx_rat_clipper_reset_f32(clipper);

        for (int i = 0; i < 500; ++i)
            nx_rat_clipper_tick_f32(clipper, 128);

        nx_rat_clipper_process_f32(clipper, input.data(), output.data(), totalSamples);
        nx_rat_clipper_tick_f32(clipper, totalSamples);

        vccOut = kClipperPreviewVcc;
        nx_rat_clipper_destroy_f32(clipper, nullptr);
        return true;
    }

    if (circuit == CircuitKind::Ts9Opamp)
    {
        nx_ts9_opamp_f32_t* opamp = nx_ts9_opamp_create_f32(nullptr);
        if (opamp == nullptr)
            return false;

        applyTs9OpampPotTaper(opamp, potTaper);
        nx_ts9_opamp_set_opamp_model_f32(opamp, model);
        nx_ts9_opamp_set_drive_control_f32(opamp, clampControl(gainControl));
        nx_ts9_opamp_prepare_f32(opamp, sampleRateHz);
        nx_ts9_opamp_reset_f32(opamp);

        for (int i = 0; i < 500; ++i)
            nx_ts9_opamp_tick_f32(opamp, 128);

        nx_ts9_opamp_process_f32(opamp, input.data(), output.data(), totalSamples);
        nx_ts9_opamp_tick_f32(opamp, totalSamples);

        vccOut = nx_ts9_opamp_get_vcc_f32(opamp);
        nx_ts9_opamp_destroy_f32(opamp, nullptr);
        return true;
    }

    if (circuit == CircuitKind::AcBoosterDrive)
    {
        nx_ac_booster_drive_f32_t* drive = nx_ac_booster_drive_create_f32(nullptr);
        if (drive == nullptr)
            return false;

        applyAcBoosterDrivePotTaper(drive, potTaper);
        nx_ac_booster_drive_set_opamp_model_f32(drive, model);
        nx_ac_booster_drive_set_gain_control_f32(drive, clampControl(gainControl));
        nx_ac_booster_drive_prepare_f32(drive, sampleRateHz);
        nx_ac_booster_drive_reset_f32(drive);

        for (int i = 0; i < 500; ++i)
            nx_ac_booster_drive_tick_f32(drive, 128);

        nx_ac_booster_drive_process_f32(drive, input.data(), output.data(), totalSamples);
        nx_ac_booster_drive_tick_f32(drive, totalSamples);

        vccOut = nx_ac_booster_drive_get_vcc_f32(drive);
        nx_ac_booster_drive_destroy_f32(drive, nullptr);
        return true;
    }

    if (circuit == CircuitKind::KlonCentaur)
    {
        nx_klon_centaur_f32_t* centaur = nx_klon_centaur_create_f32(nullptr);
        if (centaur == nullptr)
            return false;

        applyKlonCentaurPotTaper(centaur, potTaper);
        nx_klon_centaur_set_gain_control_f32(centaur, clampControl(gainControl));
        nx_klon_centaur_prepare_f32(centaur, sampleRateHz);
        nx_klon_centaur_reset_f32(centaur);

        for (int i = 0; i < 500; ++i)
            nx_klon_centaur_tick_f32(centaur, 128);

        nx_klon_centaur_process_f32(centaur, input.data(), output.data(), totalSamples);
        nx_klon_centaur_tick_f32(centaur, totalSamples);

        vccOut = nx_klon_centaur_get_vA_f32(centaur);
        nx_klon_centaur_destroy_f32(centaur, nullptr);
        return true;
    }

    if (circuit == CircuitKind::GuvnorClipper)
    {
        nx_guvnor_clipper_f32_t* clipper = nx_guvnor_clipper_create_f32(nullptr);
        if (clipper == nullptr)
            return false;

        applyGuvnorClipperPotTapers(clipper, potTaper);
        nx_guvnor_clipper_set_bass_control_f32(clipper, clampControl(gainControl));
        nx_guvnor_clipper_set_mid_control_f32(clipper, clampControl(secondaryControl));
        nx_guvnor_clipper_set_treble_control_f32(clipper, clampControl(tertiaryControl));
        nx_guvnor_clipper_prepare_f32(clipper, sampleRateHz);
        nx_guvnor_clipper_reset_f32(clipper);

        for (int i = 0; i < 500; ++i)
            nx_guvnor_clipper_tick_f32(clipper, 128);

        nx_guvnor_clipper_process_f32(clipper, input.data(), output.data(), totalSamples);
        nx_guvnor_clipper_tick_f32(clipper, totalSamples);

        vccOut = kClipperPreviewVcc;
        nx_guvnor_clipper_destroy_f32(clipper, nullptr);
        return true;
    }

    if (circuit == CircuitKind::BjtFollower)
    {
        nx_bjt_follower_f32_t* follower = nx_bjt_follower_create_f32(nullptr);
        if (follower == nullptr)
            return false;

        nx_bjt_follower_set_bjt_model_f32(follower, bjtModel);
        nx_bjt_follower_prepare_f32(follower, sampleRateHz);
        nx_bjt_follower_reset_f32(follower);

        for (int i = 0; i < 500; ++i)
            nx_bjt_follower_tick_f32(follower, 128);

        nx_bjt_follower_process_f32(follower, input.data(), output.data(), totalSamples);
        nx_bjt_follower_tick_f32(follower, totalSamples);

        vccOut = nx_bjt_follower_get_vcc_f32(follower);
        nx_bjt_follower_destroy_f32(follower, nullptr);
        return true;
    }

    if (circuit == CircuitKind::BjtFollowerOut)
    {
        nx_bjt_follower_out_f32_t* follower = nx_bjt_follower_out_create_f32(nullptr);
        if (follower == nullptr)
            return false;

        nx_bjt_follower_out_set_bjt_model_f32(follower, bjtModel);
        nx_bjt_follower_out_prepare_f32(follower, sampleRateHz);
        nx_bjt_follower_out_reset_f32(follower);

        for (int i = 0; i < 500; ++i)
            nx_bjt_follower_out_tick_f32(follower, 128);

        nx_bjt_follower_out_process_f32(follower, input.data(), output.data(), totalSamples);
        nx_bjt_follower_out_tick_f32(follower, totalSamples);

        vccOut = nx_bjt_follower_out_get_vcc_f32(follower);
        nx_bjt_follower_out_destroy_f32(follower, nullptr);
        return true;
    }

    if (circuit == CircuitKind::BjtCommonEmitter)
    {
        nx_bjt_common_emitter_f32_t* emitter = nx_bjt_common_emitter_create_f32(nullptr);
        if (emitter == nullptr)
            return false;

        nx_bjt_common_emitter_set_bjt_model_f32(emitter, bjtModel);
        nx_bjt_common_emitter_prepare_f32(emitter, sampleRateHz);
        nx_bjt_common_emitter_reset_f32(emitter);

        for (int i = 0; i < 500; ++i)
            nx_bjt_common_emitter_tick_f32(emitter, 128);

        nx_bjt_common_emitter_process_f32(emitter, input.data(), output.data(), totalSamples);
        nx_bjt_common_emitter_tick_f32(emitter, totalSamples);

        vccOut = nx_bjt_common_emitter_get_vcc_f32(emitter);
        nx_bjt_common_emitter_destroy_f32(emitter, nullptr);
        return true;
    }

    if (circuit == CircuitKind::JfetFollower)
    {
        nx_jfet_follower_f32_t* follower = nx_jfet_follower_create_f32(nullptr);
        if (follower == nullptr)
            return false;

        nx_jfet_follower_set_jfet_model_f32(follower, jfetModel);
        nx_jfet_follower_prepare_f32(follower, sampleRateHz);
        nx_jfet_follower_reset_f32(follower);

        for (int i = 0; i < 500; ++i)
            nx_jfet_follower_tick_f32(follower, 128);

        nx_jfet_follower_process_f32(follower, input.data(), output.data(), totalSamples);
        nx_jfet_follower_tick_f32(follower, totalSamples);

        vccOut = nx_jfet_follower_get_vd_f32(follower);
        nx_jfet_follower_destroy_f32(follower, nullptr);
        return true;
    }

    nx_ds1_opamp_f32_t* opamp = nx_ds1_opamp_create_f32(nullptr);
    if (opamp == nullptr)
        return false;

    applyDs1OpampPotTaper(opamp, potTaper);
    nx_ds1_opamp_set_opamp_model_f32(opamp, model);
    nx_ds1_opamp_set_gain_control_f32(opamp, clampControl(gainControl));
    nx_ds1_opamp_prepare_f32(opamp, sampleRateHz);
    nx_ds1_opamp_reset_f32(opamp);

    for (int i = 0; i < 500; ++i)
        nx_ds1_opamp_tick_f32(opamp, 128);

    nx_ds1_opamp_process_f32(opamp, input.data(), output.data(), totalSamples);
    nx_ds1_opamp_tick_f32(opamp, totalSamples);

    vccOut = nx_ds1_opamp_get_vcc_f32(opamp);
    nx_ds1_opamp_destroy_f32(opamp, nullptr);
    return true;
}
}  // namespace

AxisRange computeMagnitudeAxisEnvelope(CircuitKind circuit,
                                       nx_opamp_model_e model,
                                       nx_bjt_npn_model_e bjtModel,
                                       nx_jfet_n_model_e jfetModel,
                                       const AcSweepParams& params,
                                       double secondaryControl,
                                       double tertiaryControl,
                                       nx_pot_taper_e potTaper)
{
    juce::ignoreUnused(secondaryControl, tertiaryControl);

    AcSweepParams safe = params;
    safe.sanitise();

    const auto freqs = buildLogFrequencySweep(safe);
    const float logMin = static_cast<float>(std::log10(safe.freqMinHz));
    const float logMax = static_cast<float>(std::log10(safe.freqMaxHz));

    if (freqs.empty())
        return paddedMagnitudeAxis(0.0f, 0.0f, logMin, logMax);

    std::vector<double> magDb(freqs.size());
    std::vector<double> phaseDeg(freqs.size());

    float minMag = std::numeric_limits<float>::max();
    float maxMag = std::numeric_limits<float>::lowest();

    sweepPrimaryControlEnvelope(circuit,
                                model,
                                bjtModel,
                                jfetModel,
                                secondaryControl,
                                tertiaryControl,
                                potTaper,
                                safe.sampleRateHz,
                                freqs,
                                magDb,
                                phaseDeg,
                                minMag,
                                maxMag);

    if (minMag > maxMag)
        return paddedMagnitudeAxis(0.0f, 0.0f, logMin, logMax);

    return paddedMagnitudeAxis(minMag, maxMag, logMin, logMax);
}

AcResponse computeAcResponse(CircuitKind circuit,
                             nx_opamp_model_e model,
                             nx_bjt_npn_model_e bjtModel,
                             nx_jfet_n_model_e jfetModel,
                             double gainControl,
                             const AcSweepParams& params,
                             const AxisRange& magnitudeAxis,
                             double secondaryControl,
                             double tertiaryControl,
                             nx_pot_taper_e potTaper)
{
    AcSweepParams safe = params;
    safe.sanitise();

    AcResponse response;
    response.title = makeResponseTitle(circuit, model, bjtModel, jfetModel, gainControl, secondaryControl, tertiaryControl);

    const auto freqs = buildLogFrequencySweep(safe);
    if (freqs.empty())
        return response;

    std::vector<double> magDb(freqs.size());
    std::vector<double> phaseDeg(freqs.size());

    if (!runAcResponseSweep(circuit,
                            model,
                            bjtModel,
                            jfetModel,
                            gainControl,
                            secondaryControl,
                            tertiaryControl,
                            potTaper,
                            safe.sampleRateHz,
                            freqs,
                            magDb,
                            phaseDeg))
    {
        return response;
    }

    response.magnitudeCurve.reserve(freqs.size());
    response.phaseCurve.reserve(freqs.size());

    const float logMin = static_cast<float>(std::log10(safe.freqMinHz));
    const float logMax = static_cast<float>(std::log10(safe.freqMaxHz));

    for (size_t i = 0; i < freqs.size(); ++i)
    {
        const float logFreq = static_cast<float>(std::log10(freqs[i]));
        response.magnitudeCurve.emplace_back(logFreq, static_cast<float>(magDb[i]));
        response.phaseCurve.emplace_back(logFreq, static_cast<float>(phaseDeg[i]));
    }

    response.magnitudeAxis = magnitudeAxis;
    response.magnitudeAxis.minX = logMin;
    response.magnitudeAxis.maxX = logMax;
    response.phaseAxis = computePhaseAxis(response.phaseCurve, logMin, logMax);

    return response;
}

SineWavePreview computeSineWavePreview(CircuitKind circuit,
                                       nx_opamp_model_e model,
                                       nx_bjt_npn_model_e bjtModel,
                                       nx_jfet_n_model_e jfetModel,
                                       double gainControl,
                                       double freqHz,
                                       const AcSweepParams& params,
                                       double secondaryControl,
                                       double tertiaryControl,
                                       nx_pot_taper_e potTaper)
{
    AcSweepParams safe = params;
    safe.sanitise();

    SineWavePreview preview;
    const double clampedFreq = juce::jlimit(kPreviewFreqMinHz, kPreviewFreqMaxHz, freqHz);
    preview.title = juce::String(clampedFreq, clampedFreq >= 1000.0 ? 1 : 0) + " Hz";

    const float inputScale = previewInputScale(circuit, gainControl);
    const int periodSamples =
        juce::jmax(8, static_cast<int>(std::llround(safe.sampleRateHz / clampedFreq)));
    const int totalSamples = periodSamples * (kWarmupPeriods + 1);
    std::vector<float> input(static_cast<size_t>(totalSamples));
    std::vector<float> output(static_cast<size_t>(totalSamples));

    for (int period = 0; period <= kWarmupPeriods; ++period)
    {
        const size_t offset = static_cast<size_t>(period * periodSamples);
        for (int i = 0; i < periodSamples; ++i)
        {
            const double phase = juce::MathConstants<double>::twoPi * static_cast<double>(i)
                               / static_cast<double>(periodSamples);
            input[offset + static_cast<size_t>(i)] =
                inputScale * static_cast<float>(std::sin(phase));
        }
    }

    double vcc = 9.0;
    if (!runSineWavePreviewProcess(circuit,
                                   model,
                                   bjtModel,
                                   jfetModel,
                                   gainControl,
                                   secondaryControl,
                                   tertiaryControl,
                                   potTaper,
                                   safe.sampleRateHz,
                                   input,
                                   output,
                                   static_cast<size_t>(totalSamples),
                                   vcc))
    {
        return preview;
    }

    preview.vccHalf = static_cast<float>(vcc * 0.5);
    constexpr float kEndpointPadFraction = 0.02f;
    const float endpointPad = preview.vccHalf * kEndpointPadFraction;
    preview.axis.minY = -preview.vccHalf - endpointPad;
    preview.axis.maxY = preview.vccHalf + endpointPad;

    const size_t lastPeriodStart = static_cast<size_t>(periodSamples * kWarmupPeriods);
    const std::vector<float> lastPeriodOutput(output.begin() + static_cast<std::ptrdiff_t>(lastPeriodStart),
                                              output.end());

    preview.outputCurve.reserve(static_cast<size_t>(kPreviewDisplayPoints));

    for (int i = 0; i < kPreviewDisplayPoints; ++i)
    {
        const float t = static_cast<float>(i) / static_cast<float>(kPreviewDisplayPoints - 1);
        const float srcIndex = t * static_cast<float>(periodSamples - 1);
        const float outY = lerpWaveSample(lastPeriodOutput, srcIndex);
        preview.outputCurve.emplace_back(t, outY);
    }

    preview.axis.minX = 0.0f;
    preview.axis.maxX = 1.0f;

    return preview;
}

}  // namespace ds1_ac
