#include "PluginEditor.h"

#include "ChorusNuDspDefaults.h"

#include <cmath>

namespace
{
constexpr int kSliderLabelColumnWidth = 140;
constexpr int kSliderLabelGap = 8;

class LabeledSliderRow final : public juce::Component
{
public:
    LabeledSliderRow(const juce::String& title, atom::Slider& sliderToEmbed)
        : label("sliderRowLabel", title),
          slider(sliderToEmbed)
    {
        label.setJustificationType(juce::Justification::centredLeft);
        label.setFont(AtomLookAndFeel::getUIFont(AtomLookAndFeel::getSystemUIFontHeight(), juce::Font::plain));
        label.setMinimumHorizontalScale(1.0f);
        label.setAutoResizeEnabled(false);
        label.setInterceptsMouseClicks(false, false);
        addAndMakeVisible(label);
        addAndMakeVisible(slider);
    }

    void resized() override
    {
        auto area = getLocalBounds();
        label.setBounds(area.removeFromLeft(kSliderLabelColumnWidth));
        area.removeFromLeft(kSliderLabelGap);
        slider.setBounds(area);
    }

private:
    atom::Label label;
    atom::Slider& slider;
};

class PluginSelectorRow final : public juce::Component
{
public:
    PluginSelectorRow(const juce::String& title, atom::ComboBox& combo, int rowHeight)
        : label("pluginSelectorLabel", title),
          comboBox(combo),
          rowHeight_(rowHeight)
    {
        card.setMinPanelHeight(rowHeight);
        label.setJustificationType(juce::Justification::centredLeft);
        label.setFont(AtomLookAndFeel::getUIFont(AtomLookAndFeel::getSystemUIFontHeight(), juce::Font::plain));
        label.setMinimumHorizontalScale(1.0f);
        label.setAutoResizeEnabled(false);

        comboBox.setEditableText(false);
        comboBox.onChange = [this] { resized(); };

        card.addAndMakeVisible(label);
        card.addAndMakeVisible(comboBox);
        addAndMakeVisible(card);
    }

    void resized() override
    {
        card.setBounds(getLocalBounds());
        auto area = card.getLocalBounds().reduced(12, 8);
        const float rowH = (float) area.getHeight();

        comboBox.setSize(comboBox.getIdealWidth(), juce::roundToInt(rowH));

        const auto& font = label.getFont();
        const int labelTextWidth =
            juce::roundToInt(font.getStringWidthFloat(label.getText()) + 4.0f);

        juce::FlexBox flex;
        flex.flexDirection = juce::FlexBox::Direction::row;
        flex.alignItems = juce::FlexBox::AlignItems::center;

        juce::FlexItem labelItem((float) labelTextWidth, rowH, label);
        labelItem.flexShrink = 0.0f;

        juce::FlexItem spacer(0.0f, rowH);
        spacer.flexGrow = 1.0f;

        juce::FlexItem comboItem((float) comboBox.getWidth(), rowH, comboBox);
        comboItem.flexShrink = 0.0f;

        flex.items.addArray({ labelItem, spacer, comboItem });
        flex.performLayout(area);
    }

private:
    atom::SettingsCard card;
    atom::Label label;
    atom::ComboBox& comboBox;
    int rowHeight_;
};

atom::graph::Graph makeChorusGraph()
{
    atom::graph::Graph graph;
    graph.displayName = "Chorus";

    atom::graph::Node lfo;
    lfo.id = "lfo";
    lfo.label = "LFO";
    lfo.kind = "lfo";
    lfo.selectable = false;
    lfo.bypassable = false;

    atom::graph::Node delay;
    delay.id = "delay";
    delay.label = "Delay Line";
    delay.kind = "delay_line";
    delay.selectable = false;
    delay.bypassable = false;

    graph.nodes.push_back(std::move(lfo));
    graph.nodes.push_back(std::move(delay));
    graph.edges.push_back({ "lfo", "delay", "signal" });
    return graph;
}

atom::graph::Graph makePhase90Graph()
{
    atom::graph::Graph graph;
    graph.displayName = "Phase90";

    atom::graph::Node lfo;
    lfo.id = "lfo";
    lfo.label = "LFO";
    lfo.kind = "lfo";
    lfo.selectable = false;
    lfo.bypassable = false;

    graph.nodes.push_back(std::move(lfo));
    return graph;
}
}  // namespace

