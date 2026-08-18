#pragma once

#include "extensions/CAMEL/tuner.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

/** Streaming tuner via MuDSP CAMEL nx_tuner_f32 (same BACF path as kbuss Tuner). */
class TunerDetector
{
public:
    struct Result
    {
        bool valid = false;
        float frequencyHz = 0.0f;
        float periodicity = 0.0f;
        float midiNote = 0.0f;
        float cents = 0.0f;
        int noteIndex = 0;
        int octave = 4;
        /** 0 = mono/left, 1 = right (legacy; CAMEL mixes active channels). */
        int sourceChannel = 0;
    };

    TunerDetector() = default;

    void prepare (double sampleRate, float minFreqHz = 80.0f, float maxFreqHz = 1200.0f)
    {
        sampleRate_ = sampleRate > 0.0 ? sampleRate : 44100.0;
        minFreqHz_ = std::max (20.0f, minFreqHz);
        maxFreqHz_ = std::max (minFreqHz_ + 1.0f, maxFreqHz);
        ensureTuner();
        applyConfig();
        tuner_.prepare (sampleRate_);
        tuner_.tick (1);
        invalidate();
    }

    double getSampleRate() const noexcept { return sampleRate_; }

    void setPeriodicityThreshold (float threshold) noexcept
    {
        periodicityThreshold_ = std::clamp (threshold, 0.0f, 1.0f);
        applyConfig();
    }

    float getPeriodicityThreshold() const noexcept { return periodicityThreshold_; }

    void setInputPreampDb (float db) noexcept
    {
        inputPreampDb_ = db;
        applyConfig();
    }

    void reset()
    {
        if (tunerReady_)
            tuner_.reset();
        invalidate();
    }

    void invalidate() noexcept { last_ = {}; }

    /** Feed active channels; inactive pointers may be null. Stereo is averaged (CAMEL). */
    void process (const float* leftSamples,
                  const float* rightSamples,
                  int numChannels,
                  int numSamples)
    {
        if (numSamples <= 0 || !tunerReady_)
            return;

        const float* inPtrs[2] = {};
        float* outPtrs[2] = {};
        std::size_t channels = 0;

        if (leftSamples != nullptr)
        {
            inPtrs[channels] = leftSamples;
            ensureScratch (static_cast<std::size_t> (numSamples));
            outPtrs[channels] = scratchL_.data();
            ++channels;
        }

        if (numChannels > 1 && rightSamples != nullptr)
        {
            inPtrs[channels] = rightSamples;
            ensureScratch (static_cast<std::size_t> (numSamples));
            outPtrs[channels] = scratchR_.data();
            ++channels;
        }

        if (channels == 0)
            return;

        tuner_.tick (static_cast<std::size_t> (numSamples));
        tuner_.processMulti (inPtrs, outPtrs, static_cast<std::size_t> (numSamples), channels);

        nx_tuner_reading_t reading{};
        if (tuner_.getReading (&reading) != NX_SUCCESS)
            return;

        last_.valid = reading.valid != 0;
        last_.frequencyHz = reading.frequency_hz;
        last_.periodicity = reading.periodicity;
        last_.midiNote = reading.midi_note;
        last_.cents = reading.cents;
        last_.noteIndex = reading.note_index;
        last_.octave = reading.octave;
        last_.sourceChannel = 0;
    }

    const Result& getResult() const noexcept { return last_; }

    static const char* noteName (int noteIndex) noexcept
    {
        return nx_tuner_note_name (noteIndex);
    }

private:
    void ensureTuner()
    {
        if (tunerReady_)
            return;

        if (tuner_.getRawPointer() == nullptr)
            return;

        tunerReady_ = true;
        applyConfig();
    }

    void applyConfig()
    {
        if (!tunerReady_)
            return;

        nx_tuner_config_t cfg{};
        if (TunerF32::configInit (&cfg) != NX_SUCCESS)
            return;

        cfg.detector.min_freq_hz = minFreqHz_;
        cfg.detector.max_freq_hz = maxFreqHz_;
        cfg.detector.periodicity_threshold = periodicityThreshold_;
        cfg.level_gate_dbfs = -90.0;
        cfg.input_preamp_db = inputPreampDb_;
        cfg.mute_output = false;
        tuner_.setConfig (cfg);
    }

    void ensureScratch (std::size_t numSamples)
    {
        if (scratchL_.size() < numSamples)
            scratchL_.resize (numSamples);
        if (scratchR_.size() < numSamples)
            scratchR_.resize (numSamples);
    }

    using TunerF32 = nudsp::camel::TunerF32;

    TunerF32 tuner_;
    bool tunerReady_ = false;
    double sampleRate_ = 44100.0;
    float minFreqHz_ = 80.0f;
    float maxFreqHz_ = 1200.0f;
    float periodicityThreshold_ = 0.7f;
    /** Matches legacy Chorus pre-detector gain (+40 dB). */
    float inputPreampDb_ = 40.0f;
    Result last_;
    std::vector<float> scratchL_;
    std::vector<float> scratchR_;
};
