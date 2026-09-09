#pragma once

#include <memory>

#include <juce_atom_theme/juce_atom_theme.h>

#include "MudspAssetResolver.h"
#include "SvgCircuitSync.h"

class AudioEffectFrameworkProcessor;
class AtomLookAndFeel;

class SchematicEditorPanel final : public juce::Component, private juce::Timer
{
public:
    SchematicEditorPanel(AudioEffectFrameworkProcessor& processor, AtomLookAndFeel& lookAndFeel);
    ~SchematicEditorPanel() override;

    void setCompositeKey(const juce::String& key);
    void paint(juce::Graphics& g) override;
    void resized() override;
    void visibilityChanged() override;

private:
    class SvgView;
    class CircuitList;

    void timerCallback() override;
    void reloadComposite();
    void loadSelectedStage();
    void rebuildStageChoices();
    void updateInternalToggleVisibility();

    AudioEffectFrameworkProcessor& processor_;
    AtomLookAndFeel& atomLookAndFeel_;
    juce::String compositeKey_;

    atom::Label compositeLabel;
    atom::Label stageLabel;
    atom::ComboBox stageCombo;
    atom::ToggleButton internalToggle { "internalToggle", "Internal" };
    atom::Label signalOutputActiveLabel;
    atom::Label signalOutputOtherLabel;
    bool showInternalSvg_ = false;
    std::unique_ptr<SvgView> compositeView;
    std::unique_ptr<SvgView> stageView;
    std::unique_ptr<CircuitList> circuitList;
    white_box_lab::SvgCircuitSync sync_;
    int lastMiddleGeneration_ = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SchematicEditorPanel)
};