ChorusAudioProcessorEditor::ChorusAudioProcessorEditor(ChorusAudioProcessor& p)
    : AudioEffectFrameworkEditor(p, true),
      chorusProcessor(p),
      modelCombo("modelCombo")
{
    modelCombo.addItem("Chorus", 1);
    modelCombo.addItem("Stereo Phase90", 2);
    modelCombo.addItem("Mono Phase90", 3);
    modelCombo.setTooltip("Select Chorus, Stereo Phase90, or Mono Phase90");
    modelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        chorusProcessor.parameters.valueTreeState, ChorusAudioProcessor::kChorusModelParamId, modelCombo);

    modelSelectorRow = std::make_unique<PluginSelectorRow>("Effect", modelCombo, cardRowHeight);
    bodyContent.addAndMakeVisible(*modelSelectorRow);
    bodyComponents.add(modelSelectorRow.get());

    configureUserSlider(rateSlider, " Hz");
    configureUserSlider(depthSlider, " ms");
    configureUserSlider(mixSlider, "");
    mixSlider.setRange(0.0, 1.0, 0.01);
    mixSlider.setTooltip("Dry/wet mix (0–1). Wet = Mix × Effect Max.");
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        chorusProcessor.parameters.valueTreeState, chorusProcessor.paramMix.paramID, mixSlider);

    rateSlider.onValueChange = [this]
    {
        chorusProcessor.setMiddleParamDomain(chorusProcessor.viewParamId("rate"),
                                             static_cast<float>(rateSlider.getValue()));
    };
    depthSlider.onValueChange = [this]
    {
        chorusProcessor.setMiddleParamDomain(chorusProcessor.viewParamId("amount"),
                                             static_cast<float>(depthSlider.getValue()));
    };

    rateRow = std::make_unique<LabeledSliderRow>("Rate", rateSlider);
    depthRow = std::make_unique<LabeledSliderRow>("Depth", depthSlider);
    mixRow = std::make_unique<LabeledSliderRow>("Mix", mixSlider);
    bodyContent.addAndMakeVisible(*rateRow);
    bodyContent.addAndMakeVisible(*depthRow);
    bodyContent.addAndMakeVisible(*mixRow);
    bodyComponents.add(rateRow.get());
    bodyComponents.add(depthRow.get());
    bodyComponents.add(mixRow.get());

    const auto limits = chorusProcessor.getChorusNuDspLimits();
    delayModView.setDelayAxisLimits(limits.rateMin, limits.delayMax, limits.amountMax, limits.delayMin);
    delayModView.onBack = [this] { setSignalFlowVisible(false); };
    delayModView.onGeometryChanged = [this]
    {
        if (delayModView.isVisible())
            applyZoom(zoomFactor);
    };
    delayModView.onShapeChanged = [this](bool triangle)
    {
        chorusProcessor.setMiddleParamDomain("lfo_type", triangle ? 1.0f : 0.0f);
    };
    delayModView.onRateLimitsChanged = [this](float minHz, float maxHz)
    {
        auto writeIfChanged = [](juce::RangedAudioParameter* param, float domain)
        {
            if (param == nullptr)
                return;
            const float normalized = param->convertTo0to1(domain);
            if (std::abs(param->getValue() - normalized) > 1.0e-5f)
                param->setValueNotifyingHost(normalized);
        };
        writeIfChanged(chorusProcessor.parameters.valueTreeState.getParameter(
                           chorusProcessor.paramRateMin.paramID),
                       minHz);
        writeIfChanged(chorusProcessor.parameters.valueTreeState.getParameter(
                           chorusProcessor.paramRateMax.paramID),
                       maxHz);
        applyRateSliderRange();
        const float rate = static_cast<float>(rateSlider.getValue());
        const float clamped = juce::jlimit(minHz, maxHz, rate);
        if (std::abs(clamped - rate) > 1.0e-4f)
        {
            rateSlider.setValue(clamped, juce::dontSendNotification);
            chorusProcessor.setMiddleParamDomain(chorusProcessor.viewParamId("rate"), clamped);
        }
    };
    delayModView.onDelayChanged = [this](float delayMs)
    {
        chorusProcessor.setMiddleParamDomain(chorusProcessor.viewParamId("delay"), delayMs);
    };
    delayModView.onAmountChanged = [this](float amountMs)
    {
        chorusProcessor.setMiddleParamDomain(chorusProcessor.viewParamId("amount"), amountMs);
        if (!depthSlider.isMouseButtonDown())
            depthSlider.setValue(amountMs, juce::dontSendNotification);
    };
    delayModView.onEffectMaxChanged = [this](float effectMax)
    {
        if (auto* param = chorusProcessor.parameters.valueTreeState.getParameter(
                chorusProcessor.paramEffectMax.paramID))
            param->setValueNotifyingHost(param->convertTo0to1(effectMax));
    };
    addChildComponent(delayModView);
    delayModView.setVisible(false);

    completeBodyConstruction();
    syncTapTempo();
    applyRateSliderRange();
    refreshUserControls();
    refreshDelayModView();
    updateChorusRowVisibility();
    recalculateBodyContentHeight();
    applyZoom(1.0f);
}

