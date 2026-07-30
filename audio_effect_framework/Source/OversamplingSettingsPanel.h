#pragma once

#include <juce_atom_theme/juce_atom_theme.h>
#include "AefJuceIncludes.h"
#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>

class AudioEffectFrameworkProcessor;

/** Settings → Modeling: oversampling modes + (build-time) NL table bake. */
class OversamplingSettingsPanel final : public juce::Component
{
public:
    OversamplingSettingsPanel (AudioEffectFrameworkProcessor& processor, AtomLookAndFeel& lookAndFeel);
    ~OversamplingSettingsPanel() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    int getPreferredPanelHeight() const noexcept;

private:
    AudioEffectFrameworkProcessor& processor;
    AtomLookAndFeel& atomLookAndFeel;

    atom::Label introLabel;
    atom::ComboBox upModeCombo;
    atom::ComboBox downModeCombo;

    std::unique_ptr<juce::Component> upModeRow;
    std::unique_ptr<juce::Component> downModeRow;
    std::unique_ptr<juce::Component> section;

    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<ComboBoxAttachment> upModeAttachment;
    std::unique_ptr<ComboBoxAttachment> downModeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OversamplingSettingsPanel)
};
