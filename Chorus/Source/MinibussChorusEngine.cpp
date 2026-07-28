#include "MinibussChorusEngine.h"

#include <algorithm>
#include <cmath>

#include <juce_core/juce_core.h>

#include "plugins/builtin_plugins.hpp"

#include "nudsp/common/control_params.h"

MinibussChorusEngine::MinibussChorusEngine() = default;

MinibussChorusEngine::~MinibussChorusEngine()
{
    release();
}

minibuss::PluginDescription MinibussChorusEngine::makeDesc (const char* uid,
                                                            const char* name,
                                                            std::uint32_t io) const
{
    minibuss::PluginDescription d;
    d.uid = uid;
    d.name = name;
    d.format_name = "MinibussStatic";
    d.file_or_identifier = uid;
    d.num_inputs = io;
    d.num_outputs = io;
    return d;
}

minibuss::Processor* MinibussChorusEngine::processor (minibuss::ObjectId id) const
{
    if (engine_ == nullptr || id == minibuss::kInvalidObjectId)
        return nullptr;
    if (auto shared = engine_->processors().processor (id))
        return shared.get();
    return nullptr;
}

void MinibussChorusEngine::setParamDomain (minibuss::ObjectId processorId,
                                           std::string_view paramId,
                                           float domainValue)
{
    auto* proc = processor (processorId);
    if (proc == nullptr)
        return;

    const auto* desc = proc->parameter (paramId);
    if (desc == nullptr)
        return;

    // Dedupe: push only when the normalized value actually changes. processBlock
    // calls this every block; unconditional pushes reset NuDSP config each block
    // (smoother snaps) and cause parameter zipper noise.
    const float normalized = desc->normalize (domainValue);
    float current = 0.f;
    if (proc->get_parameter (desc->index, current) == minibuss::Status::Ok
        && std::abs (current - normalized) <= 1.0e-7f)
        return;

    (void) proc->set_parameter (desc->index, normalized);
}

void MinibussChorusEngine::setParamNormalized (minibuss::ObjectId processorId,
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
    if (proc->get_parameter (desc->index, current) == minibuss::Status::Ok
        && std::abs (current - clamped) <= 1.0e-7f)
        return;

    (void) proc->set_parameter (desc->index, clamped);
}

const minibuss::ParameterDescriptor* MinibussChorusEngine::paramDescriptor (
    minibuss::ObjectId processorId, std::string_view paramId) const
{
    if (auto* proc = processor (processorId))
        return proc->parameter (paramId);
    return nullptr;
}

float MinibussChorusEngine::mapControlToDomain (minibuss::ObjectId processorId,
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

void MinibussChorusEngine::prepare (float sampleRate, std::uint32_t maxBlockSize)
{
    release();

    auto staticFormat = std::make_unique<minibuss::StaticPluginFormat>();
    minibuss::plugins::register_builtin_plugins (*staticFormat);
    formats_.add_format (std::move (staticFormat));

    engine_ = std::make_unique<minibuss::AudioEngine> (2, maxBlockSize);
    engine_->set_format_manager (&formats_);
    engine_->prepare (sampleRate, maxBlockSize);

    auto [trackSt, trackId] = engine_->create_track ("main", 2);
    if (trackSt != minibuss::Status::Ok)
        return;
    trackId_ = trackId;

    auto create = [this] (const char* uid, const char* name, const char* instance)
        -> minibuss::ObjectId
    {
        auto [st, id] = engine_->create_processor (makeDesc (uid, name), instance);
        if (st != minibuss::Status::Ok)
            return minibuss::kInvalidObjectId;
        if (engine_->add_plugin_to_track (id, trackId_) != minibuss::Status::Ok)
            return minibuss::kInvalidObjectId;
        return id;
    };

    gainId_ = create ("com.minibuss.nudsp.gain", "Gain", "gain");
    gateId_ = create ("com.minibuss.nudsp.camel.noise_gate", "Noise Gate", "gate");
    chorusId_ = create ("com.chorus.nudsp.camel.mono_chorus", "Mono Chorus", "chorus");
    phase90Id_ = create ("com.chorus.nudsp.camel.phase90", "Phase90", "phase90");
    levelId_ = create ("com.minibuss.nudsp.level", "Level", "level");

    if (gainId_ == minibuss::kInvalidObjectId
        || gateId_ == minibuss::kInvalidObjectId
        || chorusId_ == minibuss::kInvalidObjectId
        || phase90Id_ == minibuss::kInvalidObjectId
        || levelId_ == minibuss::kInvalidObjectId)
    {
        release();
        return;
    }

    engine_->connect_audio_input (0, 0, trackId_);
    engine_->connect_audio_input (1, 1, trackId_);
    engine_->connect_audio_output (0, 0, trackId_);
    engine_->connect_audio_output (1, 1, trackId_);

    applyModelBypass();
    ready_ = true;
}

void MinibussChorusEngine::release()
{
    ready_ = false;
    engine_.reset();
    formats_ = minibuss::PluginFormatManager {};
    trackId_ = gainId_ = gateId_ = chorusId_ = phase90Id_ = levelId_ = minibuss::kInvalidObjectId;
}

void MinibussChorusEngine::sendProcessorBypass (minibuss::ObjectId processorId, bool bypassed)
{
    if (auto* proc = processor (processorId))
        proc->set_bypassed (bypassed);
}

void MinibussChorusEngine::applyModelBypass()
{
    const bool chorusActive = (model_ == EffectModel::Chorus);
    sendProcessorBypass (chorusId_, ! chorusActive || bypassAll_);
    sendProcessorBypass (phase90Id_, chorusActive || bypassAll_);
}

void MinibussChorusEngine::setEffectModel (EffectModel model)
{
    model_ = model;
    applyModelBypass();
}

void MinibussChorusEngine::setBypass (bool shouldBypass)
{
    bypassAll_ = shouldBypass;
    applyModelBypass();
}

void MinibussChorusEngine::process (std::span<const float* const> inputs,
                                    std::span<float* const> outputs,
                                    std::uint32_t numFrames)
{
    if (! ready_ || engine_ == nullptr)
        return;
    engine_->process (inputs, outputs, numFrames);
}

void MinibussChorusEngine::readPostGainPeaks (float& leftPeak, float& rightPeak) const noexcept
{
    leftPeak = 0.f;
    rightPeak = 0.f;
    if (auto* gain = processor (gainId_))
    {
        leftPeak = gain->read_meter (0);
        rightPeak = gain->read_meter (1);
    }
}
