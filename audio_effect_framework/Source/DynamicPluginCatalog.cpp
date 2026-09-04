#include "DynamicPluginCatalog.h"

#include <algorithm>
#include <filesystem>

#include <juce_core/juce_core.h>

#include "kbuss/dynamic_plugin_format.hpp"
#include "kbuss/known_plugin_list.hpp"
#include "kbuss/plugin_scanner.hpp"

namespace
{
std::vector<std::filesystem::path> toSearchPaths (const std::vector<std::string>& dirs)
{
    std::vector<std::filesystem::path> out;
    out.reserve (dirs.size());
    for (const auto& dir : dirs)
        out.emplace_back (dir);
    return out;
}

bool pathLooksLikeKbplugBundle (const std::string& path)
{
    return path.size() >= 7 && path.compare (path.size() - 7, 7, ".kbplug") == 0;
}

int pathPreferenceScore (const kbuss::PluginDescription& desc)
{
    const auto& path = desc.file_or_identifier;
    if (pathLooksLikeKbplugBundle (path))
        return 3;
    if (path.size() >= 3 && path.compare (path.size() - 3, 3, ".so") == 0)
        return 1;
    return 2;
}

std::vector<kbuss::PluginDescription> dedupeByUid (std::vector<kbuss::PluginDescription> entries)
{
    std::vector<kbuss::PluginDescription> out;
    out.reserve (entries.size());

    for (auto& desc : entries)
    {
        const auto existing = std::find_if (out.begin(), out.end(),
                                            [&] (const auto& d) { return d.uid == desc.uid; });
        if (existing == out.end())
        {
            out.push_back (std::move (desc));
            continue;
        }

        if (pathPreferenceScore (desc) > pathPreferenceScore (*existing))
            *existing = std::move (desc);
    }

    return out;
}
} // namespace

void DynamicPluginCatalog::scan (const std::vector<std::string>& searchDirs)
{
    entries_.clear();

    if (searchDirs.empty())
        return;

    kbuss::DynamicPluginFormat format;
    kbuss::KnownPluginList list;
    kbuss::PluginDirectoryScanner scanner (list,
                                           format,
                                           toSearchPaths (searchDirs),
                                           true,
                                           false);
    while (scanner.scan_next_file())
    {
    }

    entries_ = list.get_types();
    entries_ = dedupeByUid (std::move (entries_));
    std::sort (entries_.begin(), entries_.end(), [] (const auto& a, const auto& b)
    {
        if (a.name != b.name)
            return a.name < b.name;
        return a.uid < b.uid;
    });

    juce::String logLine = "DynamicPluginCatalog: found " + juce::String ((int) entries_.size()) + " plugin(s)";
    for (const auto& entry : entries_)
        logLine << " [" << entry.name << "]";
    juce::Logger::writeToLog (logLine);
}

const kbuss::PluginDescription* DynamicPluginCatalog::entry (std::size_t index) const noexcept
{
    if (index >= entries_.size())
        return nullptr;
    return &entries_[index];
}

std::optional<kbuss::PluginDescription> DynamicPluginCatalog::findByUid (std::string_view uid) const
{
    for (const auto& desc : entries_)
    {
        if (desc.uid == uid)
            return desc;
    }
    return std::nullopt;
}

juce::StringArray DynamicPluginCatalog::displayNames() const
{
    juce::StringArray names;
    for (const auto& desc : entries_)
        names.add (desc.name);
    if (names.isEmpty())
        names.add ("(no plugins found)");
    return names;
}
