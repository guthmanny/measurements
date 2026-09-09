#pragma once

#include <memory>

#include <juce_atom_theme/juce_atom_theme.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "CircuitSchematicPanel.h"
#include "Ds1OpampAcMath.h"
#include "Ds1OpampAcPanel.h"

class SchematicWidthSplitter final : public juce::Component
{
public:
    std::function<void(int deltaX)> onDragDelta;

    SchematicWidthSplitter()
    {
        setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
    }

    void paint(juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();
        g.setColour(findColour(juce::Label::textColourId).withAlpha(0.12f));
        g.fillRect(bounds);
        g.setColour(findColour(juce::Label::textColourId).withAlpha(0.35f));
        g.fillRect(bounds.withSizeKeepingCentre(2.0f, bounds.getHeight() * 0.35f));
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        dragStartX_ = e.x;
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        const int delta = e.x - dragStartX_;
        dragStartX_ = e.x;
        if (onDragDelta != nullptr && delta != 0)
            onDragDelta(delta);
    }

private:
    int dragStartX_ = 0;
};

class MainComponent final : public juce::Component
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void layoutContentArea(juce::Rectangle<int> area);
    void applyTheme();
    void updatePlotView();
    void configureCombo(atom::ComboBox& combo);
    void configureKnob(atom::Slider& knob);
    void configureInjectBox(atom::Slider& box);
    void layoutKnobColumn(juce::Rectangle<int>& area, atom::Label& label, atom::Slider& knob) const;
    void syncInjectDefaultsToCircuit(ds1_ac::CircuitKind circuit);
    ds1_ac::CircuitKind getCircuitFromSelection() const;
    nx_pot_taper_e getPotTaperFromSelection() const;
    void syncPotTaperToCircuitDefault(ds1_ac::CircuitKind circuit);
    void syncDeviceModelCombo(ds1_ac::CircuitKind circuit);
    nx_opamp_model_e getOpampModelFromSelection() const;
    nx_bjt_npn_model_e getBjtModelFromSelection() const;
    nx_jfet_n_model_e getJfetModelFromSelection() const;
    ds1_ac::PlotKind getPlotKindFromSelection() const;
    double getSampleRateFromSelection() const;

    AtomLookAndFeel atomLookAndFeel;

    atom::Label titleLabel;
    atom::Label subtitleLabel;
    atom::TextButton themeButton{"themeButton", "Switch theme"};
    atom::ComboBox circuitBox{"circuitBox"};
    atom::ComboBox plotKindBox{"plotKindBox"};
    atom::ComboBox opampModelBox{"opampModelBox"};
    atom::ComboBox sampleRateBox{"sampleRateBox"};
    atom::ComboBox taperBox{"taperBox"};

    atom::Label sampleRateLabel;
    atom::Label taperLabel;
    atom::Label circuitLabel;

    atom::Label gainLabel;
    atom::Slider gainKnob{juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox};

    atom::Label secondaryLabel;
    atom::Slider secondaryKnob{juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox};

    atom::Label tertiaryLabel;
    atom::Slider tertiaryKnob{juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox};

    atom::Label injectFreqLabel;
    atom::Slider injectFreqBox{juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight};
    atom::Label injectAmpLabel;
    atom::Slider injectAmpBox{juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight};

    std::unique_ptr<CircuitSchematicPanel> schematicPanel;
    SchematicWidthSplitter schematicSplitter;
    std::unique_ptr<Ds1OpampAcPanel> acPanel;

    int schematicPanelWidth_ = 0;

    static constexpr int kSchematicMinWidth = 280;
    static constexpr int kAcPanelMinWidth = 420;
    static constexpr int kSchematicSplitterWidth = 6;
    static constexpr int kPanelGap = 12;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
