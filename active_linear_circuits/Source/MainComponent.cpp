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
        {"DS-1", ds1_ac::CircuitKind::Ds1Opamp},
        {"RAT", ds1_ac::CircuitKind::RatOpamp},
        {"Guvnor Preamp", ds1_ac::CircuitKind::GuvnorPreamp},
        {"Guvnor Postamp", ds1_ac::CircuitKind::GuvnorPostamp},
        {"Guvnor OpAmp", ds1_ac::CircuitKind::GuvnorOpamp},
        {"Guvnor Level", ds1_ac::CircuitKind::GuvnorLevel},
        {"TS-9 Tone", ds1_ac::CircuitKind::Ts9Tone},
        {"DS-1 Tone", ds1_ac::CircuitKind::Ds1Tone},
        {"DS+", ds1_ac::CircuitKind::DsPlusOpamp},
        {"Klon Centaur Tone", ds1_ac::CircuitKind::KlonCentaurTone},
        {"AC Booster EQ", ds1_ac::CircuitKind::AcBoosterEq},
        {"DS-1 Clipper", ds1_ac::CircuitKind::Ds1Clipper},
        {"Diode Clipper", ds1_ac::CircuitKind::DiodeClipper},
        {"RAT Clipper", ds1_ac::CircuitKind::RatClipper},
        {"TS-9 OpAmp", ds1_ac::CircuitKind::Ts9Opamp},
        {"AC Booster Drive", ds1_ac::CircuitKind::AcBoosterDrive},
        {"Klon Centaur", ds1_ac::CircuitKind::KlonCentaur},
        {"Guvnor Clipper", ds1_ac::CircuitKind::GuvnorClipper},
        {"BJT Follower", ds1_ac::CircuitKind::BjtFollower},
        {"BJT Follower Out", ds1_ac::CircuitKind::BjtFollowerOut},
        {"BJT Common Emitter", ds1_ac::CircuitKind::BjtCommonEmitter},
        {"JFET Follower", ds1_ac::CircuitKind::JfetFollower},
        {"SansAmp Classic Spk", ds1_ac::CircuitKind::SansampClassicSpk},
        {"SansAmp Classic Micing", ds1_ac::CircuitKind::SansampClassicMicing},
    };

    constexpr int kKnobColumnWidth = 72;
    constexpr int kKnobSize = 56;
} // namespace

