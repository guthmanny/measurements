#pragma once

#include <juce_atom_theme/juce_atom_theme.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <string>

class AudioEffectFrameworkProcessor;

/** Host adapter: binds CircuitView to the live kbuss middle processor. */
class SignalFlowPanel final : public juce::Component, private juce::Timer
{
public:
    SignalFlowPanel(AudioEffectFrameworkProcessor& processor, AtomLookAndFeel& lookAndFeel);
    ~SignalFlowPanel() override;

    void resized() override;
    void visibilityChanged() override;
    void lookAndFeelChanged() override;

    int getPreferredWidth() const noexcept;
    int getPreferredHeight() const noexcept;
    void refreshFromProcessor();
    void applyDisplayMode();

    /** When set, CircuitView shows Back and invokes this callback. */
    std::function<void()> onBack;
    std::function<void()> onGeometryChanged;

private:
    struct OverlayBinding final : atom::schematic::Binding
    {
        explicit OverlayBinding(SignalFlowPanel& ownerIn) : owner(ownerIn) {}

        std::optional<atom::schematic::OverlayValue> read(const juce::String& overlayKey) override;
        bool write(const juce::String& overlayKey, const atom::schematic::OverlayValue& value) override;

        SignalFlowPanel& owner;
    };

    void timerCallback() override;
    juce::String currentModuleId() const;
    juce::String loadNodeSvg(const atom::graph::Node& node) const;

    static juce::String fingerprint(const atom::graph::Graph& graph);

    AudioEffectFrameworkProcessor& processor;
    AtomLookAndFeel& atomLookAndFeel;

    atom::CircuitView circuitView;
    OverlayBinding overlayBinding { *this };
    juce::String lastFingerprint_;
    std::string lastGfxKey_;
    int lastMiddleGeneration_ = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SignalFlowPanel)
};
