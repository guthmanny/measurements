#include "MudspAssetResolver.h"

#include <algorithm>

namespace white_box_lab {

MudspAssetResolver& MudspAssetResolver::get()
{
    static MudspAssetResolver instance;
    return instance;
}

juce::File MudspAssetResolver::locateRoot() const
{
#if defined(MUDSP_ROOT)
    {
        const juce::File fromDefine(MUDSP_ROOT);
        if (fromDefine.getChildFile("assets/white_box_catalog.json").existsAsFile())
            return fromDefine;
    }
#endif

    const auto exe = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
    auto dir = exe.getParentDirectory();
    for (int depth = 0; depth < 8; ++depth)
    {
        const auto sibling = dir.getChildFile("MuDSP");
        if (sibling.getChildFile("assets/white_box_catalog.json").existsAsFile())
            return sibling;
        if (! dir.getParentDirectory().exists())
            break;
        dir = dir.getParentDirectory();
    }

    const juce::File homeMudsp = juce::File::getSpecialLocation(juce::File::userHomeDirectory)
                                     .getChildFile("myCode/MuDSP");
    if (homeMudsp.getChildFile("assets/white_box_catalog.json").existsAsFile())
        return homeMudsp;

    return {};
}

bool MudspAssetResolver::load()
{
    composites_.clear();
    operators_.clear();
    root_ = locateRoot();
    if (! root_.isDirectory())
        return false;

    const bool operatorsOk = loadOperatorRegistry(root_.getChildFile("assets/registry.json"));
    const bool catalogOk = loadWhiteBoxCatalog(root_.getChildFile("assets/white_box_catalog.json"));
    return operatorsOk && catalogOk;
}

bool MudspAssetResolver::loadOperatorRegistry(const juce::File& registryFile)
{
    if (! registryFile.existsAsFile())
        return false;

    const auto parsed = juce::JSON::parse(registryFile);
    if (! parsed.isObject())
        return false;

    const auto operators = parsed.getDynamicObject()->getProperty("operators");
    if (auto* obj = operators.getDynamicObject())
    {
        for (const auto& prop : obj->getProperties())
        {
            OperatorEntry entry;
            entry.key = prop.name.toString();
            if (auto* child = prop.value.getDynamicObject())
            {
                entry.id = child->getProperty("id").toString();
                entry.name = child->getProperty("name").toString();
                entry.svgRel = child->getProperty("svg_file").toString();
                entry.hasInternal = static_cast<bool>(child->getProperty("has_internal"));
                entry.signalOutputs = schematic_assets::parseOperatorSignalOutputs(prop.value);
            }
            operators_.emplace(entry.key, std::move(entry));
        }
    }
    return ! operators_.empty();
}

bool MudspAssetResolver::loadWhiteBoxCatalog(const juce::File& catalogFile)
{
    if (! catalogFile.existsAsFile())
        return false;

    const auto parsed = juce::JSON::parse(catalogFile);
    if (! parsed.isObject())
        return false;

    const auto composites = parsed.getDynamicObject()->getProperty("composites");
    auto* obj = composites.getDynamicObject();
    if (obj == nullptr)
        return false;

    for (const auto& prop : obj->getProperties())
    {
        CompositeEntry entry;
        entry.key = prop.name.toString();
        auto* child = prop.value.getDynamicObject();
        if (child == nullptr)
            continue;

        entry.id = child->getProperty("id").toString();
        entry.name = child->getProperty("name").toString();
        entry.kbussUid = child->getProperty("kbuss_uid").toString();
        entry.kind = child->getProperty("kind").toString();
        entry.compositeSvgRel = child->getProperty("composite_svg").toString();

        if (auto* chain = child->getProperty("signal_chain").getArray())
        {
            for (const auto& stageVar : *chain)
            {
                if (auto* stage = stageVar.getDynamicObject())
                {
                    SignalStage item;
                    item.topologyId = stage->getProperty("topology_id").toString();
                    item.operatorKey = stage->getProperty("operator").toString();
                    item.label = stage->getProperty("label").toString();
                    item.signalOutputId = stage->getProperty("signal_output").toString();
                    entry.signalChain.push_back(std::move(item));
                }
            }
        }
        composites_.push_back(std::move(entry));
    }

    std::sort(composites_.begin(), composites_.end(),
              [](const CompositeEntry& a, const CompositeEntry& b) { return a.name < b.name; });
    return ! composites_.empty();
}

const CompositeEntry* MudspAssetResolver::findComposite(const juce::String& key) const
{
    for (const auto& entry : composites_)
        if (entry.key == key)
            return &entry;
    return nullptr;
}

const CompositeEntry* MudspAssetResolver::findCompositeByUid(const juce::String& kbussUid) const
{
    if (kbussUid.isEmpty())
        return nullptr;
    for (const auto& entry : composites_)
        if (entry.kbussUid == kbussUid)
            return &entry;
    return nullptr;
}

const OperatorEntry* MudspAssetResolver::findOperator(const juce::String& operatorKey) const
{
    const auto it = operators_.find(operatorKey);
    return it != operators_.end() ? &it->second : nullptr;
}

juce::File MudspAssetResolver::resolveCompositeSvg(const juce::String& compositeKey) const
{
    const auto* entry = findComposite(compositeKey);
    if (entry == nullptr || ! entry->compositeSvgRel.startsWith("assets/schematics/extensions/white_box/"))
        return {};
    return root_.getChildFile(entry->compositeSvgRel);
}

juce::String MudspAssetResolver::internalSvgRelativePath(const juce::String& externalSvgRel)
{
    if (externalSvgRel.endsWithIgnoreCase(".svg"))
        return externalSvgRel.dropLastCharacters(4) + "_internal.svg";
    return externalSvgRel + "_internal.svg";
}

bool MudspAssetResolver::operatorHasInternal(const juce::String& operatorKey) const
{
    const auto* entry = findOperator(operatorKey);
    return entry != nullptr && entry->hasInternal;
}

schematic_assets::SignalOutputInfo MudspAssetResolver::resolveStageSignalOutput(const juce::String& compositeKey,
                                                                                const juce::String& topologyId) const
{
    const auto* composite = findComposite(compositeKey);
    if (composite == nullptr)
        return {};

    for (const auto& stage : composite->signalChain)
    {
        if (stage.topologyId != topologyId || stage.signalOutputId.isEmpty())
            continue;

        const auto* operatorEntry = findOperator(stage.operatorKey);
        if (operatorEntry == nullptr)
            return {};

        return schematic_assets::buildSignalOutputInfo(operatorEntry->signalOutputs, stage.signalOutputId);
    }

    return {};
}

juce::File MudspAssetResolver::resolveOperatorSvg(const juce::String& operatorKey) const
{
    return resolveOperatorSvg(operatorKey, false);
}

juce::File MudspAssetResolver::resolveOperatorSvg(const juce::String& operatorKey, bool useInternal) const
{
    const auto* entry = findOperator(operatorKey);
    if (entry == nullptr || ! entry->svgRel.startsWith("assets/schematics/core/"))
        return {};

    const auto rel = useInternal ? internalSvgRelativePath(entry->svgRel) : entry->svgRel;
    return root_.getChildFile(rel);
}

juce::File MudspAssetResolver::operatorCgJsonPath(const juce::String& operatorKey) const
{
    const auto linear = root_.getChildFile("scripts/core/linear_circuits")
                            .getChildFile(operatorKey + "_cg.json");
    if (linear.existsAsFile())
        return linear;
    const auto nonlinear = root_.getChildFile("scripts/core/nonlinear_circuits")
                               .getChildFile(operatorKey + "_cg.json");
    if (nonlinear.existsAsFile())
        return nonlinear;
    return root_.getChildFile("scripts/core/nonlinear_circuits")
        .getChildFile(operatorKey + "_nl_cg.json");
}

std::vector<SignalStage> MudspAssetResolver::signalChain(const juce::String& compositeKey) const
{
    if (const auto* entry = findComposite(compositeKey))
        return entry->signalChain;
    return {};
}

bool MudspAssetResolver::isRichSvg(const juce::File& file)
{
    if (! file.existsAsFile())
        return false;
    return file.loadFileAsString().contains("data-cell-id=\"COMPONENT:");
}

juce::StringArray MudspAssetResolver::validateCatalogOperators() const
{
    juce::StringArray missing;
    for (const auto& composite : composites_)
        for (const auto& stage : composite.signalChain)
            if (operators_.find(stage.operatorKey) == operators_.end())
                missing.add(composite.key + "." + stage.operatorKey);
    return missing;
}

}  // namespace white_box_lab
