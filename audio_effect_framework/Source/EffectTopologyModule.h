#pragma once

#include "AefJuceIncludes.h"

/** One node in the middle effect's internal DSP chain (Settings → Signal Flow). */
struct EffectTopologyModule
{
    juce::String id;
    juce::String displayName;
    juce::String operatorId;
    juce::String osClass;
    juce::String svg;
    juce::String svgInternal;
    bool bypassed = false;
    bool bypassable = true;
};
