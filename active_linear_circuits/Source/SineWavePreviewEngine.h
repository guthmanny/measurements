#pragma once

#include <mutex>

#include "Ds1OpampAcMath.h"
#include "SchematicComponentValues.h"

#include "nudsp/linear_circuits/ac_booster_eq_f32.h"
#include "nudsp/linear_circuits/ds1_opamp_f32.h"
#include "nudsp/linear_circuits/ds_plus_opamp_f32.h"
#include "nudsp/linear_circuits/guvnor_level_f32.h"
#include "nudsp/linear_circuits/guvnor_postamp_f32.h"
#include "nudsp/linear_circuits/crunchbox_preamp_f32.h"
#include "nudsp/linear_circuits/guvnor_preamp_f32.h"
#include "nudsp/linear_circuits/klon_centaur_tone_f32.h"
#include "nudsp/linear_circuits/od1_post_f32.h"
#include "nudsp/linear_circuits/rat_opamp_f32.h"
#include "nudsp/linear_circuits/rc_level_f32.h"
#include "nudsp/linear_circuits/sansamp_classic_capture_f32.h"
#include "nudsp/linear_circuits/sansamp_classic_gain_ctrl_f32.h"
#include "nudsp/linear_circuits/sansamp_classic_output_f32.h"
#include "nudsp/linear_circuits/sansamp_classic_position_f32.h"
#include "nudsp/linear_circuits/sansamp_classic_postamp_f32.h"
#include "nudsp/linear_circuits/sansamp_classic_pre_eq_f32.h"
#include "nudsp/linear_circuits/sansamp_classic_preamp_f32.h"
#include "nudsp/linear_circuits/sansamp_classic_spk_f32.h"
#include "nudsp/linear_circuits/sd1_tone_f32.h"
#include "nudsp/linear_circuits/ts9_tone_f32.h"
#include "nudsp/nonlinear_circuits/ac_booster_drive_f32.h"
#include "nudsp/nonlinear_circuits/bjt_common_emitter_f32.h"
#include "nudsp/nonlinear_circuits/bjt_follower_f32.h"
#include "nudsp/nonlinear_circuits/bjt_follower_out_f32.h"
#include "nudsp/nonlinear_circuits/diode_clipper_f32.h"
#include "nudsp/nonlinear_circuits/ds1_clipper_f32.h"
#include "nudsp/nonlinear_circuits/crunchbox_clipper_f32.h"
#include "nudsp/nonlinear_circuits/crunchbox_clipper_presence_f32.h"
#include "nudsp/nonlinear_circuits/guvnor_clipper_f32.h"
#include "nudsp/nonlinear_circuits/jfet_follower_f32.h"
#include "nudsp/nonlinear_circuits/klon_centaur_f32.h"
#include "nudsp/nonlinear_circuits/od1_drive_f32.h"
#include "nudsp/linear_circuits/beod_bass_f32.h"
#include "nudsp/linear_circuits/beod_preamp_f32.h"
#include "nudsp/linear_circuits/beod_tight_f32.h"
#include "nudsp/linear_circuits/beod_trim_f32.h"
#include "nudsp/linear_circuits/dirty_shirley_mid_f32.h"
#include "nudsp/linear_circuits/dirty_shirley_tight_f32.h"
#include "nudsp/linear_circuits/fa1_buffer_f32.h"
#include "nudsp/linear_circuits/fa1_volume_f32.h"
#include "nudsp/linear_circuits/fa1_eq_f32.h"
#include "nudsp/linear_circuits/bad_monkey_low_f32.h"
#include "nudsp/nonlinear_circuits/ota_comp_f32.h"
#include "extensions/white_box/pedals/dirty_shirley_f32.h"
#include "nudsp/nonlinear_circuits/beod_clipper_f32.h"
#include "nudsp/nonlinear_circuits/beod_postgain_f32.h"
#include "nudsp/nonlinear_circuits/beod_pregain_f32.h"
#include "nudsp/linear_circuits/odr1_postamp_f32.h"
#include "nudsp/linear_circuits/op_buffer_f32.h"
#include "nudsp/linear_circuits/odr1_tone_f32.h"
#include "nudsp/nonlinear_circuits/odr1_clipper_f32.h"
#include "nudsp/nonlinear_circuits/odr1_drive_f32.h"
#include "nudsp/nonlinear_circuits/rat_clipper_f32.h"
#include "nudsp/nonlinear_circuits/rc_booster_drive1_f32.h"
#include "nudsp/nonlinear_circuits/ts9_opamp_f32.h"

namespace ds1_ac
{

struct SinePreviewSetupParams
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
    const SchematicComponentValues* componentValues{nullptr};
};

class SineWavePreviewEngine
{
public:
    ~SineWavePreviewEngine();

    void invalidate() noexcept;

    bool runProcess(const SinePreviewSetupParams& setup,
                    const float* input,
                    float* output,
                    std::size_t totalSamples,
                    double& vccOut);

private:
    struct StructuralKey;

