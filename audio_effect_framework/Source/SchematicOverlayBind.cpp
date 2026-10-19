#include "SchematicOverlayBind.h"

#include "kbuss/processor.hpp"

namespace aef::schematic
{
namespace
{
juce::String potFieldName(const juce::String& leaf)
{
    static constexpr const char* names[] = {
        "p1", "p2", "p3", "rt", "rv"
    };

    for (const auto* name : names)
        if (leaf.equalsIgnoreCase(name))
            return name;

    return {};
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

bool readDomain(const kbuss::Processor& processor, const kbuss::ParameterDescriptor& desc, float& out)
{
    return processor.get_parameter_domain(desc.index, out) == kbuss::Status::Ok;
}

int potIndexFromLeaf(const juce::String& leaf) noexcept
{
    if (leaf.length() < 2 || ! leaf.startsWithIgnoreCase("P"))
        return -1;
    if (! leaf.substring(1).containsOnly("0123456789"))
        return -1;
    return leaf.substring(1).getIntValue() - 1;
}
} // namespace

std::optional<ResolvedComponent> resolveComponent(const kbuss::Processor& processor,
                                                 const juce::String& moduleId,
                                                 const juce::String& overlayKey)
{
    if (atom::schematic::isDeviceOrNameOverlay(overlayKey))
        return std::nullopt;

    const auto leaf = atom::schematic::canonicalLeaf(overlayKey);
    if (leaf.isEmpty() || atom::schematic::isDeviceOrNameOverlay(leaf))
        return std::nullopt;

    juce::StringArray candidates;
    if (leaf.containsChar('.'))
        candidates.add(leaf);

    if (moduleId.isNotEmpty())
    {
        candidates.add(moduleId + "." + leaf);
        if (const auto pot = potFieldName(leaf); pot.isNotEmpty())
            candidates.add(moduleId + "." + pot + ".value");
    }

    candidates.add(leaf);
    if (const auto pot = potFieldName(leaf); pot.isNotEmpty())
        candidates.add(pot + ".value");

    const kbuss::ParameterDescriptor* desc = nullptr;
    for (const auto& id : candidates)
    {
        desc = findByIdIgnoreCase(processor, id);
        if (desc != nullptr)
            break;
    }

    if (desc == nullptr && moduleId.isNotEmpty())
    {
        const auto prefix = moduleId + ".";
        for (const auto& candidate : processor.parameters())
        {
            const juce::String id(candidate.id);
            if (! id.startsWithIgnoreCase(prefix))
                continue;

            const auto last = id.fromLastOccurrenceOf(".", false, false);
            if (last.equalsIgnoreCase(leaf))
            {
                desc = &candidate;
                break;
            }

            if (id.endsWithIgnoreCase(".value"))
            {
                const auto pot = id.dropLastCharacters(6).fromLastOccurrenceOf(".", false, false);
                if (pot.equalsIgnoreCase(leaf))
                {
                    desc = &candidate;
                    break;
                }
            }
        }
    }

    if (desc == nullptr)
    {
        const int potIndex = potIndexFromLeaf(leaf);
        if (potIndex >= 0)
        {
            std::vector<const kbuss::ParameterDescriptor*> pots;
            for (const auto& candidate : processor.parameters())
            {
                const juce::String id(candidate.id);
                if (moduleId.isNotEmpty() && ! id.startsWithIgnoreCase(moduleId + "."))
                    continue;
                if (id.endsWithIgnoreCase(".value"))
                    pots.push_back(&candidate);
            }

            if (potIndex < static_cast<int>(pots.size()))
                desc = pots[static_cast<size_t>(potIndex)];
        }
    }

    if (desc == nullptr)
        return std::nullopt;

    float value = 0.0f;
    if (! readDomain(processor, *desc, value))
        return std::nullopt;

    ResolvedComponent resolved;
    resolved.paramId = desc->id;
    resolved.unit = desc->unit;
    resolved.value = value;

    const juce::String id(desc->id);
    if (id.endsWithIgnoreCase(".value"))
    {
        const auto taperId = id.dropLastCharacters(6) + ".taper";
        if (const auto* taperDesc = findByIdIgnoreCase(processor, taperId))
        {
            float taperValue = 0.0f;
            if (readDomain(processor, *taperDesc, taperValue))
                resolved.potTaper = static_cast<nx_pot_taper_e>(
                    juce::jlimit(0, NX_POT_TAPER_COUNT - 1, juce::roundToInt(taperValue)));
        }
    }

    return resolved;
}

} // namespace aef::schematic
