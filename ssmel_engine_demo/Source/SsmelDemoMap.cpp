#include "SsmelDemoMap.h"

#include "../JuceLibraryCode/JuceHeader.h"

#include <cmath>
#include <cstdint>
#include <vector>

namespace
{
juce::File findSourcesDir()
{
#ifdef SSMEL_DEMO_SOUND_DIR
    const juce::File compiled (SSMEL_DEMO_SOUND_DIR);
    if (compiled.getChildFile ("Ep2.XDI").existsAsFile()
        || compiled.getChildFile ("ep2.xdi").existsAsFile())
        return compiled;
#endif

    const auto exeDir =
        juce::File::getSpecialLocation (juce::File::currentExecutableFile).getParentDirectory();
    const juce::File nearby[] = {
        exeDir.getChildFile ("example_sound_sources").getChildFile ("sources"),
        exeDir.getParentDirectory().getChildFile ("example_sound_sources").getChildFile ("sources"),
    };
    for (const auto& dir : nearby)
        if (dir.getChildFile ("Ep2.XDI").existsAsFile()
            || dir.getChildFile ("ep2.xdi").existsAsFile())
            return dir;

    return {};
}

juce::File findFileCI (const juce::File& dir, const juce::String& name)
{
    const auto exact = dir.getChildFile (name);
    if (exact.existsAsFile())
        return exact;

    for (const auto& entry : juce::RangedDirectoryIterator (dir, false, "*", juce::File::findFiles))
        if (entry.getFile().getFileName().equalsIgnoreCase (name))
            return entry.getFile();

    return {};
}

ssmel_buffer_t* loadWavBuffer (const juce::File& wavFile, float taggedSampleRate)
{
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (wavFile));
    if (reader == nullptr || reader->lengthInSamples <= 0)
        return nullptr;

    const auto frames = (int) reader->lengthInSamples;
    juce::AudioBuffer<float> interleaved (1, frames);
    if (reader->numChannels == 1)
    {
        reader->read (&interleaved, 0, frames, 0, true, false);
    }
    else
    {
        juce::AudioBuffer<float> multi ((int) reader->numChannels, frames);
        reader->read (&multi, 0, frames, 0, true, true);
        interleaved.copyFrom (0, 0, multi, 0, 0, frames);
        for (int ch = 1; ch < multi.getNumChannels(); ++ch)
            interleaved.addFrom (0, 0, multi, ch, 0, frames);
        interleaved.applyGain (1.0f / (float) multi.getNumChannels());
    }

    const float sr = taggedSampleRate > 0.0f ? taggedSampleRate : (float) reader->sampleRate;
    const float* channels[1] = { interleaved.getReadPointer (0) };
    return ssmel_buffer_create (channels, 1, (uint32_t) frames, sr);
}

ssmel_loop_mode_t loopModeFromXdi (const juce::String& type)
{
    if (type.equalsIgnoreCase ("Forward") || type.equalsIgnoreCase ("Forwards"))
        return SSMEL_LOOP_FORWARD;
    return SSMEL_LOOP_OFF;
}

float dbToLin (float db)
{
    return std::pow (10.0f, db / 20.0f);
}

float splitStaticDb (const juce::XmlElement& splitEl)
{
    float db = 0.0f;
    if (auto* osc = splitEl.getChildByName ("OscAmp"))
        db += (float) osc->getDoubleAttribute ("Amplitude", 0.0);
    if (auto* amp = splitEl.getChildByName ("Amplifier"))
        db += (float) amp->getDoubleAttribute ("Volume", 0.0);
    if (auto* mix = splitEl.getChildByName ("Mixer"))
        db += (float) mix->getDoubleAttribute ("Volume", 0.0);
    return db;
}

/** Linear Rate → ms. Rate 0 = slowest, 0.99 = UI-fast, 1.27 = instant. Replace after measurement. */
double xdiRateToMs (double rate)
{
    const double t_slow = 8000.0;
    const double t_fast = 10.0;
    const double r_fast = 0.99;
    if (! (rate > 0.0))
        return t_slow;
    if (rate >= 1.27)
        return 1.0;
    if (rate >= r_fast)
        return t_fast + (1.0 - t_fast) * (rate - r_fast) / (1.27 - r_fast);
    return t_slow + (t_fast - t_slow) * (rate / r_fast);
}

