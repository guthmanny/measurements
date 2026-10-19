#pragma once

#include <functional>
#include <memory>
#include <vector>

#include <juce_atom_theme/juce_atom_theme.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "IrFileLoader.h"
#include "SpectrumCurveComponent.h"

class AudioEffectFrameworkProcessor;
class AtomLookAndFeel;

namespace kbuss {
class Processor;
}

/** Inside page for ZDC: IR file load, header/start/length/delay, and frequency response. */
class ZdcInsidePanel final : public juce::Component
{
public:
    ZdcInsidePanel (AudioEffectFrameworkProcessor& processor, AtomLookAndFeel& lookAndFeel);
    ~ZdcInsidePanel() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void lookAndFeelChanged() override;
    void visibilityChanged() override;

    int getPreferredWidth() const noexcept { return 720; }
    int getPreferredHeight() const noexcept { return 700; }

    void refreshFromProcessor();

    std::function<void()> onBack;
    std::function<void()> onGeometryChanged;

private:
    class IrResponseWindow;

    void styleControls();
    void chooseIrFile();
    void reloadFromFile (bool resetSlice);
    void applyCurrentIr();
    void updateSliderRanges();
    void updateStatusText();
    void updateLatencyLabel();
    void updateHeaderSkipEnabled();
    void updateFrequencyResponse();
    void openFrequencyResponseWindow();
    void closeFrequencyResponseWindow();
    [[nodiscard]] IrLoadRequest::Format selectedFormat() const;
    [[nodiscard]] bool usingRawPayload() const;
    [[nodiscard]] kbuss::Processor* middleProcessor() const;
    [[nodiscard]] double engineSampleRate() const;

    AudioEffectFrameworkProcessor& processor;
    AtomLookAndFeel& atomLookAndFeel;

    atom::Label titleLabel { "zdcInsideTitle", "ZDC" };
    atom::ShapeButton backButton { "zdcInsideBack", AtomIconLibrary::Icon::ChevronLeft };

    atom::ToolButton loadButton { "zdcLoadIr", "Load IR" };
    atom::Label fileLabel { "zdcFileLabel", "No IR file loaded" };

    atom::Label formatLabel { "zdcFormatLabel", "Header" };
    atom::ComboBox formatCombo { "zdcFormatCombo" };

    atom::Label headerSkipLabel { "zdcHeaderSkipLabel", "Skip" };
    atom::Slider headerSkipSlider;

    atom::Label startLabel { "zdcStartLabel", "Start" };
    atom::Slider startSlider;

    atom::Label lengthLabel { "zdcLengthLabel", "IR Length" };
    atom::Slider lengthSlider;

    atom::Label delayLabel { "zdcDelayLabel", "Delay" };
    atom::Slider delaySlider;

    atom::Label maxIrLabel { "zdcMaxIrLabel", "Max IR" };
    atom::Slider maxIrSlider;

    atom::Label latencyLabel { "zdcLatencyLabel", "Engine latency: 0 samples" };
    atom::Label statusLabel { "zdcStatusLabel", "Identity IR" };

    atom::ToolButton responseButton { "zdcResponseWindow", "Frequency Response" };
    SpectrumCurveComponent responseCurve;

    std::shared_ptr<juce::FileChooser> fileChooser;
    std::unique_ptr<IrResponseWindow> responseWindow;

    juce::File currentFile;
    IrHeaderInfo lastHeader;
    std::vector<float> decodedMono;
    std::vector<float> appliedIr { 1.f };
    double decodedSampleRate = 48000.0;
    bool updating_ = false;

    static constexpr int kPad = 8;
    static constexpr int kGap = 8;
    static constexpr int kTitleHeight = 36;
    static constexpr int kIconButton = 32;
    static constexpr int kLabelWidth = 96;
    static constexpr int kComboRowHeight = 36;
    static constexpr int kSliderRowHeight = 40;
    static constexpr int kButtonHeight = 32;
    static constexpr int kStatusHeight = 22;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZdcInsidePanel)
};
