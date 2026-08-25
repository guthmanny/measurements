#pragma once

#include <juce_atom_theme/juce_atom_theme.h>

#include <memory>
#include <vector>

class AudioEffectFrameworkProcessor;
class AtomLookAndFeel;

/** Settings → Params Settings: internal middle-processor kbuss parameters. */
class ParamsSettingsPanel final : public juce::Component, private juce::Timer
{
public:
    ParamsSettingsPanel(AudioEffectFrameworkProcessor& processor, AtomLookAndFeel& lookAndFeel);
    ~ParamsSettingsPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void visibilityChanged() override;

    int getPreferredPanelHeight() const noexcept;

    void refreshFromProcessor() { rebuildFromMiddleProcessor(); }

private:
    void timerCallback() override;
    void rebuildFromMiddleProcessor();
    void clearSections();
    void layoutScrollContent();

    AudioEffectFrameworkProcessor& processor_;
    AtomLookAndFeel& atomLookAndFeel_;

    atom::Label introLabel;
    juce::Viewport scrollViewport;
    juce::Component scrollContent;

    struct SectionBundle;
    std::vector<std::unique_ptr<SectionBundle>> sections_;
    juce::OwnedArray<juce::Component> paramControls_;
    int lastMiddleGeneration_ = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParamsSettingsPanel)
};
