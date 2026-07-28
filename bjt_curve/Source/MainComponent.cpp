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
}  // namespace

MainComponent::MainComponent()
{
    setLookAndFeel(&atomLookAndFeel);

    titleLabel.setText("NuDSP BJT Curve Tracer", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(28.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centredLeft);

    subtitleLabel.setText("Ic vs Vce = NuDSP Ebers-Moll at constant Ib (device curves). Load line / Q from design_core.",
                          juce::dontSendNotification);
    subtitleLabel.setJustificationType(juce::Justification::centredLeft);
    subtitleLabel.setInterceptsMouseClicks(false, false);

    addAndMakeVisible(titleLabel);
    addAndMakeVisible(subtitleLabel);
    addAndMakeVisible(themeButton);
    addAndMakeVisible(curveKindBox);
    addAndMakeVisible(circuitBox);
    addAndMakeVisible(modelBox);

    configureCombo(curveKindBox);
    configureCombo(circuitBox);
    configureCombo(modelBox);

    ibCountLabel.setText("Ib count", juce::dontSendNotification);
    ibMinLabel.setText("Ib min", juce::dontSendNotification);
    ibStepLabel.setText("Ib step", juce::dontSendNotification);
    vceMaxLabel.setText("Vce max", juce::dontSendNotification);
    vbeMaxLabel.setText("Vbe max", juce::dontSendNotification);
    iMaxLabel.setText("Ic max", juce::dontSendNotification);
    for (auto* label : {&ibCountLabel, &ibMinLabel, &ibStepLabel, &vceMaxLabel, &vbeMaxLabel, &iMaxLabel})
    {
        label->setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(*label);
    }

    ibCountSlider.setRange(1.0, 20.0, 1.0);
    ibCountSlider.setValue(5.0, juce::dontSendNotification);
    ibCountSlider.setTextValueSuffix(" curves");
    ibCountSlider.setValueLabelPos(atom::Slider::ValueLabelPos::Right);
    configureMicroAmpSlider(ibMinSlider, 1.0, " uA");
    configureMicroAmpSlider(ibStepSlider, 1.5, " uA");
    configureVoltSlider(vceMaxSlider, bjt_curve::kDefaultMaxVce);
    configureVbeSlider(vbeMaxSlider, bjt_curve::kDefaultMaxVbe);
    configureMilliAmpSlider(iMaxSlider, bjt_curve::kDefaultMaxIcAmps * 1000.0);

    for (auto* slider : {&ibCountSlider, &ibMinSlider, &ibStepSlider, &vceMaxSlider, &vbeMaxSlider, &iMaxSlider})
    {
        slider->setValueLabelPos(atom::Slider::ValueLabelPos::Right);
        slider->onValueChange = [this]() { updateCurveViews(); };
        addAndMakeVisible(*slider);
    }

    curveKindBox.addItem("Ic vs Vbe (transfer)", 1);
    curveKindBox.addItem("Ib vs Vbe (input)", 2);
    curveKindBox.addItem("Ic vs Vce (output)", 3);
    curveKindBox.setSelectedId(1, juce::dontSendNotification);
    curveKindBox.onChange = [this]() { updateCurveViews(); };

    circuitBox.addItem("Common Emitter", 1);
    circuitBox.addItem("Follower", 2);
    circuitBox.addItem("Follower Out", 3);
    circuitBox.setSelectedId(1, juce::dontSendNotification);
    circuitBox.onChange = [this]() { updateCurveViews(); };

    modelBox.addItem("Ideal NPN", 1);
    modelBox.addItem("2N3904", 2);
    modelBox.addItem("2N2222", 3);
    modelBox.setSelectedId(1, juce::dontSendNotification);
    modelBox.onChange = [this]() { updateCurveViews(); };

    curvePanel = std::make_unique<BjtCurvePanel>();
    addAndMakeVisible(*curvePanel);

    themeButton.onClick = [this]()
    {
        atomLookAndFeel.setTheme(atomLookAndFeel.getTheme() == atom::ThemeType::Dark ? atom::ThemeType::Light
                                                                                     : atom::ThemeType::Dark);
        applyTheme();
    };

    // Theme first (no curve recompute), then one coalesced rebuild from control state.
    applyTheme();
    updateCurveViews();
    setSize(980, 720);
}

MainComponent::~MainComponent()
{
    setLookAndFeel(nullptr);
}

void MainComponent::configureCombo(atom::ComboBox& combo)
{
    combo.setJustificationType(juce::Justification::centredLeft);
}

void MainComponent::configureMicroAmpSlider(atom::Slider& slider, double value, const juce::String& suffix)
{
    slider.setRange(0.5, 5000.0, 0.5);
    slider.setValue(value, juce::dontSendNotification);
    slider.setTextValueSuffix(suffix);
    slider.setSkewFactorFromMidPoint(20.0);
}

void MainComponent::configureMilliAmpSlider(atom::Slider& slider, double value)
{
    slider.setRange(0.01, 100.0, 0.01);
    slider.setValue(value, juce::dontSendNotification);
    slider.setTextValueSuffix(" mA");
    slider.setSkewFactorFromMidPoint(2.0);
}

void MainComponent::configureVoltSlider(atom::Slider& slider, double value)
{
    slider.setRange(0.2, 20.0, 0.1);
    slider.setValue(value, juce::dontSendNotification);
    slider.setTextValueSuffix(" V");
    slider.setSkewFactorFromMidPoint(2.0);
}

void MainComponent::configureVbeSlider(atom::Slider& slider, double value)
{
    slider.setRange(0.20, 1.20, 0.01);
    slider.setValue(value, juce::dontSendNotification);
    slider.setTextValueSuffix(" V");
    slider.setSkewFactorFromMidPoint(0.70);
}

void MainComponent::syncTransferCurrentSlider(bjt_curve::CurveKind kind)
{
    if (kind == lastTransferKind_)
        return;

    // Preserve separate defaults when switching Ic ↔ Ib transfer plots.
    if (kind == bjt_curve::CurveKind::IbVsVbe)
    {
        iMaxLabel.setText("Ib max", juce::dontSendNotification);
        configureMicroAmpSlider(iMaxSlider, bjt_curve::kDefaultMaxIbAmps * 1.0e6, " uA");
        iMaxSlider.setValueLabelPos(atom::Slider::ValueLabelPos::Right);
    }
    else if (kind == bjt_curve::CurveKind::IcVsVbe)
    {
        iMaxLabel.setText("Ic max", juce::dontSendNotification);
        configureMilliAmpSlider(iMaxSlider, bjt_curve::kDefaultMaxIcAmps * 1000.0);
        iMaxSlider.setValueLabelPos(atom::Slider::ValueLabelPos::Right);
    }

    lastTransferKind_ = kind;
}

void MainComponent::applyTheme()
{
    const auto themeType = atomLookAndFeel.getTheme();
    const auto& themeColors =
        themeType == atom::ThemeType::Dark ? atom::Theme::getDarkTheme() : atom::Theme::getLightTheme();

    titleLabel.refreshTheme();
    subtitleLabel.refreshTheme();
    ibCountLabel.refreshTheme();
    ibMinLabel.refreshTheme();
    ibStepLabel.refreshTheme();
    vceMaxLabel.refreshTheme();
    vbeMaxLabel.refreshTheme();
    iMaxLabel.refreshTheme();

    if (curvePanel != nullptr)
        curvePanel->applyTheme(themeColors);

    themeButton.setButtonText(themeType == atom::ThemeType::Dark ? "Switch to Light" : "Switch to Dark");
    repaint();
}

void MainComponent::updateCurveViews()
{
    const auto kind = getCurveKindFromSelection();
    const auto circuit = getCircuitFromSelection();
    const auto ibSweep = getIbSweepFromControls();
    const bool showIbSweepControls =
        kind == bjt_curve::CurveKind::IcVsVce && circuit == bjt_curve::CircuitKind::CommonEmitter;
    const bool showTransferAxisControls =
        kind == bjt_curve::CurveKind::IcVsVbe || kind == bjt_curve::CurveKind::IbVsVbe;

    if (showTransferAxisControls)
        syncTransferCurrentSlider(kind);

    for (auto* control :
         {static_cast<juce::Component*>(&ibCountLabel), static_cast<juce::Component*>(&ibCountSlider),
          static_cast<juce::Component*>(&ibMinLabel), static_cast<juce::Component*>(&ibMinSlider),
          static_cast<juce::Component*>(&ibStepLabel), static_cast<juce::Component*>(&ibStepSlider),
          static_cast<juce::Component*>(&vceMaxLabel), static_cast<juce::Component*>(&vceMaxSlider)})
    {
        control->setVisible(showIbSweepControls);
        control->setEnabled(showIbSweepControls);
    }

    for (auto* control :
         {static_cast<juce::Component*>(&vbeMaxLabel), static_cast<juce::Component*>(&vbeMaxSlider),
          static_cast<juce::Component*>(&iMaxLabel), static_cast<juce::Component*>(&iMaxSlider)})
    {
        control->setVisible(showTransferAxisControls);
        control->setEnabled(showTransferAxisControls);
    }

    if (curvePanel != nullptr)
    {
        curvePanel->setModel(getModelFromSelection());
        curvePanel->setCircuit(circuit);
        curvePanel->setCurveKind(kind);
        curvePanel->setIbSweep(ibSweep);
        curvePanel->setVceMaxVolts(static_cast<float>(vceMaxSlider.getValue()));
        curvePanel->setVbeMaxVolts(static_cast<float>(vbeMaxSlider.getValue()));

        const float currentMaxAmps = kind == bjt_curve::CurveKind::IbVsVbe
            ? static_cast<float>(iMaxSlider.getValue() * 1.0e-6)
            : static_cast<float>(iMaxSlider.getValue() * 1.0e-3);
        curvePanel->setCurrentMaxAmps(currentMaxAmps);
    }

    resized();
}

bjt_curve::CircuitKind MainComponent::getCircuitFromSelection() const
{
    switch (circuitBox.getSelectedId())
    {
        case 2:
            return bjt_curve::CircuitKind::Follower;
        case 3:
            return bjt_curve::CircuitKind::FollowerOut;
        default:
            return bjt_curve::CircuitKind::CommonEmitter;
    }
}

bjt_curve::CurveKind MainComponent::getCurveKindFromSelection() const
{
    switch (curveKindBox.getSelectedId())
    {
        case 2:
            return bjt_curve::CurveKind::IbVsVbe;
        case 3:
            return bjt_curve::CurveKind::IcVsVce;
        default:
            return bjt_curve::CurveKind::IcVsVbe;
    }
}

nx_bjt_npn_model_e MainComponent::getModelFromSelection() const
{
    switch (modelBox.getSelectedId())
    {
        case 2:
            return NX_BJT_2N3904;
        case 3:
            return NX_BJT_2N2222;
        default:
            return NX_BJT_NPN;
    }
}

bjt_curve::IbSweepParams MainComponent::getIbSweepFromControls() const
{
    bjt_curve::IbSweepParams params;
    params.count = juce::roundToInt(ibCountSlider.getValue());
    params.minAmps = static_cast<float>(ibMinSlider.getValue() * 1.0e-6);
    params.stepAmps = static_cast<float>(ibStepSlider.getValue() * 1.0e-6);
    params.sanitise();
    return params;
}

void MainComponent::paint(juce::Graphics& g)
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
    modelBox.setBounds(toolbar.removeFromLeft(130));
    toolbar.removeFromLeft(8);
    circuitBox.setBounds(toolbar.removeFromLeft(150));
    toolbar.removeFromLeft(8);
    curveKindBox.setBounds(toolbar.removeFromLeft(220));

    area.removeFromTop(14);

    const auto kind = getCurveKindFromSelection();
    const bool showIbSweepControls = kind == bjt_curve::CurveKind::IcVsVce
                                  && getCircuitFromSelection() == bjt_curve::CircuitKind::CommonEmitter;
    const bool showTransferAxisControls =
        kind == bjt_curve::CurveKind::IcVsVbe || kind == bjt_curve::CurveKind::IbVsVbe;

    const int sweepRowHeight = 30;
    const int labelWidth = 72;
    const int rowGap = 6;

    if (showIbSweepControls)
    {
        const int sweepBlockHeight = sweepRowHeight * 4 + rowGap * 3 + 8;

        auto sweepArea = area.removeFromBottom(sweepBlockHeight);
        sweepArea.removeFromTop(8);

        auto vceRow = sweepArea.removeFromBottom(sweepRowHeight);
        vceMaxLabel.setBounds(vceRow.removeFromLeft(labelWidth));
        vceMaxSlider.setBounds(vceRow);
        sweepArea.removeFromBottom(rowGap);

        auto stepRow = sweepArea.removeFromBottom(sweepRowHeight);
        ibStepLabel.setBounds(stepRow.removeFromLeft(labelWidth));
        ibStepSlider.setBounds(stepRow);
        sweepArea.removeFromBottom(rowGap);

        auto minRow = sweepArea.removeFromBottom(sweepRowHeight);
        ibMinLabel.setBounds(minRow.removeFromLeft(labelWidth));
        ibMinSlider.setBounds(minRow);
        sweepArea.removeFromBottom(rowGap);

        auto countRow = sweepArea;
        ibCountLabel.setBounds(countRow.removeFromLeft(labelWidth));
        ibCountSlider.setBounds(countRow);

        for (auto* control :
             {static_cast<juce::Component*>(&ibCountLabel), static_cast<juce::Component*>(&ibCountSlider),
              static_cast<juce::Component*>(&ibMinLabel), static_cast<juce::Component*>(&ibMinSlider),
              static_cast<juce::Component*>(&ibStepLabel), static_cast<juce::Component*>(&ibStepSlider),
              static_cast<juce::Component*>(&vceMaxLabel), static_cast<juce::Component*>(&vceMaxSlider)})
            control->toFront(false);
    }
    else if (showTransferAxisControls)
    {
        const int sweepBlockHeight = sweepRowHeight * 2 + rowGap + 8;

        auto sweepArea = area.removeFromBottom(sweepBlockHeight);
        sweepArea.removeFromTop(8);

        auto iRow = sweepArea.removeFromBottom(sweepRowHeight);
        iMaxLabel.setBounds(iRow.removeFromLeft(labelWidth));
        iMaxSlider.setBounds(iRow);
        sweepArea.removeFromBottom(rowGap);

        auto vbeRow = sweepArea;
        vbeMaxLabel.setBounds(vbeRow.removeFromLeft(labelWidth));
        vbeMaxSlider.setBounds(vbeRow);

        for (auto* control :
             {static_cast<juce::Component*>(&vbeMaxLabel), static_cast<juce::Component*>(&vbeMaxSlider),
              static_cast<juce::Component*>(&iMaxLabel), static_cast<juce::Component*>(&iMaxSlider)})
            control->toFront(false);
    }

    if (curvePanel != nullptr)
        curvePanel->setBounds(area.reduced(0, 2));
}
