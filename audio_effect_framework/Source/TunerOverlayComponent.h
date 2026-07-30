#pragma once

#include <juce_atom_theme/juce_atom_theme.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "TunerDetector.h"

/** Full-editor dimmed overlay with 12-TET note name and cents MeterBar. */
class TunerOverlayComponent final : public juce::Component
{
public:
    static constexpr int preferredWidth = 420;
    static constexpr int preferredHeight = 500;

    TunerOverlayComponent();

    void setPitchResult (const TunerDetector::Result& result);
    void setInputDbFs (float dbFsLeft, double sampleRateHz, float dbFsRight = -100.0f);
    void clearPitch();
    void setTuningModeActive (bool active);

    void setPeriodicityThreshold (float threshold);
    float getPeriodicityThreshold() const;

    void setPointerSmoothMs (float milliseconds);
    float getPointerSmoothMs() const;

    void setLevelGateDbFs (float dbFs);
    float getLevelGateDbFs() const;

    std::function<void()> onCloseRequested;
    std::function<void (float)> onPeriodicityThresholdChanged;
    std::function<void (float)> onPointerSmoothMsChanged;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void applyCentsMeterStyle();
    void setPointerVisible (bool shouldShow);
    void beginPointerFromLeft (float targetNorm);
    void chasePointer (float centsNorm);
    void clearNoteDisplay();
    void retreatPointerHome();
    bool isPointerAtHomeRest() const;

    atom::SettingsCard panel;
    juce::Label titleLabel;
    juce::Label tuningModeLabel;
    juce::Label inputLevelLabel;
    juce::Label noteLabel;
    juce::Label freqLabel;
    juce::Label centsLabel;
    juce::Label periodicityValueLabel;
    juce::Label flatLabel;
    juce::Label sharpLabel;
    atom::MeterBar centsMeter;
    atom::Slider periodicitySlider { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    atom::Slider smoothSlider { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    atom::Slider levelGateSlider { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    juce::TextButton closeButton { "Close" };

    float lastInputDbFs = -100.0f;
    bool pointerVisible = false;
    bool pointerActive = false;   // tracking a note (not retreating/hidden)
    bool retreatingHome = false;
    int lastNoteIndex = -1;
    int lastOctave = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TunerOverlayComponent)
};

/** Dimmed backdrop that hosts TunerOverlayComponent centered. */
class TunerOverlay final : public juce::Component
{
public:
    TunerOverlay()
    {
        addAndMakeVisible (content);
        setInterceptsMouseClicks (true, true);
        setVisible (false);
    }

    TunerOverlayComponent& getContent() noexcept { return content; }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xcc000000));
    }

    void resized() override
    {
        content.setBounds (getLocalBounds().withSizeKeepingCentre (TunerOverlayComponent::preferredWidth,
                                                                   TunerOverlayComponent::preferredHeight));
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (! content.getBounds().contains (e.getPosition()) && content.onCloseRequested)
            content.onCloseRequested();
    }

private:
    TunerOverlayComponent content;
};
