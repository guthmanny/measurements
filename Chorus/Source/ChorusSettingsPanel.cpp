#include "ChorusSettingsPanel.h"

#include "PluginProcessor.h"

namespace
{
constexpr int kRowHeight = 48;
constexpr int kLabelColumnWidth = 160;
constexpr float kValueLabelReserveDlu = 72.0f;
} // namespace

namespace chorus_settings
{
class SettingsCardRow final : public juce::Component
{
public:
    SettingsCardRow (const juce::String& rowName, const juce::String& title, juce::Component& controlToEmbed, int height)
        : label (rowName + "Label", title), control (controlToEmbed), rowHeight (height)
    {
        card.setMinPanelHeight (rowHeight);
        label.setJustificationType (juce::Justification::centredLeft);
        label.setFont (AtomLookAndFeel::getUIFont (AtomLookAndFeel::getSystemUIFontHeight(), juce::Font::plain));
        label.setMinimumHorizontalScale (1.0f);
        label.setAutoResizeEnabled (false);
        card.addAndMakeVisible (label);
        card.addAndMakeVisible (control);
        addAndMakeVisible (card);
        setSize (0, rowHeight);
    }

    int getRowHeight() const noexcept { return rowHeight; }

    void resized() override
    {
        card.setBounds (getLocalBounds());
        auto area = card.getLocalBounds().reduced (12, 8);
        label.setBounds (area.removeFromLeft (kLabelColumnWidth));
        area.removeFromLeft (10);
        control.setBounds (area);
    }

private:
    atom::SettingsCard card;
    atom::Label label;
    juce::Component& control;
    int rowHeight;
};

class SettingsSection final : public juce::Component
{
public:
    explicit SettingsSection (const juce::String& title)
    {
        groupedList.setTitle (title);
        groupedList.setHeaderFont (AtomLookAndFeel::getSystemUIFont (juce::Font::bold));
        addAndMakeVisible (groupedList);
    }

    void addRow (SettingsCardRow& row)
    {
        rows.push_back (&row);
        groupedList.addItem (&row);
    }

    int getPreferredHeight() const
    {
        const int headerH = juce::roundToInt (AtomLookAndFeel::getSystemUIFontHeight()) + 10;
        int total = headerH + padding * 2;
        for (size_t i = 0; i < rows.size(); ++i)
        {
            total += rows[i]->getRowHeight();
            if (i + 1 < rows.size())
                total += itemGap;
        }
        return total;
    }

    void resized() override { groupedList.setBounds (getLocalBounds()); }

private:
    static constexpr int padding = 12;
    static constexpr int itemGap = 8;

    atom::GroupedList groupedList;
    std::vector<SettingsCardRow*> rows;
};
} // namespace chorus_settings

void ChorusSettingsPanel::configureHorizontalSlider (atom::Slider& slider, double minV, double maxV,
                                                     double interval, const juce::String& suffix)
{
    slider.setRange (minV, maxV, interval);
    slider.setTextValueSuffix (suffix);
    slider.setSliderSnapsToMousePosition (false);
    slider.setValueLabelPos (atom::Slider::ValueLabelPos::Right);

    atom::SliderStyleOverride styleOverride;
    styleOverride.metrics.linearHorizontalValueLabelReserveDlu = kValueLabelReserveDlu;
    atomLookAndFeel.setSliderStyleOverride (slider, styleOverride);
}

