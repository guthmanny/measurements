#include "SineWavePreviewEngine.h"

#include "SchematicComponentApply.h"

namespace ds1_ac
{
namespace
{

constexpr double kFallbackPreviewVcc = 9.0;

double clampControl(double control) noexcept
{
    return juce::jlimit(0.0, 1.0, control);
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

template<typename Inst>
void applySchematicValues(CircuitKind circuit, Inst* inst, const SchematicComponentValues* values)
{
    if (inst != nullptr && values != nullptr && ! values->empty())
        applySchematicComponentValues(circuit, inst, values);
}

} // namespace

struct SineWavePreviewEngine::StructuralKey
{
    CircuitKind circuit{CircuitKind::BjtFollower};
    nx_opamp_model_e model{NX_OPAMP_BA728};
    nx_diode_model_t diodeModel{NX_DIODE_1N4148};
    nx_bjt_npn_model_e bjtModel{NX_BJT_2N3904};
    nx_jfet_n_model_e jfetModel{NX_JFET_2N5457};
    double gainControl{0.5};
    double secondaryControl{0.5};
    double tertiaryControl{0.5};
    nx_pot_taper_e potTaper{NX_POT_TAPER_LINEAR};
    double sampleRateHz{kDefaultSampleRateHz};
    SchematicComponentValues schematicValues{};

