#include "KbussSynthEngine.h"

#include <cmath>

#include "plugins/builtin_plugins.hpp"

KbussSynthEngine::KbussSynthEngine() = default;

KbussSynthEngine::~KbussSynthEngine()
{
    release();
}

kbuss::PluginDescription KbussSynthEngine::makeDesc (const char* uid,
                                                           const char* name) const
{
    kbuss::PluginDescription d;
    d.uid = uid;
    d.name = name;
    d.format_name = kbuss::kPluginFormatNameStatic;
    d.file_or_identifier = uid;
    d.num_inputs = 0;
    d.num_outputs = 2;
    d.is_instrument = true;
    return d;
}

kbuss::Processor* KbussSynthEngine::processor (kbuss::ObjectId id) const
{
    if (engine_ == nullptr || id == kbuss::kInvalidObjectId)
        return nullptr;
    if (auto shared = engine_->processors().processor (id))
        return shared.get();
    return nullptr;
}

void KbussSynthEngine::prepare (float sampleRate, std::uint32_t maxBlockSize)
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

    auto [plugSt, plugId] = engine_->create_processor (
        makeDesc ("com.kbuss.nudsp.ssmel.basic_synth", "Basic Synth"), "synth");
    if (plugSt != kbuss::Status::Ok)
        return;
    synthId_ = plugId;

    if (engine_->add_plugin_to_track (synthId_, trackId_) != kbuss::Status::Ok)
        return;

    engine_->connect_audio_output (0, 0, trackId_);
    engine_->connect_audio_output (1, 1, trackId_);
    engine_->midi_dispatcher().connect_kb_to_track (0, trackId_);

    ready_ = true;
}

void KbussSynthEngine::release()
{
    ready_ = false;
    engine_.reset();
    formats_ = kbuss::PluginFormatManager {};
    trackId_ = synthId_ = kbuss::kInvalidObjectId;
}

void KbussSynthEngine::setParamDomain (kbuss::ObjectId processorId,
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

void KbussSynthEngine::sendNoteOn (int note, float velocity)
{
    if (!ready_ || engine_ == nullptr)
        return;
    (void) engine_->send_note_on (trackId_, 0, note, velocity);
}

void KbussSynthEngine::sendNoteOff (int note, float velocity)
{
    if (!ready_ || engine_ == nullptr)
        return;
    (void) engine_->send_note_off (trackId_, 0, note, velocity);
}

void KbussSynthEngine::process (std::span<float* const> outputs, std::uint32_t numFrames)
{
    if (!ready_ || engine_ == nullptr || outputs.size() < 2)
        return;

    const float* inputs[2] = { nullptr, nullptr };
    float* outs[2] = { outputs[0], outputs[1] };
    engine_->process (inputs, outs, numFrames);
}
