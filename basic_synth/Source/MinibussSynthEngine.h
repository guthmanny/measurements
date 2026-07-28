#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <string_view>

#include "minibuss/audio_engine.hpp"
#include "minibuss/plugin_format_manager.hpp"
#include "minibuss/static_plugin_format.hpp"

class MinibussSynthEngine
{
public:
    MinibussSynthEngine();
    ~MinibussSynthEngine();

    void prepare (float sampleRate, std::uint32_t maxBlockSize);
    void release();

    void setParamDomain (minibuss::ObjectId processorId, std::string_view paramId, float domainValue);

    void sendNoteOn (int note, float velocity);
    void sendNoteOff (int note, float velocity);

    void process (std::span<float* const> outputs, std::uint32_t numFrames);

    bool isReady() const noexcept { return ready_; }

    minibuss::ObjectId synthId() const noexcept { return synthId_; }

private:
    [[nodiscard]] minibuss::PluginDescription makeDesc (const char* uid, const char* name) const;
    [[nodiscard]] minibuss::Processor* processor (minibuss::ObjectId id) const;

    minibuss::PluginFormatManager formats_;
    std::unique_ptr<minibuss::AudioEngine> engine_;

    minibuss::ObjectId trackId_ = minibuss::kInvalidObjectId;
    minibuss::ObjectId synthId_ = minibuss::kInvalidObjectId;
    bool ready_ = false;
};
