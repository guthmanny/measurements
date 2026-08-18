#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <string_view>

#include "kbuss/audio_engine.hpp"
#include "kbuss/parameter.hpp"
#include "kbuss/plugin_format_manager.hpp"
#include "kbuss/static_plugin_format.hpp"
#include "kbuss/version.hpp"

/** Owns a kbuss AudioEngine with one stereo track:
    Gain → NoiseGate → Upsampler → [middle] → Downsampler → Level

    Insert your effect processor between upsampler and downsampler via
    installMiddleProcessors(). */
class KbussEffectEngine
{
public:
    KbussEffectEngine();
    ~KbussEffectEngine();

    /** Builds the graph on first call. Subsequent calls with the same sample rate
        and a block size that fits the prepared max are no-ops (e.g. ASIO channel
        toggles that re-enter prepareToPlay without needing DSP re-init). */
    void prepare (float sampleRate, std::uint32_t maxBlockSize);
    void release();

    void setBypass (bool shouldBypass);

    void setProcessorBypassed (kbuss::ObjectId processorId, bool bypassed);

    /** Push domain value to a processor parameter (uses kbuss reflection). */
    void setParamDomain (kbuss::ObjectId processorId, std::string_view paramId, float domainValue);

    /** Push normalized 0..1 directly (for discrete choice parameters). */
    void setParamNormalized (kbuss::ObjectId processorId, std::string_view paramId, float normalized);

    [[nodiscard]] const kbuss::ParameterDescriptor* paramDescriptor (
        kbuss::ObjectId processorId, std::string_view paramId) const;

    /**
     * Oversampling around the middle processor.
     * @param factor 2 / 4 / 8
     * @param upMode   nx upsampler_mode_e
     * @param downMode nx downsampler_mode_e
     */
    void setOversampling (int factor, int upMode, int downMode);

    [[nodiscard]] int oversampleFactor() const noexcept { return oversampleFactor_; }

    void process (std::span<const float* const> inputs,
                  std::span<float* const> outputs,
                  std::uint32_t numFrames);

    /** Peak levels at gain output (slot 0 = L, slot 1 = R). */
    void readPostGainPeaks (float& leftPeak, float& rightPeak) const noexcept;

    bool isReady() const noexcept { return ready_; }

    kbuss::ObjectId gainId() const noexcept { return gainId_; }
    kbuss::ObjectId gateId() const noexcept { return gateId_; }
    kbuss::ObjectId upsamplerId() const noexcept { return upsamplerId_; }
    kbuss::ObjectId downsamplerId() const noexcept { return downsamplerId_; }
    kbuss::ObjectId levelId() const noexcept { return levelId_; }
    kbuss::ObjectId middleProcessorId() const noexcept { return middleProcessorId_; }
    kbuss::Processor* getMiddleProcessor() const noexcept { return middleProcessorProc_; }

protected:
    using ProcessorCreateFn = std::function<kbuss::ObjectId (const char* uid,
                                                                  const char* name,
                                                                  const char* instance)>;

    /** Override in plugin-specific engines to insert processors between upsampler and downsampler. */
    virtual bool installMiddleProcessors (const ProcessorCreateFn& create);

    kbuss::ObjectId middleProcessorId_ = kbuss::kInvalidObjectId;

private:
    [[nodiscard]] kbuss::PluginDescription makeDesc (const char* uid,
                                                        const char* name,
                                                        std::uint32_t io = 2) const;
    void sendProcessorBypass (kbuss::ObjectId processorId, bool bypassed);
    [[nodiscard]] kbuss::Processor* processor (kbuss::ObjectId id) const noexcept;
    void cacheProcessorPointers();
    void clearProcessorPointers() noexcept;
    void applyOversamplingToPlugins();
    void reprepareTrack();

    kbuss::PluginFormatManager formats_;
    std::unique_ptr<kbuss::AudioEngine> engine_;

    kbuss::ObjectId trackId_ = kbuss::kInvalidObjectId;
    kbuss::ObjectId gainId_ = kbuss::kInvalidObjectId;
    kbuss::ObjectId gateId_ = kbuss::kInvalidObjectId;
    kbuss::ObjectId upsamplerId_ = kbuss::kInvalidObjectId;
    kbuss::ObjectId downsamplerId_ = kbuss::kInvalidObjectId;
    kbuss::ObjectId levelId_ = kbuss::kInvalidObjectId;

    kbuss::Processor* gainProc_ = nullptr;
    kbuss::Processor* gateProc_ = nullptr;
    kbuss::Processor* upsamplerProc_ = nullptr;
    kbuss::Processor* middleProcessorProc_ = nullptr;
    kbuss::Processor* downsamplerProc_ = nullptr;
    kbuss::Processor* levelProc_ = nullptr;

    bool ready_ = false;
    bool bypassAll_ = false;
    float preparedSampleRate_ = 0.f;
    std::uint32_t preparedMaxBlockSize_ = 0;

    int oversampleFactor_ = 2;
    int upsamplerMode_ = 4;   // NX_UPSAMPLER_MODE_CUBIC
    int downsamplerMode_ = 3; // NX_DOWNSAMPLER_MODE_CUBIC
};

using MinibussEffectEngine = KbussEffectEngine;
