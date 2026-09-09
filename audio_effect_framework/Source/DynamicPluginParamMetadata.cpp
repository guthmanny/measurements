#include "DynamicPluginParamMetadata.h"

#include <cstring>

#if ! JUCE_WINDOWS
#  include <dlfcn.h>
#endif

#include "kbuss/plugin_abi.h"
#include "kbuss/version.hpp"

namespace aef::dynamic_plugin_params
{
namespace
{

juce::File resolveBundleRoot (const juce::File& path)
{
    if (path.isDirectory() && path.getFileExtension() == ".kbplug")
        return path;

    if (path.getFileName() == "plugin.json")
        return path.getParentDirectory();

    return path.getParentDirectory();
}

juce::File resolveSharedLibrary (const juce::File& bundleRoot)
{
    const auto manifestFile = bundleRoot.getChildFile ("plugin.json");
    if (manifestFile.existsAsFile())
    {
        const auto parsed = juce::JSON::parse (manifestFile);
        if (auto* obj = parsed.getDynamicObject())
        {
            const auto binary = obj->getProperty ("binary").toString();
            if (binary.isNotEmpty())
            {
                const auto candidate = bundleRoot.getChildFile (binary);
                if (candidate.existsAsFile())
                    return candidate;
            }
        }
    }

    for (const auto& entry : juce::RangedDirectoryIterator (bundleRoot, false, "*", juce::File::findFiles))
    {
        const auto name = entry.getFile().getFileName();
        if (name.endsWithIgnoreCase (".so") || name.endsWithIgnoreCase (".dylib")
            || name.endsWithIgnoreCase (".dll"))
            return entry.getFile();
    }

    return {};
}

juce::String uidFromManifest (const juce::File& bundleRoot)
{
    const auto manifestFile = bundleRoot.getChildFile ("plugin.json");
    if (! manifestFile.existsAsFile())
        return {};

    const auto parsed = juce::JSON::parse (manifestFile);
    if (auto* root = parsed.getDynamicObject())
    {
        if (auto* plugins = root->getProperty ("plugins").getArray())
        {
            if (plugins->size() > 0)
            {
                if (auto* plugin = plugins->getReference (0).getDynamicObject())
                    return plugin->getProperty ("uid").toString();
            }
        }
    }

    return {};
}

Meta metaFromJsonObject (const juce::DynamicObject& obj, std::uint32_t fallbackIndex)
{
    Meta meta;
    meta.id = obj.getProperty ("id").toString();
    meta.label = obj.getProperty ("label").toString();
    if (meta.label.isEmpty())
        meta.label = meta.id;

    meta.index = (std::uint32_t) juce::jmax (0, (int) obj.getProperty ("index"));
    if (! obj.hasProperty ("index"))
        meta.index = fallbackIndex;

    if (obj.hasProperty ("min"))
        meta.minDomain = (float) obj.getProperty ("min");
    if (obj.hasProperty ("max"))
        meta.maxDomain = (float) obj.getProperty ("max");
    if (obj.hasProperty ("default"))
        meta.defaultDomain = (float) obj.getProperty ("default");

    return meta;
}

std::vector<Meta> loadFromManifest (const juce::File& bundleRoot)
{
    std::vector<Meta> out;
    const auto manifestFile = bundleRoot.getChildFile ("plugin.json");
    if (! manifestFile.existsAsFile())
        return out;

    const auto parsed = juce::JSON::parse (manifestFile);
    auto* root = parsed.getDynamicObject();
    if (root == nullptr)
        return out;

    if (auto* plugins = root->getProperty ("plugins").getArray())
    {
        for (const auto& pluginVar : *plugins)
        {
            if (auto* plugin = pluginVar.getDynamicObject())
            {
                if (auto* params = plugin->getProperty ("parameters").getArray())
                {
                    std::uint32_t i = 0;
                    for (const auto& paramVar : *params)
                    {
                        if (auto* paramObj = paramVar.getDynamicObject())
                        {
                            auto meta = metaFromJsonObject (*paramObj, i);
                            if (meta.id.isNotEmpty())
                                out.push_back (std::move (meta));
                        }
                        ++i;
                    }
                }
            }
        }
    }

    return out;
}

#if JUCE_WINDOWS
std::vector<Meta> loadFromExport (const juce::File& libraryFile, const juce::String& uid)
#else
std::vector<Meta> loadFromExport (const juce::File& libraryFile, const juce::String& uid)
#endif
{
    std::vector<Meta> out;
    if (! libraryFile.existsAsFile())
        return out;

#if JUCE_WINDOWS
    juce::ignoreUnused (libraryFile, uid);
    return out;
#else
    void* handle = dlopen (libraryFile.getFullPathName().toRawUTF8(), RTLD_NOW | RTLD_LOCAL);
    if (handle == nullptr)
        return out;

    auto* parametersFn = reinterpret_cast<KbParametersFn> (
        dlsym (handle, KB_PLUGIN_PARAMETERS_SYMBOL));

    if (parametersFn != nullptr)
    {
        const auto uidUtf8 = uid.toStdString();
        std::uint32_t count = 0;
        const KbParameterInfo* table = parametersFn (uidUtf8.c_str(), &count);
        if (table != nullptr && count > 0)
        {
            out.reserve (count);
            for (std::uint32_t i = 0; i < count; ++i)
            {
                const auto& info = table[i];
                if (info.id == nullptr || info.label == nullptr)
                    continue;

                Meta meta;
                meta.id = juce::String (info.id);
                meta.label = juce::String (info.label);
                meta.index = i;
                meta.minDomain = info.min_domain;
                meta.maxDomain = info.max_domain;
                meta.defaultDomain = info.default_domain;
                out.push_back (std::move (meta));
            }
        }
    }

    dlclose (handle);
    return out;
#endif
}

}  // namespace

float Meta::domainToNormalized (float domain) const noexcept
{
    const float span = maxDomain - minDomain;
    if (span <= 0.f)
        return 0.f;
    return juce::jlimit (0.f, 1.f, (domain - minDomain) / span);
}

juce::String Meta::displayLabel() const
{
    return label.isNotEmpty() ? label : id;
}

std::vector<Meta> loadFromBundle (const juce::File& bundlePath, const juce::String& pluginUid)
{
    const auto bundleRoot = resolveBundleRoot (bundlePath);
    if (! bundleRoot.isDirectory())
        return {};

    auto out = loadFromManifest (bundleRoot);
    if (! out.empty())
        return out;

    const auto uid = pluginUid.isNotEmpty() ? pluginUid : uidFromManifest (bundleRoot);
    return loadFromExport (resolveSharedLibrary (bundleRoot), uid);
}

}  // namespace aef::dynamic_plugin_params
