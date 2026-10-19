#pragma once

#include <functional>
#include <memory>
#include <vector>

#include <juce_atom_theme/juce_atom_theme.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "EffectTopologyModule.h"

/** Vertical internal signal chain with connectors, ON/OFF, and node selection. */
class EffectTopologyChainComponent final : public juce::Component
{
public:
    using ModuleBypassChangedFn = std::function<void(const juce::String& moduleId, bool bypassed)>;
    using ModuleSelectedFn = std::function<void(const EffectTopologyModule& module)>;

    EffectTopologyChainComponent();
    ~EffectTopologyChainComponent() override;

    void setModules(const juce::Array<EffectTopologyModule>& modules);
    void setModuleBypassChangedCallback(ModuleBypassChangedFn callback);
    void setModuleSelectedCallback(ModuleSelectedFn callback);
    void setSelectedModuleId(const juce::String& moduleId);
    [[nodiscard]] juce::String getSelectedModuleId() const noexcept { return selectedId_; }

    int getPreferredHeight() const noexcept;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    class ModuleRow final : public juce::Component
    {
    public:
        ModuleRow(const EffectTopologyModule& module,
                  ModuleBypassChangedFn onChange,
                  ModuleSelectedFn onSelect);

        void syncFromModule(const EffectTopologyModule& module, bool selected);
        int getRowHeight() const noexcept { return kRowHeight; }

        void paint(juce::Graphics& g) override;
        void resized() override;
        void mouseUp(const juce::MouseEvent& e) override;

    private:
        static constexpr int kRowHeight = 56;

        juce::String moduleId;
        ModuleBypassChangedFn onChange;
        ModuleSelectedFn onSelect;
        EffectTopologyModule module_;
        bool selected_ = false;
        atom::SettingsCard card;
        atom::Label label;
        atom::Label operatorLabel;
        atom::ToggleButton toggle;
    };

    static constexpr int kConnectorHeight = 18;
    static constexpr int kEndpointLabelHeight = 18;
    static constexpr int kRowHeight = 56;

    void rebuildRows();

    juce::Array<EffectTopologyModule> modules_;
    juce::String selectedId_;
    ModuleBypassChangedFn bypassChangedCallback_;
    ModuleSelectedFn selectedCallback_;
    std::vector<std::unique_ptr<ModuleRow>> rows_;
    atom::Label inLabel;
    atom::Label outLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectTopologyChainComponent)
};
