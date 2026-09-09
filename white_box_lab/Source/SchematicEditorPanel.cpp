#include "SchematicEditorPanel.h"

#include "AudioEffectFrameworkProcessor.h"
#include "ComponentValueCodec.h"

class SchematicEditorPanel::SvgView final : public juce::Component
{
public:
    void setSvgFile(const juce::File& file)
    {
        drawable_.reset();
        status_.clear();
        if (! file.existsAsFile())
        {
            status_ = file.getFullPathName().isEmpty() ? "No SVG path"
                                                       : "Missing: " + file.getFullPathName();
        }
        else if (auto xml = juce::parseXML(file))
        {
            drawable_ = juce::Drawable::createFromSVG(*xml);
            if (drawable_ == nullptr)
                status_ = "Failed to parse SVG: " + file.getFileName();
        }
        else
        {
            status_ = "Invalid SVG XML: " + file.getFileName();
        }
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::black);
        g.setColour(juce::Colours::white.withAlpha(0.15f));
        g.drawRect(getLocalBounds(), 1);

        if (drawable_ != nullptr)
        {
            drawable_->drawWithin(g, getLocalBounds().toFloat().reduced(8.0f),
                                  juce::RectanglePlacement::centred, 1.0f);
            return;
        }

        g.setColour(juce::Colours::white.withAlpha(0.7f));
        g.setFont(AtomLookAndFeel::getUIFont(13.0f));
        g.drawFittedText(status_.isEmpty() ? "No schematic" : status_, getLocalBounds().reduced(12),
                         juce::Justification::centred, 4);
    }

private:
    std::unique_ptr<juce::Drawable> drawable_;
    juce::String status_;
};

class SchematicEditorPanel::CircuitList final : public juce::Component
{
public:
    explicit CircuitList(AudioEffectFrameworkProcessor& processor)
        : processor_(processor)
    {
    }

    void setBindings(white_box_lab::SvgCircuitSync& sync)
    {
        sync_ = &sync;
        editors_.clear();
        labels_.clear();
        removeAllChildren();

        auto* middle = processor_.getKbussEngine().getMiddleProcessor();
        int y = 0;
        for (const auto& binding : sync.bindings())
        {
            auto label = std::make_unique<atom::Label>(
                binding.paramId + "L",
                binding.element.cellId + "  →  " + binding.paramId);
            label->setBounds(0, y, 260, 28);
            addAndMakeVisible(*label);
            labels_.push_back(std::move(label));

            auto editor = std::make_unique<juce::TextEditor>(binding.paramId + "E");
            editor->setText(binding.element.displayValue, false);
            editor->setBounds(268, y, 120, 28);
            const auto paramId = binding.paramId;
            editor->onReturnKey = [this, paramId, ed = editor.get()]
            {
                commit(paramId, ed->getText());
            };
            editor->onFocusLost = [this, paramId, ed = editor.get()]
            {
                commit(paramId, ed->getText());
            };
            addAndMakeVisible(*editor);
            editors_.push_back(std::move(editor));
            y += 32;
        }

        if (sync.bindings().empty())
        {
            empty_.setText("No structured circuit elements. Use All Params.",
                           juce::dontSendNotification);
            empty_.setBounds(0, 0, 400, 28);
            addAndMakeVisible(empty_);
            y = 32;
        }

        setSize(400, juce::jmax(32, y));
        juce::ignoreUnused(middle);
    }

private:
    void commit(const juce::String& paramId, const juce::String& text)
    {
        const auto parsed = white_box_lab::ComponentValueCodec::parse(text);
        if (! parsed)
            return;
        processor_.setMiddleParamDomain(paramId, static_cast<float>(*parsed));
        if (sync_ != nullptr)
            sync_->pullFromProcessor(processor_.getKbussEngine().getMiddleProcessor());
    }

    AudioEffectFrameworkProcessor& processor_;
    white_box_lab::SvgCircuitSync* sync_ = nullptr;
    std::vector<std::unique_ptr<atom::Label>> labels_;
    std::vector<std::unique_ptr<juce::TextEditor>> editors_;
    atom::Label empty_ { "circuitEmpty", {} };
};

SchematicEditorPanel::SchematicEditorPanel(AudioEffectFrameworkProcessor& processor,
                                           AtomLookAndFeel& lookAndFeel)
    : processor_(processor),
      atomLookAndFeel_(lookAndFeel),
      compositeLabel("compositeLabel", "Composite schematic"),
      stageLabel("stageLabel", "Stage schematic")
{
    setLookAndFeel(&atomLookAndFeel_);
    compositeLabel.setFont(AtomLookAndFeel::getUIFont(14.0f, juce::Font::bold));
    stageLabel.setFont(AtomLookAndFeel::getUIFont(14.0f, juce::Font::bold));
    addAndMakeVisible(compositeLabel);
    addAndMakeVisible(stageLabel);
    addAndMakeVisible(stageCombo);
    internalToggle.setClickingTogglesState(true);
    internalToggle.setVisible(false);
    internalToggle.onClick = [this]
    {
        showInternalSvg_ = internalToggle.getToggleState();
        loadSelectedStage();
    };
    addChildComponent(internalToggle);
    signalOutputActiveLabel.setVisible(false);
    signalOutputOtherLabel.setVisible(false);
    addChildComponent(signalOutputActiveLabel);
    addChildComponent(signalOutputOtherLabel);

    compositeView = std::make_unique<SvgView>();
    stageView = std::make_unique<SvgView>();
    circuitList = std::make_unique<CircuitList>(processor_);
    addAndMakeVisible(*compositeView);
    addAndMakeVisible(*stageView);
    addAndMakeVisible(*circuitList);

    stageCombo.onChange = [this]
    {
        showInternalSvg_ = false;
        internalToggle.setToggleState(false, juce::dontSendNotification);
        loadSelectedStage();
    };
    startTimerHz(4);
}