ChorusAudioProcessorEditor::~ChorusAudioProcessorEditor() = default;

void ChorusAudioProcessorEditor::configureUserSlider(atom::Slider& slider, const juce::String& suffix)
{
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setTextValueSuffix(suffix);
    slider.setSliderSnapsToMousePosition(false);
    slider.setValueLabelPos(atom::Slider::ValueLabelPos::Right);

    atom::SliderStyleOverride styleOverride;
    styleOverride.metrics.linearHorizontalValueLabelReserveDlu = 72.0f;
    atomLookAndFeel.setSliderStyleOverride(slider, styleOverride);
}

void ChorusAudioProcessorEditor::applyRateSliderRange()
{
    const auto limits = chorusProcessor.getChorusNuDspLimits();
    const float rateMin = chorusProcessor.hostParamValue(chorusProcessor.paramRateMin.paramID, limits.rateMin);
    const float rateMax = chorusProcessor.hostParamValue(chorusProcessor.paramRateMax.paramID, limits.rateMax);
    const float lo = juce::jmin(rateMin, rateMax);
    const float hi = juce::jmax(lo + 0.01f, juce::jmax(rateMin, rateMax));
    if (std::abs(rateSlider.getMinimum() - (double) lo) < 1.0e-4
        && std::abs(rateSlider.getMaximum() - (double) hi) < 1.0e-4)
        return;
    rateSlider.setRange(lo, hi, 0.01);
}

int ChorusAudioProcessorEditor::getBodyComponentBaseHeight(const juce::Component* component) const noexcept
{
    if (modelSelectorRow != nullptr && component == modelSelectorRow.get())
        return cardRowHeight;

    if ((rateRow != nullptr && component == rateRow.get())
        || (depthRow != nullptr && component == depthRow.get())
        || (mixRow != nullptr && component == mixRow.get()))
        return sliderRowHeight;

    return AudioEffectFrameworkEditor::getBodyComponentBaseHeight(component);
}

void ChorusAudioProcessorEditor::onEditorTimerTick()
{
    if (!delayModView.isInteracting())
        refreshUserControls();
    if (delayModView.isVisible() && !delayModView.isInteracting())
        refreshDelayModView();
    updateChorusRowVisibility();
}

juce::Component* ChorusAudioProcessorEditor::getInsidePage()
{
    return &delayModView;
}

const juce::Component* ChorusAudioProcessorEditor::getInsidePage() const
{
    return &delayModView;
}

int ChorusAudioProcessorEditor::getInsidePreferredWidth() const
{
    return delayModView.getPreferredWidth();
}

int ChorusAudioProcessorEditor::getInsidePreferredHeight() const
{
    return delayModView.getPreferredHeight();
}

void ChorusAudioProcessorEditor::onInsidePageShown()
{
    refreshDelayModView();
}

