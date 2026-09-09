#pragma once

#include <juce_core/juce_core.h>

#include <vector>

namespace schematic_assets {

struct SignalOutputInfo
{
    bool hasOptions{false};
    juce::String activeId;
    juce::String activeLabel;
    /** One-line summary (active + others). */
    juce::String summary;
    /** e.g. "Active tap: e — Emitter (ve)" */
    juce::String activeLine;
    /** e.g. "Other options: o — Post-C2 (vo)" (empty if none). */
    juce::String otherLine;
};

struct SignalOutputOption
{
    juce::String id;
    juce::String label;
};

[[nodiscard]] std::vector<SignalOutputOption> parseOperatorSignalOutputs(const juce::var& operatorEntry);
[[nodiscard]] SignalOutputInfo buildSignalOutputInfo(const std::vector<SignalOutputOption>& options,
                                                     const juce::String& activeId);
[[nodiscard]] SignalOutputInfo loadCompositeStageSignalOutput(const juce::File& mudspRoot,
                                                              const juce::String& compositeKey,
                                                              const juce::String& topologyId);

}  // namespace schematic_assets