void fillLayerEg (ssmel_layer_eg_t& dst, const SsmelXdiVoicing& src, bool isEg2)
{
    dst.enabled = true;
    if (isEg2)
    {
        dst.attack_ms = src.eg2AttackMs;
        dst.hold_ms = src.eg2HoldMs;
        dst.decay_ms = src.eg2DecayMs;
        dst.break_ms = src.eg2BreakMs;
        dst.break_level = src.eg2BreakLevel;
        dst.release_ms = src.eg2ReleaseMs;
        dst.sustain = src.eg2Sustain;
        dst.oneshot = src.eg2Oneshot;
    }
    else
    {
        dst.attack_ms = src.eg1AttackMs;
        dst.hold_ms = src.eg1HoldMs;
        dst.decay_ms = src.eg1DecayMs;
        dst.break_ms = src.eg1BreakMs;
        dst.break_level = src.eg1BreakLevel;
        dst.release_ms = src.eg1ReleaseMs;
        dst.sustain = src.eg1Sustain;
        dst.oneshot = src.eg1Oneshot;
    }
}

float xdiFilterCutoffOct (const juce::XmlElement& splitEl)
{
    auto* filter = splitEl.getChildByName ("Filter");
    if (filter == nullptr)
        return 0.0f;

    const double freq = filter->getDoubleAttribute ("Frequency", 1.0)
                        + filter->getDoubleAttribute ("FreqOffset", 0.0);
    if (! (freq > 1.0e-6) || ! std::isfinite (freq))
        return 0.0f;
    return (float) std::log2 (freq);
}

void parseFilterVel (ssmel_layer_vel_t& dst, const juce::XmlElement& splitEl)
{
    dst = {};
    auto* filter = splitEl.getChildByName ("Filter");
    if (filter == nullptr)
        return;

    dst.enabled = true;
    dst.lo_x = juce::jlimit (0, 127, filter->getIntAttribute ("FreqMinVelModX", 0));
    dst.hi_x = juce::jlimit (0, 127, filter->getIntAttribute ("FreqMaxVelModX", 127));
    dst.lo_y = (float) filter->getDoubleAttribute ("FreqMinVelModY", 1.0);
    dst.hi_y = (float) filter->getDoubleAttribute ("FreqMaxVelModY", 1.0);
}

void parseAmpVel (ssmel_layer_vel_t& dst, const juce::XmlElement& splitEl)
{
    dst = {};
    auto* amp = splitEl.getChildByName ("Amplifier");
    if (amp == nullptr)
        return;

    dst.enabled = true;
    dst.lo_x = juce::jlimit (0, 127, amp->getIntAttribute ("MinVelModX", 0));
    dst.hi_x = juce::jlimit (0, 127, amp->getIntAttribute ("MaxVelModX", 127));
    dst.lo_y = (float) amp->getDoubleAttribute ("MinVelModY", -96.0);
    dst.hi_y = (float) amp->getDoubleAttribute ("MaxVelModY", 0.0);
}

float xdiFilterEnvAmount (const juce::XmlElement& splitEl)
{
    auto* filter = splitEl.getChildByName ("Filter");
    if (filter == nullptr)
        return 0.0f;
    const double amount = filter->getDoubleAttribute ("Env2Amount", 0.0);
    if (! (amount > 0.0) || ! std::isfinite (amount))
        return 0.0f;
    return (float) amount;
}

void parseKbdTable (ssmel_layer_kbd_t& dst, const juce::XmlElement& splitEl)
{
    dst = {};
    auto* filter = splitEl.getChildByName ("Filter");
    if (filter == nullptr)
        return;

    const auto which = filter->getStringAttribute ("KbdTable");
    if (which.isEmpty() || which.equalsIgnoreCase ("Off") || which == "0")
        return;

    juce::String tag = "KbdTable1";
    if (which.containsIgnoreCase ("2") || which == "2")
        tag = "KbdTable2";
    else if (which.containsIgnoreCase ("3") || which == "3")
        tag = "KbdTable3";
    else if (which.containsIgnoreCase ("4") || which == "4")
        tag = "KbdTable4";

    auto* tableEl = splitEl.getChildByName (tag);
    if (tableEl == nullptr)
        return;

    const double freq = juce::jmax (1.0e-6, filter->getDoubleAttribute ("Frequency", 1.0)
                                                + filter->getDoubleAttribute ("FreqOffset", 0.0));

    for (auto* segEl : tableEl->getChildIterator())
    {
        if (! segEl->hasTagName ("Segment") || dst.num_points >= SSMEL_LAYER_KBD_MAX)
            continue;
        dst.keys[dst.num_points] = juce::jlimit (0, 127, segEl->getIntAttribute ("Key", 0));
        const double raw = segEl->getDoubleAttribute ("Value", 0.0);
        const double lin = juce::jmax (1.0e-6, freq + raw);
        dst.values[dst.num_points] = (float) (std::log2 (lin) - std::log2 (freq));
        ++dst.num_points;
    }
}

