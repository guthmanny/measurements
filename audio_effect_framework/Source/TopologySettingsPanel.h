#pragma once

#include <juce_atom_theme/juce_atom_theme.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>

#include "SignalFlowPanel.h"

class AudioEffectFrameworkProcessor;

/** Settings → Signal Flow: operator chain, bypass, schematic. */
class TopologySettingsPanel final : public juce::Component
{
public:
    TopologySettingsPanel(AudioEffectFrameworkProcessor& processor, AtomLookAndFeel& lookAndFeel);
    ~TopologySettingsPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    int getPreferredPanelHeight() const noexcept;

private:
    SignalFlowPanel flow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TopologySettingsPanel)
};
