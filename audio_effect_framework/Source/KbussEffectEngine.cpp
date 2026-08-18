#include "AefJuceIncludes.h"
#include "KbussEffectEngine.h"

#include <cmath>

#include <juce_core/juce_core.h>

#include "plugins/builtin_plugins.hpp"

KbussEffectEngine::KbussEffectEngine() = default;

KbussEffectEngine::~KbussEffectEngine()
{
    release();
}

kbuss::PluginDescription KbussEffectEngine::makeDesc (const char* uid,
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

kbuss::Processor* KbussEffectEngine::processor (kbuss::ObjectId id) const noexcept
{
    if (id == kbuss::kInvalidObjectId)
        return nullptr;
    if (id == gainId_)
        return gainProc_;
    if (id == gateId_)
        return gateProc_;
    if (id == upsamplerId_)
        return upsamplerProc_;
    if (id == middleProcessorId_)
        return middleProcessorProc_;
    if (id == downsamplerId_)
        return downsamplerProc_;
    if (id == levelId_)
        return levelProc_;
    return nullptr;
}

void KbussEffectEngine::clearProcessorPointers() noexcept
{
    gainProc_ = gateProc_ = upsamplerProc_ = middleProcessorProc_ = downsamplerProc_ = levelProc_ = nullptr;
}

void KbussEffectEngine::cacheProcessorPointers()
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
    upsamplerProc_ = resolve (upsamplerId_);
    middleProcessorProc_ = resolve (middleProcessorId_);
    downsamplerProc_ = resolve (downsamplerId_);
    levelProc_ = resolve (levelId_);
}

