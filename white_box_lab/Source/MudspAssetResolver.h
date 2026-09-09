#pragma once

#include <map>
#include <optional>
#include <vector>

#include "AefJuceIncludes.h"
#include "SchematicSignalOutput.h"

namespace white_box_lab {

struct SignalStage
{
    juce::String topologyId;
    juce::String operatorKey;
    juce::String label;
    juce::String signalOutputId;
};

struct CompositeEntry
{
    juce::String key;
    juce::String id;
    juce::String name;
    juce::String kbussUid;
    juce::String kind;
    juce::String compositeSvgRel;
    std::vector<SignalStage> signalChain;
};

struct OperatorEntry
{
    juce::String key;
    juce::String id;
    juce::String name;
    juce::String svgRel;
    bool hasInternal = false;
    std::vector<schematic_assets::SignalOutputOption> signalOutputs;
};

class MudspAssetResolver
{
public:
    static MudspAssetResolver& get();

    [[nodiscard]] bool load();
    [[nodiscard]] juce::File mudspRoot() const { return root_; }

    [[nodiscard]] const std::vector<CompositeEntry>& composites() const { return composites_; }
    [[nodiscard]] const CompositeEntry* findComposite(const juce::String& key) const;
    [[nodiscard]] const CompositeEntry* findCompositeByUid(const juce::String& kbussUid) const;

    [[nodiscard]] juce::File resolveCompositeSvg(const juce::String& compositeKey) const;
    [[nodiscard]] juce::File resolveOperatorSvg(const juce::String& operatorKey) const;
    [[nodiscard]] juce::File resolveOperatorSvg(const juce::String& operatorKey, bool useInternal) const;
    [[nodiscard]] bool operatorHasInternal(const juce::String& operatorKey) const;
    /** Resolved signal take-off metadata for a composite stage (empty if no catalog options). */
    [[nodiscard]] schematic_assets::SignalOutputInfo resolveStageSignalOutput(const juce::String& compositeKey,
                                                                              const juce::String& topologyId) const;
    [[nodiscard]] juce::File operatorCgJsonPath(const juce::String& operatorKey) const;
    [[nodiscard]] const OperatorEntry* findOperator(const juce::String& operatorKey) const;
    [[nodiscard]] static juce::String internalSvgRelativePath(const juce::String& externalSvgRel);
    [[nodiscard]] std::vector<SignalStage> signalChain(const juce::String& compositeKey) const;

    [[nodiscard]] static bool isRichSvg(const juce::File& file);

    [[nodiscard]] juce::StringArray validateCatalogOperators() const;

private:
    MudspAssetResolver() = default;

    juce::File locateRoot() const;
    bool loadOperatorRegistry(const juce::File& registryFile);
    bool loadWhiteBoxCatalog(const juce::File& catalogFile);

    juce::File root_;
    std::vector<CompositeEntry> composites_;
    std::map<juce::String, OperatorEntry> operators_;
};

}  // namespace white_box_lab
