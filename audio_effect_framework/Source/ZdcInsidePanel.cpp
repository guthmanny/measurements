#include "ZdcInsidePanel.h"

#include "AudioEffectFrameworkProcessor.h"
#include "atom/AtomLookAndFeel.h"
#include "kbuss/processor.hpp"

#include <algorithm>
#include <cmath>

namespace
{
void styleFieldLabel (atom::Label& label)
{
    label.setJustificationType (juce::Justification::centredLeft);
    label.setFont (AtomLookAndFeel::getUIFont (13.0f, juce::Font::plain));
    label.setMinimumHorizontalScale (1.0f);
    label.setAutoResizeEnabled (false);
    label.setInterceptsMouseClicks (false, false);
}

void styleViewSlider (atom::Slider& slider, double minValue, double maxValue, double interval,
                      const juce::String& suffix)
{
    slider.setSliderStyle (juce::Slider::LinearHorizontal);
    slider.setRange (minValue, maxValue, interval);
    slider.setTextValueSuffix (suffix);
    slider.setSliderSnapsToMousePosition (false);
    slider.setValueLabelPos (atom::Slider::ValueLabelPos::Right);
}

void layoutLabeledRow (juce::Rectangle<int> row, atom::Label& label, juce::Component& control, int labelW)
{
    label.setBounds (row.removeFromLeft (labelW));
    control.setBounds (row);
}

void computeIrMagnitudeDb (const std::vector<float>& ir, std::vector<float>& magDb, int& fftSize)
{
    int size = 2048;
    while (size < static_cast<int> (ir.size()) * 2 && size < 16384)
        size *= 2;

    int order = 0;
    while ((1 << order) < size)
        ++order;
    fftSize = 1 << order;

    juce::dsp::FFT fft (order);
    // JUCE real FFT scratch must be 2 * fftSize even for non-negative bins only.
    std::vector<float> buf (static_cast<std::size_t> (fftSize) * 2u, 0.f);
    const auto n = std::min (ir.size(), static_cast<std::size_t> (fftSize));
    if (n > 0)
        std::copy_n (ir.data(), n, buf.begin());

    fft.performFrequencyOnlyForwardTransform (buf.data(), true);

    const int bins = fftSize / 2 + 1;
    magDb.assign (static_cast<std::size_t> (bins), SpectrumCurveComponent::defaultMagMinDb);
    float peak = 1.0e-12f;
    for (int i = 0; i < bins; ++i)
        peak = juce::jmax (peak, buf[static_cast<std::size_t> (i)]);

    for (int i = 0; i < bins; ++i)
    {
        const float mag = buf[static_cast<std::size_t> (i)];
        magDb[static_cast<std::size_t> (i)] =
            20.0f * std::log10 (juce::jmax (mag, 1.0e-12f) / peak);
    }
}
} // namespace

class ZdcInsidePanel::IrResponseWindow final : public juce::DocumentWindow
{
public:
    explicit IrResponseWindow (AtomLookAndFeel& lookAndFeel)
        : juce::DocumentWindow ("ZDC Frequency Response",
                                lookAndFeel.findColour (juce::ResizableWindow::backgroundColourId),
                                juce::DocumentWindow::closeButton | juce::DocumentWindow::minimiseButton)
    {
        setUsingNativeTitleBar (true);
        setResizable (true, false);
        auto* content = new Content();
        setContentOwned (content, true);
        centreWithSize (640, 460);
        setVisible (true);
        toFront (true);
    }

    SpectrumCurveComponent& curve() noexcept
    {
        return static_cast<Content*> (getContentComponent())->curve;
    }

    void closeButtonPressed() override { setVisible (false); }

private:
    struct Content final : juce::Component
    {
        Content()
        {
            addAndMakeVisible (curve);
            setSize (640, 460);
        }

        void resized() override { curve.setBounds (getLocalBounds().reduced (12)); }

        SpectrumCurveComponent curve;
    };
};

