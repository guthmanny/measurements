#include "AefJuceIncludes.h"
#include "OversamplingSettingsPanel.h"

#include "AudioEffectFrameworkProcessor.h"

namespace
{
constexpr int kRowHeight = 48;
constexpr int kLabelColumnWidth = 180;
constexpr float kIntroFontHeight = 16.0f;
} // namespace

namespace oversampling_settings
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
        auto labelArea = area.removeFromLeft (kLabelColumnWidth);
        area.removeFromLeft (10);
        label.setBounds (labelArea);

        if (auto* combo = dynamic_cast<atom::ComboBox*> (&control))
        {
            combo->setSize (combo->getIdealWidth(), area.getHeight());
            combo->setBounds (area.getX(), area.getY(),
                              juce::jmin (combo->getWidth(), area.getWidth()), area.getHeight());
        }
        else
        {
            control.setBounds (area);
        }
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
} // namespace oversampling_settings

OversamplingSettingsPanel::OversamplingSettingsPanel (AudioEffectFrameworkProcessor& processorIn,
                                                      AtomLookAndFeel& lookAndFeel)
    : processor (processorIn),
      atomLookAndFeel (lookAndFeel),
      introLabel ("modelingIntro", "Modeling")
{
    setLookAndFeel (&atomLookAndFeel);

    introLabel.setHintText ("QUALITY in the footer sets the oversampling factor (2× / 4× / 8×). "
                            "Modes control how intermediate samples are generated and anti-aliased. "
                            "Nonlinear lookup tables for white-box DS-1 are selected at build time "
                            "(NUDSP_NL_TABLE_PROFILE=none|full|balanced|tiny).");
    introLabel.setFont (AtomLookAndFeel::getUIFont (kIntroFontHeight, juce::Font::bold));
    addAndMakeVisible (introLabel);

    upModeCombo.addItemList (processor.paramUpsamplerMode.items, 1);
    downModeCombo.addItemList (processor.paramDownsamplerMode.items, 1);
    upModeCombo.setJustificationType (juce::Justification::centredLeft);
    downModeCombo.setJustificationType (juce::Justification::centredLeft);

    upModeRow = std::make_unique<oversampling_settings::SettingsCardRow> (
        "upMode", "Upsample Mode", upModeCombo, kRowHeight);
    downModeRow = std::make_unique<oversampling_settings::SettingsCardRow> (
        "downMode", "Downsample Mode", downModeCombo, kRowHeight);

    auto* sectionPtr = new oversampling_settings::SettingsSection ("Algorithms");
    section.reset (sectionPtr);
    sectionPtr->addRow (static_cast<oversampling_settings::SettingsCardRow&> (*upModeRow));
    sectionPtr->addRow (static_cast<oversampling_settings::SettingsCardRow&> (*downModeRow));
    addAndMakeVisible (*section);

    auto& vts = processor.parameters.valueTreeState;
    upModeAttachment = std::make_unique<ComboBoxAttachment> (vts, processor.paramUpsamplerMode.paramID, upModeCombo);
    downModeAttachment = std::make_unique<ComboBoxAttachment> (vts, processor.paramDownsamplerMode.paramID, downModeCombo);
}

OversamplingSettingsPanel::~OversamplingSettingsPanel()
{
    upModeAttachment.reset();
    downModeAttachment.reset();
    setLookAndFeel (nullptr);
}

int OversamplingSettingsPanel::getPreferredPanelHeight() const noexcept
{
    const int introH = 56;
    const int sectionH = section != nullptr
        ? static_cast<oversampling_settings::SettingsSection&> (*section).getPreferredHeight()
        : 160;
    return introH + 12 + sectionH + 16;
}

void OversamplingSettingsPanel::paint (juce::Graphics& g)
{
    g.fillAll (findColour (juce::ResizableWindow::backgroundColourId));
}

void OversamplingSettingsPanel::resized()
{
    auto bounds = getLocalBounds().reduced (16);
    introLabel.setBounds (bounds.removeFromTop (56));
    bounds.removeFromTop (12);
    if (section != nullptr)
        section->setBounds (bounds.removeFromTop (
            static_cast<oversampling_settings::SettingsSection&> (*section).getPreferredHeight()));
}
