#include "AefJuceIncludes.h"
#include "SignalFlowPanel.h"

#include "AudioEffectFrameworkProcessor.h"
#include "MudspAssetPaths.h"
#include "SchematicDeviceModels.h"
#include "SchematicOverlayBind.h"

SignalFlowPanel::SignalFlowPanel(AudioEffectFrameworkProcessor& processorIn,
                                 AtomLookAndFeel& lookAndFeel)
    : processor(processorIn),
      atomLookAndFeel(lookAndFeel)
{
    setLookAndFeel(&atomLookAndFeel);

    circuitView.onNodeBypassChanged = [this](juce::String id, bool bypassed)
    {
        processor.setEffectTopologyModuleBypassed(id, bypassed);
    };
    circuitView.loadSchematicSvg = [this](const atom::graph::Node& node)
    {
        return loadNodeSvg(node);
    };
    circuitView.loadCompositeSvg = [this]()
    {
        const auto relative = juce::String(circuitView.getGraph().compositeSvg);
        if (relative.isEmpty())
            return juce::String{};
        const auto file = mudspAssetFile(relative);
        if (file.existsAsFile())
            return file.loadFileAsString();
        return juce::String{};
    };
    circuitView.setBinding(&overlayBinding);
    circuitView.onGeometryChanged = [this]
    {
        if (onGeometryChanged)
            onGeometryChanged();
    };
    addAndMakeVisible(circuitView);

    applyDisplayMode();
    refreshFromProcessor();
    startTimerHz(10);
}

SignalFlowPanel::~SignalFlowPanel()
{
    stopTimer();
    circuitView.setBinding(nullptr);
    setLookAndFeel(nullptr);
}

void SignalFlowPanel::visibilityChanged()
{
    if (isShowing())
    {
        refreshFromProcessor();
        startTimerHz(10);
    }
    else
    {
        stopTimer();
    }
}

void SignalFlowPanel::timerCallback()
{
    if (isShowing())
        refreshFromProcessor();
}

void SignalFlowPanel::lookAndFeelChanged()
{
    applyDisplayMode();
}

void SignalFlowPanel::applyDisplayMode()
{
    circuitView.applyDisplayMode();
}

juce::String SignalFlowPanel::currentModuleId() const
{
    if (const auto* node = circuitView.getSelectedNode())
        return juce::String(node->id);
    return circuitView.getSelectedId();
}

juce::String SignalFlowPanel::loadNodeSvg(const atom::graph::Node& node) const
{
    const juce::String relative(!node.svg.empty() ? node.svg : node.svgInternal);
    const auto file = mudspAssetFile(relative);
    if (file.existsAsFile())
        return file.loadFileAsString();
    return {};
}

std::optional<atom::schematic::OverlayValue> SignalFlowPanel::OverlayBinding::read(const juce::String& overlayKey)
{
    auto* middle = owner.processor.getKbussEngine().getMiddleProcessor();
    if (middle == nullptr)
        return std::nullopt;

    const auto moduleId = owner.currentModuleId();
    if (const auto device = aef::schematic::resolveDevice(*middle, moduleId, overlayKey))
    {
        atom::schematic::OverlayValue value;
        value.kind = atom::schematic::ValueKind::Device;
        value.leaf = atom::schematic::canonicalLeaf(overlayKey);
        value.enumCode = device->enumCode;
        value.comboId = device->comboId;
        return value;
    }

    const auto resolved = aef::schematic::resolveComponent(*middle, moduleId, overlayKey);
    if (! resolved)
        return std::nullopt;

    atom::schematic::OverlayValue value;
    value.kind = atom::schematic::ValueKind::Component;
    value.leaf = atom::schematic::canonicalLeaf(overlayKey);
    value.unit = resolved->unit;
    value.numeric = resolved->value;
    if (resolved->potTaper < NX_POT_TAPER_COUNT)
        value.potTaper = static_cast<int>(resolved->potTaper);
    return value;
}

bool SignalFlowPanel::OverlayBinding::write(const juce::String& overlayKey,
                                           const atom::schematic::OverlayValue& value)
{
    auto* middle = owner.processor.getKbussEngine().getMiddleProcessor();
    if (middle == nullptr)
        return false;

    const auto moduleId = owner.currentModuleId();

    if (value.kind == atom::schematic::ValueKind::Device)
    {
        const auto resolved = aef::schematic::resolveDevice(*middle, moduleId, overlayKey);
        if (! resolved)
            return false;

        if (value.enumCode == resolved->enumCode)
            return false;

        const auto* desc = middle->parameter(resolved->paramId.toStdString());
        if (desc == nullptr)
            return false;

        owner.processor.setMiddleParamDomain(resolved->paramId,
                                             aef::schematic::deviceWriteDomain(*desc, value.enumCode));
        return true;
    }

    if (value.kind != atom::schematic::ValueKind::Component)
        return false;

    const auto resolved = aef::schematic::resolveComponent(*middle, moduleId, overlayKey);
    if (! resolved)
        return false;

    owner.processor.setMiddleParamDomain(resolved->paramId, static_cast<float>(value.numeric));
    return true;
}

juce::String SignalFlowPanel::fingerprint(const atom::graph::Graph& graph)
{
    juce::String text;
    text << graph.gfxKey << '\n' << graph.chainHash << '\n' << graph.displayName << '\n';
    for (const auto& node : graph.nodes)
        text << node.id << '\t' << node.op << '\t' << node.label << '\t' << (int)node.bypassed << '\n';
    for (const auto& edge : graph.edges)
        text << edge.from << '\t' << edge.to << '\t' << edge.kind << '\n';
    return text;
}

void SignalFlowPanel::refreshFromProcessor()
{
    if (circuitView.getSvgView().isOverlayBeingEdited())
        return;

    const int generation = processor.middleProcessorGeneration();
    const auto graph = processor.getEffectSignalFlowGraph();
    const auto nextFingerprint = fingerprint(graph);
    const bool modelChanged = generation != lastMiddleGeneration_ || graph.gfxKey != lastGfxKey_;

    lastMiddleGeneration_ = generation;
    lastGfxKey_ = graph.gfxKey;

    circuitView.onBack = onBack;

    if (modelChanged || nextFingerprint != lastFingerprint_)
    {
        lastFingerprint_ = nextFingerprint;
        circuitView.setGraph(graph);
    }

    if (!graph.empty())
        circuitView.refreshOverlays();
}

int SignalFlowPanel::getPreferredWidth() const noexcept
{
    return circuitView.getPreferredWidth();
}

int SignalFlowPanel::getPreferredHeight() const noexcept
{
    return circuitView.getPreferredHeight();
}

void SignalFlowPanel::resized()
{
    circuitView.onBack = onBack;
    circuitView.setBounds(getLocalBounds());
}