ZdcInsidePanel::ZdcInsidePanel (AudioEffectFrameworkProcessor& processorIn, AtomLookAndFeel& lookAndFeel)
    : processor (processorIn),
      atomLookAndFeel (lookAndFeel)
{
    setOpaque (true);
    setLookAndFeel (&atomLookAndFeel);

    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setEditable (false, false, false);
    titleLabel.setInterceptsMouseClicks (false, false);
    titleLabel.setHonorAssignedFont (true);
    titleLabel.setFont (juce::Font (18.0f, juce::Font::bold));
    addAndMakeVisible (titleLabel);

    backButton.setTooltip ("Return to the effect window");
    backButton.onClick = [this]
    {
        if (onBack)
            onBack();
    };
    addChildComponent (backButton);

    loadButton.onClick = [this] { chooseIrFile(); };
    addAndMakeVisible (loadButton);

    fileLabel.setJustificationType (juce::Justification::centredLeft);
    fileLabel.setMinimumHorizontalScale (0.7f);
    fileLabel.setAutoResizeEnabled (false);
    addAndMakeVisible (fileLabel);

    formatCombo.addItem ("Auto detect", 1);
    formatCombo.addItem ("WAV", 2);
    formatCombo.addItem ("AIFF", 3);
    formatCombo.addItem ("Raw float32 LE", 4);
    formatCombo.addItem ("Raw int16 LE", 5);
    formatCombo.setSelectedId (1, juce::dontSendNotification);
    formatCombo.onChange = [this]
    {
        updateHeaderSkipEnabled();
        if (currentFile.existsAsFile())
            reloadFromFile (true);
    };
    addAndMakeVisible (formatLabel);
    addAndMakeVisible (formatCombo);

    styleViewSlider (headerSkipSlider, 0.0, 65536.0, 1.0, " bytes");
    styleViewSlider (startSlider, 0.0, 1.0, 1.0, " smp");
    styleViewSlider (lengthSlider, 1.0, 2.0, 1.0, " smp");
    styleViewSlider (delaySlider, 0.0, 48000.0, 1.0, " smp");
    styleViewSlider (maxIrSlider, 64.0, 262144.0, 1.0, " smp");
    delaySlider.setValue (0.0, juce::dontSendNotification);
    maxIrSlider.setValue (8192.0, juce::dontSendNotification);
    lengthSlider.setValue (1.0, juce::dontSendNotification);

    auto bindApply = [this] (atom::Slider& slider)
    {
        slider.onValueChange = [this]
        {
            if (! updating_)
                applyCurrentIr();
        };
    };
    bindApply (headerSkipSlider);
    headerSkipSlider.onValueChange = [this]
    {
        if (updating_)
            return;
        if (usingRawPayload() && currentFile.existsAsFile())
            reloadFromFile (false);
        else
            applyCurrentIr();
    };
    bindApply (startSlider);
    bindApply (lengthSlider);
    bindApply (delaySlider);
    bindApply (maxIrSlider);

    addAndMakeVisible (headerSkipLabel);
    addAndMakeVisible (headerSkipSlider);
    addAndMakeVisible (startLabel);
    addAndMakeVisible (startSlider);
    addAndMakeVisible (lengthLabel);
    addAndMakeVisible (lengthSlider);
    addAndMakeVisible (delayLabel);
    addAndMakeVisible (delaySlider);
    addAndMakeVisible (maxIrLabel);
    addAndMakeVisible (maxIrSlider);

    latencyLabel.setJustificationType (juce::Justification::centredLeft);
    latencyLabel.setAutoResizeEnabled (false);
    statusLabel.setJustificationType (juce::Justification::centredLeft);
    statusLabel.setAutoResizeEnabled (false);
    statusLabel.setMinimumHorizontalScale (0.65f);
    addAndMakeVisible (latencyLabel);
    addAndMakeVisible (statusLabel);

    responseButton.onClick = [this] { openFrequencyResponseWindow(); };
    addAndMakeVisible (responseButton);
    addAndMakeVisible (responseCurve);

    styleControls();
    updateHeaderSkipEnabled();
    updateFrequencyResponse();
}

