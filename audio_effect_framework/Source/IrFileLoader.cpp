#include "IrFileLoader.h"

#include <juce_audio_formats/juce_audio_formats.h>

#include <algorithm>
#include <cstring>

namespace
{
std::uint16_t read16le (const std::uint8_t* p) noexcept
{
    return static_cast<std::uint16_t> (p[0] | (static_cast<std::uint16_t> (p[1]) << 8));
}

std::uint32_t read32le (const std::uint8_t* p) noexcept
{
    return static_cast<std::uint32_t> (p[0])
         | (static_cast<std::uint32_t> (p[1]) << 8)
         | (static_cast<std::uint32_t> (p[2]) << 16)
         | (static_cast<std::uint32_t> (p[3]) << 24);
}

std::uint16_t read16be (const std::uint8_t* p) noexcept
{
    return static_cast<std::uint16_t> ((static_cast<std::uint16_t> (p[0]) << 8) | p[1]);
}

std::uint32_t read32be (const std::uint8_t* p) noexcept
{
    return (static_cast<std::uint32_t> (p[0]) << 24)
         | (static_cast<std::uint32_t> (p[1]) << 16)
         | (static_cast<std::uint32_t> (p[2]) << 8)
         | static_cast<std::uint32_t> (p[3]);
}

bool readExact (juce::InputStream& in, void* dest, int bytes)
{
    return in.read (dest, bytes) == bytes;
}

IrHeaderInfo makeUnknown (const juce::String& note = {})
{
    IrHeaderInfo info;
    info.kind = IrHeaderInfo::Kind::Unknown;
    info.description = note.isEmpty() ? "Unknown / raw" : note;
    return info;
}

IrHeaderInfo detectWav (juce::InputStream& in, juce::int64 fileSize)
{
    char riff[12] {};
    if (! readExact (in, riff, 12)
        || std::memcmp (riff, "RIFF", 4) != 0
        || std::memcmp (riff + 8, "WAVE", 4) != 0)
    {
        return {};
    }

    IrHeaderInfo info;
    info.kind = IrHeaderInfo::Kind::Wav;

    while (in.getPosition() + 8 <= fileSize)
    {
        char id[4] {};
        std::uint8_t sizeBytes[4] {};
        if (! readExact (in, id, 4) || ! readExact (in, sizeBytes, 4))
            break;

        const auto chunkSize = static_cast<juce::int64> (read32le (sizeBytes));
        const auto dataPos = in.getPosition();

        if (std::memcmp (id, "fmt ", 4) == 0 && chunkSize >= 16)
        {
            std::uint8_t fmt[16] {};
            if (readExact (in, fmt, 16))
            {
                info.numChannels = read16le (fmt + 2);
                info.sampleRate = static_cast<int> (read32le (fmt + 4));
                info.bitsPerSample = read16le (fmt + 14);
            }
        }
        else if (std::memcmp (id, "data", 4) == 0)
        {
            info.headerBytes = static_cast<int> (dataPos);
            const int bytesPerFrame = juce::jmax (1, info.numChannels
                                                      * juce::jmax (1, info.bitsPerSample / 8));
            info.numFrames = chunkSize / bytesPerFrame;
            break;
        }

        in.setPosition (dataPos + chunkSize + (chunkSize & 1));
    }

    info.description = "WAV " + juce::String (info.sampleRate) + " Hz, "
                     + juce::String (juce::jmax (0, info.numChannels)) + " ch, header "
                     + juce::String (info.headerBytes) + " bytes";
    return info;
}

IrHeaderInfo detectAiff (juce::InputStream& in, juce::int64 fileSize)
{
    char form[12] {};
    if (! readExact (in, form, 12) || std::memcmp (form, "FORM", 4) != 0)
        return {};
    if (std::memcmp (form + 8, "AIFF", 4) != 0 && std::memcmp (form + 8, "AIFC", 4) != 0)
        return {};

    IrHeaderInfo info;
    info.kind = IrHeaderInfo::Kind::Aiff;

    while (in.getPosition() + 8 <= fileSize)
    {
        char id[4] {};
        std::uint8_t sizeBytes[4] {};
        if (! readExact (in, id, 4) || ! readExact (in, sizeBytes, 4))
            break;

        const auto chunkSize = static_cast<juce::int64> (read32be (sizeBytes));
        const auto dataPos = in.getPosition();

        if (std::memcmp (id, "COMM", 4) == 0 && chunkSize >= 8)
        {
            std::uint8_t comm[8] {};
            if (readExact (in, comm, 8))
            {
                info.numChannels = read16be (comm);
                info.numFrames = read32be (comm + 2);
                info.bitsPerSample = read16be (comm + 6);
            }
        }
        else if (std::memcmp (id, "SSND", 4) == 0 && chunkSize >= 8)
        {
            std::uint8_t ssnd[8] {};
            if (readExact (in, ssnd, 8))
            {
                const auto offset = static_cast<juce::int64> (read32be (ssnd));
                info.headerBytes = static_cast<int> (dataPos + 8 + offset);
            }
            break;
        }

        in.setPosition (dataPos + chunkSize + (chunkSize & 1));
    }

    info.description = "AIFF " + juce::String (juce::jmax (0, info.numChannels)) + " ch, header "
                     + juce::String (info.headerBytes) + " bytes";
    return info;
}

IrLoadResult fail (const juce::String& error, IrHeaderInfo header = {})
{
    IrLoadResult result;
    result.ok = false;
    result.error = error;
    result.header = std::move (header);
    return result;
}

std::vector<float> mixToMono (const juce::AudioBuffer<float>& buffer)
{
    std::vector<float> mono (static_cast<std::size_t> (buffer.getNumSamples()), 0.f);
    const int channels = buffer.getNumChannels();
    const int frames = buffer.getNumSamples();
    if (channels <= 0 || frames <= 0)
        return mono;

    const float scale = 1.0f / static_cast<float> (channels);
    for (int ch = 0; ch < channels; ++ch)
    {
        const float* src = buffer.getReadPointer (ch);
        for (int i = 0; i < frames; ++i)
            mono[static_cast<std::size_t> (i)] += src[i] * scale;
    }
    return mono;
}

IrLoadResult loadWithReader (const juce::File& file,
                             juce::int64 startSample,
                             juce::int64 lengthSamples,
                             const IrHeaderInfo& header)
{
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));
    if (reader == nullptr)
        return fail ("Could not decode audio file", header);

    const auto total = reader->lengthInSamples;
    const auto start = juce::jlimit (static_cast<juce::int64> (0), juce::jmax ((juce::int64) 0, total), startSample);
    auto remaining = juce::jmax ((juce::int64) 0, total - start);
    if (lengthSamples > 0)
        remaining = juce::jmin (remaining, lengthSamples);
    if (remaining <= 0)
        return fail ("No samples after start offset", header);

    juce::AudioBuffer<float> buffer (static_cast<int> (reader->numChannels), static_cast<int> (remaining));
    if (! reader->read (&buffer, 0, static_cast<int> (remaining), start, true, true))
        return fail ("Failed to read audio frames", header);

    IrLoadResult result;
    result.ok = true;
    result.header = header;
    result.sampleRate = reader->sampleRate;
    result.samples = mixToMono (buffer);
    result.header.sampleRate = static_cast<int> (reader->sampleRate);
    result.header.numChannels = static_cast<int> (reader->numChannels);
    result.header.numFrames = total;
    result.header.bitsPerSample = static_cast<int> (reader->bitsPerSample);
    return result;
}

