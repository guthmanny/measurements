#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <string_view>

#include "kbuss/audio_engine.hpp"
#include "kbuss/plugin_format_manager.hpp"
#include "kbuss/static_plugin_format.hpp"
#include "kbuss/version.hpp"

class KbussSynthEngine
{
public:
    KbussSynthEngine();
    ~KbussSynthEngine();

    void prepare (float sampleRate, std::uint32_t maxBlockSize);
    void release();

    void setParamDomain (kbuss::ObjectId processorId, std::string_view paramId, float domainValue);

    void sendNoteOn (int note, float velocity);
    void sendNoteOff (int note, float velocity);

    void process (std::span<float* const> outputs, std::uint32_t numFrames);

    bool isReady() const noexcept { return ready_; }

    kbuss::ObjectId synthId() const noexcept { return synthId_; }

private:
    [[nodiscard]] kbuss::PluginDescription makeDesc (const char* uid, const char* name) const;
    [[nodiscard]] kbuss::Processor* processor (kbuss::ObjectId id) const;

    kbuss::PluginFormatManager formats_;
    std::unique_ptr<kbuss::AudioEngine> engine_;

    kbuss::ObjectId trackId_ = kbuss::kInvalidObjectId;
    kbuss::ObjectId synthId_ = kbuss::kInvalidObjectId;
    bool ready_ = false;
};

using MinibussSynthEngine = KbussSynthEngine;
