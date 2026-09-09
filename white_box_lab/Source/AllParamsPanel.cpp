#include "AllParamsPanel.h"

#include <map>

#include "AudioEffectFrameworkProcessor.h"
#include "KbussParamSliderUtils.h"
#include "KbussParamUiUtils.h"

namespace {
constexpr int kRowHeight = 40;
constexpr int kIntroHeight = 40;
constexpr int kScrollMinHeight = 280;

juce::String groupTitleForParam(const kbuss::ParameterDescriptor& desc)
{
    const std::string_view id = desc.id;
    const auto dot = id.find('.');
    if (dot == std::string_view::npos)
        return "General";
    return juce::String(std::string(id.substr(0, dot)));
}
}  // namespace

struct AllParamsPanel::SectionBundle
{
    std::unique_ptr<atom::Label> title;
    std::vector<std::unique_ptr<atom::Label>> labels;
    int height = 0;
};

AllParamsPanel::AllParamsPanel(AudioEffectFrameworkProcessor& processor, AtomLookAndFeel& lookAndFeel)
    : processor_(processor), atomLookAndFeel_(lookAndFeel), introLabel("allParamsIntro", "All kbuss interfaces")
{
    setLookAndFeel(&atomLookAndFeel_);
    introLabel.setFont(AtomLookAndFeel::getUIFont(16.0f, juce::Font::bold));
    introLabel.setHintText("Every reflected parameter on the middle white_box processor.");
    addAndMakeVisible(introLabel);

    scrollViewport.setViewedComponent(&scrollContent, false);
    scrollViewport.setScrollBarsShown(true, false);
    addAndMakeVisible(scrollViewport);
    startTimerHz(4);
}

AllParamsPanel::~AllParamsPanel()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void AllParamsPanel::paint(juce::Graphics& g)
{
    g.fillAll(findColour(juce::ResizableWindow::backgroundColourId));
}

void AllParamsPanel::visibilityChanged()
{
    if (isVisible())
        rebuild();
}

void AllParamsPanel::refreshFromProcessor()
{
    rebuild();
}

void AllParamsPanel::timerCallback()
{
    const int generation = processor_.middleProcessorGeneration();
    if (generation != lastMiddleGeneration_)
        rebuild();
}

void AllParamsPanel::clearSections()
{
    sections_.clear();
    paramControls_.clear();
    scrollContent.removeAllChildren();
}

void AllParamsPanel::rebuild()
{
    clearSections();

    auto* middle = processor_.getKbussEngine().getMiddleProcessor();
    if (middle == nullptr)
    {
        resized();
        return;
    }

    std::map<juce::String, std::vector<const kbuss::ParameterDescriptor*>> grouped;
    for (const auto& desc : middle->parameters())
        grouped[groupTitleForParam(desc)].push_back(&desc);

    int contentHeight = 0;
    for (auto& [title, params] : grouped)
    {
        auto bundle = std::make_unique<SectionBundle>();
        bundle->title = std::make_unique<atom::Label>(title + "Title", title);
        bundle->title->setFont(AtomLookAndFeel::getUIFont(13.0f, juce::Font::bold));
        scrollContent.addAndMakeVisible(*bundle->title);
        contentHeight += 28;

        for (const auto* desc : params)
        {
            const juce::String paramId(desc->id);
            const float initial = processor_.getMiddleParamDomain(paramId, desc->default_domain);

            auto label = std::make_unique<atom::Label>(paramId + "Label",
                                                       aef::kbuss_param_ui::paramDisplayLabel(*desc));
            scrollContent.addAndMakeVisible(*label);
            bundle->labels.push_back(std::move(label));

            if (desc->type == kbuss::ParameterType::Bool)
            {
                auto* toggle = new atom::ToggleButton(paramId, juce::String());
                paramControls_.add(toggle);
                toggle->setToggleState(initial >= 0.5f, juce::dontSendNotification);
                toggle->onClick = [this, paramId, toggle]
                {
                    processor_.setMiddleParamDomain(paramId, toggle->getToggleState() ? 1.0f : 0.0f);
                };
                scrollContent.addAndMakeVisible(toggle);
            }
            else
            {
                auto* slider = new atom::Slider();
                paramControls_.add(slider);
                aef::kbuss_param_ui::configureKbussParamSlider(
                    *slider, atomLookAndFeel_, desc->min_domain, desc->max_domain,
                    aef::kbuss_param_ui::paramSliderInterval(*desc),
                    aef::kbuss_param_ui::paramUnitSuffix(*desc));
                slider->setValue(initial, juce::dontSendNotification);
                slider->onValueChange = [this, paramId, slider]
                {
                    processor_.setMiddleParamDomain(paramId, static_cast<float>(slider->getValue()));
                };
                scrollContent.addAndMakeVisible(slider);
            }
            contentHeight += kRowHeight;
        }

        bundle->height = 28 + (int) params.size() * kRowHeight;
        sections_.push_back(std::move(bundle));
        contentHeight += 8;
    }

    scrollContent.setSize(juce::jmax(400, getWidth()), juce::jmax(contentHeight, kScrollMinHeight));
    layoutScrollContent();
    resized();
    lastMiddleGeneration_ = processor_.middleProcessorGeneration();
}

void AllParamsPanel::layoutScrollContent()
{
    auto area = scrollContent.getLocalBounds().reduced(8, 0);
    int controlIndex = 0;
    for (auto& bundle : sections_)
    {
        bundle->title->setBounds(area.removeFromTop(24));
        area.removeFromTop(4);
        for (auto& label : bundle->labels)
        {
            auto row = area.removeFromTop(kRowHeight);
            label->setBounds(row.removeFromLeft(220));
            row.removeFromLeft(8);
            if (auto* control = paramControls_[controlIndex++])
                control->setBounds(row.reduced(0, 4));
        }
        area.removeFromTop(8);
    }
}

int AllParamsPanel::getPreferredPanelHeight() const noexcept
{
    return kIntroHeight + kScrollMinHeight + 16;
}

void AllParamsPanel::resized()
{
    auto bounds = getLocalBounds().reduced(12, 8);
    introLabel.setBounds(bounds.removeFromTop(kIntroHeight));
    bounds.removeFromTop(6);
    scrollViewport.setBounds(bounds);
    scrollContent.setSize(juce::jmax(1, scrollViewport.getWidth()), scrollContent.getHeight());
    layoutScrollContent();
}
