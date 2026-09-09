#pragma once

#include <memory>
#include <vector>

#include <juce_atom_theme/juce_atom_theme.h>

class AudioEffectFrameworkProcessor;
class AtomLookAndFeel;

/** Settings-style list of every kbuss parameter on the middle processor. */
class AllParamsPanel final : public juce::Component, private juce::Timer
{
public:
    AllParamsPanel(AudioEffectFrameworkProcessor& processor, AtomLookAndFeel& lookAndFeel);
    ~AllParamsPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void visibilityChanged() override;

    [[nodiscard]] int getPreferredPanelHeight() const noexcept;
    void refreshFromProcessor();

private:
    void timerCallback() override;
    void rebuild();
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AllParamsPanel)
};