void KbussEffectEngine::setParamDomain (kbuss::ObjectId processorId,
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

void KbussEffectEngine::setParamNormalized (kbuss::ObjectId processorId,
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

const kbuss::ParameterDescriptor* KbussEffectEngine::paramDescriptor (
    kbuss::ObjectId processorId, std::string_view paramId) const
{
    if (auto* proc = processor (processorId))
        return proc->parameter (paramId);
    return nullptr;
}

bool KbussEffectEngine::installMiddleProcessors (const ProcessorCreateFn& /*create*/)
{
    return true;
}

void KbussEffectEngine::applyOversamplingToPlugins()
{
    setParamDomain (upsamplerId_, "factor", (float) oversampleFactor_);
    setParamDomain (downsamplerId_, "factor", (float) oversampleFactor_);
    setParamDomain (upsamplerId_, "mode", (float) upsamplerMode_);
    setParamDomain (downsamplerId_, "mode", (float) downsamplerMode_);
}

void KbussEffectEngine::reprepareTrack()
{
    if (engine_ == nullptr || ! ready_)
        return;
    engine_->prepare (preparedSampleRate_, preparedMaxBlockSize_);
}

void KbussEffectEngine::setOversampling (int factor, int upMode, int downMode)
{
    const int clampedFactor = factor >= 8 ? 8 : (factor >= 4 ? 4 : 2);
    const int clampedUp = juce::jlimit (0, 4, upMode);
    const int clampedDown = juce::jlimit (0, 3, downMode);

    const bool factorChanged = clampedFactor != oversampleFactor_;
    const bool modeChanged = clampedUp != upsamplerMode_ || clampedDown != downsamplerMode_;

    oversampleFactor_ = clampedFactor;
    upsamplerMode_ = clampedUp;
    downsamplerMode_ = clampedDown;

    if (! ready_)
        return;

    applyOversamplingToPlugins();

    if (factorChanged)
        reprepareTrack();
    else if (modeChanged)
    {
        // Mode can allocate inside MuDSP; keep it off the hot path when factor is stable
        // by only touching the resampling plugins.
        if (upsamplerProc_ != nullptr)
            upsamplerProc_->prepare (preparedSampleRate_, preparedMaxBlockSize_);
        if (downsamplerProc_ != nullptr)
            downsamplerProc_->prepare (preparedSampleRate_ * (float) oversampleFactor_,
                                       preparedMaxBlockSize_ * (std::uint32_t) oversampleFactor_);
    }
}

void KbussEffectEngine::prepare (float sampleRate, std::uint32_t maxBlockSize)
{
    maxBlockSize = juce::jmax (std::uint32_t (1), maxBlockSize);

    // Device reopen for input-channel mask changes re-enters prepareToPlay with the
    // same rate/block size. Keep the live graph so MuDSP modules are not re-inited.
    if (ready_ && engine_ != nullptr
        && std::abs (sampleRate - preparedSampleRate_) <= 0.5f
        && maxBlockSize <= preparedMaxBlockSize_)
    {
        return;
    }

    // Same sample rate but larger host block: soft-prepare without destroying plugins.
    if (ready_ && engine_ != nullptr
        && std::abs (sampleRate - preparedSampleRate_) <= 0.5f
        && maxBlockSize > preparedMaxBlockSize_)
    {
        preparedMaxBlockSize_ = maxBlockSize;
        engine_->prepare (sampleRate, maxBlockSize);
        applyOversamplingToPlugins();
        return;
    }

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
    upsamplerId_ = create ("com.kbuss.nudsp.up_sampler", "Upsampler", "upsampler");

    if (gainId_ == kbuss::kInvalidObjectId
        || gateId_ == kbuss::kInvalidObjectId
        || upsamplerId_ == kbuss::kInvalidObjectId
        || ! installMiddleProcessors (create))
    {
        release();
        return;
    }

    downsamplerId_ = create ("com.kbuss.nudsp.down_sampler", "Downsampler", "downsampler");
    levelId_ = create ("com.kbuss.nudsp.level", "Level", "level");

    if (downsamplerId_ == kbuss::kInvalidObjectId
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
    if (gainProc_ == nullptr || gateProc_ == nullptr || levelProc_ == nullptr
        || upsamplerProc_ == nullptr || downsamplerProc_ == nullptr
        || (middleProcessorId_ != kbuss::kInvalidObjectId && middleProcessorProc_ == nullptr))
    {
        release();
        return;
    }

    preparedSampleRate_ = sampleRate;
    preparedMaxBlockSize_ = maxBlockSize;
    ready_ = true;

    applyOversamplingToPlugins();
    // Full chain exists now — rate-aware prepare so middle runs at host * factor.
    engine_->prepare (sampleRate, maxBlockSize);
}

void KbussEffectEngine::release()
{
    ready_ = false;
    preparedSampleRate_ = 0.f;
    preparedMaxBlockSize_ = 0;
    clearProcessorPointers();
    engine_.reset();
    formats_ = kbuss::PluginFormatManager {};
    trackId_ = gainId_ = gateId_ = upsamplerId_ = downsamplerId_ = levelId_
        = middleProcessorId_ = kbuss::kInvalidObjectId;
}

void KbussEffectEngine::sendProcessorBypass (kbuss::ObjectId processorId, bool bypassed)
{
    if (auto* proc = processor (processorId))
        proc->set_bypassed (bypassed);
}

void KbussEffectEngine::setProcessorBypassed (kbuss::ObjectId processorId, bool bypassed)
{
    sendProcessorBypass (processorId, bypassed);
}

void KbussEffectEngine::setBypass (bool shouldBypass)
{
    if (bypassAll_ == shouldBypass)
        return;
    bypassAll_ = shouldBypass;
    if (middleProcessorId_ != kbuss::kInvalidObjectId)
        sendProcessorBypass (middleProcessorId_, shouldBypass);
}

void KbussEffectEngine::process (std::span<const float* const> inputs,
                                    std::span<float* const> outputs,
                                    std::uint32_t numFrames)
{
    if (! ready_ || engine_ == nullptr)
        return;
    engine_->process (inputs, outputs, numFrames);
}

void KbussEffectEngine::readPostGainPeaks (float& leftPeak, float& rightPeak) const noexcept
{
    leftPeak = 0.f;
    rightPeak = 0.f;
    if (gainProc_ != nullptr)
    {
        leftPeak = gainProc_->read_meter (0);
        rightPeak = gainProc_->read_meter (1);
    }
}
