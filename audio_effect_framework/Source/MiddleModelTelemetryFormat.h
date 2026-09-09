#pragma once

#include "AefJuceIncludes.h"

#include "kbuss/plugin_abi.h"
#include "nudsp/common/zin_staging.h"

namespace aef::model_telemetry
{

[[nodiscard]] inline juce::String formatResistance (double ohms)
{
    if (!(ohms > 0.0))
        return "0";

    if (ohms >= 1.0e6)
        return juce::String (ohms / 1.0e6, ohms >= 10.0e6 ? 0 : 2) + "M";

    if (ohms >= 1.0e3)
        return juce::String (ohms / 1.0e3, ohms >= 10.0e3 ? 0 : 1) + "k";

    return juce::String (ohms, ohms >= 100.0 ? 0 : 1);
}

[[nodiscard]] inline juce::String formatCapacitance (double farads)
{
    if (!(farads > 0.0))
        return {};

    if (farads >= 1.0e-6)
        return juce::String (farads * 1.0e6, farads >= 1.0e-5 ? 1 : 2) + "uF";

    if (farads >= 1.0e-9)
        return juce::String (farads * 1.0e9, farads >= 1.0e-8 ? 0 : 1) + "nF";

    return juce::String (farads * 1.0e12, 0) + "pF";
}

[[nodiscard]] inline juce::String formatZinStaging (const KbModelTelemetry& telemetry)
{
    const auto topology = static_cast<nx_zin_staging_topology_e> (telemetry.zin_topology);

    if (topology == NX_ZIN_STAGING_SERIES_C_SHUNT_R)
    {
        juce::String text = formatResistance (telemetry.zin_r_load_ohm);
        if (const auto cSeries = formatCapacitance (telemetry.zin_c_series_f); cSeries.isNotEmpty())
            text << " + " << cSeries;
        if (telemetry.zin_r_series_ohm > 0.0)
            text << " + " << formatResistance (telemetry.zin_r_series_ohm);
        return text;
    }

    juce::String text = formatResistance (telemetry.zin_r_load_ohm);
    if (const auto cShunt = formatCapacitance (telemetry.zin_c_shunt_f); cShunt.isNotEmpty())
        text << " // " << cShunt;
    return text;
}

[[nodiscard]] inline juce::String formatVoutStaging (const KbModelTelemetry& telemetry)
{
    if (!(telemetry.vout_target_v > 0.0))
        return "—";

    juce::String text = juce::String (telemetry.vout_target_v, 2) + " Vpk";
    if (telemetry.vout_max_v > 0.0 && telemetry.vout_atten > 0.0)
        text << " (" << juce::String (telemetry.vout_max_v, 2) << " x "
             << juce::String (telemetry.vout_atten, 2) << ")";
    return text;
}

[[nodiscard]] inline juce::String formatModelVersion (const KbModelTelemetry& telemetry)
{
    juce::String hash;
    if (telemetry.chain_hash_hex != nullptr)
        hash = juce::String (telemetry.chain_hash_hex).substring (0, 8);

    juce::String profile;
    if (telemetry.rom_profile != nullptr && telemetry.rom_profile[0] != '\0')
        profile = juce::String (telemetry.rom_profile);

    if (telemetry.backbone_fs > 0)
    {
        if (profile.isNotEmpty())
            profile << '@' << (int) telemetry.backbone_fs;
        else
            profile = juce::String ((int) telemetry.backbone_fs) + " Hz";
    }

    if (hash.isEmpty() && profile.isEmpty())
        return "—";

    if (hash.isNotEmpty() && profile.isNotEmpty())
        return hash + " " + profile;

    return hash.isNotEmpty() ? hash : profile;
}

[[nodiscard]] inline juce::String formatFooterText (const KbModelTelemetry& telemetry)
{
    const auto version = formatModelVersion (telemetry);
    const auto zin = formatZinStaging (telemetry);
    const auto vout = formatVoutStaging (telemetry);

    juce::StringArray parts;
    if (version != "—")
        parts.add ("Model " + version);
    if (zin.isNotEmpty())
        parts.add ("Zin " + zin);
    if (vout != "—")
        parts.add ("Out " + vout);

    return parts.joinIntoString ("  |  ");
}

}  // namespace aef::model_telemetry