SchematicEditorPanel::~SchematicEditorPanel()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void SchematicEditorPanel::setCompositeKey(const juce::String& key)
{
    if (compositeKey_ == key)
        return;
    compositeKey_ = key;
    reloadComposite();
}

void SchematicEditorPanel::paint(juce::Graphics& g)
{
    g.fillAll(findColour(juce::ResizableWindow::backgroundColourId));
}

void SchematicEditorPanel::visibilityChanged()
{
    if (isVisible())
        reloadComposite();
}

void SchematicEditorPanel::timerCallback()
{
    const int generation = processor_.middleProcessorGeneration();
    if (generation != lastMiddleGeneration_)
        reloadComposite();
    else if (isVisible())
        sync_.pullFromProcessor(processor_.getKbussEngine().getMiddleProcessor());
}

void SchematicEditorPanel::rebuildStageChoices()
{
    stageCombo.clear(juce::dontSendNotification);
    const auto chain = white_box_lab::MudspAssetResolver::get().signalChain(compositeKey_);
    int id = 1;
    for (const auto& stage : chain)
        stageCombo.addItem(stage.topologyId + " — " + stage.label, id++);
    if (stageCombo.getNumItems() > 0)
        stageCombo.setSelectedItemIndex(0, juce::dontSendNotification);
}

void SchematicEditorPanel::reloadComposite()
{
    auto& assets = white_box_lab::MudspAssetResolver::get();
    compositeView->setSvgFile(assets.resolveCompositeSvg(compositeKey_));
    rebuildStageChoices();
    loadSelectedStage();
    lastMiddleGeneration_ = processor_.middleProcessorGeneration();
}

void SchematicEditorPanel::updateInternalToggleVisibility()
{
    auto& assets = white_box_lab::MudspAssetResolver::get();
    const auto chain = assets.signalChain(compositeKey_);
    const int index = stageCombo.getSelectedItemIndex();
    const bool visible = juce::isPositiveAndBelow(index, (int) chain.size())
                         && assets.operatorHasInternal(chain[(size_t) index].operatorKey);
    internalToggle.setVisible(visible);
    if (! visible)
    {
        showInternalSvg_ = false;
        internalToggle.setToggleState(false, juce::dontSendNotification);
    }
}

void SchematicEditorPanel::loadSelectedStage()
{
    auto& assets = white_box_lab::MudspAssetResolver::get();
    const auto chain = assets.signalChain(compositeKey_);
    const int index = stageCombo.getSelectedItemIndex();
    updateInternalToggleVisibility();
    if (! juce::isPositiveAndBelow(index, (int) chain.size()))
    {
        stageView->setSvgFile({});
        circuitList->setBindings(sync_);
        return;
    }

    const auto& stage = chain[(size_t) index];
    const auto displaySvg = assets.resolveOperatorSvg(stage.operatorKey, showInternalSvg_);
    const auto bindingSvg = assets.resolveOperatorSvg(stage.operatorKey, false);
    signalOutputActiveLabel.setVisible(false);
    signalOutputOtherLabel.setVisible(false);
    if (showInternalSvg_)
    {
        const auto signalOutput = assets.resolveStageSignalOutput(compositeKey_, stage.topologyId);
        if (signalOutput.activeLine.isNotEmpty())
        {
            signalOutputActiveLabel.setVisible(true);
            signalOutputActiveLabel.setText(signalOutput.activeLine, juce::dontSendNotification);
        }
        if (signalOutput.otherLine.isNotEmpty())
        {
            signalOutputOtherLabel.setVisible(true);
            signalOutputOtherLabel.setText(signalOutput.otherLine, juce::dontSendNotification);
        }
    }
    stageView->setSvgFile(displaySvg);
    sync_.loadModule(processor_.getKbussEngine().getMiddleProcessor(), stage.topologyId, bindingSvg,
                     assets.operatorCgJsonPath(stage.operatorKey));
    circuitList->setBindings(sync_);
}

void SchematicEditorPanel::resized()
{
    auto bounds = getLocalBounds().reduced(12, 8);
    auto top = bounds.removeFromTop(bounds.getHeight() / 2);
    compositeLabel.setBounds(top.removeFromTop(22));
    compositeView->setBounds(top);

    bounds.removeFromTop(8);
    auto header = bounds.removeFromTop(28);
    stageLabel.setBounds(header.removeFromLeft(140));
    stageCombo.setBounds(header.removeFromLeft(260));
    if (internalToggle.isVisible())
    {
        header.removeFromLeft(8);
        internalToggle.setBounds(header.removeFromLeft(96).withHeight(28));
    }
    if (signalOutputActiveLabel.isVisible())
    {
        bounds.removeFromTop(4);
        signalOutputActiveLabel.setBounds(bounds.removeFromTop(18));
    }
    if (signalOutputOtherLabel.isVisible())
    {
        bounds.removeFromTop(2);
        signalOutputOtherLabel.setBounds(bounds.removeFromTop(16));
    }

    auto bottom = bounds;
    stageView->setBounds(bottom.removeFromLeft(juce::jmax(220, bottom.getWidth() / 2)));
    bottom.removeFromLeft(8);
    circuitList->setBounds(bottom);
}
