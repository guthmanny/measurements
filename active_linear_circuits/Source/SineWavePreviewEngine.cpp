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
    if (odr1Drive_ != nullptr)
    {
        nx_odr1_drive_destroy_f32(odr1Drive_, nullptr);
        odr1Drive_ = nullptr;
    }
    if (odr1Clipper_ != nullptr)
    {
        nx_odr1_clipper_destroy_f32(odr1Clipper_, nullptr);
        odr1Clipper_ = nullptr;
    }
    if (odr1Tone_ != nullptr)
    {
        nx_odr1_tone_destroy_f32(odr1Tone_, nullptr);
        odr1Tone_ = nullptr;
    }
    if (odr1Postamp_ != nullptr)
    {
        nx_odr1_postamp_destroy_f32(odr1Postamp_, nullptr);
        odr1Postamp_ = nullptr;
    }
    if (beodPreamp_ != nullptr)
    {
        nx_beod_preamp_destroy_f32(beodPreamp_, nullptr);
        beodPreamp_ = nullptr;
    }
    if (beodTight_ != nullptr)
    {
        nx_beod_tight_destroy_f32(beodTight_, nullptr);
        beodTight_ = nullptr;
    }
    if (dirtyShirleyTight_ != nullptr)
    {
        nx_dirty_shirley_tight_destroy_f32(dirtyShirleyTight_, nullptr);
        dirtyShirleyTight_ = nullptr;
    }
    if (fa1Buffer_ != nullptr)
    {
        nx_fa1_buffer_destroy_f32(fa1Buffer_, nullptr);
        fa1Buffer_ = nullptr;
    }
    if (fa1Volume_ != nullptr)
    {
        nx_fa1_volume_destroy_f32(fa1Volume_, nullptr);
        fa1Volume_ = nullptr;
    }
    if (fa1Eq_ != nullptr)
    {
        nx_fa1_eq_destroy_f32(fa1Eq_, nullptr);
        fa1Eq_ = nullptr;
    }
    if (badMonkeyLow_ != nullptr)
    {
        nx_bad_monkey_low_destroy_f32(badMonkeyLow_, nullptr);
        badMonkeyLow_ = nullptr;
    }
    if (otaComp_ != nullptr)
    {
        nx_ota_comp_destroy_f32(otaComp_, nullptr);
        otaComp_ = nullptr;
    }
    if (dirtyShirleyMid_ != nullptr)
    {
        nx_dirty_shirley_mid_destroy_f32(dirtyShirleyMid_, nullptr);
        dirtyShirleyMid_ = nullptr;
    }
    if (dirtyShirley_ != nullptr)
    {
        nx_dirty_shirley_destroy_f32(dirtyShirley_, nullptr);
        dirtyShirley_ = nullptr;
    }
    if (beodPregain_ != nullptr)
    {
        nx_beod_pregain_destroy_f32(beodPregain_, nullptr);
        beodPregain_ = nullptr;
    }
    if (beodPostgain_ != nullptr)
    {
        nx_beod_postgain_destroy_f32(beodPostgain_, nullptr);
        beodPostgain_ = nullptr;
    }
    if (beodTrim_ != nullptr)
    {
        nx_beod_trim_destroy_f32(beodTrim_, nullptr);
        beodTrim_ = nullptr;
    }
    if (beodClipper_ != nullptr)
    {
        nx_beod_clipper_destroy_f32(beodClipper_, nullptr);
        beodClipper_ = nullptr;
    }
    if (beodBass_ != nullptr)
    {
        nx_beod_bass_destroy_f32(beodBass_, nullptr);
        beodBass_ = nullptr;
    }
    if (opBuffer_ != nullptr)
    {
        nx_op_buffer_destroy_f32(opBuffer_, nullptr);
        opBuffer_ = nullptr;
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
    if (crunchboxPreamp_ != nullptr)
    {
        nx_crunchbox_preamp_destroy_f32(crunchboxPreamp_, nullptr);
        crunchboxPreamp_ = nullptr;
    }
    if (crunchboxClipper_ != nullptr)
    {
        nx_crunchbox_clipper_destroy_f32(crunchboxClipper_, nullptr);
        crunchboxClipper_ = nullptr;
    }
    if (crunchboxClipperPresence_ != nullptr)
    {
        nx_crunchbox_clipper_presence_destroy_f32(crunchboxClipperPresence_, nullptr);
        crunchboxClipperPresence_ = nullptr;
    }
    if (diodeClipper_ != nullptr)
    {
        nx_diode_clipper_destroy_f32(diodeClipper_, nullptr);
        diodeClipper_ = nullptr;
    }
    if (ratOpamp_ != nullptr)
    {
        nx_rat_opamp_destroy_f32(ratOpamp_, nullptr);
        ratOpamp_ = nullptr;
    }
    if (ratClipper_ != nullptr)
    {
        nx_rat_clipper_destroy_f32(ratClipper_, nullptr);
        ratClipper_ = nullptr;
    }
    if (jfetFollower_ != nullptr)
    {
        nx_jfet_follower_destroy_f32(jfetFollower_, nullptr);
        jfetFollower_ = nullptr;
    }
    if (sansampPreEq_ != nullptr)
    {
        nx_sansamp_classic_pre_eq_destroy_f32(sansampPreEq_, nullptr);
        sansampPreEq_ = nullptr;
    }
    if (sansampPreamp_ != nullptr)
    {
        nx_sansamp_classic_preamp_destroy_f32(sansampPreamp_, nullptr);
        sansampPreamp_ = nullptr;
    }
    if (sansampGainCtrl_ != nullptr)
    {
        nx_sansamp_classic_gain_ctrl_destroy_f32(sansampGainCtrl_, nullptr);
        sansampGainCtrl_ = nullptr;
    }
    if (sansampPostamp_ != nullptr)
    {
        nx_sansamp_classic_postamp_destroy_f32(sansampPostamp_, nullptr);
        sansampPostamp_ = nullptr;
    }
    if (sansampSpk_ != nullptr)
    {
        nx_sansamp_classic_spk_destroy_f32(sansampSpk_, nullptr);
        sansampSpk_ = nullptr;
    }
    if (sansampPosition_ != nullptr)
    {
        nx_sansamp_classic_position_destroy_f32(sansampPosition_, nullptr);
        sansampPosition_ = nullptr;
    }
    if (sansampCapture_ != nullptr)
    {
        nx_sansamp_classic_capture_destroy_f32(sansampCapture_, nullptr);
        sansampCapture_ = nullptr;
    }
    if (sansampOutput_ != nullptr)
    {
        nx_sansamp_classic_output_destroy_f32(sansampOutput_, nullptr);
        sansampOutput_ = nullptr;
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

    case CircuitKind::CrunchboxPreamp:
        crunchboxPreamp_ = nx_crunchbox_preamp_create_f32(nullptr);
        if (crunchboxPreamp_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, crunchboxPreamp_, setup.componentValues);
        updatePotTaper(crunchboxPreamp_, nx_crunchbox_preamp_get_gain_pot_f32, nx_crunchbox_preamp_set_gain_pot_f32,
                       setup.potTaper);
        nx_crunchbox_preamp_set_opamp_model_f32(crunchboxPreamp_, setup.model);
        nx_crunchbox_preamp_set_gain_control_f32(crunchboxPreamp_, clampControl(setup.gainControl));
        nx_crunchbox_preamp_prepare_f32(crunchboxPreamp_, setup.sampleRateHz);
        nx_crunchbox_preamp_reset_f32(crunchboxPreamp_);
        break;

    case CircuitKind::CrunchboxClipper:
        crunchboxClipper_ = nx_crunchbox_clipper_create_f32(nullptr);
        if (crunchboxClipper_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, crunchboxClipper_, setup.componentValues);
        updatePotTaper(crunchboxClipper_, nx_crunchbox_clipper_get_tone_pot_f32, nx_crunchbox_clipper_set_tone_pot_f32,
                       setup.potTaper);
        updatePotTaper(crunchboxClipper_, nx_crunchbox_clipper_get_level_pot_f32, nx_crunchbox_clipper_set_level_pot_f32,
                       setup.potTaper);
        nx_crunchbox_clipper_set_diode_model_f32(crunchboxClipper_, setup.diodeModel);
        nx_crunchbox_clipper_set_tone_control_f32(crunchboxClipper_, clampControl(setup.gainControl));
        nx_crunchbox_clipper_set_level_control_f32(crunchboxClipper_, clampControl(setup.secondaryControl));
        nx_crunchbox_clipper_prepare_f32(crunchboxClipper_, setup.sampleRateHz);
        nx_crunchbox_clipper_reset_f32(crunchboxClipper_);
        break;

    case CircuitKind::CrunchboxClipperPresence:
        crunchboxClipperPresence_ = nx_crunchbox_clipper_presence_create_f32(nullptr);
        if (crunchboxClipperPresence_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, crunchboxClipperPresence_, setup.componentValues);
        updatePotTaper(crunchboxClipperPresence_, nx_crunchbox_clipper_presence_get_tone_pot_f32,
                       nx_crunchbox_clipper_presence_set_tone_pot_f32, setup.potTaper);
        updatePotTaper(crunchboxClipperPresence_, nx_crunchbox_clipper_presence_get_presence_pot_f32,
                       nx_crunchbox_clipper_presence_set_presence_pot_f32, setup.potTaper);
        nx_crunchbox_clipper_presence_set_diode_model_f32(crunchboxClipperPresence_, setup.diodeModel);
        nx_crunchbox_clipper_presence_set_tone_control_f32(crunchboxClipperPresence_, clampControl(setup.gainControl));
        nx_crunchbox_clipper_presence_set_presence_control_f32(crunchboxClipperPresence_,
                                                               clampControl(setup.secondaryControl));
        nx_crunchbox_clipper_presence_prepare_f32(crunchboxClipperPresence_, setup.sampleRateHz);
        nx_crunchbox_clipper_presence_reset_f32(crunchboxClipperPresence_);
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

    case CircuitKind::RatOpamp:
        ratOpamp_ = nx_rat_opamp_create_f32(nullptr);
        if (ratOpamp_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, ratOpamp_, setup.componentValues);
        updatePotTaper(ratOpamp_, nx_rat_opamp_get_distortion_pot_f32, nx_rat_opamp_set_distortion_pot_f32, setup.potTaper);
        nx_rat_opamp_set_opamp_model_f32(ratOpamp_, setup.model);
        nx_rat_opamp_set_distortion_control_f32(ratOpamp_, clampControl(setup.gainControl));
        nx_rat_opamp_prepare_f32(ratOpamp_, setup.sampleRateHz);
        nx_rat_opamp_reset_f32(ratOpamp_);
        break;

    case CircuitKind::RatClipper:
        ratClipper_ = nx_rat_clipper_create_f32(nullptr);
        if (ratClipper_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, ratClipper_, setup.componentValues);
        updatePotTaper(ratClipper_, nx_rat_clipper_get_filter_pot_f32, nx_rat_clipper_set_filter_pot_f32, setup.potTaper);
        nx_rat_clipper_set_diode_model_f32(ratClipper_, setup.diodeModel);
        nx_rat_clipper_set_filter_control_f32(ratClipper_, clampControl(setup.gainControl));
        nx_rat_clipper_prepare_f32(ratClipper_, setup.sampleRateHz);
        nx_rat_clipper_reset_f32(ratClipper_);
        break;

    case CircuitKind::JfetFollower:
        jfetFollower_ = nx_jfet_follower_create_f32(nullptr);
        if (jfetFollower_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, jfetFollower_, setup.componentValues);
        nx_jfet_follower_set_jfet_model_f32(jfetFollower_, setup.jfetModel);
        nx_jfet_follower_prepare_f32(jfetFollower_, setup.sampleRateHz);
        nx_jfet_follower_reset_f32(jfetFollower_);
        break;

    case CircuitKind::SansampPreEq:
        sansampPreEq_ = nx_sansamp_classic_pre_eq_create_f32(nullptr);
        if (sansampPreEq_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, sansampPreEq_, setup.componentValues);
        nx_sansamp_classic_pre_eq_prepare_f32(sansampPreEq_, setup.sampleRateHz);
        nx_sansamp_classic_pre_eq_reset_f32(sansampPreEq_);
        break;

    case CircuitKind::SansampPreamp:
        sansampPreamp_ = nx_sansamp_classic_preamp_create_f32(nullptr);
        if (sansampPreamp_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, sansampPreamp_, setup.componentValues);
        updatePotTaper(sansampPreamp_, nx_sansamp_classic_preamp_get_p1_pot_f32, nx_sansamp_classic_preamp_set_p1_pot_f32, setup.potTaper);
        nx_sansamp_classic_preamp_set_opamp_model_f32(sansampPreamp_, setup.model);
        nx_sansamp_classic_preamp_set_p1_control_f32(sansampPreamp_, clampControl(setup.gainControl));
        nx_sansamp_classic_preamp_prepare_f32(sansampPreamp_, setup.sampleRateHz);
        nx_sansamp_classic_preamp_reset_f32(sansampPreamp_);
        break;

    case CircuitKind::SansampGainCtrl:
        sansampGainCtrl_ = nx_sansamp_classic_gain_ctrl_create_f32(nullptr);
        if (sansampGainCtrl_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, sansampGainCtrl_, setup.componentValues);
        updatePotTaper(sansampGainCtrl_, nx_sansamp_classic_gain_ctrl_get_p1_pot_f32, nx_sansamp_classic_gain_ctrl_set_p1_pot_f32, setup.potTaper);
        nx_sansamp_classic_gain_ctrl_set_p1_control_f32(sansampGainCtrl_, clampControl(setup.gainControl));
        nx_sansamp_classic_gain_ctrl_prepare_f32(sansampGainCtrl_, setup.sampleRateHz);
        nx_sansamp_classic_gain_ctrl_reset_f32(sansampGainCtrl_);
        break;

    case CircuitKind::SansampPostamp:
        sansampPostamp_ = nx_sansamp_classic_postamp_create_f32(nullptr);
        if (sansampPostamp_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, sansampPostamp_, setup.componentValues);
        nx_sansamp_classic_postamp_set_opamp_model_f32(sansampPostamp_, setup.model);
        nx_sansamp_classic_postamp_prepare_f32(sansampPostamp_, setup.sampleRateHz);
        nx_sansamp_classic_postamp_reset_f32(sansampPostamp_);
        break;

    case CircuitKind::SansampSpk:
        sansampSpk_ = nx_sansamp_classic_spk_create_f32(nullptr);
        if (sansampSpk_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, sansampSpk_, setup.componentValues);
        nx_sansamp_classic_spk_set_opamp_model_f32(sansampSpk_, setup.model);
        nx_sansamp_classic_spk_prepare_f32(sansampSpk_, setup.sampleRateHz);
        nx_sansamp_classic_spk_reset_f32(sansampSpk_);
        break;

    case CircuitKind::SansampPosition:
        sansampPosition_ = nx_sansamp_classic_position_create_f32(nullptr);
        if (sansampPosition_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, sansampPosition_, setup.componentValues);
        nx_sansamp_classic_position_set_opamp_model_f32(sansampPosition_, setup.model);
        nx_sansamp_classic_position_prepare_f32(sansampPosition_, setup.sampleRateHz);
        nx_sansamp_classic_position_reset_f32(sansampPosition_);
        break;

    case CircuitKind::SansampCapture:
        sansampCapture_ = nx_sansamp_classic_capture_create_f32(nullptr);
        if (sansampCapture_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, sansampCapture_, setup.componentValues);
        updatePotTaper(sansampCapture_, nx_sansamp_classic_capture_get_p1_pot_f32, nx_sansamp_classic_capture_set_p1_pot_f32, setup.potTaper);
        nx_sansamp_classic_capture_set_opamp_model_f32(sansampCapture_, setup.model);
        nx_sansamp_classic_capture_set_p1_control_f32(sansampCapture_, clampControl(setup.gainControl));
        nx_sansamp_classic_capture_prepare_f32(sansampCapture_, setup.sampleRateHz);
        nx_sansamp_classic_capture_reset_f32(sansampCapture_);
        break;

    case CircuitKind::SansampOutput:
        sansampOutput_ = nx_sansamp_classic_output_create_f32(nullptr);
        if (sansampOutput_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, sansampOutput_, setup.componentValues);
        updatePotTaper(sansampOutput_, nx_sansamp_classic_output_get_p1_pot_f32, nx_sansamp_classic_output_set_p1_pot_f32, setup.potTaper);
        nx_sansamp_classic_output_set_p1_control_f32(sansampOutput_, clampControl(setup.gainControl));
        nx_sansamp_classic_output_prepare_f32(sansampOutput_, setup.sampleRateHz);
        nx_sansamp_classic_output_reset_f32(sansampOutput_);
        break;

    case CircuitKind::Odr1Drive:
        odr1Drive_ = nx_odr1_drive_create_f32(nullptr);
        if (odr1Drive_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, odr1Drive_, setup.componentValues);
        updatePotTaper(odr1Drive_, nx_odr1_drive_get_drive_pot_f32, nx_odr1_drive_set_drive_pot_f32, setup.potTaper);
        nx_odr1_drive_set_opamp_model_f32(odr1Drive_, setup.model);
        nx_odr1_drive_set_diode_model_f32(odr1Drive_, setup.diodeModel);
        nx_odr1_drive_set_drive_control_f32(odr1Drive_, clampControl(setup.gainControl));
        nx_odr1_drive_prepare_f32(odr1Drive_, setup.sampleRateHz);
        nx_odr1_drive_reset_f32(odr1Drive_);
        break;

    case CircuitKind::Odr1Clipper:
        odr1Clipper_ = nx_odr1_clipper_create_f32(nullptr);
        if (odr1Clipper_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, odr1Clipper_, setup.componentValues);
        nx_odr1_clipper_set_diode_model_f32(odr1Clipper_, setup.diodeModel);
        nx_odr1_clipper_prepare_f32(odr1Clipper_, setup.sampleRateHz);
        nx_odr1_clipper_reset_f32(odr1Clipper_);
        break;

    case CircuitKind::Odr1Tone:
        odr1Tone_ = nx_odr1_tone_create_f32(nullptr);
        if (odr1Tone_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, odr1Tone_, setup.componentValues);
        updatePotTaper(odr1Tone_, nx_odr1_tone_get_tone_pot_f32, nx_odr1_tone_set_tone_pot_f32, setup.potTaper);
        nx_odr1_tone_set_opamp_model_f32(odr1Tone_, setup.model);
        nx_odr1_tone_set_tone_control_f32(odr1Tone_, clampControl(setup.gainControl));
        nx_odr1_tone_prepare_f32(odr1Tone_, setup.sampleRateHz);
        nx_odr1_tone_reset_f32(odr1Tone_);
        break;

    case CircuitKind::Odr1Postamp:
        odr1Postamp_ = nx_odr1_postamp_create_f32(nullptr);
        if (odr1Postamp_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, odr1Postamp_, setup.componentValues);
        nx_odr1_postamp_set_opamp_model_f32(odr1Postamp_, setup.model);
        nx_odr1_postamp_prepare_f32(odr1Postamp_, setup.sampleRateHz);
        nx_odr1_postamp_reset_f32(odr1Postamp_);
        break;

    case CircuitKind::BeodPreamp:
        beodPreamp_ = nx_beod_preamp_create_f32(nullptr);
        if (beodPreamp_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, beodPreamp_, setup.componentValues);
        nx_beod_preamp_set_opamp_model_f32(beodPreamp_, setup.model);
        nx_beod_preamp_prepare_f32(beodPreamp_, setup.sampleRateHz);
        nx_beod_preamp_reset_f32(beodPreamp_);
        break;

    case CircuitKind::BeodTight:
        beodTight_ = nx_beod_tight_create_f32(nullptr);
        if (beodTight_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, beodTight_, setup.componentValues);
        updatePotTaper(beodTight_, nx_beod_tight_get_tight_pot_f32, nx_beod_tight_set_tight_pot_f32, setup.potTaper);
        nx_beod_tight_set_tight_control_f32(beodTight_, clampControl(setup.gainControl));
        nx_beod_tight_prepare_f32(beodTight_, setup.sampleRateHz);
        nx_beod_tight_reset_f32(beodTight_);
        break;

    case CircuitKind::BeodPregain:
        beodPregain_ = nx_beod_pregain_create_f32(nullptr);
        if (beodPregain_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, beodPregain_, setup.componentValues);
        updatePotTaper(beodPregain_, nx_beod_pregain_get_gain_pot_f32, nx_beod_pregain_set_gain_pot_f32, setup.potTaper);
        nx_beod_pregain_set_opamp_model_f32(beodPregain_, setup.model);
        nx_beod_pregain_set_diode_model_f32(beodPregain_, setup.diodeModel);
        nx_beod_pregain_set_gain_control_f32(beodPregain_, clampControl(setup.gainControl));
        nx_beod_pregain_prepare_f32(beodPregain_, setup.sampleRateHz);
        nx_beod_pregain_reset_f32(beodPregain_);
        break;

    case CircuitKind::BeodPostgain:
        beodPostgain_ = nx_beod_postgain_create_f32(nullptr);
        if (beodPostgain_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, beodPostgain_, setup.componentValues);
        nx_beod_postgain_set_opamp_model_f32(beodPostgain_, setup.model);
        nx_beod_postgain_set_diode_model_f32(beodPostgain_, setup.diodeModel);
        nx_beod_postgain_prepare_f32(beodPostgain_, setup.sampleRateHz);
        nx_beod_postgain_reset_f32(beodPostgain_);
        break;

    case CircuitKind::BeodTrim:
        beodTrim_ = nx_beod_trim_create_f32(nullptr);
        if (beodTrim_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, beodTrim_, setup.componentValues);
        updatePotTaper(beodTrim_, nx_beod_trim_get_trim_pot_f32, nx_beod_trim_set_trim_pot_f32, setup.potTaper);
        nx_beod_trim_set_opamp_model_f32(beodTrim_, setup.model);
        nx_beod_trim_set_trim_control_f32(beodTrim_, clampControl(setup.gainControl));
        nx_beod_trim_prepare_f32(beodTrim_, setup.sampleRateHz);
        nx_beod_trim_reset_f32(beodTrim_);
        break;

    case CircuitKind::BeodClipper:
        beodClipper_ = nx_beod_clipper_create_f32(nullptr);
        if (beodClipper_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, beodClipper_, setup.componentValues);
        updatePotTaper(beodClipper_, nx_beod_clipper_get_presence_pot_f32, nx_beod_clipper_set_presence_pot_f32,
                       NX_POT_TAPER_C);
        updatePotTaper(beodClipper_, nx_beod_clipper_get_treble_pot_f32, nx_beod_clipper_set_treble_pot_f32,
                       NX_POT_TAPER_LINEAR);
        nx_beod_clipper_set_diode_model_f32(beodClipper_, setup.diodeModel);
        nx_beod_clipper_set_presence_control_f32(beodClipper_, clampControl(setup.gainControl));
        nx_beod_clipper_set_treble_control_f32(beodClipper_, clampControl(setup.secondaryControl));
        nx_beod_clipper_prepare_f32(beodClipper_, setup.sampleRateHz);
        nx_beod_clipper_reset_f32(beodClipper_);
        break;

    case CircuitKind::BeodBass:
        beodBass_ = nx_beod_bass_create_f32(nullptr);
        if (beodBass_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, beodBass_, setup.componentValues);
        updatePotTaper(beodBass_, nx_beod_bass_get_bass_pot_f32, nx_beod_bass_set_bass_pot_f32, NX_POT_TAPER_C);
        nx_beod_bass_set_opamp_model_f32(beodBass_, setup.model);
        nx_beod_bass_set_bass_control_f32(beodBass_, clampControl(setup.gainControl));
        nx_beod_bass_prepare_f32(beodBass_, setup.sampleRateHz);
        nx_beod_bass_tick_f32(beodBass_, 64);
        nx_beod_bass_reset_f32(beodBass_);
        break;

    case CircuitKind::OpBuffer:
        opBuffer_ = nx_op_buffer_create_f32(nullptr);
        if (opBuffer_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, opBuffer_, setup.componentValues);
        nx_op_buffer_set_opamp_model_f32(opBuffer_, setup.model);
        nx_op_buffer_prepare_f32(opBuffer_, setup.sampleRateHz);
        nx_op_buffer_reset_f32(opBuffer_);
        break;

    case CircuitKind::DirtyShirleyTight:
        dirtyShirleyTight_ = nx_dirty_shirley_tight_create_f32(nullptr);
        if (dirtyShirleyTight_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, dirtyShirleyTight_, setup.componentValues);
        nx_dirty_shirley_tight_prepare_f32(dirtyShirleyTight_, setup.sampleRateHz);
        nx_dirty_shirley_tight_reset_f32(dirtyShirleyTight_);
        break;

    case CircuitKind::Fa1Buffer:
        fa1Buffer_ = nx_fa1_buffer_create_f32(nullptr);
        if (fa1Buffer_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, fa1Buffer_, setup.componentValues);
        nx_fa1_buffer_set_jfet_model_f32(fa1Buffer_, setup.jfetModel);
        nx_fa1_buffer_prepare_f32(fa1Buffer_, setup.sampleRateHz);
        nx_fa1_buffer_tick_f32(fa1Buffer_, 64);
        nx_fa1_buffer_reset_f32(fa1Buffer_);
        break;

    case CircuitKind::Fa1Volume:
        fa1Volume_ = nx_fa1_volume_create_f32(nullptr);
        if (fa1Volume_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, fa1Volume_, setup.componentValues);
        updatePotTaper(fa1Volume_, nx_fa1_volume_get_volume_pot_f32, nx_fa1_volume_set_volume_pot_f32, setup.potTaper);
        nx_fa1_volume_set_opamp_model_f32(fa1Volume_, setup.model);
        nx_fa1_volume_set_volume_control_f32(fa1Volume_, clampControl(setup.gainControl));
        nx_fa1_volume_prepare_f32(fa1Volume_, setup.sampleRateHz);
        nx_fa1_volume_tick_f32(fa1Volume_, 64);
        nx_fa1_volume_reset_f32(fa1Volume_);
        break;

    case CircuitKind::Fa1Eq:
        fa1Eq_ = nx_fa1_eq_create_f32(nullptr);
        if (fa1Eq_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, fa1Eq_, setup.componentValues);
        updatePotTaper(fa1Eq_, nx_fa1_eq_get_bass_pot_f32, nx_fa1_eq_set_bass_pot_f32, setup.potTaper);
        updatePotTaper(fa1Eq_, nx_fa1_eq_get_treble_pot_f32, nx_fa1_eq_set_treble_pot_f32, setup.potTaper);
        nx_fa1_eq_set_opamp_model_f32(fa1Eq_, setup.model);
        nx_fa1_eq_set_bass_control_f32(fa1Eq_, clampControl(setup.gainControl));
        nx_fa1_eq_set_treble_control_f32(fa1Eq_, clampControl(setup.secondaryControl));
        nx_fa1_eq_prepare_f32(fa1Eq_, setup.sampleRateHz);
        nx_fa1_eq_tick_f32(fa1Eq_, 64);
        nx_fa1_eq_reset_f32(fa1Eq_);
        break;

    case CircuitKind::BadMonkeyLow:
        badMonkeyLow_ = nx_bad_monkey_low_create_f32(nullptr);
        if (badMonkeyLow_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, badMonkeyLow_, setup.componentValues);
        updatePotTaper(badMonkeyLow_, nx_bad_monkey_low_get_low_pot_f32, nx_bad_monkey_low_set_low_pot_f32,
                       setup.potTaper);
        nx_bad_monkey_low_set_opamp_model_f32(badMonkeyLow_, setup.model);
        nx_bad_monkey_low_set_bjt_model_f32(badMonkeyLow_, setup.bjtModel);
        nx_bad_monkey_low_set_low_control_f32(badMonkeyLow_, clampControl(setup.gainControl));
        nx_bad_monkey_low_prepare_f32(badMonkeyLow_, setup.sampleRateHz);
        nx_bad_monkey_low_tick_f32(badMonkeyLow_, 64);
        nx_bad_monkey_low_reset_f32(badMonkeyLow_);
        break;

    case CircuitKind::DynaCompCore:
        otaComp_ = nx_ota_comp_create_f32(nullptr);
        if (otaComp_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, otaComp_, setup.componentValues);
        updatePotTaper(otaComp_, nx_ota_comp_get_sensitivity_pot_f32, nx_ota_comp_set_sensitivity_pot_f32,
                       setup.potTaper);
        nx_ota_comp_set_diode_model_f32(otaComp_, setup.diodeModel);
        nx_ota_comp_set_bjt_model_f32(otaComp_, setup.bjtModel);
        nx_ota_comp_set_ota_model_f32(otaComp_, NX_OTA_CA3080);
        nx_ota_comp_set_sensitivity_control_f32(otaComp_, clampControl(setup.gainControl));
        nx_ota_comp_prepare_f32(otaComp_, setup.sampleRateHz);
        nx_ota_comp_tick_f32(otaComp_, 64);
        nx_ota_comp_reset_f32(otaComp_);
        break;

    case CircuitKind::DirtyShirleyMid:
        dirtyShirleyMid_ = nx_dirty_shirley_mid_create_f32(nullptr);
        if (dirtyShirleyMid_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, dirtyShirleyMid_, setup.componentValues);
        updatePotTaper(dirtyShirleyMid_, nx_dirty_shirley_mid_get_level_pot_f32, nx_dirty_shirley_mid_set_level_pot_f32,
                       NX_POT_TAPER_A30);
        updatePotTaper(dirtyShirleyMid_, nx_dirty_shirley_mid_get_mid_pot_f32, nx_dirty_shirley_mid_set_mid_pot_f32,
                       NX_POT_TAPER_C);
        nx_dirty_shirley_mid_set_opamp_model_f32(dirtyShirleyMid_, setup.model);
        nx_dirty_shirley_mid_set_level_control_f32(dirtyShirleyMid_, clampControl(setup.gainControl));
        nx_dirty_shirley_mid_set_mid_control_f32(dirtyShirleyMid_, clampControl(setup.secondaryControl));
        nx_dirty_shirley_mid_prepare_f32(dirtyShirleyMid_, setup.sampleRateHz);
        nx_dirty_shirley_mid_tick_f32(dirtyShirleyMid_, 64);
        nx_dirty_shirley_mid_reset_f32(dirtyShirleyMid_);
        break;

    case CircuitKind::DirtyShirleyComposite:
        dirtyShirley_ = nx_dirty_shirley_create_f32(nullptr);
        if (dirtyShirley_ == nullptr)
            return false;
        applySchematicValues(setup.circuit, dirtyShirley_, setup.componentValues);
        updatePotTaper(dirtyShirley_, nx_dirty_shirley_get_gain_pot_f32, nx_dirty_shirley_set_gain_pot_f32,
                       NX_POT_TAPER_A30);
        updatePotTaper(dirtyShirley_, nx_dirty_shirley_get_level_pot_f32, nx_dirty_shirley_set_level_pot_f32,
                       NX_POT_TAPER_A30);
        updatePotTaper(dirtyShirley_, nx_dirty_shirley_get_mid_pot_f32, nx_dirty_shirley_set_mid_pot_f32, NX_POT_TAPER_C);
        nx_dirty_shirley_set_opamp_model_f32(dirtyShirley_, setup.model);
        nx_dirty_shirley_set_gain_control_f32(dirtyShirley_, clampControl(setup.gainControl));
        nx_dirty_shirley_set_level_control_f32(dirtyShirley_, clampControl(setup.secondaryControl));
        nx_dirty_shirley_set_mid_control_f32(dirtyShirley_, clampControl(setup.tertiaryControl));
        if (nx_dirty_shirley_prepare_f32(dirtyShirley_, setup.sampleRateHz) != NX_SUCCESS)
            return false;
        nx_dirty_shirley_tick_f32(dirtyShirley_, 64);
        nx_dirty_shirley_reset_f32(dirtyShirley_);
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
    case CircuitKind::Odr1Drive:
        if (odr1Drive_ == nullptr)
            return false;
        nx_odr1_drive_reset_f32(odr1Drive_);
        return true;
    case CircuitKind::Odr1Clipper:
        if (odr1Clipper_ == nullptr)
            return false;
        nx_odr1_clipper_reset_f32(odr1Clipper_);
        return true;
    case CircuitKind::Odr1Tone:
        if (odr1Tone_ == nullptr)
            return false;
        nx_odr1_tone_reset_f32(odr1Tone_);
        return true;
    case CircuitKind::Odr1Postamp:
        if (odr1Postamp_ == nullptr)
            return false;
        nx_odr1_postamp_reset_f32(odr1Postamp_);
        return true;
    case CircuitKind::BeodPreamp:
        if (beodPreamp_ == nullptr)
            return false;
        nx_beod_preamp_reset_f32(beodPreamp_);
        return true;
    case CircuitKind::BeodTight:
        if (beodTight_ == nullptr)
            return false;
        nx_beod_tight_reset_f32(beodTight_);
        return true;
    case CircuitKind::BeodPregain:
        if (beodPregain_ == nullptr)
            return false;
        nx_beod_pregain_reset_f32(beodPregain_);
        return true;
    case CircuitKind::BeodPostgain:
        if (beodPostgain_ == nullptr)
            return false;
        nx_beod_postgain_reset_f32(beodPostgain_);
        return true;
    case CircuitKind::BeodTrim:
        if (beodTrim_ == nullptr)
            return false;
        nx_beod_trim_reset_f32(beodTrim_);
        return true;
    case CircuitKind::BeodClipper:
        if (beodClipper_ == nullptr)
            return false;
        nx_beod_clipper_reset_f32(beodClipper_);
        return true;
    case CircuitKind::BeodBass:
        if (beodBass_ == nullptr)
            return false;
        nx_beod_bass_reset_f32(beodBass_);
        return true;
    case CircuitKind::OpBuffer:
        if (opBuffer_ == nullptr)
            return false;
        nx_op_buffer_reset_f32(opBuffer_);
        return true;
    case CircuitKind::DirtyShirleyTight:
        if (dirtyShirleyTight_ == nullptr)
            return false;
        nx_dirty_shirley_tight_reset_f32(dirtyShirleyTight_);
        return true;
    case CircuitKind::Fa1Buffer:
        if (fa1Buffer_ == nullptr)
            return false;
        nx_fa1_buffer_reset_f32(fa1Buffer_);
        return true;
    case CircuitKind::Fa1Volume:
        if (fa1Volume_ == nullptr)
            return false;
        nx_fa1_volume_reset_f32(fa1Volume_);
        return true;
    case CircuitKind::Fa1Eq:
        if (fa1Eq_ == nullptr)
            return false;
        nx_fa1_eq_reset_f32(fa1Eq_);
        return true;
    case CircuitKind::BadMonkeyLow:
        if (badMonkeyLow_ == nullptr)
            return false;
        nx_bad_monkey_low_reset_f32(badMonkeyLow_);
        return true;
    case CircuitKind::DynaCompCore:
        if (otaComp_ == nullptr)
            return false;
        nx_ota_comp_reset_f32(otaComp_);
        return true;
    case CircuitKind::DirtyShirleyMid:
        if (dirtyShirleyMid_ == nullptr)
            return false;
        nx_dirty_shirley_mid_reset_f32(dirtyShirleyMid_);
        return true;
    case CircuitKind::DirtyShirleyComposite:
        if (dirtyShirley_ == nullptr)
            return false;
        nx_dirty_shirley_reset_f32(dirtyShirley_);
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
    case CircuitKind::CrunchboxPreamp:
        if (crunchboxPreamp_ == nullptr)
            return false;
        nx_crunchbox_preamp_reset_f32(crunchboxPreamp_);
        return true;
    case CircuitKind::CrunchboxClipper:
        if (crunchboxClipper_ == nullptr)
            return false;
        nx_crunchbox_clipper_reset_f32(crunchboxClipper_);
        return true;
    case CircuitKind::CrunchboxClipperPresence:
        if (crunchboxClipperPresence_ == nullptr)
            return false;
        nx_crunchbox_clipper_presence_reset_f32(crunchboxClipperPresence_);
        return true;
    case CircuitKind::DiodeClipper:
        if (diodeClipper_ == nullptr)
            return false;
        nx_diode_clipper_reset_f32(diodeClipper_);
        return true;
    case CircuitKind::RatOpamp:
        if (ratOpamp_ == nullptr)
            return false;
        nx_rat_opamp_reset_f32(ratOpamp_);
        return true;
    case CircuitKind::RatClipper:
        if (ratClipper_ == nullptr)
            return false;
        nx_rat_clipper_reset_f32(ratClipper_);
        return true;
    case CircuitKind::JfetFollower:
        if (jfetFollower_ == nullptr)
            return false;
        nx_jfet_follower_reset_f32(jfetFollower_);
        return true;
    case CircuitKind::SansampPreEq:
        if (sansampPreEq_ == nullptr)
            return false;
        nx_sansamp_classic_pre_eq_reset_f32(sansampPreEq_);
        return true;
    case CircuitKind::SansampPreamp:
        if (sansampPreamp_ == nullptr)
            return false;
        nx_sansamp_classic_preamp_reset_f32(sansampPreamp_);
        return true;
    case CircuitKind::SansampGainCtrl:
        if (sansampGainCtrl_ == nullptr)
            return false;
        nx_sansamp_classic_gain_ctrl_reset_f32(sansampGainCtrl_);
        return true;
    case CircuitKind::SansampPostamp:
        if (sansampPostamp_ == nullptr)
            return false;
        nx_sansamp_classic_postamp_reset_f32(sansampPostamp_);
        return true;
    case CircuitKind::SansampSpk:
        if (sansampSpk_ == nullptr)
            return false;
        nx_sansamp_classic_spk_reset_f32(sansampSpk_);
        return true;
    case CircuitKind::SansampPosition:
        if (sansampPosition_ == nullptr)
            return false;
        nx_sansamp_classic_position_reset_f32(sansampPosition_);
        return true;
    case CircuitKind::SansampCapture:
        if (sansampCapture_ == nullptr)
            return false;
        nx_sansamp_classic_capture_reset_f32(sansampCapture_);
        return true;
    case CircuitKind::SansampOutput:
        if (sansampOutput_ == nullptr)
            return false;
        nx_sansamp_classic_output_reset_f32(sansampOutput_);
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
    case CircuitKind::Odr1Drive:
        if (odr1Drive_ == nullptr)
            return false;
        nx_odr1_drive_process_f32(odr1Drive_, input, output, totalSamples);
        vccOut = nx_odr1_drive_get_vcc_f32(odr1Drive_);
        return true;
    case CircuitKind::Odr1Clipper:
        if (odr1Clipper_ == nullptr)
            return false;
        nx_odr1_clipper_process_f32(odr1Clipper_, input, output, totalSamples);
        vccOut = kFallbackPreviewVcc;
        return true;
    case CircuitKind::Odr1Tone:
        if (odr1Tone_ == nullptr)
            return false;
        nx_odr1_tone_process_f32(odr1Tone_, input, output, totalSamples);
        vccOut = nx_odr1_tone_get_vcc_f32(odr1Tone_);
        return true;
    case CircuitKind::Odr1Postamp:
        if (odr1Postamp_ == nullptr)
            return false;
        nx_odr1_postamp_process_f32(odr1Postamp_, input, output, totalSamples);
        vccOut = nx_odr1_postamp_get_vcc_f32(odr1Postamp_);
        return true;
    case CircuitKind::BeodPreamp:
        if (beodPreamp_ == nullptr)
            return false;
        nx_beod_preamp_process_f32(beodPreamp_, input, output, totalSamples);
        vccOut = nx_beod_preamp_get_vcc_f32(beodPreamp_);
        return true;
    case CircuitKind::BeodTight:
        if (beodTight_ == nullptr)
            return false;
        nx_beod_tight_process_f32(beodTight_, input, output, totalSamples);
        vccOut = kFallbackPreviewVcc;
        return true;
    case CircuitKind::BeodPregain:
        if (beodPregain_ == nullptr)
            return false;
        nx_beod_pregain_process_f32(beodPregain_, input, output, totalSamples);
        vccOut = nx_beod_pregain_get_vcc_f32(beodPregain_);
        return true;
    case CircuitKind::BeodPostgain:
        if (beodPostgain_ == nullptr)
            return false;
        nx_beod_postgain_process_f32(beodPostgain_, input, output, totalSamples);
        vccOut = nx_beod_postgain_get_vcc_f32(beodPostgain_);
        return true;
    case CircuitKind::BeodTrim:
        if (beodTrim_ == nullptr)
            return false;
        nx_beod_trim_process_f32(beodTrim_, input, output, totalSamples);
        vccOut = nx_beod_trim_get_vcc_f32(beodTrim_);
        return true;
    case CircuitKind::BeodClipper:
        if (beodClipper_ == nullptr)
            return false;
        nx_beod_clipper_process_f32(beodClipper_, input, output, totalSamples);
        vccOut = kFallbackPreviewVcc;
        return true;
    case CircuitKind::BeodBass:
        if (beodBass_ == nullptr)
            return false;
        nx_beod_bass_process_f32(beodBass_, input, output, totalSamples);
        vccOut = nx_beod_bass_get_vcc_f32(beodBass_);
        return true;
    case CircuitKind::OpBuffer:
        if (opBuffer_ == nullptr)
            return false;
        nx_op_buffer_process_f32(opBuffer_, input, output, totalSamples);
        vccOut = nx_op_buffer_get_vcc_f32(opBuffer_);
        return true;
    case CircuitKind::DirtyShirleyTight:
        if (dirtyShirleyTight_ == nullptr)
            return false;
        nx_dirty_shirley_tight_process_f32(dirtyShirleyTight_, input, output, totalSamples);
        vccOut = kFallbackPreviewVcc;
        return true;
    case CircuitKind::Fa1Buffer:
        if (fa1Buffer_ == nullptr)
            return false;
        nx_fa1_buffer_process_f32(fa1Buffer_, input, output, totalSamples);
        vccOut = kFallbackPreviewVcc;
        return true;
    case CircuitKind::Fa1Volume:
        if (fa1Volume_ == nullptr)
            return false;
        nx_fa1_volume_process_f32(fa1Volume_, input, output, totalSamples);
        vccOut = nx_fa1_volume_get_vcc_f32(fa1Volume_);
        return true;
    case CircuitKind::Fa1Eq:
        if (fa1Eq_ == nullptr)
            return false;
        nx_fa1_eq_process_f32(fa1Eq_, input, output, totalSamples);
        vccOut = nx_fa1_eq_get_vcc_f32(fa1Eq_);
        return true;
    case CircuitKind::BadMonkeyLow:
        if (badMonkeyLow_ == nullptr)
            return false;
        nx_bad_monkey_low_process_f32(badMonkeyLow_, input, output, totalSamples);
        vccOut = nx_bad_monkey_low_get_vcc_f32(badMonkeyLow_);
        return true;
    case CircuitKind::DynaCompCore:
        if (otaComp_ == nullptr)
            return false;
        nx_ota_comp_process_f32(otaComp_, input, output, totalSamples);
        vccOut = nx_ota_comp_get_vcc_f32(otaComp_);
        return true;
    case CircuitKind::DirtyShirleyMid:
        if (dirtyShirleyMid_ == nullptr)
            return false;
        nx_dirty_shirley_mid_process_f32(dirtyShirleyMid_, input, output, totalSamples);
        vccOut = nx_dirty_shirley_mid_get_vcc_f32(dirtyShirleyMid_);
        return true;
    case CircuitKind::DirtyShirleyComposite:
        if (dirtyShirley_ == nullptr)
            return false;
        nx_dirty_shirley_tick_f32(dirtyShirley_, totalSamples);
        nx_dirty_shirley_process_f32(dirtyShirley_, input, output, totalSamples);
        vccOut = nx_dirty_shirley_get_vcc_f32(dirtyShirley_);
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
    case CircuitKind::CrunchboxPreamp:
        if (crunchboxPreamp_ == nullptr)
            return false;
        nx_crunchbox_preamp_process_f32(crunchboxPreamp_, input, output, totalSamples);
        vccOut = nx_crunchbox_preamp_get_vcc_f32(crunchboxPreamp_);
        return true;
    case CircuitKind::CrunchboxClipper:
        if (crunchboxClipper_ == nullptr)
            return false;
        nx_crunchbox_clipper_process_f32(crunchboxClipper_, input, output, totalSamples);
        vccOut = kFallbackPreviewVcc;
        return true;
    case CircuitKind::CrunchboxClipperPresence:
        if (crunchboxClipperPresence_ == nullptr)
            return false;
        nx_crunchbox_clipper_presence_process_f32(crunchboxClipperPresence_, input, output, totalSamples);
        vccOut = kFallbackPreviewVcc;
        return true;
    case CircuitKind::DiodeClipper:
        if (diodeClipper_ == nullptr)
            return false;
        nx_diode_clipper_process_f32(diodeClipper_, input, output, totalSamples);
        vccOut = kFallbackPreviewVcc;
        return true;
    case CircuitKind::RatOpamp:
        if (ratOpamp_ == nullptr)
            return false;
        nx_rat_opamp_process_f32(ratOpamp_, input, output, totalSamples);
        vccOut = nx_rat_opamp_get_vcc_f32(ratOpamp_);
        return true;
    case CircuitKind::RatClipper:
        if (ratClipper_ == nullptr)
            return false;
        nx_rat_clipper_process_f32(ratClipper_, input, output, totalSamples);
        vccOut = kFallbackPreviewVcc;
        return true;
    case CircuitKind::JfetFollower:
        if (jfetFollower_ == nullptr)
            return false;
        nx_jfet_follower_process_f32(jfetFollower_, input, output, totalSamples);
        vccOut = kFallbackPreviewVcc;
        return true;
    case CircuitKind::SansampPreEq:
        if (sansampPreEq_ == nullptr)
            return false;
        nx_sansamp_classic_pre_eq_process_f32(sansampPreEq_, input, output, totalSamples);
        vccOut = kFallbackPreviewVcc;
        return true;
    case CircuitKind::SansampPreamp:
        if (sansampPreamp_ == nullptr)
            return false;
        nx_sansamp_classic_preamp_process_f32(sansampPreamp_, input, output, totalSamples);
        vccOut = nx_sansamp_classic_preamp_get_vcc_f32(sansampPreamp_);
        return true;
    case CircuitKind::SansampGainCtrl:
        if (sansampGainCtrl_ == nullptr)
            return false;
        nx_sansamp_classic_gain_ctrl_process_f32(sansampGainCtrl_, input, output, totalSamples);
        vccOut = kFallbackPreviewVcc;
        return true;
    case CircuitKind::SansampPostamp:
        if (sansampPostamp_ == nullptr)
            return false;
        nx_sansamp_classic_postamp_process_f32(sansampPostamp_, input, output, totalSamples);
        vccOut = nx_sansamp_classic_postamp_get_vcc_f32(sansampPostamp_);
        return true;
    case CircuitKind::SansampSpk:
        if (sansampSpk_ == nullptr)
            return false;
        nx_sansamp_classic_spk_process_f32(sansampSpk_, input, output, totalSamples);
        vccOut = nx_sansamp_classic_spk_get_vcc_f32(sansampSpk_);
        return true;
    case CircuitKind::SansampPosition:
        if (sansampPosition_ == nullptr)
            return false;
        nx_sansamp_classic_position_process_f32(sansampPosition_, input, output, totalSamples);
        vccOut = nx_sansamp_classic_position_get_vcc_f32(sansampPosition_);
        return true;
    case CircuitKind::SansampCapture:
        if (sansampCapture_ == nullptr)
            return false;
        nx_sansamp_classic_capture_process_f32(sansampCapture_, input, output, totalSamples);
        vccOut = nx_sansamp_classic_capture_get_vcc_f32(sansampCapture_);
        return true;
    case CircuitKind::SansampOutput:
        if (sansampOutput_ == nullptr)
            return false;
        nx_sansamp_classic_output_process_f32(sansampOutput_, input, output, totalSamples);
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
