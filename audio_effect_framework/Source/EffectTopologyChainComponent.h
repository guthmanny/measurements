#pragma once

#include <functional>
#include <memory>
#include <vector>

#include <juce_atom_theme/juce_atom_theme.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "EffectTopologyModule.h"

/** Vertical internal signal chain with connectors and per-module ON/OFF toggles. */
class EffectTopologyChainComponent final : public juce::Component
{
public:
    using ModuleBypassChangedFn = std::function<void (const juce::String& moduleId, bool bypassed)>;

    EffectTopologyChainComponent();
    ~EffectTopologyChainComponent() override;

    void setModules (const juce::Array<EffectTopologyModule>& modules);
    void setModuleBypassChangedCallback (ModuleBypassChangedFn callback);

    int getPreferredHeight() const noexcept;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    class ModuleRow final : public juce::Component
    {
    public:
        ModuleRow (const EffectTopologyModule& module, ModuleBypassChangedFn onChange);

        void syncFromModule (const EffectTopologyModule& module);
        int getRowHeight() const noexcept { return kRowHeight; }

        void resized() override;

    private:
        static constexpr int kRowHeight = 48;
        static constexpr int kLabelColumnWidth = 180;

        juce::String moduleId;
        ModuleBypassChangedFn onChange;
        atom::SettingsCard card;
        atom::Label label;
        atom::ToggleButton toggle;
    };

    static constexpr int kConnectorHeight = 20;
    static constexpr int kEndpointLabelHeight = 18;

    void rebuildRows();

    juce::Array<EffectTopologyModule> modules_;
    ModuleBypassChangedFn bypassChangedCallback_;
    std::vector<std::unique_ptr<ModuleRow>> rows_;
    atom::Label inLabel;
    atom::Label outLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectTopologyChainComponent)
};