IrLoadResult loadRaw (const juce::File& file,
                      bool asFloat32,
                      int headerSkipBytes,
                      juce::int64 startSample,
                      juce::int64 lengthSamples,
                      IrHeaderInfo header)
{
    juce::FileInputStream in (file);
    if (! in.openedOk())
        return fail ("Could not open file", header);

    const auto skip = juce::jlimit (0, static_cast<int> (juce::jmax ((juce::int64) 0, file.getSize())),
                                    headerSkipBytes);
    if (! in.setPosition (skip))
        return fail ("Header skip is past end of file", header);

    juce::MemoryBlock bytes;
    const auto remaining = file.getSize() - skip;
    if (remaining <= 0)
        return fail ("No payload after header skip", header);

    if (in.readIntoMemoryBlock (bytes, remaining) <= 0)
        return fail ("Failed to read raw payload", header);

    const int sampleBytes = asFloat32 ? 4 : 2;
    const auto totalSamples = static_cast<juce::int64> (bytes.getSize() / static_cast<size_t> (sampleBytes));
    const auto start = juce::jlimit ((juce::int64) 0, totalSamples, startSample);
    auto count = juce::jmax ((juce::int64) 0, totalSamples - start);
    if (lengthSamples > 0)
        count = juce::jmin (count, lengthSamples);
    if (count <= 0)
        return fail ("No samples after start offset", header);

    header.kind = IrHeaderInfo::Kind::Raw;
    header.headerBytes = skip;
    header.numChannels = 1;
    header.bitsPerSample = asFloat32 ? 32 : 16;
    header.numFrames = totalSamples;
    header.description = juce::String (asFloat32 ? "Raw float32 LE" : "Raw int16 LE")
                       + ", header " + juce::String (skip) + " bytes, "
                       + juce::String (totalSamples) + " samples";

    IrLoadResult result;
    result.ok = true;
    result.header = header;
    result.sampleRate = 48000.0;
    result.samples.resize (static_cast<std::size_t> (count));

    const auto* data = static_cast<const std::uint8_t*> (bytes.getData());
    const auto base = static_cast<std::size_t> (start) * static_cast<std::size_t> (sampleBytes);
    for (juce::int64 i = 0; i < count; ++i)
    {
        const auto off = base + static_cast<std::size_t> (i) * static_cast<std::size_t> (sampleBytes);
        if (asFloat32)
        {
            float sample = 0.f;
            std::memcpy (&sample, data + off, sizeof (float));
            result.samples[static_cast<std::size_t> (i)] = sample;
        }
        else
        {
            const auto raw = static_cast<std::int16_t> (read16le (data + off));
            result.samples[static_cast<std::size_t> (i)] = static_cast<float> (raw) / 32768.f;
        }
    }
    return result;
}
} // namespace

