#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <string_view>

#include "kbuss/audio_engine.hpp"
#include "kbuss/parameter.hpp"
#include "kbuss/plugin_format_manager.hpp"
#include "kbuss/static_plugin_format.hpp"

/** Owns a kbuss AudioEngine with one stereo track:
    Gain → NoiseGate → MonoChorus → Phase90 → Level
    Exactly one of Chorus/Phase90 is active based on EffectModel. */
class KbussChorusEngine
{
public:
    enum class EffectModel
    {
        Chorus = 0,
        Phase90 = 1
    };

    KbussChorusEngine();
    ~KbussChorusEngine();

    void prepare (float sampleRate, std::uint32_t maxBlockSize);
    void release();

    void setEffectModel (EffectModel model);
    EffectModel getEffectModel() const noexcept { return model_; }

    void setBypass (bool shouldBypass);

    /** Push domain value to a processor parameter (uses kbuss reflection). */
    void setParamDomain (kbuss::ObjectId processorId, std::string_view paramId, float domainValue);

    /** Push normalized 0..1 directly (for discrete choice parameters). */
    void setParamNormalized (kbuss::ObjectId processorId, std::string_view paramId, float normalized);

    [[nodiscard]] const kbuss::ParameterDescriptor* paramDescriptor (
        kbuss::ObjectId processorId, std::string_view paramId) const;

    /** Map MIDI CC (0..1 control) to domain using MuDSP taper from @p paramId. */
    [[nodiscard]] float mapControlToDomain (kbuss::ObjectId processorId,
                                            std::string_view paramId,
                                            float controlNormalized,
                                            float minDomain,
                                            float maxDomain) const noexcept;

    void process (std::span<const float* const> inputs,
                  std::span<float* const> outputs,
                  std::uint32_t numFrames);

    /** Peak levels at gain output (slot 0 = L, slot 1 = R). */
    void readPostGainPeaks (float& leftPeak, float& rightPeak) const noexcept;

    bool isReady() const noexcept { return ready_; }

    kbuss::ObjectId gainId() const noexcept { return gainId_; }
    kbuss::ObjectId gateId() const noexcept { return gateId_; }
    kbuss::ObjectId chorusId() const noexcept { return chorusId_; }
    kbuss::ObjectId phase90Id() const noexcept { return phase90Id_; }
    kbuss::ObjectId levelId() const noexcept { return levelId_; }

private:
    [[nodiscard]] kbuss::PluginDescription makeDesc (const char* uid,
                                                        const char* name,
                                                        std::uint32_t io = 2) const;
    void applyModelBypass();
    void sendProcessorBypass (kbuss::ObjectId processorId, bool bypassed);
    [[nodiscard]] kbuss::Processor* processor (kbuss::ObjectId id) const noexcept;
    void cacheProcessorPointers();
    void clearProcessorPointers() noexcept;

    kbuss::PluginFormatManager formats_;
    std::unique_ptr<kbuss::AudioEngine> engine_;

    kbuss::ObjectId trackId_ = kbuss::kInvalidObjectId;
    kbuss::ObjectId gainId_ = kbuss::kInvalidObjectId;
    kbuss::ObjectId gateId_ = kbuss::kInvalidObjectId;
    kbuss::ObjectId chorusId_ = kbuss::kInvalidObjectId;
    kbuss::ObjectId phase90Id_ = kbuss::kInvalidObjectId;
    kbuss::ObjectId levelId_ = kbuss::kInvalidObjectId;

    kbuss::Processor* gainProc_ = nullptr;
    kbuss::Processor* gateProc_ = nullptr;
    kbuss::Processor* chorusProc_ = nullptr;
    kbuss::Processor* phase90Proc_ = nullptr;
    kbuss::Processor* levelProc_ = nullptr;

    EffectModel model_ = EffectModel::Chorus;
    bool ready_ = false;
    bool bypassAll_ = false;
};

using MinibussChorusEngine = KbussChorusEngine;