SsmelXdiVoicing parseEnvelope (const juce::XmlElement* env, const SsmelXdiVoicing& fallback, bool isEg2)
{
    SsmelXdiVoicing out = fallback;
    if (env == nullptr)
        return out;

    struct Seg
    {
        double rate = 1.0;
        double level = 0.0;
    };

    std::vector<Seg> segs;
    for (auto* segEl : env->getChildIterator())
    {
        if (! segEl->hasTagName ("Segment"))
            continue;
        segs.push_back ({ segEl->getDoubleAttribute ("Rate", 1.0),
                          segEl->getDoubleAttribute ("Level", 0.0) });
    }

    if (segs.empty())
        return out;

    /* Point 1 = initial (no Rate). Point 2 = attack.
       Gated: last point is release; points 3..n-1 run while held
       (point 3 = first decay, last-but-one = sustain).
       4-point: sustain = point 3, release = point 4.
       5-point: break = point 3, sustain = point 4, release = point 5.
       SustainPoint < 0 = oneshot (do not return to 0 on key-off). */
    const int sustainPoint = env->getIntAttribute ("SustainPoint", -1);
    const bool oneshot = sustainPoint < 0;

    const double initial = segs[0].level;
    const double attackLevel = segs.size() >= 2 ? segs[1].level : initial;
    const double peak = juce::jmax (initial, attackLevel);

    double attackMs = 0.0;
    const bool instantAttack = initial + 0.01 >= 1.0 && attackLevel + 0.01 >= peak
                               && segs.size() >= 2 && segs[1].rate >= 0.99;
    if (segs.size() >= 2 && ! instantAttack && attackLevel > initial + 0.01)
        attackMs = juce::jmax (1.0, xdiRateToMs (segs[1].rate));

    const double holdMs = 0.0;
    double decayMs = 1.0;
    double breakMs = 0.0;
    float breakLevel = 0.0f;
    float sustain = (float) attackLevel;
    double releaseMs = xdiRateToMs (segs.back().rate);

    if (oneshot)
    {
        sustain = (float) segs.back().level;
        if (attackLevel + 0.01 < peak)
        {
            breakMs = xdiRateToMs (segs[1].rate);
            breakLevel = (float) attackLevel;
        }
        if (segs.size() >= 3)
            decayMs = xdiRateToMs (segs[2].rate);
        for (size_t k = 3; k < segs.size(); ++k)
            decayMs += xdiRateToMs (segs[k].rate);
    }
    else if (segs.size() >= 4)
    {
        const size_t sustainIdx = segs.size() - 2;
        sustain = (float) segs[sustainIdx].level;
        releaseMs = xdiRateToMs (segs.back().rate);
        if (sustainIdx == 2)
        {
            decayMs = xdiRateToMs (segs[2].rate);
        }
        else
        {
            breakMs = xdiRateToMs (segs[2].rate);
            breakLevel = (float) segs[2].level;
            decayMs = 0.0;
            for (size_t k = 3; k <= sustainIdx; ++k)
                decayMs += xdiRateToMs (segs[k].rate);
        }
    }
    else if (segs.size() >= 3)
    {
        sustain = (float) segs[2].level;
        decayMs = xdiRateToMs (segs[2].rate);
    }

    if (! oneshot && segs.size() >= 2 && attackLevel + 0.01 < peak && breakMs <= 0.0)
    {
        breakMs = xdiRateToMs (segs[1].rate);
        breakLevel = (float) attackLevel;
    }

    decayMs = juce::jmax (1.0, decayMs);

    if (isEg2)
    {
        out.eg2AttackMs = attackMs;
        out.eg2HoldMs = holdMs;
        out.eg2DecayMs = decayMs;
        out.eg2BreakMs = breakMs;
        out.eg2BreakLevel = breakLevel;
        out.eg2Sustain = sustain;
        out.eg2ReleaseMs = releaseMs;
        out.eg2Oneshot = sustainPoint < 0;
    }
    else
    {
        out.eg1AttackMs = attackMs;
        out.eg1HoldMs = holdMs;
        out.eg1DecayMs = decayMs;
        out.eg1BreakMs = breakMs;
        out.eg1BreakLevel = breakLevel;
        out.eg1Sustain = sustain;
        out.eg1ReleaseMs = releaseMs;
        out.eg1Oneshot = sustainPoint < 0;
    }
    return out;
}

