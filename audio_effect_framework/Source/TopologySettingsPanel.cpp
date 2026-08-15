#include "AefJuceIncludes.h"
#include "TopologySettingsPanel.h"

#include "AudioEffectFrameworkProcessor.h"

namespace
{
constexpr float kIntroFontHeight = 16.0f;
constexpr int kSectionPad = 16;
} // namespace

TopologySettingsPanel::TopologySettingsPanel (AudioEffectFrameworkProcessor& processorIn,
                                              AtomLookAndFeel& lookAndFeel)
    : processor (processorIn),
      atomLookAndFeel (lookAndFeel),
      introLabel ("topologyIntro", "Topology"),
      emptyLabel ("topologyEmpty", "No internal module topology for this effect.")
{
    setLookAndFeel (&atomLookAndFeel);

    introLabel.setFont (AtomLookAndFeel::getUIFont (kIntroFontHeight, juce::Font::bold));
    introLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (introLabel);

    emptyLabel.setJustificationType (juce::Justification::centredLeft);
    emptyLabel.setFont (AtomLookAndFeel::getUIFont (AtomLookAndFeel::getSystemUIFontHeight(), juce::Font::plain));
    emptyLabel.setAlpha (0.7f);
    addChildComponent (emptyLabel);

    chain.setModuleBypassChangedCallback ([this] (const juce::String& moduleId, bool bypassed)
    {
        processor.setEffectTopologyModuleBypassed (moduleId, bypassed);
    });
    addChildComponent (chain);

    refreshFromProcessor();
}

TopologySettingsPanel::~TopologySettingsPanel()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void TopologySettingsPanel::visibilityChanged()
{
    if (isVisible())
    {
        refreshFromProcessor();
        startTimerHz (10);
    }
    else
    {
        stopTimer();
    }
}

void TopologySettingsPanel::timerCallback()
{
    if (isVisible())
        refreshFromProcessor();
}

void TopologySettingsPanel::refreshFromProcessor()
{
    const auto modules = processor.getEffectTopologyModules();
    const bool hasModules = ! modules.isEmpty();

    const auto title = processor.getEffectTopologyTitle();
    introLabel.setText (title.isNotEmpty() ? title : juce::String ("Topology"), juce::dontSendNotification);

    emptyLabel.setVisible (! hasModules);

    if (hasModules)
    {
        if (! chain.isShowing())
            addAndMakeVisible (chain);
        chain.setModules (modules);
    }
    else
    {
        chain.setVisible (false);
    }

    chain.setVisible (hasModules);

    resized();
}

int TopologySettingsPanel::getPreferredPanelHeight() const noexcept
{
    const int introH = 28;
    if (chain.isVisible())
        return introH + kSectionPad + chain.getPreferredHeight() + kSectionPad;

    return introH + kSectionPad + 24 + kSectionPad;
}

void TopologySettingsPanel::paint (juce::Graphics& g)
{
    g.fillAll (findColour (juce::ResizableWindow::backgroundColourId));
}

void TopologySettingsPanel::resized()
{
    auto bounds = getLocalBounds().reduced (kSectionPad);
    introLabel.setBounds (bounds.removeFromTop (28));
    bounds.removeFromTop (8);

    if (chain.isVisible())
        chain.setBounds (bounds);
    else
        emptyLabel.setBounds (bounds.removeFromTop (24));
}
