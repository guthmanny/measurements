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

    [[nodiscard]] bool hasResolvedMiddlePlugin() const noexcept { return hasMiddleDesc_; }

protected:
    void registerPluginFormats() override;
    bool installMiddleProcessors (const ProcessorCreateFn& create) override;

private:
    const DynamicPluginCatalog& catalog_;
    int selectedIndex_ = 0;
    kbuss::PluginDescription middleDesc_;
    bool hasMiddleDesc_ = false;
    std::shared_ptr<kbuss::ModuleLoader> moduleLoader_;
};