IrHeaderInfo IrFileLoader::detectHeader (const juce::File& file)
{
    if (! file.existsAsFile())
        return makeUnknown ("File not found");

    juce::FileInputStream in (file);
    if (! in.openedOk())
        return makeUnknown ("Could not open file");

    const auto size = file.getSize();
    auto wav = detectWav (in, size);
    if (wav.kind == IrHeaderInfo::Kind::Wav)
        return wav;

    in.setPosition (0);
    auto aiff = detectAiff (in, size);
    if (aiff.kind == IrHeaderInfo::Kind::Aiff)
        return aiff;

    IrHeaderInfo raw;
    raw.kind = IrHeaderInfo::Kind::Raw;
    raw.headerBytes = 0;
    raw.numChannels = 1;
    raw.bitsPerSample = 32;
    raw.numFrames = size / 4;
    raw.description = "Raw float32 LE (no container header)";
    return raw;
}

IrLoadResult IrFileLoader::load (const IrLoadRequest& request)
{
    if (! request.file.existsAsFile())
        return fail ("File not found");

    auto detected = detectHeader (request.file);
    auto format = request.format;
    if (format == IrLoadRequest::Format::Auto)
    {
        if (detected.kind == IrHeaderInfo::Kind::Wav)
            format = IrLoadRequest::Format::Wav;
        else if (detected.kind == IrHeaderInfo::Kind::Aiff)
            format = IrLoadRequest::Format::Aiff;
        else
            format = IrLoadRequest::Format::RawF32;
    }

    if (format == IrLoadRequest::Format::Wav || format == IrLoadRequest::Format::Aiff)
        return loadWithReader (request.file, request.startSample, request.lengthSamples, detected);

    const bool asFloat32 = format != IrLoadRequest::Format::RawI16;
    const int skip = request.headerSkipBytes > 0 ? request.headerSkipBytes : detected.headerBytes;
    return loadRaw (request.file, asFloat32, skip, request.startSample, request.lengthSamples, detected);
}

std::vector<float> IrFileLoader::buildAppliedIr (const std::vector<float>& decodedMono,
                                                 juce::int64 startSample,
                                                 juce::int64 lengthSamples,
                                                 int delaySamples,
                                                 int maxIrLen)
{
    const std::vector<float> identity { 1.f };
    const auto& source = decodedMono.empty() ? identity : decodedMono;
    const auto delay = juce::jmax (0, delaySamples);
    const auto cap = juce::jmax (1, maxIrLen);
    const auto start = juce::jlimit ((juce::int64) 0,
                                     static_cast<juce::int64> (source.size()),
                                     startSample);
    auto count = juce::jmax ((juce::int64) 0, static_cast<juce::int64> (source.size()) - start);
    if (lengthSamples > 0)
        count = juce::jmin (count, lengthSamples);

    const auto payload = juce::jmin (count, static_cast<juce::int64> (juce::jmax (0, cap - delay)));
    std::vector<float> applied (static_cast<std::size_t> (delay + payload), 0.f);
    if (payload > 0)
    {
        std::copy_n (source.data() + static_cast<std::size_t> (start),
                     static_cast<std::size_t> (payload),
                     applied.data() + static_cast<std::size_t> (delay));
    }
    if (applied.empty())
        applied = identity;
    return applied;
}
