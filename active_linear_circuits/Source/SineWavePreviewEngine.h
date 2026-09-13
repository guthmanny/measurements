#pragma once

#include <mutex>

#include "Ds1OpampAcMath.h"
#include "SchematicComponentValues.h"

#include "nudsp/linear_circuits/ac_booster_eq_f32.h"
#include "nudsp/linear_circuits/ds1_opamp_f32.h"
#include "nudsp/linear_circuits/ds_plus_opamp_f32.h"
#include "nudsp/linear_circuits/guvnor_level_f32.h"
#include "nudsp/linear_circuits/guvnor_postamp_f32.h"
#include "nudsp/linear_circuits/guvnor_preamp_f32.h"
#include "nudsp/linear_circuits/klon_centaur_tone_f32.h"
#include "nudsp/linear_circuits/od1_post_f32.h"
#include "nudsp/linear_circuits/rc_level_f32.h"
#include "nudsp/linear_circuits/sd1_tone_f32.h"
#include "nudsp/linear_circuits/ts9_tone_f32.h"
#include "nudsp/nonlinear_circuits/ac_booster_drive_f32.h"
#include "nudsp/nonlinear_circuits/bjt_common_emitter_f32.h"
#include "nudsp/nonlinear_circuits/bjt_follower_f32.h"
#include "nudsp/nonlinear_circuits/bjt_follower_out_f32.h"
#include "nudsp/nonlinear_circuits/diode_clipper_f32.h"
#include "nudsp/nonlinear_circuits/ds1_clipper_f32.h"
#include "nudsp/nonlinear_circuits/guvnor_clipper_f32.h"
#include "nudsp/nonlinear_circuits/klon_centaur_f32.h"
#include "nudsp/nonlinear_circuits/od1_drive_f32.h"
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
    nx_diode_clipper_f32_t* diodeClipper_{nullptr};
};

} // namespace ds1_ac
