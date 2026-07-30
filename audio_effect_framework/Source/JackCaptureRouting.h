#pragma once

#include <juce_audio_devices/juce_audio_devices.h>

/** JACK/PipeWire helpers: prefer hardware capture_* inputs and avoid monitor feedback. */
namespace effect_jack
{
void ensureJackUsesCaptureInput (juce::AudioDeviceManager& deviceManager,
                                 const juce::String& appClientName);

/** Re-run capture routing when opening the tuner (break out→in monitor loops). */
void prepareJackInputForTuning (juce::AudioDeviceManager& deviceManager,
                                const juce::String& appClientName);
}  // namespace effect_jack
