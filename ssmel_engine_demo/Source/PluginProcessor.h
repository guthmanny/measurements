#pragma once

#include <atomic>
#include <cmath>

#include "../JuceLibraryCode/JuceHeader.h"
#include "SsmelDemoMap.h"

struct ssmel_engine;

class SsmelDemoProcessor : public juce::AudioProcessor,
                           private juce::AudioProcessorValueTreeState::Listener
{
public:
    static constexpr const char* kOutputGainParamId = "output_gain";
    static constexpr const char* kFilterCutoffParamId = "filter_cutoff";
    static constexpr const char* kPresetParamId = "instrument_preset";

    SsmelDemoProcessor();
    ~SsmelDemoProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    using AudioProcessor::processBlock;
    void processBlock (juce::AudioSampleBuffer&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState parameters;
    juce::MidiKeyboardState keyboardState;

private:
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void handleIncomingMidi (const juce::MidiBuffer& midiMessages);
    void applyEngineParameters();
    void bindSampleMapToEngine();
    void releaseEngine();
    int currentPresetIndex() const;

    ssmel_engine* engine_ = nullptr;
    SsmelDemoMap demoMap_;
    juce::AudioBuffer<float> processBuffer_;
    double lastSampleRate_ = 0.0;
    int lastBlockSize_ = 0;
    std::atomic<bool> presetPending_ { false };
};
