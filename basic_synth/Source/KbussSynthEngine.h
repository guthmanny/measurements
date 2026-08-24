#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>

#include "kbuss/audio_engine.hpp"
#include "kbuss/plugin_format_manager.hpp"
#include "kbuss/static_plugin_format.hpp"
#include "kbuss/version.hpp"

enum class SynthInstrument : int
{
    BasicSynth = 0,
    KsFlute = 1,
};

inline constexpr const char* synthInstrumentUid (SynthInstrument instrument)
{
    switch (instrument)
    {
        case SynthInstrument::KsFlute:
            return "com.kbuss.nudsp.ssmel.ks_flute";
        case SynthInstrument::BasicSynth:
        default:
            return "com.kbuss.nudsp.ssmel.basic_synth";
    }
}

inline constexpr const char* synthInstrumentName (SynthInstrument instrument)
{
    switch (instrument)
    {
        case SynthInstrument::KsFlute:
            return "KS Flute";
        case SynthInstrument::BasicSynth:
        default:
            return "Basic Synth";
    }
}

class KbussSynthEngine
{
public:
    KbussSynthEngine();
    ~KbussSynthEngine();

    void prepare (float sampleRate, std::uint32_t maxBlockSize,
                  SynthInstrument instrument = SynthInstrument::BasicSynth);
    void release();

    void setParamDomain (kbuss::ObjectId processorId, std::string_view paramId, float domainValue);

    void sendNoteOn (int note, float velocity);
    void sendNoteOff (int note, float velocity);

    void process (std::span<float* const> outputs, std::uint32_t numFrames);

    bool isReady() const noexcept { return ready_; }

    kbuss::ObjectId synthId() const noexcept { return synthId_; }
    SynthInstrument instrument() const noexcept { return instrument_; }

private:
    [[nodiscard]] kbuss::PluginDescription makeDesc (const char* uid, const char* name) const;
    [[nodiscard]] kbuss::Processor* processor (kbuss::ObjectId id) const;

    kbuss::PluginFormatManager formats_;
    std::unique_ptr<kbuss::AudioEngine> engine_;

    kbuss::ObjectId trackId_ = kbuss::kInvalidObjectId;
    kbuss::ObjectId synthId_ = kbuss::kInvalidObjectId;
    SynthInstrument instrument_ = SynthInstrument::BasicSynth;
    std::uint32_t maxBlockSize_ = 0;
    bool ready_ = false;
};

using MinibussSynthEngine = KbussSynthEngine;
