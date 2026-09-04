#pragma once

#include <optional>
#include <string>
#include <vector>

#include "AefJuceIncludes.h"
#include "kbuss/plugin_description.hpp"

/** Scans .kbplug directories and holds discovered plugin descriptors (no live instances). */
class DynamicPluginCatalog
{
public:
    void scan (const std::vector<std::string>& searchDirs);

    [[nodiscard]] const std::vector<kbuss::PluginDescription>& entries() const noexcept { return entries_; }
    [[nodiscard]] std::size_t size() const noexcept { return entries_.size(); }
    [[nodiscard]] bool empty() const noexcept { return entries_.empty(); }

    [[nodiscard]] const kbuss::PluginDescription* entry (std::size_t index) const noexcept;
    [[nodiscard]] std::optional<kbuss::PluginDescription> findByUid (std::string_view uid) const;

    [[nodiscard]] juce::StringArray displayNames() const;

private:
    std::vector<kbuss::PluginDescription> entries_;
};
