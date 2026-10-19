#include "SchematicDeviceModels.h"

#include "kbuss/processor.hpp"

namespace aef::schematic
{
namespace
{
int familyBase(DeviceKind kind) noexcept
{
    switch (kind)
    {
    case DeviceKind::Diode:
        return 0x1000;
    case DeviceKind::BjtNpn:
        return 0x2000;
    case DeviceKind::BjtPnp:
        return 0x3000;
    case DeviceKind::Opamp:
        return 0x4000;
    case DeviceKind::Jfet:
        return 0x5000;
    case DeviceKind::Switch:
    case DeviceKind::None:
        return 0;
    }
    return 0;
}

int defaultEnumCode(DeviceKind kind) noexcept
{
    switch (kind)
    {
    case DeviceKind::Opamp:
        return 0x4003;
    case DeviceKind::Diode:
        return 0x1000;
    case DeviceKind::BjtNpn:
        return 0x2001;
    case DeviceKind::BjtPnp:
        return 0x3001;
    case DeviceKind::Jfet:
        return 0x5002;
    case DeviceKind::Switch:
    case DeviceKind::None:
        return 0;
    }
    return 0;
}

const char* paramLeafForKind(DeviceKind kind) noexcept
{
    switch (kind)
    {
    case DeviceKind::Opamp:
        return "opamp_model";
    case DeviceKind::Diode:
        return "diode_model";
    case DeviceKind::BjtNpn:
    case DeviceKind::BjtPnp:
        return "bjt";
    case DeviceKind::Jfet:
        return "jfet";
    case DeviceKind::Switch:
    case DeviceKind::None:
        return "";
    }
    return "";
}

const kbuss::ParameterDescriptor* findByIdIgnoreCase(const kbuss::Processor& processor,
                                                    const juce::String& id)
{
    if (auto* desc = processor.parameter(id.toStdString()))
        return desc;

    for (const auto& desc : processor.parameters())
        if (id.equalsIgnoreCase(juce::String(desc.id)))
            return &desc;

    return nullptr;
}

const kbuss::ParameterDescriptor* findDeviceParam(const kbuss::Processor& processor,
                                                 const juce::String& moduleId,
                                                 DeviceKind kind)
{
    const juce::String leaf(paramLeafForKind(kind));
    if (leaf.isEmpty())
        return nullptr;

    juce::StringArray ids;
    if (moduleId.isNotEmpty())
        ids.add(moduleId + "." + leaf);
    ids.add(leaf);

    for (const auto& id : ids)
        if (auto* desc = findByIdIgnoreCase(processor, id))
            return desc;

    if (moduleId.isEmpty())
        return nullptr;

    const auto prefix = moduleId + ".";
    for (const auto& desc : processor.parameters())
    {
        const juce::String id(desc.id);
        if (id.startsWithIgnoreCase(prefix)
            && id.fromLastOccurrenceOf(".", false, false).equalsIgnoreCase(leaf))
            return &desc;
    }

    return nullptr;
}

int enumCodeFromDomain(const kbuss::ParameterDescriptor& desc, float domain, DeviceKind kind)
{
    const int rounded = juce::roundToInt(domain);
    if (rounded >= 0x1000)
        return rounded;

    if (desc.max_domain <= 64.0f && rounded >= 0)
        return familyBase(kind) + rounded;

    return defaultEnumCode(kind);
}
} // namespace

float deviceWriteDomain(const kbuss::ParameterDescriptor& desc, int enumCode)
{
    if (desc.max_domain <= 64.0f)
        return static_cast<float>(enumCode & 0xFF);
    return static_cast<float>(enumCode);
}

std::optional<ResolvedDevice> resolveDevice(const kbuss::Processor& processor,
                                            const juce::String& moduleId,
                                            const juce::String& overlayKey)
{
    const auto kind = atom::schematic::deviceKindFromOverlay(overlayKey);
    if (kind == DeviceKind::None || kind == DeviceKind::Switch)
        return std::nullopt;

    const auto* desc = findDeviceParam(processor, moduleId, kind);
    if (desc == nullptr)
        return std::nullopt;

    float domain = 0.0f;
    if (processor.get_parameter_domain(desc->index, domain) != kbuss::Status::Ok)
        return std::nullopt;

    ResolvedDevice resolved;
    resolved.paramId = desc->id;
    resolved.enumCode = enumCodeFromDomain(*desc, domain, kind);
    resolved.comboId = atom::schematic::deviceComboId(kind, resolved.enumCode);
    resolved.writeDomain = deviceWriteDomain(*desc, resolved.enumCode);
    return resolved;
}

} // namespace aef::schematic
