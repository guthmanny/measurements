#pragma once

#include <juce_atom_theme/juce_atom_theme.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>

#include "EffectTopologyChainComponent.h"

class AudioEffectFrameworkProcessor;

/** Settings → Topology: middle effect internal module chain. */
class TopologySettingsPanel final : public juce::Component, private juce::Timer
{
public:
    TopologySettingsPanel (AudioEffectFrameworkProcessor& processor, AtomLookAndFeel& lookAndFeel);
    ~TopologySettingsPanel() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void visibilityChanged() override;

    int getPreferredPanelHeight() const noexcept;

private:
    void timerCallback() override;
    void refreshFromProcessor();

    AudioEffectFrameworkProcessor& processor;
    AtomLookAndFeel& atomLookAndFeel;

    atom::Label introLabel;
    atom::Label emptyLabel;
    EffectTopologyChainComponent chain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TopologySettingsPanel)
};