float maxSplitDb (const juce::XmlElement& instrument)
{
    float maxDb = -1000.0f;
    bool any = false;
    for (auto* layerEl : instrument.getChildIterator())
    {
        if (! layerEl->hasTagName ("Layer"))
            continue;
        for (auto* splitEl : layerEl->getChildIterator())
        {
            if (! splitEl->hasTagName ("Split"))
                continue;
            maxDb = juce::jmax (maxDb, splitStaticDb (*splitEl));
            any = true;
        }
    }
    return any ? maxDb : 0.0f;
}

bool addXdiSplits (ssmel_map_t* map, const juce::File& xdiFile, const juce::File& wavDir,
                   SsmelXdiVoicing* voicing)
{
    auto xml = juce::XmlDocument::parse (xdiFile);
    if (xml == nullptr || ! xml->hasTagName ("instrument"))
        return false;

    const float peakDb = maxSplitDb (*xml);
    bool haveEg = false;
    int added = 0;

    for (auto* layerEl : xml->getChildIterator())
    {
        if (! layerEl->hasTagName ("Layer"))
            continue;

        const auto triggerName = layerEl->getStringAttribute ("Trigger");
        const bool isRelease = triggerName.containsIgnoreCase ("off")
                               || triggerName.containsIgnoreCase ("release");

        for (auto* splitEl : layerEl->getChildIterator())
        {
            if (! splitEl->hasTagName ("Split"))
                continue;

            auto* waveEl = splitEl->getChildByName ("Wave");
            if (waveEl == nullptr)
                continue;

            const auto sampleName = waveEl->getStringAttribute ("SampleName");
            const auto wavFile = findFileCI (wavDir, sampleName);
            if (! wavFile.existsAsFile())
                return false;

            const int coarse = waveEl->getIntAttribute ("CoarseTune", 0);
            const double fine = waveEl->getDoubleAttribute ("FineTune", 0.0);
            const int unity = waveEl->getIntAttribute ("UnityNote", 60);
            const double taggedSr = waveEl->getDoubleAttribute ("SampleRate", 0.0);
            const double retune = std::pow (2.0, -((double) coarse + fine / 100.0) / 12.0);
            const float playbackSr = taggedSr > 0.0 ? (float) (taggedSr * retune) : 0.0f;

            auto* buffer = loadWavBuffer (wavFile, playbackSr);
            if (buffer == nullptr)
                return false;

            const int loKey = juce::jlimit (0, 127, splitEl->getIntAttribute ("StartNote", 0));
            const int hiKey = juce::jlimit (0, 127, splitEl->getIntAttribute ("EndNote", 127));
            const uint32_t zone = ssmel_map_add_zone (map, loKey, hiKey);
            if (zone == UINT32_MAX)
            {
                ssmel_buffer_destroy (buffer);
                return false;
            }

            const float relDb = splitStaticDb (*splitEl) - peakDb;

            ssmel_layer_desc_t def {};
            def.lo_velocity = splitEl->getIntAttribute ("MinVel", 0);
            def.hi_velocity = splitEl->getIntAttribute ("MaxVel", 127);
            def.root_note = juce::jlimit (0, 127, unity);
            def.start_frame = (uint32_t) juce::jmax (0, waveEl->getIntAttribute ("SampleStart", 0));
            def.loop_start = (uint32_t) juce::jmax (0, waveEl->getIntAttribute ("LoopStart", 0));
            def.loop_end = (uint32_t) juce::jmax (0, waveEl->getIntAttribute ("LoopEnd", 0));
            def.loop_mode = loopModeFromXdi (waveEl->getStringAttribute ("LoopType"));
            def.trigger = isRelease ? SSMEL_TRIGGER_RELEASE : SSMEL_TRIGGER_ATTACK;
            def.gain = dbToLin (relDb);

            SsmelXdiVoicing splitVoice;
            splitVoice = parseEnvelope (splitEl->getChildByName ("Envelope1"), splitVoice, false);
            splitVoice = parseEnvelope (splitEl->getChildByName ("Envelope2"), splitVoice, true);
            fillLayerEg (def.amp_eg, splitVoice, false);
            fillLayerEg (def.filter_eg, splitVoice, true);
            parseKbdTable (def.filter_kbd, *splitEl);
            parseFilterVel (def.filter_vel, *splitEl);
            parseAmpVel (def.amp_vel, *splitEl);
            def.filter_cutoff_oct = xdiFilterCutoffOct (*splitEl);
            def.filter_env_amount = xdiFilterEnvAmount (*splitEl);

            if (ssmel_map_add_layer (map, zone, buffer, &def) == UINT32_MAX)
            {
                ssmel_buffer_destroy (buffer);
                return false;
            }
            ++added;

            if (voicing != nullptr && ! haveEg && loKey <= 60 && hiKey >= 60)
            {
                *voicing = parseEnvelope (splitEl->getChildByName ("Envelope1"), *voicing, false);
                *voicing = parseEnvelope (splitEl->getChildByName ("Envelope2"), *voicing, true);
                haveEg = true;
            }
        }
    }

    if (voicing != nullptr && ! haveEg)
    {
        for (auto* layerEl : xml->getChildIterator())
        {
            if (! layerEl->hasTagName ("Layer"))
                continue;
            for (auto* splitEl : layerEl->getChildIterator())
            {
                if (! splitEl->hasTagName ("Split"))
                    continue;
                *voicing = parseEnvelope (splitEl->getChildByName ("Envelope1"), *voicing, false);
                *voicing = parseEnvelope (splitEl->getChildByName ("Envelope2"), *voicing, true);
                break;
            }
            break;
        }
    }

    return added > 0;
}

