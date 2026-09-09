#include "MainComponent.h"

namespace
{
    juce::Colour makeBackgroundColour(atom::ThemeType themeType)
    {
        return themeType == atom::ThemeType::Dark ? juce::Colour(0xFF101217) : juce::Colour(0xFFF4F5F8);
    }

    juce::Colour makePanelColour(atom::ThemeType themeType)
    {
        return themeType == atom::ThemeType::Dark ? juce::Colour(0xFF1A1E27) : juce::Colour(0xFFFFFFFF);
    }

    juce::Colour makeBorderColour(atom::ThemeType themeType)
    {
        return themeType == atom::ThemeType::Dark ? juce::Colour(0xFF2B3240) : juce::Colour(0xFFD7DCE4);
    }

    struct CircuitMenuEntry
    {
        const char *label;
        ds1_ac::CircuitKind kind;
    };

    constexpr CircuitMenuEntry kCircuitMenuEntries[] = {
        {"input — Input BJT", ds1_ac::CircuitKind::BjtFollower},
        {"emitter — BJT Emitter", ds1_ac::CircuitKind::BjtCommonEmitter},
        {"opamp — Op Amp", ds1_ac::CircuitKind::Ds1Opamp},
        {"clipper — Clipper", ds1_ac::CircuitKind::Ds1Clipper},
        {"tone — Tone", ds1_ac::CircuitKind::Ds1Tone},
        {"level — Level", ds1_ac::CircuitKind::RcLevel},
        {"output — Output BJT", ds1_ac::CircuitKind::BjtFollowerOut},
    };

    constexpr int kKnobColumnWidth = 72;
    constexpr int kKnobSize = 56;
} // namespace

