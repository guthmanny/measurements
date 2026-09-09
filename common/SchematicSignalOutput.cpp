#include "SchematicSignalOutput.h"

namespace schematic_assets {
namespace {

std::vector<SignalOutputOption> parseSignalOutputsArray(const juce::var& optionsVar)
{
    std::vector<SignalOutputOption> options;
    if (auto* array = optionsVar.getArray())
    {
        for (const auto& item : *array)
        {
            if (auto* obj = item.getDynamicObject())
            {
                SignalOutputOption option;
                option.id = obj->getProperty("id").toString();
                option.label = obj->getProperty("label").toString();
                if (option.id.isEmpty())
                    continue;
                options.push_back(std::move(option));
            }
        }
    }
    return options;
}

const juce::var* registryOperatorsObject(const juce::File& mudspRoot)
{
    static juce::var cached;
    static juce::String cachedRoot;
    if (cachedRoot != mudspRoot.getFullPathName())
    {
        cachedRoot = mudspRoot.getFullPathName();
        cached = juce::var();

        const auto registry = mudspRoot.getChildFile("assets/registry.json");
        if (registry.existsAsFile())
        {
            const auto parsed = juce::JSON::parse(registry);
            if (auto* root = parsed.getDynamicObject())
                cached = root->getProperty("operators");
        }
    }

    if (cached.getDynamicObject() != nullptr)
        return &cached;
    return nullptr;
}

const juce::var* catalogCompositesObject(const juce::File& mudspRoot)
{
    static juce::var cached;
    static juce::String cachedRoot;
    if (cachedRoot != mudspRoot.getFullPathName())
    {
        cachedRoot = mudspRoot.getFullPathName();
        cached = juce::var();

        const auto catalog = mudspRoot.getChildFile("assets/white_box_catalog.json");
        if (catalog.existsAsFile())
        {
            const auto parsed = juce::JSON::parse(catalog);
            if (auto* root = parsed.getDynamicObject())
                cached = root->getProperty("composites");
        }
    }

    if (cached.getDynamicObject() != nullptr)
        return &cached;
    return nullptr;
}

juce::String findStageSignalOutputId(const juce::var& compositeEntry, const juce::String& topologyId)
{
    if (auto* composite = compositeEntry.getDynamicObject())
    {
        if (auto* chain = composite->getProperty("signal_chain").getArray())
        {
            for (const auto& stageVar : *chain)
            {
                if (auto* stage = stageVar.getDynamicObject())
                {
                    if (stage->getProperty("topology_id").toString() == topologyId)
                        return stage->getProperty("signal_output").toString();
                }
            }
        }
    }
    return {};
}

juce::String findStageOperatorKey(const juce::var& compositeEntry, const juce::String& topologyId)
{
    if (auto* composite = compositeEntry.getDynamicObject())
    {
        if (auto* chain = composite->getProperty("signal_chain").getArray())
        {
            for (const auto& stageVar : *chain)
            {
                if (auto* stage = stageVar.getDynamicObject())
                {
                    if (stage->getProperty("topology_id").toString() == topologyId)
                        return stage->getProperty("operator").toString();
                }
            }
        }
    }
    return {};
}

juce::String buildSummary(const std::vector<SignalOutputOption>& options,
                          const juce::String& activeId,
                          const juce::String& activeLabel)
{
    if (options.empty() || activeId.isEmpty())
        return {};

    juce::String others;
    for (const auto& option : options)
    {
        if (option.id == activeId)
            continue;

        const auto token = option.id + " — " + option.label;
        others += (others.isEmpty() ? "" : ", ") + token;
    }

    const auto activeToken = activeId + " — " + activeLabel;
    if (others.isEmpty())
        return "Active tap: " + activeToken;

    return "Active tap: " + activeToken + "  |  Other options: " + others;
}

}  // namespace

std::vector<SignalOutputOption> parseOperatorSignalOutputs(const juce::var& operatorEntry)
{
    if (auto* obj = operatorEntry.getDynamicObject())
        return parseSignalOutputsArray(obj->getProperty("signal_outputs"));
    return {};
}

SignalOutputInfo buildSignalOutputInfo(const std::vector<SignalOutputOption>& options,
                                       const juce::String& activeId)
{
    SignalOutputInfo info;
    if (options.empty() || activeId.isEmpty())
        return info;

    for (const auto& option : options)
    {
        if (option.id != activeId)
            continue;

        info.hasOptions = true;
        info.activeId = option.id;
        info.activeLabel = option.label;
        info.activeLine = "Active tap: " + option.id + " — " + option.label;
        info.summary = buildSummary(options, activeId, option.label);

        juce::String others;
        for (const auto& alt : options)
        {
            if (alt.id == activeId)
                continue;
            const auto token = alt.id + " — " + alt.label;
            others += (others.isEmpty() ? "" : ", ") + token;
        }
        if (others.isNotEmpty())
            info.otherLine = "Other options: " + others;

        return info;
    }

    return info;
}

SignalOutputInfo loadCompositeStageSignalOutput(const juce::File& mudspRoot,
                                                const juce::String& compositeKey,
                                                const juce::String& topologyId)
{
    SignalOutputInfo info;
    if (! mudspRoot.isDirectory() || compositeKey.isEmpty() || topologyId.isEmpty())
        return info;

    const auto* composites = catalogCompositesObject(mudspRoot);
    if (composites == nullptr)
        return info;

    const auto compositeEntry = composites->getDynamicObject()->getProperty(compositeKey);
    const auto operatorKey = findStageOperatorKey(compositeEntry, topologyId);
    const auto activeId = findStageSignalOutputId(compositeEntry, topologyId);
    if (operatorKey.isEmpty() || activeId.isEmpty())
        return info;

    const auto* operators = registryOperatorsObject(mudspRoot);
    if (operators == nullptr)
        return info;

    const auto operatorEntry = operators->getDynamicObject()->getProperty(operatorKey);
    return buildSignalOutputInfo(parseOperatorSignalOutputs(operatorEntry), activeId);
}

}  // namespace schematic_assets
