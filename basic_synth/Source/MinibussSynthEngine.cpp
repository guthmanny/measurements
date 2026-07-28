#include "MinibussSynthEngine.h"

#include <cmath>

#include "plugins/builtin_plugins.hpp"

MinibussSynthEngine::MinibussSynthEngine() = default;

MinibussSynthEngine::~MinibussSynthEngine()
{
    release();
}

minibuss::PluginDescription MinibussSynthEngine::makeDesc (const char* uid,
                                                           const char* name) const
{
    minibuss::PluginDescription d;
    d.uid = uid;
    d.name = name;
    d.format_name = "MinibussStatic";
    d.file_or_identifier = uid;
    d.num_inputs = 0;
    d.num_outputs = 2;
    d.is_instrument = true;
    return d;
}

minibuss::Processor* MinibussSynthEngine::processor (minibuss::ObjectId id) const
{
    if (engine_ == nullptr || id == minibuss::kInvalidObjectId)
        return nullptr;
    if (auto shared = engine_->processors().processor (id))
        return shared.get();
    return nullptr;
}

void MinibussSynthEngine::prepare (float sampleRate, std::uint32_t maxBlockSize)
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

    auto [plugSt, plugId] = engine_->create_processor (
        makeDesc ("com.minibuss.nudsp.ssmel.basic_synth", "Basic Synth"), "synth");
    if (plugSt != minibuss::Status::Ok)
        return;
    synthId_ = plugId;

    if (engine_->add_plugin_to_track (synthId_, trackId_) != minibuss::Status::Ok)
        return;

    engine_->connect_audio_output (0, 0, trackId_);
    engine_->connect_audio_output (1, 1, trackId_);
    engine_->midi_dispatcher().connect_kb_to_track (0, trackId_);

    ready_ = true;
}

void MinibussSynthEngine::release()
{
    ready_ = false;
    engine_.reset();
    formats_ = minibuss::PluginFormatManager {};
    trackId_ = synthId_ = minibuss::kInvalidObjectId;
}

void MinibussSynthEngine::setParamDomain (minibuss::ObjectId processorId,
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
    if (proc->get_parameter (desc->index, current) == minibuss::Status::Ok
        && std::abs (current - normalized) <= 1.0e-7f)
        return;

    (void) proc->set_parameter (desc->index, normalized);
}

void MinibussSynthEngine::sendNoteOn (int note, float velocity)
{
    if (!ready_ || engine_ == nullptr)
        return;
    (void) engine_->send_note_on (trackId_, 0, note, velocity);
}

void MinibussSynthEngine::sendNoteOff (int note, float velocity)
{
    if (!ready_ || engine_ == nullptr)
        return;
    (void) engine_->send_note_off (trackId_, 0, note, velocity);
}

void MinibussSynthEngine::process (std::span<float* const> outputs, std::uint32_t numFrames)
{
    if (!ready_ || engine_ == nullptr || outputs.size() < 2)
        return;

    const float* inputs[2] = { nullptr, nullptr };
    float* outs[2] = { outputs[0], outputs[1] };
    engine_->process (inputs, outs, numFrames);
}