ZdcInsidePanel::~ZdcInsidePanel()
{
    closeFrequencyResponseWindow();
    setLookAndFeel (nullptr);
}

void ZdcInsidePanel::styleControls()
{
    styleFieldLabel (formatLabel);
    styleFieldLabel (headerSkipLabel);
    styleFieldLabel (startLabel);
    styleFieldLabel (lengthLabel);
    styleFieldLabel (delayLabel);
    styleFieldLabel (maxIrLabel);
    styleFieldLabel (latencyLabel);
    styleFieldLabel (statusLabel);
    styleFieldLabel (fileLabel);
}

void ZdcInsidePanel::paint (juce::Graphics& g)
{
    g.fillAll (findColour (juce::ResizableWindow::backgroundColourId));
}

void ZdcInsidePanel::resized()
{
    auto bounds = getLocalBounds().reduced (kPad);

    auto title = bounds.removeFromTop (kTitleHeight);
    backButton.setVisible (static_cast<bool> (onBack));
    if (backButton.isVisible())
    {
        backButton.setBounds (title.removeFromLeft (kIconButton).withSizeKeepingCentre (kIconButton, kIconButton));
        backButton.toFront (false);
    }
    titleLabel.setBounds (title);

    bounds.removeFromTop (kGap);
    auto fileRow = bounds.removeFromTop (kButtonHeight);
    loadButton.setBounds (fileRow.removeFromLeft (110));
    fileRow.removeFromLeft (8);
    fileLabel.setBounds (fileRow);

    bounds.removeFromTop (kGap);
    layoutLabeledRow (bounds.removeFromTop (kComboRowHeight), formatLabel, formatCombo, kLabelWidth);
    layoutLabeledRow (bounds.removeFromTop (kSliderRowHeight), headerSkipLabel, headerSkipSlider, kLabelWidth);
    layoutLabeledRow (bounds.removeFromTop (kSliderRowHeight), startLabel, startSlider, kLabelWidth);
    layoutLabeledRow (bounds.removeFromTop (kSliderRowHeight), lengthLabel, lengthSlider, kLabelWidth);
    layoutLabeledRow (bounds.removeFromTop (kSliderRowHeight), delayLabel, delaySlider, kLabelWidth);
    layoutLabeledRow (bounds.removeFromTop (kSliderRowHeight), maxIrLabel, maxIrSlider, kLabelWidth);

    bounds.removeFromTop (4);
    latencyLabel.setBounds (bounds.removeFromTop (kStatusHeight));
    statusLabel.setBounds (bounds.removeFromTop (kStatusHeight));

    bounds.removeFromTop (kGap);
    responseButton.setBounds (bounds.removeFromTop (kButtonHeight).removeFromLeft (180));
    bounds.removeFromTop (kGap);
    responseCurve.setBounds (bounds);
}

void ZdcInsidePanel::lookAndFeelChanged()
{
    styleControls();
    if (responseWindow != nullptr)
        responseWindow->setBackgroundColour (findColour (juce::ResizableWindow::backgroundColourId));
}

void ZdcInsidePanel::visibilityChanged()
{
    if (isVisible())
        refreshFromProcessor();
}

IrLoadRequest::Format ZdcInsidePanel::selectedFormat() const
{
    switch (formatCombo.getSelectedId())
    {
        case 2:  return IrLoadRequest::Format::Wav;
        case 3:  return IrLoadRequest::Format::Aiff;
        case 4:  return IrLoadRequest::Format::RawF32;
        case 5:  return IrLoadRequest::Format::RawI16;
        default: return IrLoadRequest::Format::Auto;
    }
}

bool ZdcInsidePanel::usingRawPayload() const
{
    const auto format = selectedFormat();
    if (format == IrLoadRequest::Format::RawF32 || format == IrLoadRequest::Format::RawI16)
        return true;
    if (format == IrLoadRequest::Format::Auto)
        return lastHeader.kind == IrHeaderInfo::Kind::Raw || lastHeader.kind == IrHeaderInfo::Kind::Unknown;
    return false;
}

