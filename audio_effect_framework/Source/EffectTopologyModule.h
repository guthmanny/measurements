#pragma once

#include "AefJuceIncludes.h"

/** One node in the middle effect's internal DSP chain (Settings → Topology). */
struct EffectTopologyModule
{
    juce::String id;
    juce::String displayName;
    bool bypassed = false;
};