    bool ensureStructuralReady(const SinePreviewSetupParams& setup);
    bool applyRuntimeControls(const SinePreviewSetupParams& setup);
    void destroyInstance() noexcept;
    bool resetStates() noexcept;
    bool runHotProcess(const float* input, float* output, std::size_t totalSamples, double& vccOut) noexcept;

    std::mutex processMutex_;

    CircuitKind circuit_{CircuitKind::BjtFollower};
    StructuralKey* key_{nullptr};

    nx_ds1_opamp_f32_t* ds1Opamp_{nullptr};
    nx_ds1_clipper_f32_t* ds1Clipper_{nullptr};
    nx_bjt_follower_f32_t* bjtFollower_{nullptr};
    nx_bjt_follower_out_f32_t* bjtFollowerOut_{nullptr};
    nx_bjt_common_emitter_f32_t* bjtCommonEmitter_{nullptr};
    nx_od1_drive_f32_t* od1Drive_{nullptr};
    nx_odr1_drive_f32_t* odr1Drive_{nullptr};
    nx_odr1_clipper_f32_t* odr1Clipper_{nullptr};
    nx_odr1_tone_f32_t* odr1Tone_{nullptr};
    nx_odr1_postamp_f32_t* odr1Postamp_{nullptr};
    nx_beod_preamp_f32_t* beodPreamp_{nullptr};
    nx_beod_tight_f32_t* beodTight_{nullptr};
    nx_dirty_shirley_tight_f32_t* dirtyShirleyTight_{nullptr};
    nx_fa1_buffer_f32_t* fa1Buffer_{nullptr};
    nx_fa1_volume_f32_t* fa1Volume_{nullptr};
    nx_fa1_eq_f32_t* fa1Eq_{nullptr};
    nx_bad_monkey_low_f32_t* badMonkeyLow_{nullptr};
    nx_ota_comp_f32_t* otaComp_{nullptr};
    nx_dirty_shirley_mid_f32_t* dirtyShirleyMid_{nullptr};
    nx_dirty_shirley_f32_t* dirtyShirley_{nullptr};
    nx_beod_pregain_f32_t* beodPregain_{nullptr};
    nx_beod_postgain_f32_t* beodPostgain_{nullptr};
    nx_beod_trim_f32_t* beodTrim_{nullptr};
    nx_beod_clipper_f32_t* beodClipper_{nullptr};
    nx_beod_bass_f32_t* beodBass_{nullptr};
    nx_op_buffer_f32_t* opBuffer_{nullptr};
    nx_sd1_tone_f32_t* sd1Tone_{nullptr};
    nx_rc_level_f32_t* rcLevel_{nullptr};
    nx_od1_post_f32_t* od1Post_{nullptr};
    nx_ac_booster_drive_f32_t* acBoosterDrive_{nullptr};
    nx_ac_booster_eq_f32_t* acBoosterEq_{nullptr};
    nx_rc_booster_drive1_f32_t* rcBoosterDrive1_{nullptr};
    nx_ds_plus_opamp_f32_t* dsPlusOpamp_{nullptr};
    nx_ts9_opamp_f32_t* ts9Opamp_{nullptr};
    nx_ts9_tone_f32_t* ts9Tone_{nullptr};
    nx_klon_centaur_f32_t* klonCentaur_{nullptr};
    nx_klon_centaur_tone_f32_t* klonCentaurTone_{nullptr};
    nx_guvnor_preamp_f32_t* guvnorPreamp_{nullptr};
    nx_guvnor_postamp_f32_t* guvnorPostamp_{nullptr};
    nx_guvnor_clipper_f32_t* guvnorClipper_{nullptr};
    nx_guvnor_level_f32_t* guvnorLevel_{nullptr};
    nx_crunchbox_preamp_f32_t* crunchboxPreamp_{nullptr};
    nx_crunchbox_clipper_f32_t* crunchboxClipper_{nullptr};
    nx_crunchbox_clipper_presence_f32_t* crunchboxClipperPresence_{nullptr};
    nx_diode_clipper_f32_t* diodeClipper_{nullptr};
    nx_rat_opamp_f32_t* ratOpamp_{nullptr};
    nx_rat_clipper_f32_t* ratClipper_{nullptr};
    nx_jfet_follower_f32_t* jfetFollower_{nullptr};
    nx_sansamp_classic_pre_eq_f32_t* sansampPreEq_{nullptr};
    nx_sansamp_classic_preamp_f32_t* sansampPreamp_{nullptr};
    nx_sansamp_classic_gain_ctrl_f32_t* sansampGainCtrl_{nullptr};
    nx_sansamp_classic_postamp_f32_t* sansampPostamp_{nullptr};
    nx_sansamp_classic_spk_f32_t* sansampSpk_{nullptr};
    nx_sansamp_classic_position_f32_t* sansampPosition_{nullptr};
    nx_sansamp_classic_capture_f32_t* sansampCapture_{nullptr};
    nx_sansamp_classic_output_f32_t* sansampOutput_{nullptr};
};

} // namespace ds1_ac