kbuss::Processor* ZdcInsidePanel::middleProcessor() const
{
    return processor.getKbussEngine().getMiddleProcessor();
}

double ZdcInsidePanel::engineSampleRate() const
{
    const double sr = processor.getSampleRate();
    return sr > 0.0 ? sr : decodedSampleRate;
}

void ZdcInsidePanel::updateHeaderSkipEnabled()
{
    headerSkipSlider.setEnabled (usingRawPayload());
}

void ZdcInsidePanel::refreshFromProcessor()
{
    auto* middle = middleProcessor();
    if (middle == nullptr)
        return;

    updating_ = true;
    if (middle->ir_capacity() > 0)
        maxIrSlider.setValue ((double) middle->ir_capacity(), juce::dontSendNotification);
    updating_ = false;

    if (decodedMono.empty() && middle->ir_len() > 0)
    {
        appliedIr.resize (middle->ir_len());
        middle->copy_ir (appliedIr.data(), appliedIr.size());
        if (appliedIr.empty())
            appliedIr = { 1.f };
        updateFrequencyResponse();
    }

    updateLatencyLabel();
    updateStatusText();
}

void ZdcInsidePanel::chooseIrFile()
{
    const auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
    const auto startDir = currentFile.existsAsFile()
                              ? currentFile.getParentDirectory()
                              : juce::File::getSpecialLocation (juce::File::userDocumentsDirectory);

    fileChooser = std::make_shared<juce::FileChooser> ("Load impulse response",
                                                       startDir,
                                                       "*.wav;*.aiff;*.aif;*.raw;*.pcm;*.irs;*");
    fileChooser->launchAsync (flags, [safe = juce::Component::SafePointer<ZdcInsidePanel> (this),
                                      chooser = fileChooser] (const juce::FileChooser& fc)
    {
        if (safe == nullptr)
            return;
        const auto file = fc.getResult();
        if (file == juce::File())
            return;
        safe->currentFile = file;
        safe->reloadFromFile (true);
    });
}

void ZdcInsidePanel::reloadFromFile (bool resetSlice)
{
    if (! currentFile.existsAsFile())
        return;

    lastHeader = IrFileLoader::detectHeader (currentFile);

    updating_ = true;
    if (! usingRawPayload() && lastHeader.headerBytes >= 0)
        headerSkipSlider.setValue ((double) lastHeader.headerBytes, juce::dontSendNotification);
    updating_ = false;

    IrLoadRequest request;
    request.file = currentFile;
    request.format = selectedFormat();
    request.headerSkipBytes = static_cast<int> (headerSkipSlider.getValue());
    request.startSample = 0;
    request.lengthSamples = 0;

    const auto result = IrFileLoader::load (request);
    if (! result.ok)
    {
        statusLabel.setText (result.error, juce::dontSendNotification);
        return;
    }

    decodedMono = std::move (result.samples);
    decodedSampleRate = result.sampleRate > 0.0 ? result.sampleRate : 48000.0;
    lastHeader = result.header;
    fileLabel.setText (currentFile.getFileName() + "  ·  " + lastHeader.description,
                       juce::dontSendNotification);

    updateSliderRanges();
    updating_ = true;
    if (resetSlice)
    {
        startSlider.setValue (0.0, juce::dontSendNotification);
        lengthSlider.setValue ((double) juce::jmax ((std::size_t) 1, decodedMono.size()),
                               juce::dontSendNotification);
    }
    updating_ = false;

    updateHeaderSkipEnabled();
    applyCurrentIr();
}

