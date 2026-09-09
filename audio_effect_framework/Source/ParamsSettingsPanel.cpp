#include "ParamsSettingsPanel.h"

#include "AudioEffectFrameworkProcessor.h"
#include "DynamicPluginParamMetadata.h"
#include "KbussParamSliderUtils.h"
#include "KbussParamUiUtils.h"

#include <map>

namespace
{
constexpr int kRowHeight = 48;
constexpr int kLabelColumnWidth = 200;
constexpr float kIntroFontHeight = 16.0f;
constexpr int kIntroHeight = 52;
constexpr int kScrollMinHeight = 280;

class SettingsCardRow final : public juce::Component
{
public:
    SettingsCardRow(const juce::String& rowName, const juce::String& title, juce::Component& controlToEmbed)
        : label(rowName + "Label", title), control(controlToEmbed)
    {
        card.setMinPanelHeight(kRowHeight);
        label.setJustificationType(juce::Justification::centredLeft);
        label.setFont(AtomLookAndFeel::getUIFont(AtomLookAndFeel::getSystemUIFontHeight(), juce::Font::plain));
        card.addAndMakeVisible(label);
        card.addAndMakeVisible(control);
        addAndMakeVisible(card);
        setSize(0, kRowHeight);
    }

    int getRowHeight() const noexcept { return kRowHeight; }

    void resized() override
    {
        card.setBounds(getLocalBounds());
        auto area = card.getLocalBounds().reduced(12, 8);
        auto labelArea = area.removeFromLeft(kLabelColumnWidth);
        area.removeFromLeft(10);

        if (auto* toggle = dynamic_cast<atom::ToggleButton*>(&control))
        {
            toggle->setFontHeight(static_cast<float>(AtomLookAndFeel::getSystemUIFontHeight()));
            control.setBounds(area);
            const int labelH = juce::jmax(1, juce::roundToInt(label.getFont().getHeight()));
            const float tickCentreY = static_cast<float>(area.getY()) + static_cast<float>(toggle->getFixedHeight()) * 0.5f;
            const int labelY = juce::roundToInt(tickCentreY - static_cast<float>(labelH) * 0.5f);
            label.setBounds(labelArea.getX(), labelY, labelArea.getWidth(), labelH);
        }
        else
        {
            control.setBounds(area);
            label.setBounds(labelArea);
        }
    }

private:
    atom::SettingsCard card;
    atom::Label label;
    juce::Component& control;
};

class SettingsSection final : public juce::Component
{
public:
    explicit SettingsSection(const juce::String& title)
    {
        groupedList.setTitle(title);
        groupedList.setHeaderFont(AtomLookAndFeel::getSystemUIFont(juce::Font::bold));
        addAndMakeVisible(groupedList);
    }

    void addRow(SettingsCardRow& row)
    {
        rows.push_back(&row);
        groupedList.addItem(&row);
    }

    int getPreferredHeight() const
    {
        const int headerH = juce::roundToInt(AtomLookAndFeel::getSystemUIFontHeight()) + 10;
        int total = headerH + 24;
        for (std::size_t i = 0; i < rows.size(); ++i)
        {
            total += rows[i]->getRowHeight();
            if (i + 1 < rows.size())
                total += 8;
        }
        return total;
    }

    void resized() override { groupedList.setBounds(getLocalBounds()); }

private:
    atom::GroupedList groupedList;
    std::vector<SettingsCardRow*> rows;
};

juce::String groupTitleForParam(const kbuss::ParameterDescriptor& desc)
{
    const std::string_view id = desc.id;
    const auto dot = id.find('.');
    if (dot == std::string_view::npos)
        return "General";

    juce::String prefix = juce::String(std::string(id.substr(0, dot)));
    return prefix.replaceCharacter('[', ' ').replaceCharacter(']', ' ').trim();
}
}  // namespace

struct ParamsSettingsPanel::SectionBundle
{
    std::unique_ptr<SettingsSection> section;
    std::vector<std::unique_ptr<SettingsCardRow>> rows;
};

ParamsSettingsPanel::ParamsSettingsPanel(AudioEffectFrameworkProcessor& processor,
                                         AtomLookAndFeel& lookAndFeel)
    : processor_(processor), atomLookAndFeel_(lookAndFeel), introLabel("paramsIntro", "Effect Params")
{
    setLookAndFeel(&atomLookAndFeel_);

    introLabel.setHintText(
        "Internal model parameters for the middle effect processor (smoother, range, taper). "
        "User-facing controls remain on the main panel.");
    introLabel.setFont(AtomLookAndFeel::getUIFont(kIntroFontHeight, juce::Font::bold));
    addAndMakeVisible(introLabel);

    scrollViewport.setViewedComponent(&scrollContent, false);
    scrollViewport.setScrollBarsShown(true, false);
    addAndMakeVisible(scrollViewport);

    startTimerHz(4);
}

ParamsSettingsPanel::~ParamsSettingsPanel()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void ParamsSettingsPanel::paint(juce::Graphics& g)
{
    g.fillAll(findColour(juce::ResizableWindow::backgroundColourId));
}

void ParamsSettingsPanel::visibilityChanged()
{
    if (isVisible())
        rebuildFromMiddleProcessor();
}

void ParamsSettingsPanel::timerCallback()
{
    const int generation = processor_.middleProcessorGeneration();
    if (generation == lastMiddleGeneration_ && ! sections_.empty())
        return;

    rebuildFromMiddleProcessor();
}

void ParamsSettingsPanel::clearSections()
{
    sections_.clear();
    paramControls_.clear();
    scrollContent.removeAllChildren();
}