ssmel_map_t* loadXdiMap (const juce::File& wavDir, const juce::String& xdiName,
                                     SsmelXdiVoicing* voicing)
{
    const auto xdiFile = findFileCI (wavDir, xdiName);
    if (! xdiFile.existsAsFile())
        return nullptr;

    auto* map = ssmel_map_create();
    if (map == nullptr)
        return nullptr;

    if (! addXdiSplits (map, xdiFile, wavDir, voicing))
    {
        ssmel_map_destroy (map);
        return nullptr;
    }
    return map;
}
} // namespace

SsmelDemoMap::~SsmelDemoMap()
{
    reset();
}

void SsmelDemoMap::reset()
{
    if (pianoMap_ != nullptr)
    {
        ssmel_map_destroy (pianoMap_);
        pianoMap_ = nullptr;
    }
    if (epMap_ != nullptr)
    {
        ssmel_map_destroy (epMap_);
        epMap_ = nullptr;
    }
    pianoVoicing_ = {};
    epVoicing_ = {};
}

bool SsmelDemoMap::load()
{
    reset();

    const auto sources = findSourcesDir();
    if (! sources.isDirectory())
        return false;

    pianoMap_ = loadXdiMap (sources, "Piano2ry.XDI", &pianoVoicing_);
    epMap_ = loadXdiMap (sources, "Ep2.XDI", &epVoicing_);
    if (pianoMap_ == nullptr || epMap_ == nullptr)
    {
        reset();
        return false;
    }
    return true;
}

ssmel_map_t* SsmelDemoMap::map (int presetIndex) noexcept
{
    return presetIndex == 1 ? epMap_ : pianoMap_;
}

const ssmel_map_t* SsmelDemoMap::map (int presetIndex) const noexcept
{
    return presetIndex == 1 ? epMap_ : pianoMap_;
}

const SsmelXdiVoicing& SsmelDemoMap::voicing (int presetIndex) const noexcept
{
    return presetIndex == 1 ? epVoicing_ : pianoVoicing_;
}
