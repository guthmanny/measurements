#pragma once

#include <memory>
#include <string>
#include <vector>

#include "DynamicPluginCatalog.h"
#include "KbussEffectEngine.h"

#include "kbuss/dynamic_plugin_format.hpp"

/** kbuss chain with a swappable middle slot loaded from scanned .kbplug bundles. */
class DynamicMiddleProcessorEffectEngine final : public KbussEffectEngine
{
public:
    DynamicMiddleProcessorEffectEngine (const DynamicPluginCatalog& catalog, int selectedIndex);

    void setSelectedPluginIndex (int index) noexcept;
    [[nodiscard]] int selectedPluginIndex() const noexcept { return selectedIndex_; }

    void reprepare (float sampleRate, std::uint32_t maxBlockSize);

    /** Phase 1 of an off-audio-thread switch: builds and settles the newly selected plugin
        while the current one keeps processing. Returns an invalid swap when the graph needs
        a full reprepare instead (sample rate change, block growth, not yet ready) — call
        reprepare() under suspension in that case. */
    [[nodiscard]] MiddleSwap buildSelectedPlugin (float sampleRate, std::uint32_t maxBlockSize);

    [[nodiscard]] bool hasResolvedMiddlePlugin() const noexcept { return hasMiddleDesc_; }

    [[nodiscard]] std::optional<std::string> dynamicMiddlePluginBundlePath() const override;
    [[nodiscard]] std::string_view dynamicMiddlePluginUid() const override;

protected:
    void registerPluginFormats() override;
    bool installMiddleProcessors (const ProcessorCreateFn& create) override;

private:
    [[nodiscard]] bool resolveSelectedMiddleDesc() noexcept;

    const DynamicPluginCatalog& catalog_;
    int selectedIndex_ = 0;
    kbuss::PluginDescription middleDesc_;
    bool hasMiddleDesc_ = false;
    std::shared_ptr<kbuss::ModuleLoader> moduleLoader_;
};
