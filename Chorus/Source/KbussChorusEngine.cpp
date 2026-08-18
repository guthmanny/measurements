#include "KbussChorusEngine.h"

#include <algorithm>
#include <cmath>

#include <juce_core/juce_core.h>

#include "plugins/builtin_plugins.hpp"

#include "kbuss/version.hpp"
#include "nudsp/common/control_params.h"

KbussChorusEngine::KbussChorusEngine() = default;

KbussChorusEngine::~KbussChorusEngine()
{
    release();
}

kbuss::PluginDescription KbussChorusEngine::makeDesc (const char* uid,
                                                            const char* name,
                                                            std::uint32_t io) const
{
    kbuss::PluginDescription d;
    d.uid = uid;
    d.name = name;
    d.format_name = kbuss::kPluginFormatNameStatic;
    d.file_or_identifier = uid;
    d.num_inputs = io;
    d.num_outputs = io;
    return d;
}

kbuss::Processor* KbussChorusEngine::processor (kbuss::ObjectId id) const noexcept
{
    if (id == kbuss::kInvalidObjectId)
        return nullptr;
    if (id == gainId_)
        return gainProc_;
    if (id == gateId_)
        return gateProc_;
    if (id == chorusId_)
        return chorusProc_;
    if (id == phase90Id_)
        return phase90Proc_;
    if (id == levelId_)
        return levelProc_;
    return nullptr;
}

void KbussChorusEngine::clearProcessorPointers() noexcept
{
    gainProc_ = gateProc_ = chorusProc_ = phase90Proc_ = levelProc_ = nullptr;
}

void KbussChorusEngine::cacheProcessorPointers()
{
    clearProcessorPointers();
    if (engine_ == nullptr)
        return;

    auto resolve = [this] (kbuss::ObjectId id) -> kbuss::Processor*
    {
        if (id == kbuss::kInvalidObjectId)
            return nullptr;
        if (auto shared = engine_->processors().processor (id))
            return shared.get();
        return nullptr;
    };

    gainProc_ = resolve (gainId_);
    gateProc_ = resolve (gateId_);
    chorusProc_ = resolve (chorusId_);
    phase90Proc_ = resolve (phase90Id_);
    levelProc_ = resolve (levelId_);
}

void KbussChorusEngine::setParamDomain (kbuss::ObjectId processorId,
                                           std::string_view paramId,
                                           float domainValue)
{
    auto* proc = processor (processorId);
    if (proc == nullptr)
        return;

    const auto* desc = proc->parameter (paramId);
    if (desc == nullptr)
        return;

    const float normalized = desc->normalize (domainValue);
    float current = 0.f;
    if (proc->get_parameter (desc->index, current) == kbuss::Status::Ok
        && std::abs (current - normalized) <= 1.0e-7f)
        return;

    (void) proc->set_parameter (desc->index, normalized);
}

void KbussChorusEngine::setParamNormalized (kbuss::ObjectId processorId,
                                               std::string_view paramId,
                                               float normalized)
{
    auto* proc = processor (processorId);
    if (proc == nullptr)
        return;

    const auto* desc = proc->parameter (paramId);
    if (desc == nullptr)
        return;

    const float clamped = juce::jlimit (0.f, 1.f, normalized);
    float current = 0.f;
    if (proc->get_parameter (desc->index, current) == kbuss::Status::Ok
        && std::abs (current - clamped) <= 1.0e-7f)
        return;

    (void) proc->set_parameter (desc->index, clamped);
}

const kbuss::ParameterDescriptor* KbussChorusEngine::paramDescriptor (
    kbuss::ObjectId processorId, std::string_view paramId) const
{
    if (auto* proc = processor (processorId))
        return proc->parameter (paramId);
    return nullptr;
}

float KbussChorusEngine::mapControlToDomain (kbuss::ObjectId processorId,
                                               std::string_view paramId,
                                               float controlNormalized,
                                               float minDomain,
                                               float maxDomain) const noexcept
{
    const auto* desc = paramDescriptor (processorId, paramId);
    if (desc == nullptr || maxDomain <= minDomain)
        return minDomain;

    nx_control_config_t cfg = desc->to_control_config();
    cfg.min_value = minDomain;
    cfg.max_value = maxDomain;
    const double control = juce::jlimit (0.0, 1.0, (double) controlNormalized);
    return static_cast<float> (control_map (&cfg, control));
}

