#pragma once

#include <optional>

#include <juce_atom_theme/juce_atom_theme.h>
#include <juce_core/juce_core.h>

namespace kbuss
{
class Processor;
struct ParameterDescriptor;
}

namespace aef::schematic
{

using DeviceKind = atom::schematic::DeviceKind;

struct ResolvedDevice
{
    juce::String paramId;
    float writeDomain = 0.0f;
    int enumCode = 0;
    int comboId = 1;
};

std::optional<ResolvedDevice> resolveDevice(const kbuss::Processor& processor,
                                            const juce::String& moduleId,
                                            const juce::String& overlayKey);

float deviceWriteDomain(const kbuss::ParameterDescriptor& desc, int enumCode);

} // namespace aef::schematic
