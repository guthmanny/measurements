#include "AefJuceIncludes.h"
#include "TopologySettingsPanel.h"

TopologySettingsPanel::TopologySettingsPanel(AudioEffectFrameworkProcessor& processor,
                                             AtomLookAndFeel& lookAndFeel)
    : flow(processor, lookAndFeel)
{
    addAndMakeVisible(flow);
}

TopologySettingsPanel::~TopologySettingsPanel() = default;

int TopologySettingsPanel::getPreferredPanelHeight() const noexcept
{
    return flow.getPreferredHeight();
}

void TopologySettingsPanel::paint(juce::Graphics& g)
{
    g.fillAll(findColour(juce::ResizableWindow::backgroundColourId));
}

void TopologySettingsPanel::resized()
{
    flow.setBounds(getLocalBounds());
}
