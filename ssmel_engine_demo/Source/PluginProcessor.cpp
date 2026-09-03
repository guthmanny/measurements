#include "PluginProcessor.h"
#include "PluginEditor.h"

#include "ssmel/ssmel_engine.h"

namespace
{
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    return {
        std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { SsmelDemoProcessor::kOutputGainParamId, 1 },
            "Output Gain", juce::NormalisableRange<float> (0.0f, 2.0f, 0.001f), 0.85f),
        std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { SsmelDemoProcessor::kFilterCutoffParamId, 1 },
            "Filter Cutoff", juce::NormalisableRange<float> (80.0f, 16000.0f, 1.0f, 0.35f), 16000.0f),
        std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { SsmelDemoProcessor::kPresetParamId, 1 },
            "Instrument Preset", juce::StringArray { "Piano", "EP" }, 0),
    };
}
} // namespace

SsmelDemoProcessor::SsmelDemoProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "PARAMS", createParameterLayout())
{
    for (auto* id : { kOutputGainParamId, kFilterCutoffParamId, kPresetParamId })
        parameters.addParameterListener (id, this);
}

SsmelDemoProcessor::~SsmelDemoProcessor()
{
    for (auto* id : { kOutputGainParamId, kFilterCutoffParamId, kPresetParamId })
        parameters.removeParameterListener (id, this);

    releaseEngine();
}

int SsmelDemoProcessor::currentPresetIndex() const
{
    if (auto* raw = parameters.getRawParameterValue (kPresetParamId))
        return juce::jlimit (0, 1, (int) std::lround (raw->load()));

    return 0;
}

void SsmelDemoProcessor::releaseEngine()
{
    if (engine_ != nullptr)
    {
        ssmel_engine_destroy (engine_);
        engine_ = nullptr;
    }

    demoMap_.reset();
}

void SsmelDemoProcessor::bindSampleMapToEngine()
{
    if (engine_ == nullptr)
        return;

    const int presetIndex = currentPresetIndex();
    auto* map = demoMap_.map (presetIndex);
    if (map == nullptr)
        return;

    const auto preset = presetIndex == 1 ? SSMEL_INSTRUMENT_EP : SSMEL_INSTRUMENT_UPRIGHT;
    ssmel_engine_set_sample_map (engine_, map, preset, SSMEL_CHANNEL_STEREO);

    ssmel_engine_set_resonance_enabled (engine_, 0);
    ssmel_engine_set_harmonic_resonance_enabled (engine_, 0);

    const auto& voice = demoMap_.voicing (presetIndex);
    ssmel_engine_set_eg_attack_ms (engine_, 0, voice.eg1AttackMs);
    ssmel_engine_set_eg_decay_ms (engine_, 0, voice.eg1DecayMs);
    ssmel_engine_set_eg_sustain_level (engine_, 0, voice.eg1Sustain);
    ssmel_engine_set_eg_release_ms (engine_, 0, voice.eg1ReleaseMs);
    ssmel_engine_set_eg_attack_ms (engine_, 1, voice.eg2AttackMs);
    ssmel_engine_set_eg_decay_ms (engine_, 1, voice.eg2DecayMs);
    ssmel_engine_set_eg_sustain_level (engine_, 1, voice.eg2Sustain);
    ssmel_engine_set_eg_release_ms (engine_, 1, voice.eg2ReleaseMs);

    applyEngineParameters();
}

void SsmelDemoProcessor::applyEngineParameters()
{
    if (engine_ == nullptr)
        return;

    ssmel_engine_set_mod_amount (engine_, SSMEL_MOD_VEL, SSMEL_MOD_DST_AMP, 1.0f);
    ssmel_engine_set_mod_amount (engine_, SSMEL_MOD_VEL, SSMEL_MOD_DST_FILTER, 0.0f);
    ssmel_engine_set_mod_amount (engine_, SSMEL_MOD_EG2, SSMEL_MOD_DST_FILTER, 0.0f);

    if (auto* gain = parameters.getRawParameterValue (kOutputGainParamId))
        ssmel_engine_set_output_gain (engine_, gain->load());

    if (auto* cutoff = parameters.getRawParameterValue (kFilterCutoffParamId))
        ssmel_engine_set_filter_cutoff_hz (engine_, (double) cutoff->load());
}

void SsmelDemoProcessor::parameterChanged (const juce::String& parameterID, float newValue)
{
    juce::ignoreUnused (newValue);

    if (parameterID == kPresetParamId)
    {
        presetPending_.store (true);
        return;
    }

    applyEngineParameters();
}

void SsmelDemoProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    if (! (sampleRate > 0.0) || samplesPerBlock <= 0)
        return;

    lastSampleRate_ = sampleRate;
    lastBlockSize_ = samplesPerBlock;

    releaseEngine();

    if (! demoMap_.load())
        return;

    engine_ = ssmel_engine_create();
    if (engine_ == nullptr)
    {
        demoMap_.reset();
        return;
    }

    ssmel_engine_prepare (engine_, sampleRate);
    bindSampleMapToEngine();
    keyboardState.reset();
    processBuffer_.setSize (2, samplesPerBlock);
}

void SsmelDemoProcessor::releaseResources()
{
    releaseEngine();
}

void SsmelDemoProcessor::handleIncomingMidi (const juce::MidiBuffer& midiMessages)
{
    if (engine_ == nullptr)
        return;

    for (const auto metadata : midiMessages)
    {
        const auto message = metadata.getMessage();
        if (message.isNoteOn())
        {
            ssmel_engine_note_on (engine_,
                                  message.getNoteNumber(),
                                  message.getFloatVelocity(),
                                  message.getNoteNumber());
        }
        else if (message.isNoteOff())
        {
            ssmel_engine_note_off (engine_,
                                   message.getNoteNumber(),
                                   message.getFloatVelocity(),
                                   message.getNoteNumber());
        }
        else if (message.isController())
        {
            ssmel_engine_control_change (engine_,
                                         message.getControllerNumber(),
                                         message.getControllerValue());
        }
        else if (message.isAllNotesOff() || message.isAllSoundOff())
        {
            ssmel_engine_all_notes_off (engine_);
        }
    }
}

void SsmelDemoProcessor::processBlock (juce::AudioSampleBuffer& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    if (numSamples <= 0)
        return;

    if (presetPending_.exchange (false))
        bindSampleMapToEngine();

    keyboardState.processNextMidiBuffer (midiMessages, 0, numSamples, true);
    handleIncomingMidi (midiMessages);

    processBuffer_.setSize (2, numSamples, false, false, true);
    processBuffer_.clear();

    if (engine_ != nullptr && demoMap_.map (currentPresetIndex()) != nullptr)
    {
        ssmel_engine_render_stereo (engine_,
                                    processBuffer_.getWritePointer (0),
                                    processBuffer_.getWritePointer (1),
                                    (std::size_t) numSamples);
    }

    const int outCh = juce::jmin (buffer.getNumChannels(), 2);
    for (int ch = 0; ch < outCh; ++ch)
        buffer.copyFrom (ch, 0, processBuffer_, ch, 0, numSamples);

    for (int ch = outCh; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);
}

juce::AudioProcessorEditor* SsmelDemoProcessor::createEditor()
{
    return new SsmelDemoEditor (*this);
}

void SsmelDemoProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void SsmelDemoProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SsmelDemoProcessor();
}