MainComponent::MainComponent()
{
    setLookAndFeel(&atomLookAndFeel);

    titleLabel.setText("Boss DS-1 White Box", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(28.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centredLeft);

    subtitleLabel.setText("input → emitter → opamp → clipper → tone → level → output", juce::dontSendNotification);
    subtitleLabel.setJustificationType(juce::Justification::centredLeft);
    subtitleLabel.setInterceptsMouseClicks(false, false);

    addAndMakeVisible(titleLabel);
    addAndMakeVisible(subtitleLabel);
    addAndMakeVisible(themeButton);
    addAndMakeVisible(circuitBox);
    addAndMakeVisible(plotKindBox);
    addAndMakeVisible(opampModelBox);
    addAndMakeVisible(sampleRateBox);
    addAndMakeVisible(taperBox);

    configureCombo(circuitBox);
    configureCombo(plotKindBox);
    configureCombo(opampModelBox);
    configureCombo(sampleRateBox);
    configureCombo(taperBox);

    circuitLabel.setText("Stage", juce::dontSendNotification);
    circuitLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(circuitLabel);

    for (size_t i = 0; i < std::size(kCircuitMenuEntries); ++i)
        circuitBox.addItem(kCircuitMenuEntries[i].label, static_cast<int>(i + 1));

    circuitBox.setSelectedId(1, juce::dontSendNotification);
    circuitBox.onChange = [this]()
    {
        const auto circuit = getCircuitFromSelection();
        syncPotTaperToCircuitDefault(circuit);
        syncDeviceModelCombo(circuit);
        syncInjectDefaultsToCircuit(circuit);
        updatePlotView();
    };

    taperLabel.setText("Taper", juce::dontSendNotification);
    taperLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(taperLabel);

    taperBox.addItem("Linear", 1);
    taperBox.addItem("Multiplicative", 2);
    taperBox.addItem("A15", 3);
    taperBox.addItem("A30", 4);
    taperBox.addItem("A45", 5);
    taperBox.addItem("G (4B)", 6);
    taperBox.addItem("C", 7);
    taperBox.addItem("3B", 8);
    taperBox.setSelectedId(ds1_ac::potTaperComboId(ds1_ac::defaultPotTaper(ds1_ac::CircuitKind::BjtFollower)),
                           juce::dontSendNotification);
    taperBox.onChange = [this]()
    { updatePlotView(); };

    sampleRateLabel.setText("Fs", juce::dontSendNotification);
    sampleRateLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(sampleRateLabel);

    sampleRateBox.addItem("48 kHz", 1);
    sampleRateBox.addItem("96 kHz", 2);
    sampleRateBox.addItem("192 kHz", 3);
    sampleRateBox.addItem("384 kHz", 4);
    sampleRateBox.setSelectedId(2, juce::dontSendNotification);
    sampleRateBox.onChange = [this]()
    { updatePlotView(); };

    gainLabel.setText("Gain", juce::dontSendNotification);
    gainLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(gainLabel);

    configureKnob(gainKnob);
    gainKnob.setRange(0.0, 1.0, 0.01);
    gainKnob.setValue(0.5, juce::dontSendNotification);
    gainKnob.onValueChange = [this]()
    { updatePlotView(); };
    addAndMakeVisible(gainKnob);

    secondaryLabel.setText("Treble", juce::dontSendNotification);
    secondaryLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(secondaryLabel);

    configureKnob(secondaryKnob);
    secondaryKnob.setRange(0.0, 1.0, 0.01);
    secondaryKnob.setValue(0.5, juce::dontSendNotification);
    secondaryKnob.onValueChange = [this]()
    { updatePlotView(); };
    addAndMakeVisible(secondaryKnob);

    tertiaryLabel.setText("Treble", juce::dontSendNotification);
    tertiaryLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(tertiaryLabel);

    configureKnob(tertiaryKnob);
    tertiaryKnob.setRange(0.0, 1.0, 0.01);
    tertiaryKnob.setValue(0.5, juce::dontSendNotification);
    tertiaryKnob.onValueChange = [this]()
    { updatePlotView(); };
    addAndMakeVisible(tertiaryKnob);

    injectFreqLabel.setText("Inject f", juce::dontSendNotification);
    injectFreqLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(injectFreqLabel);

    configureInjectBox(injectFreqBox);
    injectFreqBox.setRange(ds1_ac::kPreviewFreqMinHz, ds1_ac::kPreviewFreqMaxHz, 0.0);
    injectFreqBox.setSkewFactorFromMidPoint(1000.0);
    injectFreqBox.setValue(ds1_ac::kDefaultPreviewFreqHz, juce::dontSendNotification);
    injectFreqBox.setTextValueSuffix(" Hz");
    injectFreqBox.onValueChange = [this]()
    { updatePlotView(); };
    addAndMakeVisible(injectFreqBox);

    injectAmpLabel.setText("Inject A", juce::dontSendNotification);
    injectAmpLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(injectAmpLabel);

    configureInjectBox(injectAmpBox);
    injectAmpBox.setRange(ds1_ac::kPreviewAmpMin, ds1_ac::kPreviewAmpMax, 0.0);
    injectAmpBox.setSkewFactorFromMidPoint(0.01);
    injectAmpBox.setValue(ds1_ac::defaultPreviewAmplitude(ds1_ac::CircuitKind::BjtFollower),
                           juce::dontSendNotification);
    injectAmpBox.onValueChange = [this]()
    { updatePlotView(); };
    addAndMakeVisible(injectAmpBox);

    plotKindBox.addItem("Magnitude", 1);
    plotKindBox.addItem("Phase", 2);
    plotKindBox.addItem("Magnitude + Phase", 3);
    plotKindBox.setSelectedId(3, juce::dontSendNotification);
    plotKindBox.onChange = [this]()
    { updatePlotView(); };

    ds1_ac::populateOpampModelCombo(opampModelBox);
    opampModelBox.setSelectedId(ds1_ac::opampModelComboId(NX_OPAMP_BA728), juce::dontSendNotification);
    opampModelBox.onChange = [this]()
    { updatePlotView(); };

    schematicPanel = std::make_unique<CircuitSchematicPanel>();
    schematicPanel->onOpampModelChanged = [this](nx_opamp_model_e model)
    {
        opampModelBox.setSelectedId(ds1_ac::opampModelComboId(model), juce::dontSendNotification);
        if (acPanel != nullptr)
            acPanel->setOpampModel(model);
    };
    schematicPanel->onDiodeModelChanged = [this](nx_diode_model_t model)
    {
        opampModelBox.setSelectedId(ds1_ac::diodeModelComboId(model), juce::dontSendNotification);
        if (acPanel != nullptr)
            acPanel->setDiodeModel(model);
    };
    schematicPanel->onBjtModelChanged = [this](nx_bjt_npn_model_e model)
    {
        if (acPanel != nullptr)
            acPanel->setBjtModel(model);
    };
    schematicPanel->onJfetModelChanged = [this](nx_jfet_n_model_e model)
    {
        if (acPanel != nullptr)
            acPanel->setJfetModel(model);
    };
    schematicPanel->onComponentValuesChanged = [this](const ds1_ac::SchematicComponentValues& values)
    {
        if (acPanel != nullptr)
            acPanel->setSchematicComponentValues(values);
    };
    addAndMakeVisible(*schematicPanel);

    addAndMakeVisible(schematicSplitter);
    schematicSplitter.onDragDelta = [this](int deltaX)
    {
        const int maxWidth = getWidth() - 68 - kAcPanelMinWidth - kSchematicSplitterWidth - kPanelGap;
        schematicPanelWidth_ =
            juce::jlimit(kSchematicMinWidth, juce::jmax(kSchematicMinWidth, maxWidth), schematicPanelWidth_ + deltaX);
        resized();
    };

    acPanel = std::make_unique<Ds1OpampAcPanel>();
    addAndMakeVisible(*acPanel);

    themeButton.onClick = [this]()
    {
        atomLookAndFeel.setTheme(atomLookAndFeel.getTheme() == atom::ThemeType::Dark ? atom::ThemeType::Light
                                                                                     : atom::ThemeType::Dark);
        applyTheme();
    };

    applyTheme();
    syncDeviceModelCombo(getCircuitFromSelection());
    updatePlotView();
    setSize(1480, 880);
}

MainComponent::~MainComponent()
{
    setLookAndFeel(nullptr);
}

void MainComponent::configureCombo(atom::ComboBox &combo)
{
    combo.setJustificationType(juce::Justification::centredLeft);
}

void MainComponent::configureKnob(atom::Slider &knob)
{
    knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    knob.setValueLabelPos(atom::Slider::ValueLabelPos::Below);
}

void MainComponent::configureInjectBox(atom::Slider &box)
{
    box.setSliderStyle(juce::Slider::LinearHorizontal);
    box.setTextBoxStyle(juce::Slider::TextBoxRight, false, 72, 24);
    box.setScrollWheelEnabled(false);
}

void MainComponent::syncInjectDefaultsToCircuit(ds1_ac::CircuitKind circuit)
{
    injectAmpBox.setValue(ds1_ac::defaultPreviewAmplitude(circuit), juce::dontSendNotification);
}

void MainComponent::layoutKnobColumn(juce::Rectangle<int> &area, atom::Label &label, atom::Slider &knob) const
{
    auto column = area.removeFromLeft(kKnobColumnWidth);
    label.setBounds(column.removeFromTop(18));
    column.removeFromTop(2);
    knob.setBounds(column.withSizeKeepingCentre(kKnobSize, kKnobSize));
    area.removeFromLeft(6);
}

void MainComponent::applyTheme()
{
    const auto themeType = atomLookAndFeel.getTheme();
    const auto &themeColors =
        themeType == atom::ThemeType::Dark ? atom::Theme::getDarkTheme() : atom::Theme::getLightTheme();

    titleLabel.refreshTheme();
    subtitleLabel.refreshTheme();
    gainLabel.refreshTheme();
    secondaryLabel.refreshTheme();
    tertiaryLabel.refreshTheme();
    circuitLabel.refreshTheme();
    injectFreqLabel.refreshTheme();
    injectAmpLabel.refreshTheme();
    sampleRateLabel.refreshTheme();
    taperLabel.refreshTheme();

    if (schematicPanel != nullptr)
        schematicPanel->applyTheme(themeColors);

    if (acPanel != nullptr)
        acPanel->applyTheme(themeColors);

    themeButton.setButtonText(themeType == atom::ThemeType::Dark ? "Switch to Light" : "Switch to Dark");
    repaint();
}

void MainComponent::updatePlotView()
{
    if (acPanel == nullptr)
        return;

    const auto circuit = getCircuitFromSelection();
    const bool usesOpamp = ds1_ac::circuitUsesOpampModel(circuit);
    const bool usesDiode = ds1_ac::circuitUsesDiodeModel(circuit);
    const bool usesBjt = ds1_ac::circuitUsesBjtModel(circuit);
    const bool usesJfet = ds1_ac::circuitUsesJfetModel(circuit);
    const bool hasPrimary = ds1_ac::circuitHasPrimaryControl(circuit);
    const bool usesPotTaper = ds1_ac::circuitUsesPotTaper(circuit);
    const bool hasSecondary = ds1_ac::circuitHasSecondaryControl(circuit);
    const bool hasTertiary = ds1_ac::circuitHasTertiaryControl(circuit);

    if (schematicPanel != nullptr)
        schematicPanel->setCircuitKind(circuit);

    acPanel->setCircuitKind(circuit);
    if (usesOpamp && schematicPanel != nullptr)
        acPanel->setOpampModel(schematicPanel->getOpampModel());
    else if (usesOpamp)
        acPanel->setOpampModel(getOpampModelFromSelection());
    if (usesDiode && schematicPanel != nullptr)
        acPanel->setDiodeModel(schematicPanel->getDiodeModel());
    if (usesBjt && schematicPanel != nullptr)
        acPanel->setBjtModel(schematicPanel->getBjtModel());
    else if (usesBjt)
        acPanel->setBjtModel(getBjtModelFromSelection());
    if (usesJfet && schematicPanel != nullptr)
        acPanel->setJfetModel(schematicPanel->getJfetModel());
    else if (usesJfet)
        acPanel->setJfetModel(getJfetModelFromSelection());
    acPanel->setGainControl(gainKnob.getValue());
    acPanel->setSecondaryControl(secondaryKnob.getValue());
    acPanel->setTertiaryControl(tertiaryKnob.getValue());
    acPanel->setPotTaper(getPotTaperFromSelection());
    acPanel->setPreviewFrequencyHz(injectFreqBox.getValue());
    acPanel->setPreviewAmplitude(injectAmpBox.getValue());
    acPanel->setSampleRateHz(getSampleRateFromSelection());
    acPanel->setPlotKind(getPlotKindFromSelection());

    gainLabel.setText(ds1_ac::controlParameterName(circuit), juce::dontSendNotification);
    secondaryLabel.setText(ds1_ac::secondaryControlParameterName(circuit), juce::dontSendNotification);
    tertiaryLabel.setText(ds1_ac::tertiaryControlParameterName(circuit), juce::dontSendNotification);
    subtitleLabel.setText("White-box stage " + ds1_ac::circuitStageMenuLabel(circuit)
                              + "  |  " + juce::String(ds1_ac::circuitOperatorKey(circuit))
                              + "  |  " + juce::String(ds1_ac::circuitProcessFunctionName(circuit)),
                          juce::dontSendNotification);

    opampModelBox.setVisible(false);
    opampModelBox.setEnabled(false);

    taperLabel.setVisible(usesPotTaper);
    taperLabel.setEnabled(usesPotTaper);
    taperBox.setVisible(usesPotTaper);
    taperBox.setEnabled(usesPotTaper);

    gainLabel.setVisible(hasPrimary);
    gainLabel.setEnabled(hasPrimary);
    gainKnob.setVisible(hasPrimary);
    gainKnob.setEnabled(hasPrimary);

    secondaryLabel.setVisible(hasSecondary);
    secondaryLabel.setEnabled(hasSecondary);
    secondaryKnob.setVisible(hasSecondary);
    secondaryKnob.setEnabled(hasSecondary);

    tertiaryLabel.setVisible(hasTertiary);
    tertiaryLabel.setEnabled(hasTertiary);
    tertiaryKnob.setVisible(hasTertiary);
    tertiaryKnob.setEnabled(hasTertiary);

    const bool showInjectControls = getPlotKindFromSelection() != ds1_ac::PlotKind::Magnitude;
    injectFreqLabel.setVisible(showInjectControls);
    injectFreqLabel.setEnabled(showInjectControls);
    injectFreqBox.setVisible(showInjectControls);
    injectFreqBox.setEnabled(showInjectControls);
    injectAmpLabel.setVisible(showInjectControls);
    injectAmpLabel.setEnabled(showInjectControls);
    injectAmpBox.setVisible(showInjectControls);
    injectAmpBox.setEnabled(showInjectControls);
    resized();
}

ds1_ac::CircuitKind MainComponent::getCircuitFromSelection() const
{
    const int selectedId = circuitBox.getSelectedId();
    if (selectedId >= 1 && selectedId <= static_cast<int>(std::size(kCircuitMenuEntries)))
        return kCircuitMenuEntries[static_cast<size_t>(selectedId - 1)].kind;

    return ds1_ac::CircuitKind::BjtFollower;
}

nx_pot_taper_e MainComponent::getPotTaperFromSelection() const
{
    return ds1_ac::potTaperFromComboId(taperBox.getSelectedId());
}

void MainComponent::syncPotTaperToCircuitDefault(ds1_ac::CircuitKind circuit)
{
    taperBox.setSelectedId(ds1_ac::potTaperComboId(ds1_ac::defaultPotTaper(circuit)),
                           juce::dontSendNotification);
}

void MainComponent::syncDeviceModelCombo(ds1_ac::CircuitKind circuit)
{
    opampModelBox.clear(juce::dontSendNotification);

    if (ds1_ac::circuitUsesOpampModel(circuit))
    {
        ds1_ac::populateOpampModelCombo(opampModelBox);
        opampModelBox.setSelectedId(ds1_ac::opampModelComboId(NX_OPAMP_BA728), juce::dontSendNotification);
        return;
    }

    if (ds1_ac::circuitUsesDiodeModel(circuit))
    {
        ds1_ac::populateDiodeModelCombo(opampModelBox);
        opampModelBox.setSelectedId(ds1_ac::diodeModelComboId(ds1_ac::defaultDiodeModelForCircuit(circuit)),
                                    juce::dontSendNotification);
        return;
    }

    if (ds1_ac::circuitUsesBjtModel(circuit))
    {
        opampModelBox.addItem("Generic NPN", 1);
        opampModelBox.addItem("2N3904", 2);
        opampModelBox.addItem("2N2222", 3);
        opampModelBox.setSelectedId(ds1_ac::bjtModelComboId(ds1_ac::defaultBjtModel(circuit)),
                                    juce::dontSendNotification);
        return;
    }

    if (ds1_ac::circuitUsesJfetModel(circuit))
    {
        opampModelBox.addItem("Generic N-JFET", 1);
        opampModelBox.addItem("J201", 2);
        opampModelBox.addItem("2N5457", 3);
        opampModelBox.setSelectedId(ds1_ac::jfetModelComboId(ds1_ac::defaultJfetModel(circuit)),
                                    juce::dontSendNotification);
    }
}

nx_opamp_model_e MainComponent::getOpampModelFromSelection() const
{
    return ds1_ac::opampModelFromComboId(opampModelBox.getSelectedId());
}

nx_bjt_npn_model_e MainComponent::getBjtModelFromSelection() const
{
    return ds1_ac::bjtModelFromComboId(opampModelBox.getSelectedId());
}

nx_jfet_n_model_e MainComponent::getJfetModelFromSelection() const
{
    return ds1_ac::jfetModelFromComboId(opampModelBox.getSelectedId());
}

ds1_ac::PlotKind MainComponent::getPlotKindFromSelection() const
{
    switch (plotKindBox.getSelectedId())
    {
    case 1:
        return ds1_ac::PlotKind::Magnitude;
    case 2:
        return ds1_ac::PlotKind::Phase;
    default:
        return ds1_ac::PlotKind::Both;
    }
}

double MainComponent::getSampleRateFromSelection() const
{
    switch (sampleRateBox.getSelectedId())
    {
    case 1:
        return ds1_ac::kSampleRate48kHz;
    case 3:
        return ds1_ac::kSampleRate192kHz;
    case 4:
        return ds1_ac::kSampleRate384kHz;
    default:
        return ds1_ac::kSampleRate96kHz;
    }
}

void MainComponent::paint(juce::Graphics &g)
{
    const auto themeType = atomLookAndFeel.getTheme();
    g.fillAll(makeBackgroundColour(themeType));

    auto bounds = getLocalBounds().reduced(18);
    g.setColour(makePanelColour(themeType));
    g.fillRoundedRectangle(bounds.toFloat(), 20.0f);

    g.setColour(makeBorderColour(themeType));
    g.drawRoundedRectangle(bounds.toFloat(), 20.0f, 1.5f);
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced(34);

    auto headerArea = area.removeFromTop(74);
    titleLabel.setBounds(headerArea.removeFromTop(36));
    subtitleLabel.setBounds(headerArea.removeFromTop(26));

    area.removeFromTop(10);

    auto toolbar = area.removeFromTop(34);
    themeButton.setBounds(toolbar.removeFromRight(160));
    circuitLabel.setBounds(toolbar.removeFromLeft(48));
    circuitBox.setBounds(toolbar.removeFromLeft(196));
    toolbar.removeFromLeft(8);
    opampModelBox.setBounds(toolbar.removeFromLeft(110));
    toolbar.removeFromLeft(8);
    sampleRateLabel.setBounds(toolbar.removeFromLeft(24));
    sampleRateBox.setBounds(toolbar.removeFromLeft(84));
    toolbar.removeFromLeft(8);
    plotKindBox.setBounds(toolbar.removeFromLeft(168));
    toolbar.removeFromLeft(8);
    taperLabel.setBounds(toolbar.removeFromLeft(40));
    taperBox.setBounds(toolbar.removeFromLeft(96));
    if (injectFreqBox.isVisible())
    {
        toolbar.removeFromLeft(8);
        injectFreqLabel.setBounds(toolbar.removeFromLeft(52));
        injectFreqBox.setBounds(toolbar.removeFromLeft(108));
        toolbar.removeFromLeft(8);
        injectAmpLabel.setBounds(toolbar.removeFromLeft(52));
        injectAmpBox.setBounds(toolbar.removeFromLeft(88));
    }

    area.removeFromTop(8);

    auto knobRow = area.removeFromTop(92);
    layoutKnobColumn(knobRow, gainLabel, gainKnob);
    layoutKnobColumn(knobRow, secondaryLabel, secondaryKnob);
    layoutKnobColumn(knobRow, tertiaryLabel, tertiaryKnob);

    area.removeFromTop(10);

    layoutContentArea(area);
}

void MainComponent::layoutContentArea(juce::Rectangle<int> area)
{
    const int maxSchematicWidth =
        area.getWidth() - kAcPanelMinWidth - kSchematicSplitterWidth - kPanelGap;

    if (schematicPanelWidth_ <= 0)
    {
        schematicPanelWidth_ = juce::jlimit(kSchematicMinWidth,
                                            juce::jmax(kSchematicMinWidth, maxSchematicWidth),
                                            area.getWidth() * 38 / 100);
    }

    schematicPanelWidth_ =
        juce::jlimit(kSchematicMinWidth, juce::jmax(kSchematicMinWidth, maxSchematicWidth), schematicPanelWidth_);

    if (schematicPanel != nullptr)
        schematicPanel->setBounds(area.removeFromLeft(schematicPanelWidth_));

    schematicSplitter.setBounds(area.removeFromLeft(kSchematicSplitterWidth));
    area.removeFromLeft(kPanelGap);

    if (acPanel != nullptr)
        acPanel->setBounds(area.reduced(0, 2));
}
