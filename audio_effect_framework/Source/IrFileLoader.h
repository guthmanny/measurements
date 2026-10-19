#pragma once

#include "AefJuceIncludes.h"

#include <cstdint>
#include <vector>

/** Decode convolution IRs from WAV/AIFF or raw PCM, with header auto-detect. */
struct IrHeaderInfo
{
    enum class Kind
    {
        Unknown = 0,
        Wav,
        Aiff,
        Raw
    };

    Kind kind = Kind::Unknown;
    int headerBytes = 0;
    int sampleRate = 0;
    int numChannels = 0;
    int bitsPerSample = 0;
    juce::int64 numFrames = 0;
    juce::String description;
};

struct IrLoadRequest
{
    enum class Format
    {
        Auto = 0,
        Wav,
        Aiff,
        RawF32,
        RawI16
    };

    juce::File file;
    Format format = Format::Auto;
    int headerSkipBytes = 0;
    juce::int64 startSample = 0;
    juce::int64 lengthSamples = 0; /**< 0 = remaining samples after start. */
};

struct IrLoadResult
{
    bool ok = false;
    juce::String error;
    IrHeaderInfo header;
    std::vector<float> samples;
    double sampleRate = 48000.0;
};

class IrFileLoader
{
public:
    [[nodiscard]] static IrHeaderInfo detectHeader (const juce::File& file);
    [[nodiscard]] static IrLoadResult load (const IrLoadRequest& request);

    /** Mix decoded channels to mono, then slice [start, start+length) and prepend delay zeros. */
    [[nodiscard]] static std::vector<float> buildAppliedIr (const std::vector<float>& decodedMono,
                                                            juce::int64 startSample,
                                                            juce::int64 lengthSamples,
                                                            int delaySamples,
                                                            int maxIrLen);
};
