#include "SineWavePreviewEngine.h"

#include "SchematicComponentApply.h"

namespace ds1_ac
{
namespace
{

constexpr double kClipperPreviewVcc = 9.0;

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

void applyDs1OpampPotTaper(nx_ds1_opamp_f32_t* opamp, nx_pot_taper_e taper) noexcept
{
    updatePotTaper(opamp, nx_ds1_opamp_get_gain_pot_f32, nx_ds1_opamp_set_gain_pot_f32, taper);
}

void applyDs1ClipperPotTapers(nx_ds1_clipper_f32_t* clipper, nx_pot_taper_e taper) noexcept
{
    updatePotTaper(clipper, nx_ds1_clipper_get_tone_pot_f32, nx_ds1_clipper_set_tone_pot_f32, taper);
    updatePotTaper(clipper, nx_ds1_clipper_get_level_pot_f32, nx_ds1_clipper_set_level_pot_f32, taper);
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
        applyDs1ClipperPotTapers(ds1Clipper_, setup.potTaper);
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

    case CircuitKind::Ds1Opamp:
    default:
        ds1Opamp_ = nx_ds1_opamp_create_f32(nullptr);
        if (ds1Opamp_ == nullptr)
            return false;
        applySchematicValues(CircuitKind::Ds1Opamp, ds1Opamp_, setup.componentValues);
        applyDs1OpampPotTaper(ds1Opamp_, setup.potTaper);
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
        vccOut = kClipperPreviewVcc;
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