void ParamsSettingsPanel::rebuildFromMiddleProcessor()
{
    clearSections();

    if (! processor_.getKbussEngine().isReady())
    {
        resized();
        return;
    }

    auto* middle = processor_.getKbussEngine().getMiddleProcessor();
    if (middle == nullptr)
    {
        resized();
        return;
    }

    const auto& params = middle->parameters();
    const auto topLevelIds = aef::kbuss_param_ui::collectTopLevelParamIds(params);

    std::map<juce::String, std::vector<const kbuss::ParameterDescriptor*>> grouped;
    for (const auto& desc : params)
    {
        if (! aef::kbuss_param_ui::isInternalParam(desc, topLevelIds))
            continue;

        grouped[groupTitleForParam(desc)].push_back(&desc);
    }

    int contentHeight = 0;
    for (auto& [title, paramsInGroup] : grouped)
    {
        auto bundle = std::make_unique<SectionBundle>();
        bundle->section = std::make_unique<SettingsSection>(title);
        scrollContent.addAndMakeVisible(bundle->section.get());

        for (const auto* desc : paramsInGroup)
        {
            const juce::String paramId(desc->id);
            const juce::String labelText = aef::kbuss_param_ui::paramDisplayLabel(*desc);
            const float initial = processor_.getMiddleParamDomain(paramId, desc->default_domain);

            juce::Component* controlRaw = nullptr;

            if (desc->type == kbuss::ParameterType::Bool)
            {
                auto* toggle = new atom::ToggleButton(paramId, juce::String());
                paramControls_.add(toggle);
                toggle->setToggleState(initial >= 0.5f, juce::dontSendNotification);
                toggle->onClick = [this, paramId, toggle]() {
                    processor_.setMiddleParamDomain(paramId, toggle->getToggleState() ? 1.0f : 0.0f);
                };
                controlRaw = toggle;
            }
            else
            {
                auto* slider = new atom::Slider();
                paramControls_.add(slider);
                aef::kbuss_param_ui::configureKbussParamSlider(
                    *slider,
                    atomLookAndFeel_,
                    desc->min_domain,
                    desc->max_domain,
                    aef::kbuss_param_ui::paramSliderInterval(*desc),
                    aef::kbuss_param_ui::paramUnitSuffix(*desc));
                slider->setValue(initial, juce::dontSendNotification);
                slider->onValueChange = [this, paramId, slider]() {
                    processor_.setMiddleParamDomain(paramId, static_cast<float>(slider->getValue()));
                };
                controlRaw = slider;
            }

            auto row = std::make_unique<SettingsCardRow>(paramId + "Row", labelText, *controlRaw);
            bundle->section->addRow(*row);
            bundle->rows.push_back(std::move(row));
        }

        contentHeight += bundle->section->getPreferredHeight() + 12;
        sections_.push_back(std::move(bundle));
    }

    if (sections_.empty())
    {
        const auto dynamicParams = processor_.getDynamicMiddleParamMetadata();
        if (! dynamicParams.empty())
        {
            auto bundle = std::make_unique<SectionBundle>();
            bundle->section = std::make_unique<SettingsSection>("Effect Controls");
            scrollContent.addAndMakeVisible(bundle->section.get());

            for (const auto& meta : dynamicParams)
            {
                if (aef::kbuss_param_ui::isFooterQualityParam(meta.id.toStdString()))
                    continue;

                const juce::String paramId = meta.id;
                const juce::String labelText = meta.displayLabel();
                const float initial = processor_.getMiddleParamDomain(paramId, meta.defaultDomain);

                kbuss::ParameterDescriptor desc;
                desc.id = meta.id.toStdString();
                desc.label = meta.label.toStdString();
                desc.min_domain = meta.minDomain;
                desc.max_domain = meta.maxDomain;
                desc.default_domain = meta.defaultDomain;

                auto* slider = new atom::Slider();
                paramControls_.add(slider);
                aef::kbuss_param_ui::configureKbussParamSlider(
                    *slider,
                    atomLookAndFeel_,
                    meta.minDomain,
                    meta.maxDomain,
                    aef::kbuss_param_ui::paramSliderInterval(desc),
                    aef::kbuss_param_ui::paramUnitSuffix(desc));
                slider->setValue(initial, juce::dontSendNotification);
                slider->onValueChange = [this, paramId, slider]() {
                    processor_.setMiddleParamDomain(paramId, static_cast<float>(slider->getValue()));
                };

                auto row = std::make_unique<SettingsCardRow>(paramId + "Row", labelText, *slider);
                bundle->section->addRow(*row);
                bundle->rows.push_back(std::move(row));
            }

            contentHeight += bundle->section->getPreferredHeight() + 12;
            sections_.push_back(std::move(bundle));
        }
    }

    scrollContent.setSize(juce::jmax(400, getWidth()), juce::jmax(contentHeight, kScrollMinHeight));
    layoutScrollContent();
    resized();

    lastMiddleGeneration_ = processor_.middleProcessorGeneration();
}

void ParamsSettingsPanel::layoutScrollContent()
{
    auto area = scrollContent.getLocalBounds().reduced(8, 0);
    for (auto& bundle : sections_)
    {
        const int h = bundle->section->getPreferredHeight();
        bundle->section->setBounds(area.removeFromTop(h));
        area.removeFromTop(12);
    }
}

int ParamsSettingsPanel::getPreferredPanelHeight() const noexcept
{
    return kIntroHeight + kScrollMinHeight + 16;
}

void ParamsSettingsPanel::resized()
{
    auto bounds = getLocalBounds().reduced(16, 12);
    introLabel.setBounds(bounds.removeFromTop(kIntroHeight));
    bounds.removeFromTop(8);
    scrollViewport.setBounds(bounds);

    const int viewportW = juce::jmax(1, scrollViewport.getWidth());
    scrollContent.setSize(viewportW, scrollContent.getHeight());
    layoutScrollContent();
}