void ZdcInsidePanel::updateSliderRanges()
{
    updating_ = true;
    const double maxStart = juce::jmax (1.0, (double) decodedMono.size());
    const double maxLen = juce::jmax (2.0, (double) decodedMono.size());
    startSlider.setRange (0.0, maxStart, 1.0);
    lengthSlider.setRange (1.0, maxLen, 1.0);
    if (startSlider.getValue() > maxStart)
        startSlider.setValue (maxStart, juce::dontSendNotification);
    if (lengthSlider.getValue() > maxLen)
        lengthSlider.setValue (maxLen, juce::dontSendNotification);
    updating_ = false;
}

void ZdcInsidePanel::applyCurrentIr()
{
    auto* middle = middleProcessor();
    const int delay = juce::roundToInt (delaySlider.getValue());
    const int start = juce::roundToInt (startSlider.getValue());
    const int length = juce::roundToInt (lengthSlider.getValue());
    int maxLen = juce::roundToInt (maxIrSlider.getValue());
    const int payload = decodedMono.empty()
                            ? 1
                            : juce::jmax (0, juce::jmin (length, (int) decodedMono.size() - start));
    const int needed = delay + payload;
    if (needed > maxLen)
    {
        maxLen = juce::jmin (262144, juce::jmax (64, needed));
        updating_ = true;
        maxIrSlider.setValue ((double) maxLen, juce::dontSendNotification);
        updating_ = false;
    }

    appliedIr = IrFileLoader::buildAppliedIr (decodedMono, start, length, delay, maxLen);

    if (middle != nullptr && middle->supports_ir())
    {
        processor.suspendProcessing (true);
        kbuss::Status status = kbuss::Status::Ok;
        if (middle->ir_capacity() < static_cast<std::uint32_t> (maxLen))
            status = middle->set_ir_capacity (static_cast<std::uint32_t> (maxLen));
        if (status == kbuss::Status::Ok)
            status = middle->load_ir (appliedIr.data(), appliedIr.size());
        processor.suspendProcessing (false);

        if (status != kbuss::Status::Ok)
        {
            statusLabel.setText ("Failed to apply IR to ZDC", juce::dontSendNotification);
            updateFrequencyResponse();
            return;
        }
    }

    updateLatencyLabel();
    updateStatusText();
    updateFrequencyResponse();
}

void ZdcInsidePanel::updateStatusText()
{
    const auto delay = juce::roundToInt (delaySlider.getValue());
    const auto sr = engineSampleRate();
    const auto delayMs = sr > 0.0 ? (1000.0 * (double) delay / sr) : 0.0;
    juce::String text;
    text << "Applied " << (int) appliedIr.size() << " samples";
    text << "  ·  delay " << juce::String (delayMs, 2) << " ms";
    if (lastHeader.description.isNotEmpty())
        text << "  ·  " << lastHeader.description;
    statusLabel.setText (text, juce::dontSendNotification);
}

void ZdcInsidePanel::updateLatencyLabel()
{
    std::size_t latency = 0;
    if (auto* middle = middleProcessor())
        latency = middle->latency_samples();
    latencyLabel.setText ("Engine latency: " + juce::String ((int) latency) + " samples (ZDC head is zero-delay)",
                          juce::dontSendNotification);
}

void ZdcInsidePanel::updateFrequencyResponse()
{
    std::vector<float> magDb;
    int fftSize = 0;
    computeIrMagnitudeDb (appliedIr, magDb, fftSize);
    const double sr = engineSampleRate();
    const auto& mag = magDb;
    responseCurve.setSpectrumMagnitudes (mag, sr, fftSize);
    if (responseWindow != nullptr && responseWindow->isVisible())
        responseWindow->curve().setSpectrumMagnitudes (mag, sr, fftSize);
}

void ZdcInsidePanel::openFrequencyResponseWindow()
{
    if (responseWindow == nullptr)
        responseWindow = std::make_unique<IrResponseWindow> (atomLookAndFeel);

    responseWindow->setVisible (true);
    responseWindow->toFront (true);
    updateFrequencyResponse();
}

void ZdcInsidePanel::closeFrequencyResponseWindow()
{
    responseWindow.reset();
}
