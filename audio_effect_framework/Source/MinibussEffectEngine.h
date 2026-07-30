#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <string_view>

#include "minibuss/audio_engine.hpp"
#include "minibuss/parameter.hpp"
#include "minibuss/plugin_format_manager.hpp"
#include "minibuss/static_plugin_format.hpp"

/** Owns a minibuss AudioEngine with one stereo track:
    Gain → NoiseGate → Upsampler → [middle] → Downsampler → Level

    Insert your effect processor between upsampler and downsampler via
    installMiddleProcessors(). */
class MinibussEffectEngine
{
public:
    MinibussEffectEngine();
    ~MinibussEffectEngine();

    /** Builds the graph on first call. Subsequent calls with the same sample rate
        and a block size that fits the prepared max are no-ops (e.g. ASIO channel
        toggles that re-enter prepareToPlay without needing DSP re-init). */
    void prepare (float sampleRate, std::uint32_t maxBlockSize);
    void release();

    void setBypass (bool shouldBypass);

    void setProcessorBypassed (minibuss::ObjectId processorId, bool bypassed);

    /** Push domain value to a processor parameter (uses minibuss reflection). */
    void setParamDomain (minibuss::ObjectId processorId, std::string_view paramId, float domainValue);

    /** Push normalized 0..1 directly (for discrete choice parameters). */
    void setParamNormalized (minibuss::ObjectId processorId, std::string_view paramId, float normalized);

    [[nodiscard]] const minibuss::ParameterDescriptor* paramDescriptor (
        minibuss::ObjectId processorId, std::string_view paramId) const;

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

    minibuss::ObjectId gainId() const noexcept { return gainId_; }
    minibuss::ObjectId gateId() const noexcept { return gateId_; }
    minibuss::ObjectId upsamplerId() const noexcept { return upsamplerId_; }
    minibuss::ObjectId downsamplerId() const noexcept { return downsamplerId_; }
    minibuss::ObjectId levelId() const noexcept { return levelId_; }
    minibuss::ObjectId middleProcessorId() const noexcept { return middleProcessorId_; }

protected:
    using ProcessorCreateFn = std::function<minibuss::ObjectId (const char* uid,
                                                                  const char* name,
                                                                  const char* instance)>;

    /** Override in plugin-specific engines to insert processors between upsampler and downsampler. */
    virtual bool installMiddleProcessors (const ProcessorCreateFn& create);

    minibuss::ObjectId middleProcessorId_ = minibuss::kInvalidObjectId;

private:
    [[nodiscard]] minibuss::PluginDescription makeDesc (const char* uid,
                                                        const char* name,
                                                        std::uint32_t io = 2) const;
    void sendProcessorBypass (minibuss::ObjectId processorId, bool bypassed);
    [[nodiscard]] minibuss::Processor* processor (minibuss::ObjectId id) const noexcept;
    void cacheProcessorPointers();
    void clearProcessorPointers() noexcept;
    void applyOversamplingToPlugins();
    void reprepareTrack();

    minibuss::PluginFormatManager formats_;
    std::unique_ptr<minibuss::AudioEngine> engine_;

    minibuss::ObjectId trackId_ = minibuss::kInvalidObjectId;
    minibuss::ObjectId gainId_ = minibuss::kInvalidObjectId;
    minibuss::ObjectId gateId_ = minibuss::kInvalidObjectId;
    minibuss::ObjectId upsamplerId_ = minibuss::kInvalidObjectId;
    minibuss::ObjectId downsamplerId_ = minibuss::kInvalidObjectId;
    minibuss::ObjectId levelId_ = minibuss::kInvalidObjectId;

    minibuss::Processor* gainProc_ = nullptr;
    minibuss::Processor* gateProc_ = nullptr;
    minibuss::Processor* upsamplerProc_ = nullptr;
    minibuss::Processor* middleProcessorProc_ = nullptr;
    minibuss::Processor* downsamplerProc_ = nullptr;
    minibuss::Processor* levelProc_ = nullptr;

    bool ready_ = false;
    bool bypassAll_ = false;
    float preparedSampleRate_ = 0.f;
    std::uint32_t preparedMaxBlockSize_ = 0;

    int oversampleFactor_ = 2;
    int upsamplerMode_ = 4;   // NX_UPSAMPLER_MODE_CUBIC
    int downsamplerMode_ = 3; // NX_DOWNSAMPLER_MODE_CUBIC
};