MainComponent::MainComponent()
{
    setLookAndFeel(&atomLookAndFeel);

    titleLabel.setText("NuDSP OpAmp AC Tracer", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(28.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centredLeft);

    subtitleLabel.setText("Bode plot + 1-period sine preview", juce::dontSendNotification);
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

    circuitLabel.setText("Circuit", juce::dontSendNotification);
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
    taperBox.setSelectedId(ds1_ac::potTaperComboId(ds1_ac::defaultPotTaper(ds1_ac::CircuitKind::Ds1Opamp)),
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

    sineFreqLabel.setText("Sine Hz", juce::dontSendNotification);
    sineFreqLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(sineFreqLabel);

    configureKnob(sineFreqKnob);
    sineFreqKnob.setRange(ds1_ac::kPreviewFreqMinHz, ds1_ac::kPreviewFreqMaxHz, 1.0);
    sineFreqKnob.setValue(ds1_ac::kDefaultPreviewFreqHz, juce::dontSendNotification);
    sineFreqKnob.setSkewFactorFromMidPoint(1000.0);
    sineFreqKnob.setTextValueSuffix(" Hz");
    sineFreqKnob.onValueChange = [this]()
    { updatePlotView(); };
    addAndMakeVisible(sineFreqKnob);

    plotKindBox.addItem("Magnitude", 1);
    plotKindBox.addItem("Phase", 2);
    plotKindBox.addItem("Magnitude + Phase", 3);
    plotKindBox.setSelectedId(3, juce::dontSendNotification);
    plotKindBox.onChange = [this]()
    { updatePlotView(); };

    opampModelBox.addItem("Ideal", 1);
    opampModelBox.addItem("LM741", 2);
    opampModelBox.addItem("JRC4558", 3);
    opampModelBox.addItem("BA728", 4);
    opampModelBox.addItem("LM308", 5);
    opampModelBox.addItem("TL072", 6);
    opampModelBox.setSelectedId(4, juce::dontSendNotification);
    opampModelBox.onChange = [this]()
    { updatePlotView(); };

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
    setSize(1180, 760);
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
    sineFreqLabel.refreshTheme();
    sampleRateLabel.refreshTheme();
    taperLabel.refreshTheme();

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
    const bool usesBjt = ds1_ac::circuitUsesBjtModel(circuit);
    const bool usesJfet = ds1_ac::circuitUsesJfetModel(circuit);
    const bool hasPrimary = ds1_ac::circuitHasPrimaryControl(circuit);
    const bool usesPotTaper = ds1_ac::circuitUsesPotTaper(circuit);
    const bool hasSecondary = ds1_ac::circuitHasSecondaryControl(circuit);
    const bool hasTertiary = ds1_ac::circuitHasTertiaryControl(circuit);

    acPanel->setCircuitKind(circuit);
    acPanel->setOpampModel(getOpampModelFromSelection());
    acPanel->setBjtModel(getBjtModelFromSelection());
    acPanel->setJfetModel(getJfetModelFromSelection());
    acPanel->setGainControl(gainKnob.getValue());
    acPanel->setSecondaryControl(secondaryKnob.getValue());
    acPanel->setTertiaryControl(tertiaryKnob.getValue());
    acPanel->setPotTaper(getPotTaperFromSelection());
    acPanel->setPreviewFrequencyHz(sineFreqKnob.getValue());
    acPanel->setSampleRateHz(getSampleRateFromSelection());
    acPanel->setPlotKind(getPlotKindFromSelection());

    gainLabel.setText(ds1_ac::controlParameterName(circuit), juce::dontSendNotification);
    secondaryLabel.setText(ds1_ac::secondaryControlParameterName(circuit), juce::dontSendNotification);
    tertiaryLabel.setText(ds1_ac::tertiaryControlParameterName(circuit), juce::dontSendNotification);
    subtitleLabel.setText("Bode plot + 1-period sine preview (" + juce::String(ds1_ac::circuitProcessFunctionName(circuit)) + ")",
                          juce::dontSendNotification);

    opampModelBox.setVisible(usesOpamp || usesBjt || usesJfet);
    opampModelBox.setEnabled(usesOpamp || usesBjt || usesJfet);

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

    const bool showSineControls = getPlotKindFromSelection() != ds1_ac::PlotKind::Magnitude;
    sineFreqLabel.setVisible(showSineControls);
    sineFreqLabel.setEnabled(showSineControls);
    sineFreqKnob.setVisible(showSineControls);
    sineFreqKnob.setEnabled(showSineControls);
    resized();
}

ds1_ac::CircuitKind MainComponent::getCircuitFromSelection() const
{
    const int selectedId = circuitBox.getSelectedId();
    if (selectedId >= 1 && selectedId <= static_cast<int>(std::size(kCircuitMenuEntries)))
        return kCircuitMenuEntries[static_cast<size_t>(selectedId - 1)].kind;

    return ds1_ac::CircuitKind::Ds1Opamp;
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
        opampModelBox.addItem("Ideal", 1);
        opampModelBox.addItem("LM741", 2);
        opampModelBox.addItem("JRC4558", 3);
        opampModelBox.addItem("BA728", 4);
        opampModelBox.addItem("LM308", 5);
        opampModelBox.addItem("TL072", 6);
        opampModelBox.setSelectedId(4, juce::dontSendNotification);
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
    switch (opampModelBox.getSelectedId())
    {
    case 1:
        return NX_OPAMP_IDEAL;
    case 2:
        return NX_OPAMP_LM741;
    case 3:
        return NX_OPAMP_JRC4558;
    case 5:
        return NX_OPAMP_LM308;
    case 6:
        return NX_OPAMP_TL072;
    default:
        return NX_OPAMP_BA728;
    }
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
    circuitBox.setBounds(toolbar.removeFromLeft(132));
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

    area.removeFromTop(8);

    auto knobRow = area.removeFromTop(92);
    layoutKnobColumn(knobRow, gainLabel, gainKnob);
    layoutKnobColumn(knobRow, secondaryLabel, secondaryKnob);
    layoutKnobColumn(knobRow, tertiaryLabel, tertiaryKnob);
    layoutKnobColumn(knobRow, sineFreqLabel, sineFreqKnob);

    area.removeFromTop(10);

    if (acPanel != nullptr)
        acPanel->setBounds(area.reduced(0, 2));
}