    bool operator==(const StructuralKey& other) const noexcept
    {
        return circuit == other.circuit && model == other.model && diodeModel == other.diodeModel
               && bjtModel == other.bjtModel && jfetModel == other.jfetModel
               && juce::approximatelyEqual(gainControl, other.gainControl)
               && juce::approximatelyEqual(secondaryControl, other.secondaryControl)
               && juce::approximatelyEqual(tertiaryControl, other.tertiaryControl) && potTaper == other.potTaper
               && juce::approximatelyEqual(sampleRateHz, other.sampleRateHz)
               && schematicValues == other.schematicValues;
    }
};

SineWavePreviewEngine::~SineWavePreviewEngine()
{
    destroyInstance();
    delete key_;
    key_ = nullptr;
}

void SineWavePreviewEngine::invalidate() noexcept
{
    destroyInstance();
    delete key_;
    key_ = nullptr;
}

void SineWavePreviewEngine::destroyInstance() noexcept
{
    if (ds1Clipper_ != nullptr)
    {
        nx_ds1_clipper_destroy_f32(ds1Clipper_, nullptr);
        ds1Clipper_ = nullptr;
    }
    if (bjtFollower_ != nullptr)
    {
        nx_bjt_follower_destroy_f32(bjtFollower_, nullptr);
        bjtFollower_ = nullptr;
    }
    if (bjtFollowerOut_ != nullptr)
    {
        nx_bjt_follower_out_destroy_f32(bjtFollowerOut_, nullptr);
        bjtFollowerOut_ = nullptr;
    }
    if (bjtCommonEmitter_ != nullptr)
    {
        nx_bjt_common_emitter_destroy_f32(bjtCommonEmitter_, nullptr);
        bjtCommonEmitter_ = nullptr;
    }
    if (ds1Opamp_ != nullptr)
    {
        nx_ds1_opamp_destroy_f32(ds1Opamp_, nullptr);
        ds1Opamp_ = nullptr;
    }
    if (od1Drive_ != nullptr)
    {
        nx_od1_drive_destroy_f32(od1Drive_, nullptr);
        od1Drive_ = nullptr;
    }
    if (sd1Tone_ != nullptr)
    {
        nx_sd1_tone_destroy_f32(sd1Tone_, nullptr);
        sd1Tone_ = nullptr;
    }
    if (rcLevel_ != nullptr)
    {
        nx_rc_level_destroy_f32(rcLevel_, nullptr);
        rcLevel_ = nullptr;
    }
    if (od1Post_ != nullptr)
    {
        nx_od1_post_destroy_f32(od1Post_, nullptr);
        od1Post_ = nullptr;
    }
    if (acBoosterDrive_ != nullptr)
    {
        nx_ac_booster_drive_destroy_f32(acBoosterDrive_, nullptr);
        acBoosterDrive_ = nullptr;
    }
    if (acBoosterEq_ != nullptr)
    {
        nx_ac_booster_eq_destroy_f32(acBoosterEq_, nullptr);
        acBoosterEq_ = nullptr;
    }
    if (rcBoosterDrive1_ != nullptr)
    {
        nx_rc_booster_drive1_destroy_f32(rcBoosterDrive1_, nullptr);
        rcBoosterDrive1_ = nullptr;
    }
    if (dsPlusOpamp_ != nullptr)
    {
        nx_ds_plus_opamp_destroy_f32(dsPlusOpamp_, nullptr);
        dsPlusOpamp_ = nullptr;
    }
    if (ts9Opamp_ != nullptr)
    {
        nx_ts9_opamp_destroy_f32(ts9Opamp_, nullptr);
        ts9Opamp_ = nullptr;
    }
    if (ts9Tone_ != nullptr)
    {
        nx_ts9_tone_destroy_f32(ts9Tone_, nullptr);
        ts9Tone_ = nullptr;
    }
    if (klonCentaur_ != nullptr)
    {
        nx_klon_centaur_destroy_f32(klonCentaur_, nullptr);
        klonCentaur_ = nullptr;
    }
    if (klonCentaurTone_ != nullptr)
    {
        nx_klon_centaur_tone_destroy_f32(klonCentaurTone_, nullptr);
        klonCentaurTone_ = nullptr;
    }
    if (guvnorPreamp_ != nullptr)
    {
        nx_guvnor_preamp_destroy_f32(guvnorPreamp_, nullptr);
        guvnorPreamp_ = nullptr;
    }
    if (guvnorPostamp_ != nullptr)
    {
        nx_guvnor_postamp_destroy_f32(guvnorPostamp_, nullptr);
        guvnorPostamp_ = nullptr;
    }
    if (guvnorClipper_ != nullptr)
    {
        nx_guvnor_clipper_destroy_f32(guvnorClipper_, nullptr);
        guvnorClipper_ = nullptr;
    }
    if (guvnorLevel_ != nullptr)
    {
        nx_guvnor_level_destroy_f32(guvnorLevel_, nullptr);
        guvnorLevel_ = nullptr;
    }
    if (diodeClipper_ != nullptr)
    {
        nx_diode_clipper_destroy_f32(diodeClipper_, nullptr);
        diodeClipper_ = nullptr;
    }
}

bool SineWavePreviewEngine::ensureStructuralReady(const SinePreviewSetupParams& setup)
{
    StructuralKey candidate;
    candidate.circuit = setup.circuit;
    candidate.model = setup.model;
    candidate.diodeModel = setup.diodeModel;
    candidate.bjtModel = setup.bjtModel;
    candidate.jfetModel = setup.jfetModel;
    candidate.gainControl = setup.gainControl;
    candidate.secondaryControl = setup.secondaryControl;
    candidate.tertiaryControl = setup.tertiaryControl;
    candidate.potTaper = setup.potTaper;
    candidate.sampleRateHz = setup.sampleRateHz;
    if (setup.componentValues != nullptr)
        candidate.schematicValues = *setup.componentValues;

    if (key_ != nullptr && *key_ == candidate && circuit_ == setup.circuit)
        return true;

    destroyInstance();
    circuit_ = setup.circuit;

    switch (setup.circuit)
    {
    case CircuitKind::Ds1Clipper:
        ds1Clipper_ = nx_ds1_clipper_create_f32(nullptr);
        if (ds1Clipper_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, ds1Clipper_, setup.componentValues);
        updatePotTaper(ds1Clipper_, nx_ds1_clipper_get_tone_pot_f32, nx_ds1_clipper_set_tone_pot_f32, setup.potTaper);
        updatePotTaper(ds1Clipper_, nx_ds1_clipper_get_level_pot_f32, nx_ds1_clipper_set_level_pot_f32, setup.potTaper);
        nx_ds1_clipper_set_diode_model_f32(ds1Clipper_, setup.diodeModel);
        nx_ds1_clipper_set_tone_control_f32(ds1Clipper_, clampControl(setup.gainControl));
        nx_ds1_clipper_set_level_control_f32(ds1Clipper_, clampControl(setup.secondaryControl));
        nx_ds1_clipper_prepare_f32(ds1Clipper_, setup.sampleRateHz);
        nx_ds1_clipper_reset_f32(ds1Clipper_);
        break;

    case CircuitKind::BjtFollower:
        bjtFollower_ = nx_bjt_follower_create_f32(nullptr);
        if (bjtFollower_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, bjtFollower_, setup.componentValues);
        nx_bjt_follower_set_bjt_model_f32(bjtFollower_, setup.bjtModel);
        nx_bjt_follower_prepare_f32(bjtFollower_, setup.sampleRateHz);
        nx_bjt_follower_reset_f32(bjtFollower_);
        break;

    case CircuitKind::BjtFollowerOut:
        bjtFollowerOut_ = nx_bjt_follower_out_create_f32(nullptr);
        if (bjtFollowerOut_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, bjtFollowerOut_, setup.componentValues);
        nx_bjt_follower_out_set_bjt_model_f32(bjtFollowerOut_, setup.bjtModel);
        nx_bjt_follower_out_prepare_f32(bjtFollowerOut_, setup.sampleRateHz);
        nx_bjt_follower_out_reset_f32(bjtFollowerOut_);
        break;

    case CircuitKind::BjtCommonEmitter:
        bjtCommonEmitter_ = nx_bjt_common_emitter_create_f32(nullptr);
        if (bjtCommonEmitter_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, bjtCommonEmitter_, setup.componentValues);
        nx_bjt_common_emitter_set_bjt_model_f32(bjtCommonEmitter_, setup.bjtModel);
        nx_bjt_common_emitter_prepare_f32(bjtCommonEmitter_, setup.sampleRateHz);
        nx_bjt_common_emitter_reset_f32(bjtCommonEmitter_);
        break;

    case CircuitKind::Od1Drive:
        od1Drive_ = nx_od1_drive_create_f32(nullptr);
        if (od1Drive_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, od1Drive_, setup.componentValues);
        updatePotTaper(od1Drive_, nx_od1_drive_get_drive_pot_f32, nx_od1_drive_set_drive_pot_f32, setup.potTaper);
        nx_od1_drive_set_opamp_model_f32(od1Drive_, setup.model);
        nx_od1_drive_set_diode_model_f32(od1Drive_, setup.diodeModel);
        nx_od1_drive_set_drive_control_f32(od1Drive_, clampControl(setup.gainControl));
        nx_od1_drive_prepare_f32(od1Drive_, setup.sampleRateHz);
        nx_od1_drive_reset_f32(od1Drive_);
        break;

    case CircuitKind::Sd1Tone:
        sd1Tone_ = nx_sd1_tone_create_f32(nullptr);
        if (sd1Tone_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, sd1Tone_, setup.componentValues);
        updatePotTaper(sd1Tone_, nx_sd1_tone_get_tone_pot_f32, nx_sd1_tone_set_tone_pot_f32, setup.potTaper);
        nx_sd1_tone_set_opamp_model_f32(sd1Tone_, setup.model);
        nx_sd1_tone_set_tone_control_f32(sd1Tone_, clampControl(setup.gainControl));
        nx_sd1_tone_prepare_f32(sd1Tone_, setup.sampleRateHz);
        nx_sd1_tone_reset_f32(sd1Tone_);
        break;

    case CircuitKind::RcLevel:
        rcLevel_ = nx_rc_level_create_f32(nullptr);
        if (rcLevel_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, rcLevel_, setup.componentValues);
        updatePotTaper(rcLevel_, nx_rc_level_get_level_pot_f32, nx_rc_level_set_level_pot_f32, setup.potTaper);
        nx_rc_level_set_level_control_f32(rcLevel_, clampControl(setup.gainControl));
        nx_rc_level_prepare_f32(rcLevel_, setup.sampleRateHz);
        nx_rc_level_reset_f32(rcLevel_);
        break;

    case CircuitKind::Od1Post:
        od1Post_ = nx_od1_post_create_f32(nullptr);
        if (od1Post_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, od1Post_, setup.componentValues);
        nx_od1_post_set_opamp_model_f32(od1Post_, setup.model);
        nx_od1_post_prepare_f32(od1Post_, setup.sampleRateHz);
        nx_od1_post_reset_f32(od1Post_);
        break;

    case CircuitKind::AcBoosterDrive:
        acBoosterDrive_ = nx_ac_booster_drive_create_f32(nullptr);
        if (acBoosterDrive_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, acBoosterDrive_, setup.componentValues);
        updatePotTaper(acBoosterDrive_, nx_ac_booster_drive_get_gain_pot_f32, nx_ac_booster_drive_set_gain_pot_f32, setup.potTaper);
        nx_ac_booster_drive_set_opamp_model_f32(acBoosterDrive_, setup.model);
        nx_ac_booster_drive_set_diode_model_f32(acBoosterDrive_, setup.diodeModel);
        nx_ac_booster_drive_set_gain_control_f32(acBoosterDrive_, clampControl(setup.gainControl));
        nx_ac_booster_drive_prepare_f32(acBoosterDrive_, setup.sampleRateHz);
        nx_ac_booster_drive_reset_f32(acBoosterDrive_);
        break;

    case CircuitKind::AcBoosterEq:
        acBoosterEq_ = nx_ac_booster_eq_create_f32(nullptr);
        if (acBoosterEq_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, acBoosterEq_, setup.componentValues);
        updatePotTaper(acBoosterEq_, nx_ac_booster_eq_get_bass_pot_f32, nx_ac_booster_eq_set_bass_pot_f32, setup.potTaper);
        updatePotTaper(acBoosterEq_, nx_ac_booster_eq_get_treble_pot_f32, nx_ac_booster_eq_set_treble_pot_f32, setup.potTaper);
        nx_ac_booster_eq_set_opamp_model_f32(acBoosterEq_, setup.model);
        nx_ac_booster_eq_set_bass_control_f32(acBoosterEq_, clampControl(setup.gainControl));
        nx_ac_booster_eq_set_treble_control_f32(acBoosterEq_, clampControl(setup.secondaryControl));
        nx_ac_booster_eq_prepare_f32(acBoosterEq_, setup.sampleRateHz);
        nx_ac_booster_eq_reset_f32(acBoosterEq_);
        break;

    case CircuitKind::RcBoosterDrive1:
        rcBoosterDrive1_ = nx_rc_booster_drive1_create_f32(nullptr);
        if (rcBoosterDrive1_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, rcBoosterDrive1_, setup.componentValues);
        updatePotTaper(rcBoosterDrive1_, nx_rc_booster_drive1_get_gain_pot_f32, nx_rc_booster_drive1_set_gain_pot_f32, setup.potTaper);
        nx_rc_booster_drive1_set_opamp_model_f32(rcBoosterDrive1_, setup.model);
        nx_rc_booster_drive1_set_diode_model_f32(rcBoosterDrive1_, setup.diodeModel);
        nx_rc_booster_drive1_set_gain_control_f32(rcBoosterDrive1_, clampControl(setup.gainControl));
        nx_rc_booster_drive1_prepare_f32(rcBoosterDrive1_, setup.sampleRateHz);
        nx_rc_booster_drive1_reset_f32(rcBoosterDrive1_);
        break;

    case CircuitKind::DsPlusOpamp:
        dsPlusOpamp_ = nx_ds_plus_opamp_create_f32(nullptr);
        if (dsPlusOpamp_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, dsPlusOpamp_, setup.componentValues);
        updatePotTaper(dsPlusOpamp_, nx_ds_plus_opamp_get_distortion_pot_f32, nx_ds_plus_opamp_set_distortion_pot_f32, setup.potTaper);
        nx_ds_plus_opamp_set_opamp_model_f32(dsPlusOpamp_, setup.model);
        nx_ds_plus_opamp_set_distortion_control_f32(dsPlusOpamp_, clampControl(setup.gainControl));
        nx_ds_plus_opamp_prepare_f32(dsPlusOpamp_, setup.sampleRateHz);
        nx_ds_plus_opamp_reset_f32(dsPlusOpamp_);
        break;

    case CircuitKind::Ts9Opamp:
        ts9Opamp_ = nx_ts9_opamp_create_f32(nullptr);
        if (ts9Opamp_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, ts9Opamp_, setup.componentValues);
        updatePotTaper(ts9Opamp_, nx_ts9_opamp_get_drive_pot_f32, nx_ts9_opamp_set_drive_pot_f32, setup.potTaper);
        nx_ts9_opamp_set_opamp_model_f32(ts9Opamp_, setup.model);
        nx_ts9_opamp_set_diode_model_f32(ts9Opamp_, setup.diodeModel);
        nx_ts9_opamp_set_drive_control_f32(ts9Opamp_, clampControl(setup.gainControl));
        nx_ts9_opamp_prepare_f32(ts9Opamp_, setup.sampleRateHz);
        nx_ts9_opamp_reset_f32(ts9Opamp_);
        break;

    case CircuitKind::Ts9Tone:
        ts9Tone_ = nx_ts9_tone_create_f32(nullptr);
        if (ts9Tone_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, ts9Tone_, setup.componentValues);
        updatePotTaper(ts9Tone_, nx_ts9_tone_get_tone_pot_f32, nx_ts9_tone_set_tone_pot_f32, setup.potTaper);
        nx_ts9_tone_set_opamp_model_f32(ts9Tone_, setup.model);
        nx_ts9_tone_set_tone_control_f32(ts9Tone_, clampControl(setup.gainControl));
        nx_ts9_tone_prepare_f32(ts9Tone_, setup.sampleRateHz);
        nx_ts9_tone_reset_f32(ts9Tone_);
        break;

    case CircuitKind::KlonCentaur:
        klonCentaur_ = nx_klon_centaur_create_f32(nullptr);
        if (klonCentaur_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, klonCentaur_, setup.componentValues);
        updatePotTaper(klonCentaur_, nx_klon_centaur_get_gain_pot_f32, nx_klon_centaur_set_gain_pot_f32, setup.potTaper);
        nx_klon_centaur_set_opamp_model_f32(klonCentaur_, setup.model);
        nx_klon_centaur_set_diode_model_f32(klonCentaur_, setup.diodeModel);
        nx_klon_centaur_set_gain_control_f32(klonCentaur_, clampControl(setup.gainControl));
        nx_klon_centaur_prepare_f32(klonCentaur_, setup.sampleRateHz);
        nx_klon_centaur_reset_f32(klonCentaur_);
        break;

    case CircuitKind::KlonCentaurTone:
        klonCentaurTone_ = nx_klon_centaur_tone_create_f32(nullptr);
        if (klonCentaurTone_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, klonCentaurTone_, setup.componentValues);
        updatePotTaper(klonCentaurTone_, nx_klon_centaur_tone_get_treble_pot_f32, nx_klon_centaur_tone_set_treble_pot_f32, setup.potTaper);
        nx_klon_centaur_tone_set_opamp_model_f32(klonCentaurTone_, setup.model);
        nx_klon_centaur_tone_set_treble_control_f32(klonCentaurTone_, clampControl(setup.gainControl));
        nx_klon_centaur_tone_prepare_f32(klonCentaurTone_, setup.sampleRateHz);
        nx_klon_centaur_tone_reset_f32(klonCentaurTone_);
        break;

    case CircuitKind::GuvnorPreamp:
        guvnorPreamp_ = nx_guvnor_preamp_create_f32(nullptr);
        if (guvnorPreamp_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, guvnorPreamp_, setup.componentValues);
        updatePotTaper(guvnorPreamp_, nx_guvnor_preamp_get_gain_pot_f32, nx_guvnor_preamp_set_gain_pot_f32, setup.potTaper);
        nx_guvnor_preamp_set_opamp_model_f32(guvnorPreamp_, setup.model);
        nx_guvnor_preamp_set_gain_control_f32(guvnorPreamp_, clampControl(setup.gainControl));
        nx_guvnor_preamp_prepare_f32(guvnorPreamp_, setup.sampleRateHz);
        nx_guvnor_preamp_reset_f32(guvnorPreamp_);
        break;

    case CircuitKind::GuvnorPostamp:
        guvnorPostamp_ = nx_guvnor_postamp_create_f32(nullptr);
        if (guvnorPostamp_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, guvnorPostamp_, setup.componentValues);
        updatePotTaper(guvnorPostamp_, nx_guvnor_postamp_get_gain_pot_f32, nx_guvnor_postamp_set_gain_pot_f32, setup.potTaper);
        nx_guvnor_postamp_set_opamp_model_f32(guvnorPostamp_, setup.model);
        nx_guvnor_postamp_set_gain_control_f32(guvnorPostamp_, clampControl(setup.gainControl));
        nx_guvnor_postamp_prepare_f32(guvnorPostamp_, setup.sampleRateHz);
        nx_guvnor_postamp_reset_f32(guvnorPostamp_);
        break;

    case CircuitKind::GuvnorClipper:
        guvnorClipper_ = nx_guvnor_clipper_create_f32(nullptr);
        if (guvnorClipper_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, guvnorClipper_, setup.componentValues);
        updatePotTaper(guvnorClipper_, nx_guvnor_clipper_get_bass_pot_f32, nx_guvnor_clipper_set_bass_pot_f32, setup.potTaper);
        updatePotTaper(guvnorClipper_, nx_guvnor_clipper_get_mid_pot_f32, nx_guvnor_clipper_set_mid_pot_f32, setup.potTaper);
        updatePotTaper(guvnorClipper_, nx_guvnor_clipper_get_treble_pot_f32, nx_guvnor_clipper_set_treble_pot_f32, setup.potTaper);
        nx_guvnor_clipper_set_diode_model_f32(guvnorClipper_, setup.diodeModel);
        nx_guvnor_clipper_set_bass_control_f32(guvnorClipper_, clampControl(setup.gainControl));
        nx_guvnor_clipper_set_mid_control_f32(guvnorClipper_, clampControl(setup.secondaryControl));
        nx_guvnor_clipper_set_treble_control_f32(guvnorClipper_, clampControl(setup.tertiaryControl));
        nx_guvnor_clipper_prepare_f32(guvnorClipper_, setup.sampleRateHz);
        nx_guvnor_clipper_reset_f32(guvnorClipper_);
        break;

    case CircuitKind::GuvnorLevel:
        guvnorLevel_ = nx_guvnor_level_create_f32(nullptr);
        if (guvnorLevel_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, guvnorLevel_, setup.componentValues);
        updatePotTaper(guvnorLevel_, nx_guvnor_level_get_level_pot_f32, nx_guvnor_level_set_level_pot_f32, setup.potTaper);
        nx_guvnor_level_set_level_control_f32(guvnorLevel_, clampControl(setup.gainControl));
        nx_guvnor_level_prepare_f32(guvnorLevel_, setup.sampleRateHz);
        nx_guvnor_level_reset_f32(guvnorLevel_);
        break;

    case CircuitKind::DiodeClipper:
        diodeClipper_ = nx_diode_clipper_create_f32(nullptr);
        if (diodeClipper_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, diodeClipper_, setup.componentValues);
        updatePotTaper(diodeClipper_, nx_diode_clipper_get_level_pot_f32, nx_diode_clipper_set_level_pot_f32, setup.potTaper);
        nx_diode_clipper_set_diode_model_f32(diodeClipper_, setup.diodeModel);
        nx_diode_clipper_set_level_control_f32(diodeClipper_, clampControl(setup.gainControl));
        nx_diode_clipper_prepare_f32(diodeClipper_, setup.sampleRateHz);
        nx_diode_clipper_reset_f32(diodeClipper_);
        break;

    case CircuitKind::Ds1Opamp:
    default:
        ds1Opamp_ = nx_ds1_opamp_create_f32(nullptr);
        if (ds1Opamp_ == nullptr)
            return false;
        applySchematicValues(CircuitKind::Ds1Opamp, ds1Opamp_, setup.componentValues);
        updatePotTaper(ds1Opamp_, nx_ds1_opamp_get_gain_pot_f32, nx_ds1_opamp_set_gain_pot_f32, setup.potTaper);
        nx_ds1_opamp_set_opamp_model_f32(ds1Opamp_, setup.model);
        nx_ds1_opamp_set_gain_control_f32(ds1Opamp_, clampControl(setup.gainControl));
        nx_ds1_opamp_prepare_f32(ds1Opamp_, setup.sampleRateHz);
        nx_ds1_opamp_reset_f32(ds1Opamp_);
        circuit_ = CircuitKind::Ds1Opamp;
        break;
    }

    if (key_ == nullptr)
        key_ = new StructuralKey();
    *key_ = candidate;
    return true;
}

bool SineWavePreviewEngine::resetStates() noexcept
{
    switch (circuit_)
    {
    case CircuitKind::Ds1Clipper:
        if (ds1Clipper_ == nullptr)
            return false;
        nx_ds1_clipper_reset_f32(ds1Clipper_);
        return true;
    case CircuitKind::BjtFollower:
        if (bjtFollower_ == nullptr)
            return false;
        nx_bjt_follower_reset_f32(bjtFollower_);
        return true;
    case CircuitKind::BjtFollowerOut:
        if (bjtFollowerOut_ == nullptr)
            return false;
        nx_bjt_follower_out_reset_f32(bjtFollowerOut_);
        return true;
    case CircuitKind::BjtCommonEmitter:
        if (bjtCommonEmitter_ == nullptr)
            return false;
        nx_bjt_common_emitter_reset_f32(bjtCommonEmitter_);
        return true;
    case CircuitKind::Od1Drive:
        if (od1Drive_ == nullptr)
            return false;
        nx_od1_drive_reset_f32(od1Drive_);
        return true;
    case CircuitKind::Sd1Tone:
        if (sd1Tone_ == nullptr)
            return false;
        nx_sd1_tone_reset_f32(sd1Tone_);
        return true;
    case CircuitKind::RcLevel:
        if (rcLevel_ == nullptr)
            return false;
        nx_rc_level_reset_f32(rcLevel_);
        return true;
    case CircuitKind::Od1Post:
        if (od1Post_ == nullptr)
            return false;
        nx_od1_post_reset_f32(od1Post_);
        return true;
    case CircuitKind::AcBoosterDrive:
        if (acBoosterDrive_ == nullptr)
            return false;
        nx_ac_booster_drive_reset_f32(acBoosterDrive_);
        return true;
    case CircuitKind::AcBoosterEq:
        if (acBoosterEq_ == nullptr)
            return false;
        nx_ac_booster_eq_reset_f32(acBoosterEq_);
        return true;
    case CircuitKind::RcBoosterDrive1:
        if (rcBoosterDrive1_ == nullptr)
            return false;
        nx_rc_booster_drive1_reset_f32(rcBoosterDrive1_);
        return true;
    case CircuitKind::DsPlusOpamp:
        if (dsPlusOpamp_ == nullptr)
            return false;
        nx_ds_plus_opamp_reset_f32(dsPlusOpamp_);
        return true;
    case CircuitKind::Ts9Opamp:
        if (ts9Opamp_ == nullptr)
            return false;
        nx_ts9_opamp_reset_f32(ts9Opamp_);
        return true;
    case CircuitKind::Ts9Tone:
        if (ts9Tone_ == nullptr)
            return false;
        nx_ts9_tone_reset_f32(ts9Tone_);
        return true;
    case CircuitKind::KlonCentaur:
        if (klonCentaur_ == nullptr)
            return false;
        nx_klon_centaur_reset_f32(klonCentaur_);
        return true;
    case CircuitKind::KlonCentaurTone:
        if (klonCentaurTone_ == nullptr)
            return false;
        nx_klon_centaur_tone_reset_f32(klonCentaurTone_);
        return true;
    case CircuitKind::GuvnorPreamp:
        if (guvnorPreamp_ == nullptr)
            return false;
        nx_guvnor_preamp_reset_f32(guvnorPreamp_);
        return true;
    case CircuitKind::GuvnorPostamp:
        if (guvnorPostamp_ == nullptr)
            return false;
        nx_guvnor_postamp_reset_f32(guvnorPostamp_);
        return true;
    case CircuitKind::GuvnorClipper:
        if (guvnorClipper_ == nullptr)
            return false;
        nx_guvnor_clipper_reset_f32(guvnorClipper_);
        return true;
    case CircuitKind::GuvnorLevel:
        if (guvnorLevel_ == nullptr)
            return false;
        nx_guvnor_level_reset_f32(guvnorLevel_);
        return true;
    case CircuitKind::DiodeClipper:
        if (diodeClipper_ == nullptr)
            return false;
        nx_diode_clipper_reset_f32(diodeClipper_);
        return true;
    case CircuitKind::Ds1Opamp:
        if (ds1Opamp_ == nullptr)
            return false;
        nx_ds1_opamp_reset_f32(ds1Opamp_);
        return true;
    }

    return false;
}

bool SineWavePreviewEngine::runHotProcess(const float* input,
                                          float* output,
                                          std::size_t totalSamples,
                                          double& vccOut) noexcept
{
    switch (circuit_)
    {
    case CircuitKind::Ds1Clipper:
        if (ds1Clipper_ == nullptr)
            return false;
        nx_ds1_clipper_process_f32(ds1Clipper_, input, output, totalSamples);
        vccOut = kFallbackPreviewVcc;
        return true;
    case CircuitKind::BjtFollower:
        if (bjtFollower_ == nullptr)
            return false;
        nx_bjt_follower_process_f32(bjtFollower_, input, output, totalSamples);
        vccOut = nx_bjt_follower_get_vcc_f32(bjtFollower_);
        return true;
    case CircuitKind::BjtFollowerOut:
        if (bjtFollowerOut_ == nullptr)
            return false;
        nx_bjt_follower_out_process_f32(bjtFollowerOut_, input, output, totalSamples);
        vccOut = nx_bjt_follower_out_get_vcc_f32(bjtFollowerOut_);
        return true;
    case CircuitKind::BjtCommonEmitter:
        if (bjtCommonEmitter_ == nullptr)
            return false;
        nx_bjt_common_emitter_process_f32(bjtCommonEmitter_, input, output, totalSamples);
        vccOut = nx_bjt_common_emitter_get_vcc_f32(bjtCommonEmitter_);
        return true;
    case CircuitKind::Od1Drive:
        if (od1Drive_ == nullptr)
            return false;
        nx_od1_drive_process_f32(od1Drive_, input, output, totalSamples);
        vccOut = nx_od1_drive_get_vcc_f32(od1Drive_);
        return true;
    case CircuitKind::Sd1Tone:
        if (sd1Tone_ == nullptr)
            return false;
        nx_sd1_tone_process_f32(sd1Tone_, input, output, totalSamples);
        vccOut = nx_sd1_tone_get_vcc_f32(sd1Tone_);
        return true;
    case CircuitKind::RcLevel:
        if (rcLevel_ == nullptr)
            return false;
        nx_rc_level_process_f32(rcLevel_, input, output, totalSamples);
        vccOut = kFallbackPreviewVcc;
        return true;
    case CircuitKind::Od1Post:
        if (od1Post_ == nullptr)
            return false;
        nx_od1_post_process_f32(od1Post_, input, output, totalSamples);
        vccOut = nx_od1_post_get_vcc_f32(od1Post_);
        return true;
    case CircuitKind::AcBoosterDrive:
        if (acBoosterDrive_ == nullptr)
            return false;
        nx_ac_booster_drive_process_f32(acBoosterDrive_, input, output, totalSamples);
        vccOut = nx_ac_booster_drive_get_vcc_f32(acBoosterDrive_);
        return true;
    case CircuitKind::AcBoosterEq:
        if (acBoosterEq_ == nullptr)
            return false;
        nx_ac_booster_eq_process_f32(acBoosterEq_, input, output, totalSamples);
        vccOut = nx_ac_booster_eq_get_vcc_f32(acBoosterEq_);
        return true;
    case CircuitKind::RcBoosterDrive1:
        if (rcBoosterDrive1_ == nullptr)
            return false;
        nx_rc_booster_drive1_process_f32(rcBoosterDrive1_, input, output, totalSamples);
        vccOut = nx_rc_booster_drive1_get_vcc_f32(rcBoosterDrive1_);
        return true;
    case CircuitKind::DsPlusOpamp:
        if (dsPlusOpamp_ == nullptr)
            return false;
        nx_ds_plus_opamp_process_f32(dsPlusOpamp_, input, output, totalSamples);
        vccOut = nx_ds_plus_opamp_get_vcc_f32(dsPlusOpamp_);
        return true;
    case CircuitKind::Ts9Opamp:
        if (ts9Opamp_ == nullptr)
            return false;
        nx_ts9_opamp_process_f32(ts9Opamp_, input, output, totalSamples);
        vccOut = nx_ts9_opamp_get_vcc_f32(ts9Opamp_);
        return true;
    case CircuitKind::Ts9Tone:
        if (ts9Tone_ == nullptr)
            return false;
        nx_ts9_tone_process_f32(ts9Tone_, input, output, totalSamples);
        vccOut = nx_ts9_tone_get_vcc_f32(ts9Tone_);
        return true;
    case CircuitKind::KlonCentaur:
        if (klonCentaur_ == nullptr)
            return false;
        nx_klon_centaur_process_f32(klonCentaur_, input, output, totalSamples);
        vccOut = nx_klon_centaur_get_vcc_f32(klonCentaur_);
        return true;
    case CircuitKind::KlonCentaurTone:
        if (klonCentaurTone_ == nullptr)
            return false;
        nx_klon_centaur_tone_process_f32(klonCentaurTone_, input, output, totalSamples);
        vccOut = nx_klon_centaur_tone_get_vcc_f32(klonCentaurTone_);
        return true;
    case CircuitKind::GuvnorPreamp:
        if (guvnorPreamp_ == nullptr)
            return false;
        nx_guvnor_preamp_process_f32(guvnorPreamp_, input, output, totalSamples);
        vccOut = nx_guvnor_preamp_get_vcc_f32(guvnorPreamp_);
        return true;
    case CircuitKind::GuvnorPostamp:
        if (guvnorPostamp_ == nullptr)
            return false;
        nx_guvnor_postamp_process_f32(guvnorPostamp_, input, output, totalSamples);
        vccOut = nx_guvnor_postamp_get_vcc_f32(guvnorPostamp_);
        return true;
    case CircuitKind::GuvnorClipper:
        if (guvnorClipper_ == nullptr)
            return false;
        nx_guvnor_clipper_process_f32(guvnorClipper_, input, output, totalSamples);
        vccOut = kFallbackPreviewVcc;
        return true;
    case CircuitKind::GuvnorLevel:
        if (guvnorLevel_ == nullptr)
            return false;
        nx_guvnor_level_process_f32(guvnorLevel_, input, output, totalSamples);
        vccOut = kFallbackPreviewVcc;
        return true;
    case CircuitKind::DiodeClipper:
        if (diodeClipper_ == nullptr)
            return false;
        nx_diode_clipper_process_f32(diodeClipper_, input, output, totalSamples);
        vccOut = kFallbackPreviewVcc;
        return true;
    case CircuitKind::Ds1Opamp:
        if (ds1Opamp_ == nullptr)
            return false;
        nx_ds1_opamp_process_f32(ds1Opamp_, input, output, totalSamples);
        vccOut = nx_ds1_opamp_get_vcc_f32(ds1Opamp_);
        return true;
    }

    return false;
}

bool SineWavePreviewEngine::runProcess(const SinePreviewSetupParams& setup,
                                       const float* input,
                                       float* output,
                                       std::size_t totalSamples,
                                       double& vccOut)
{
    if (input == nullptr || output == nullptr || totalSamples == 0)
        return false;

    if (! ensureStructuralReady(setup))
        return false;

    if (! resetStates())
        return false;

    return runHotProcess(input, output, totalSamples, vccOut);
}

} // namespace ds1_ac
