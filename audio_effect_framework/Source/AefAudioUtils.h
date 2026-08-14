#pragma once

#include "AefJuceIncludes.h"

namespace aef
{
inline void setParameterDefault (juce::AudioProcessorValueTreeState& state,
                                 const juce::String& paramId,
                                 float domainValue)
{
    if (auto* param = state.getParameter (paramId))
        param->setValueNotifyingHost (param->convertTo0to1 (domainValue));
}

/** Mix L/R to mono on both channels (mono pedal input path). */
inline void mixBufferToMonoDual (juce::AudioSampleBuffer& buffer, int numChannels, int numSamples)
{
    if (numChannels < 2 || numSamples <= 0)
        return;

    float* left = buffer.getWritePointer (0);
    float* right = buffer.getWritePointer (1);

    for (int i = 0; i < numSamples; ++i)
    {
        const float mono = 0.5f * (left[i] + right[i]);
        left[i] = mono;
        right[i] = mono;
    }
}

/** Copy channel 0 to channel 1 (mono pedal stereo output). */
inline void duplicateMonoToStereoOutput (juce::AudioSampleBuffer& buffer, int numOutputChannels, int numSamples)
{
    if (numOutputChannels >= 2 && numSamples > 0)
        buffer.copyFrom (1, 0, buffer, 0, 0, numSamples);
}
} // namespace aef
