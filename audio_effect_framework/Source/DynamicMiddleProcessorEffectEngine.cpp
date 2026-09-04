#include "DynamicMiddleProcessorEffectEngine.h"

#include <juce_core/juce_core.h>

DynamicMiddleProcessorEffectEngine::DynamicMiddleProcessorEffectEngine (
    const DynamicPluginCatalog& catalog,
    int selectedIndex)
    : catalog_ (catalog),
      selectedIndex_ (selectedIndex)
{
}

void DynamicMiddleProcessorEffectEngine::setSelectedPluginIndex (int index) noexcept
{
    if (catalog_.empty())
    {
        selectedIndex_ = 0;
        return;
    }

    selectedIndex_ = juce::jlimit (0, (int) catalog_.size() - 1, index);
}

void DynamicMiddleProcessorEffectEngine::reprepare (float sampleRate, std::uint32_t maxBlockSize)
{
    release();
    prepare (sampleRate, maxBlockSize);
}

void DynamicMiddleProcessorEffectEngine::registerPluginFormats()
{
    KbussEffectEngine::registerPluginFormats();

    hasMiddleDesc_ = false;
    middleDesc_ = {};

    const auto* selected = catalog_.entry ((std::size_t) selectedIndex_);
    if (selected == nullptr)
        return;

    moduleLoader_ = std::make_shared<kbuss::ModuleLoader>();
    addPluginFormat (std::make_unique<kbuss::DynamicPluginFormat> (moduleLoader_));

    middleDesc_ = *selected;
    hasMiddleDesc_ = true;
    juce::Logger::writeToLog ("DynamicMiddleProcessorEffectEngine: selected "
                              + juce::String (middleDesc_.name)
                              + " ("
                              + juce::String (middleDesc_.uid)
                              + ")");
}

bool DynamicMiddleProcessorEffectEngine::installMiddleProcessors (
    const ProcessorCreateFn& /*create*/)
{
    if (! hasMiddleDesc_ || engine() == nullptr || trackId() == kbuss::kInvalidObjectId)
        return true;

    auto [st, id] = engine()->create_processor (middleDesc_, "dynamic_fx");
    if (st != kbuss::Status::Ok)
    {
        juce::Logger::writeToLog ("DynamicMiddleProcessorEffectEngine: create_processor failed (status="
                                  + juce::String (static_cast<int> (st)) + ")");
        return false;
    }

    if (engine()->add_plugin_to_track (id, trackId()) != kbuss::Status::Ok)
        return false;

    middleProcessorId_ = id;
    return true;
}