void KbussChorusEngine::prepare (float sampleRate, std::uint32_t maxBlockSize)
{
    release();

    auto staticFormat = std::make_unique<kbuss::StaticPluginFormat>();
    kbuss::plugins::register_builtin_plugins (*staticFormat);
    formats_.add_format (std::move (staticFormat));

    engine_ = std::make_unique<kbuss::AudioEngine> (2, maxBlockSize);
    engine_->set_format_manager (&formats_);
    engine_->prepare (sampleRate, maxBlockSize);

    auto [trackSt, trackId] = engine_->create_track ("main", 2);
    if (trackSt != kbuss::Status::Ok)
        return;
    trackId_ = trackId;

    auto create = [this] (const char* uid, const char* name, const char* instance)
        -> kbuss::ObjectId
    {
        auto [st, id] = engine_->create_processor (makeDesc (uid, name), instance);
        if (st != kbuss::Status::Ok)
            return kbuss::kInvalidObjectId;
        if (engine_->add_plugin_to_track (id, trackId_) != kbuss::Status::Ok)
            return kbuss::kInvalidObjectId;
        return id;
    };

    gainId_ = create ("com.kbuss.nudsp.gain", "Gain", "gain");
    gateId_ = create ("com.kbuss.nudsp.camel.noise_gate", "Noise Gate", "gate");
    chorusId_ = create ("com.chorus.nudsp.camel.mono_chorus", "Mono Chorus", "chorus");
    phase90Id_ = create ("com.chorus.nudsp.camel.phase90", "Phase90", "phase90");
    levelId_ = create ("com.kbuss.nudsp.level", "Level", "level");

    if (gainId_ == kbuss::kInvalidObjectId
        || gateId_ == kbuss::kInvalidObjectId
        || chorusId_ == kbuss::kInvalidObjectId
        || phase90Id_ == kbuss::kInvalidObjectId
        || levelId_ == kbuss::kInvalidObjectId)
    {
        release();
        return;
    }

    engine_->connect_audio_input (0, 0, trackId_);
    engine_->connect_audio_input (1, 1, trackId_);
    engine_->connect_audio_output (0, 0, trackId_);
    engine_->connect_audio_output (1, 1, trackId_);

    cacheProcessorPointers();
    if (gainProc_ == nullptr || gateProc_ == nullptr || chorusProc_ == nullptr
        || phase90Proc_ == nullptr || levelProc_ == nullptr)
    {
        release();
        return;
    }

    applyModelBypass();
    ready_ = true;
}

void KbussChorusEngine::release()
{
    ready_ = false;
    clearProcessorPointers();
    engine_.reset();
    formats_ = kbuss::PluginFormatManager {};
    trackId_ = gainId_ = gateId_ = chorusId_ = phase90Id_ = levelId_ = kbuss::kInvalidObjectId;
}

void KbussChorusEngine::sendProcessorBypass (kbuss::ObjectId processorId, bool bypassed)
{
    if (auto* proc = processor (processorId))
        proc->set_bypassed (bypassed);
}

void KbussChorusEngine::applyModelBypass()
{
    const bool chorusActive = (model_ == EffectModel::Chorus);
    sendProcessorBypass (chorusId_, ! chorusActive || bypassAll_);
    sendProcessorBypass (phase90Id_, chorusActive || bypassAll_);
}

void KbussChorusEngine::setEffectModel (EffectModel model)
{
    if (model_ == model)
        return;
    model_ = model;
    applyModelBypass();
}

void KbussChorusEngine::setBypass (bool shouldBypass)
{
    if (bypassAll_ == shouldBypass)
        return;
    bypassAll_ = shouldBypass;
    applyModelBypass();
}

void KbussChorusEngine::process (std::span<const float* const> inputs,
                                    std::span<float* const> outputs,
                                    std::uint32_t numFrames)
{
    if (! ready_ || engine_ == nullptr)
        return;
    engine_->process (inputs, outputs, numFrames);
}

void KbussChorusEngine::readPostGainPeaks (float& leftPeak, float& rightPeak) const noexcept
{
    leftPeak = 0.f;
    rightPeak = 0.f;
    if (gainProc_ != nullptr)
    {
        leftPeak = gainProc_->read_meter (0);
        rightPeak = gainProc_->read_meter (1);
    }
}
