#pragma once

#include <juce_audio_devices/juce_audio_devices.h>

/** JACK/PipeWire helpers: prefer hardware capture_* inputs and avoid monitor feedback. */
namespace chorus_jack
{
void ensureJackUsesCaptureInput (juce::AudioDeviceManager& deviceManager);

/** Re-run capture routing when opening the tuner (break out→in monitor loops). */
void prepareJackInputForTuning (juce::AudioDeviceManager& deviceManager);
}  // namespace chorus_jack
