#pragma once

#include "AefJuceIncludes.h"

#include <cstdlib>

/** Resolve the MuDSP checkout used for catalog / operator SVGs. */
[[nodiscard]] inline juce::File mudspRoot()
{
    if (const char* env = std::getenv("MUDSP_ROOT"); env != nullptr && env[0] != '\0')
    {
        const juce::File fromEnv(env);
        if (fromEnv.isDirectory())
            return fromEnv;
    }
#ifdef MUDSP_ROOT
    {
        const juce::File fromDefine(MUDSP_ROOT);
        if (fromDefine.isDirectory())
            return fromDefine;
    }
#endif
    if (const char* home = std::getenv("HOME"); home != nullptr && home[0] != '\0')
    {
        const juce::File fromHome = juce::File(home).getChildFile("MuDSP");
        if (fromHome.isDirectory())
            return fromHome;
    }
    return {};
}

[[nodiscard]] inline juce::File mudspCatalogFile()
{
    return mudspRoot().getChildFile("assets").getChildFile("white_box_catalog.json");
}

[[nodiscard]] inline juce::File mudspRegistryFile()
{
    return mudspRoot().getChildFile("assets").getChildFile("registry.json");
}

[[nodiscard]] inline juce::File mudspAssetFile(const juce::String& relativePath)
{
    if (relativePath.isEmpty())
        return {};
    return mudspRoot().getChildFile(relativePath);
}