ChorusSettingsPanel::ChorusSettingsPanel (ChorusAudioProcessor& processorIn, AtomLookAndFeel& lookAndFeel)
    : processor (processorIn),
      atomLookAndFeel (lookAndFeel),
      introLabel ("chorusSettingsIntro", "Chorus")
{
    setLookAndFeel (&atomLookAndFeel);

    const auto limits = processor.getChorusNuDspLimits();

    introLabel.setHintText (
        "Set Chorus engine limits. Max delay, LFO rate, and amount cap the main controls; changing those "
        "resets Rate, Delay, and Amount to NuDSP defaults. LFO min freq sets the slowest rate and the "
        "LFO chart time axis (one period).");
    introLabel.setFont (AtomLookAndFeel::getUIFont (16.0f, juce::Font::bold));
    introLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (introLabel);

    configureHorizontalSlider (maxDelaySlider, limits.delayMin, limits.delayMax, 0.1, " ms");
    configureHorizontalSlider (minLfoFreqSlider, limits.rateMin, 1.0, 0.01, " Hz");
    configureHorizontalSlider (maxLfoFreqSlider, limits.rateMin, limits.rateMax, 0.01, " Hz");
    configureHorizontalSlider (maxAmountSlider, limits.amountMin, limits.amountMax, 0.1, " ms");

    maxDelayRow = std::make_unique<chorus_settings::SettingsCardRow> (
        "maxDelayRow", "MAX DELAY TIME", maxDelaySlider, kRowHeight);
    minLfoFreqRow = std::make_unique<chorus_settings::SettingsCardRow> (
        "minLfoFreqRow", "LFO MIN FREQ", minLfoFreqSlider, kRowHeight);
    maxLfoFreqRow = std::make_unique<chorus_settings::SettingsCardRow> (
        "maxLfoFreqRow", "MAX LFO FREQ", maxLfoFreqSlider, kRowHeight);
    maxAmountRow = std::make_unique<chorus_settings::SettingsCardRow> (
        "maxAmountRow", "MAX CHORUS AMOUNT", maxAmountSlider, kRowHeight);

    auto chorusSection = std::make_unique<chorus_settings::SettingsSection> ("Engine Limits");
    chorusSection->addRow (static_cast<chorus_settings::SettingsCardRow&> (*maxDelayRow));
    chorusSection->addRow (static_cast<chorus_settings::SettingsCardRow&> (*minLfoFreqRow));
    chorusSection->addRow (static_cast<chorus_settings::SettingsCardRow&> (*maxLfoFreqRow));
    chorusSection->addRow (static_cast<chorus_settings::SettingsCardRow&> (*maxAmountRow));
    section = std::move (chorusSection);
    addAndMakeVisible (*section);

    auto& vts = processor.parameters.valueTreeState;
    maxDelayAttachment =
        std::make_unique<SliderAttachment> (vts, processor.paramChorusMaxDelayTime.paramID, maxDelaySlider);
    minLfoFreqAttachment =
        std::make_unique<SliderAttachment> (vts, processor.paramChorusMinLfoFreq.paramID, minLfoFreqSlider);
    maxLfoFreqAttachment =
        std::make_unique<SliderAttachment> (vts, processor.paramChorusMaxLfoFreq.paramID, maxLfoFreqSlider);
    maxAmountAttachment =
        std::make_unique<SliderAttachment> (vts, processor.paramChorusMaxAmount.paramID, maxAmountSlider);

    vts.addParameterListener (processor.paramChorusMaxDelayTime.paramID, this);
    vts.addParameterListener (processor.paramChorusMinLfoFreq.paramID, this);
    vts.addParameterListener (processor.paramChorusMaxLfoFreq.paramID, this);
    vts.addParameterListener (processor.paramChorusMaxAmount.paramID, this);
}

ChorusSettingsPanel::~ChorusSettingsPanel()
{
    auto& vts = processor.parameters.valueTreeState;
    vts.removeParameterListener (processor.paramChorusMaxDelayTime.paramID, this);
    vts.removeParameterListener (processor.paramChorusMinLfoFreq.paramID, this);
    vts.removeParameterListener (processor.paramChorusMaxLfoFreq.paramID, this);
    vts.removeParameterListener (processor.paramChorusMaxAmount.paramID, this);
    maxDelayAttachment.reset();
    minLfoFreqAttachment.reset();
    maxLfoFreqAttachment.reset();
    maxAmountAttachment.reset();
    setLookAndFeel (nullptr);
}

void ChorusSettingsPanel::parameterChanged (const juce::String& parameterID, float /*newValue*/)
{
    if (parameterID == processor.paramChorusMaxDelayTime.paramID
        || parameterID == processor.paramChorusMaxLfoFreq.paramID
        || parameterID == processor.paramChorusMaxAmount.paramID)
    {
        processor.applyChorusLimits (true);
    }
    else if (parameterID == processor.paramChorusMinLfoFreq.paramID)
    {
        processor.applyChorusLimits (false);
    }
}

void ChorusSettingsPanel::paint (juce::Graphics& g)
{
    g.fillAll (findColour (juce::ResizableWindow::backgroundColourId));
}

int ChorusSettingsPanel::getPreferredPanelHeight() const noexcept
{
    int sectionH = 0;
    if (auto* chorusSection = dynamic_cast<const chorus_settings::SettingsSection*> (section.get()))
        sectionH = chorusSection->getPreferredHeight();

    constexpr int pad = 16;
    constexpr int introH = 56;
    constexpr int gap = 12;
    return pad + introH + gap + sectionH + pad;
}

void ChorusSettingsPanel::resized()
{
    auto bounds = getLocalBounds().reduced (16);
    introLabel.setBounds (bounds.removeFromTop (56));
    bounds.removeFromTop (12);

    if (auto* chorusSection = dynamic_cast<chorus_settings::SettingsSection*> (section.get()))
    {
        const int h = chorusSection->getPreferredHeight();
        chorusSection->setBounds (bounds.removeFromTop (h));
    }
}