void ChorusAudioProcessorEditor::syncTapTempo()
{
    headerBar.getTapTempo().onBPMChanged = [this](double bpm)
    {
        const auto limits = chorusProcessor.getChorusNuDspLimits();
        const float minHz = chorusProcessor.hostParamValue(chorusProcessor.paramRateMin.paramID, limits.rateMin);
        const float maxHz = chorusProcessor.hostParamValue(chorusProcessor.paramRateMax.paramID, limits.rateMax);
        const float rateHz = juce::jlimit(juce::jmin(minHz, maxHz), juce::jmax(minHz, maxHz),
                                          (float) (bpm / 60.0));
        chorusProcessor.setMiddleParamDomain(chorusProcessor.viewParamId("rate"), rateHz);
        rateSlider.setValue(rateHz, juce::dontSendNotification);
    };
}

void ChorusAudioProcessorEditor::updateChorusRowVisibility()
{
    const bool chorus = !chorusProcessor.isPhase90();
    const bool changed = (depthRow != nullptr && depthRow->isVisible() != chorus)
                         || (mixRow != nullptr && mixRow->isVisible() != chorus);
    if (depthRow != nullptr)
        depthRow->setVisible(chorus);
    if (mixRow != nullptr)
        mixRow->setVisible(chorus);
    if (changed)
    {
        recalculateBodyContentHeight();
        applyZoom(zoomFactor);
    }
}

void ChorusAudioProcessorEditor::refreshUserControls()
{
    applyRateSliderRange();
    const auto limits = chorusProcessor.getChorusNuDspLimits();
    depthSlider.setRange(limits.amountMin, limits.amountMax, 0.1);

    const float rate =
        chorusProcessor.getMiddleParamDomain(chorusProcessor.viewParamId("rate"), limits.rateDefault);
    if (!rateSlider.isMouseButtonDown())
        rateSlider.setValue(juce::jlimit(static_cast<float>(rateSlider.getMinimum()),
                                         static_cast<float>(rateSlider.getMaximum()), rate),
                            juce::dontSendNotification);

    if (chorusProcessor.isPhase90())
        return;

    const float amount =
        chorusProcessor.getMiddleParamDomain(chorusProcessor.viewParamId("amount"), limits.amountDefault);
    if (!depthSlider.isMouseButtonDown())
        depthSlider.setValue(amount, juce::dontSendNotification);
}

void ChorusAudioProcessorEditor::refreshDelayModView()
{
    const int modelIndex = chorusProcessor.currentModelIndex();
    const int generation = chorusProcessor.middleProcessorGeneration();
    const bool phase90 = chorusProcessor.isPhase90();

    if (modelIndex != lastModelIndex_ || generation != lastMiddleGeneration_)
    {
        lastModelIndex_ = modelIndex;
        lastMiddleGeneration_ = generation;
        auto graph = chorusProcessor.getEffectSignalFlowGraph();
        if (graph.empty())
            graph = phase90 ? makePhase90Graph() : makeChorusGraph();
        delayModView.setGraph(std::move(graph));
        delayModView.setDelayCurveVisible(!phase90);
    }

    const auto limits = chorusProcessor.getChorusNuDspLimits();
    delayModView.setRateLimits(
        chorusProcessor.hostParamValue(chorusProcessor.paramRateMin.paramID, limits.rateMin),
        chorusProcessor.hostParamValue(chorusProcessor.paramRateMax.paramID, limits.rateMax));
    delayModView.setEffectMax(
        chorusProcessor.hostParamValue(chorusProcessor.paramEffectMax.paramID,
                                       chorusProcessor.paramEffectMax.defaultValue));

    const float rate =
        chorusProcessor.getMiddleParamDomain(chorusProcessor.viewParamId("rate"), limits.rateDefault);
    const float delay = phase90 ? limits.delayDefault
                                : chorusProcessor.getMiddleParamDomain(chorusProcessor.viewParamId("delay"),
                                                                        limits.delayDefault);
    const float amount = phase90 ? 0.0f
                                 : chorusProcessor.getMiddleParamDomain(chorusProcessor.viewParamId("amount"),
                                                                         limits.amountDefault);
    const bool triangle = chorusProcessor.getMiddleParamDomain("lfo_type", 0.0f) >= 0.5f;
    delayModView.setDelayParameters(rate, delay, amount, triangle);
}
