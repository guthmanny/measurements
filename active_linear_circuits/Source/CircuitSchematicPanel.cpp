#include "CircuitSchematicPanel.h"

#include "SchematicSvgAdapter.h"

#include <string_view>

namespace
{
juce::File locateMudspRoot()
{
#if defined(MUDSP_ROOT)
    {
        const juce::File fromDefine(juce::CharPointer_UTF8(MUDSP_ROOT));
        if (fromDefine.getChildFile("assets/registry.json").existsAsFile())
            return fromDefine;
    }
#endif

    auto dir = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory();
    for (int depth = 0; depth < 10; ++depth)
    {
        const auto sibling = dir.getChildFile("MuDSP");
        if (sibling.getChildFile("assets/registry.json").existsAsFile())
            return sibling;

        if (! dir.getParentDirectory().exists() || dir.getParentDirectory() == dir)
            break;
        dir = dir.getParentDirectory();
    }

    const auto homeMudsp = juce::File::getSpecialLocation(juce::File::userHomeDirectory)
                               .getChildFile("myCode")
                               .getChildFile("MuDSP");
    if (homeMudsp.getChildFile("assets/registry.json").existsAsFile())
        return homeMudsp;

    return {};
}

juce::File resolveSvgRelative(const char* relative)
{
    if (relative == nullptr || relative[0] == '\0')
        return {};

    const auto root = locateMudspRoot();
    if (! root.isDirectory())
        return {};

    return root.getChildFile(relative);
}

juce::String internalSvgRelativePath(const juce::String& externalSvgRel)
{
    if (externalSvgRel.endsWithIgnoreCase(".svg"))
        return externalSvgRel.dropLastCharacters(4) + "_internal.svg";
    return externalSvgRel + "_internal.svg";
}

const juce::var* registryOperatorsObject()
{
    static juce::var cached;
    static bool loaded = false;
    if (! loaded)
    {
        loaded = true;
        const auto registry = locateMudspRoot().getChildFile("assets/registry.json");
        if (registry.existsAsFile())
        {
            const auto parsed = juce::JSON::parse(registry);
            if (auto* root = parsed.getDynamicObject())
                cached = root->getProperty("operators");
        }
    }

    if (cached.getDynamicObject() != nullptr)
        return &cached;
    return nullptr;
}

bool operatorHasInternalFromRegistry(const char* operatorKey)
{
    if (operatorKey == nullptr || operatorKey[0] == '\0')
        return false;

    const auto* operators = registryOperatorsObject();
    if (operators == nullptr)
        return false;

    if (auto* entry = operators->getDynamicObject()->getProperty(operatorKey).getDynamicObject())
        return static_cast<bool>(entry->getProperty("has_internal"));

    return false;
}

juce::File resolveCircuitSvgFile(ds1_ac::CircuitKind circuit, bool useInternal)
{
    const auto external = juce::String(ds1_ac::circuitSvgRelativePath(circuit));
    const auto rel = useInternal ? internalSvgRelativePath(external) : external;
    return resolveSvgRelative(rel.toRawUTF8());
}

juce::File resolveCompositeSvgFile()
{
    return resolveSvgRelative(ds1_ac::compositeSvgRelativePath());
}

void loadSvgIntoView(atom::SvgView& view, const juce::File& file, juce::String& status)
{
    view.setSvg(std::string_view{});
    if (! file.existsAsFile())
    {
        status = file.getFullPathName().isEmpty() ? "Missing schematic path"
                                                  : "Missing: " + file.getFileName();
        return;
    }

    const auto xml = file.loadFileAsString();
    if (xml.isEmpty())
    {
        status = "Empty SVG: " + file.getFileName();
        return;
    }

    view.setSvg(xml.toRawUTF8(), static_cast<size_t>(xml.getNumBytesAsUTF8()));
    if (view.getSvgNaturalBounds().isEmpty())
        status = "Failed to parse SVG: " + file.getFileName();
}
} // namespace

CircuitSchematicPanel::CircuitSchematicPanel()
    : themeColors_(atom::Theme::getCurrent())
{
    titleLabel.setText("Stage schematic", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centredLeft);

    signalOutputActiveLabel.setJustificationType(juce::Justification::centredLeft);
    signalOutputActiveLabel.setVisible(false);
    signalOutputOtherLabel.setJustificationType(juce::Justification::centredLeft);
    signalOutputOtherLabel.setVisible(false);
    svgView.setOverlayClassifier(ds1_ac::classifyCircuitSvgId);
    svgView.configureOverlaySelector = [](const juce::String& key, atom::ComboBox& combo)
    {
        if (key.startsWithIgnoreCase("OPAMP_"))
            ds1_ac::populateOpampModelCombo(combo);
        else if (key.startsWithIgnoreCase("DIODE_"))
            ds1_ac::populateDiodeModelCombo(combo);
        else if (key.startsWithIgnoreCase("BJT_NPN_") || key.startsWithIgnoreCase("BJT_PNP_"))
            ds1_ac::populateBjtModelCombo(combo);
        else if (key.startsWithIgnoreCase("JFET_"))
            ds1_ac::populateJfetModelCombo(combo);
    };
    svgView.onOverlaySelectionChanged = [this](const juce::String& key, int selectedId)
    {
        if (key.startsWithIgnoreCase("OPAMP_"))
            handleOpampOverlaySelection(key, selectedId);
        else if (key.startsWithIgnoreCase("DIODE_"))
            handleDiodeOverlaySelection(key, selectedId);
        else if (key.startsWithIgnoreCase("BJT_NPN_") || key.startsWithIgnoreCase("BJT_PNP_"))
            handleBjtOverlaySelection(key, selectedId);
        else if (key.startsWithIgnoreCase("JFET_"))
            handleJfetOverlaySelection(key, selectedId);
    };
    svgView.onOverlayTextChanged = [this](const juce::String& key, const juce::String& text)
    {
        handleOverlayTextChange(key, text);
    };
    internalToggle.setClickingTogglesState(true);
    internalToggle.setVisible(false);
    internalToggle.onClick = [this]
    {
        showInternalSvg_ = internalToggle.getToggleState();
        syncSignalOutputDisplay();
        reloadSvg();
    };

    addAndMakeVisible(titleLabel);
    addChildComponent(signalOutputActiveLabel);
    addChildComponent(signalOutputOtherLabel);
    addChildComponent(internalToggle);
    addAndMakeVisible(svgView);
    reloadSvg();
}

void CircuitSchematicPanel::setOpampModel(nx_opamp_model_e model)
{
    opampModel_ = model;
    syncOpampOverlaySelectors();
}

void CircuitSchematicPanel::setDiodeModel(nx_diode_model_t model)
{
    diodeModel_ = model;
    syncDiodeOverlaySelectors();
}

void CircuitSchematicPanel::setBjtModel(nx_bjt_npn_model_e model)
{
    bjtModel_ = model;
    syncBjtOverlaySelectors();
}

void CircuitSchematicPanel::setJfetModel(nx_jfet_n_model_e model)
{
    jfetModel_ = model;
    syncJfetOverlaySelectors();
}

void CircuitSchematicPanel::syncOpampModelFromSvg()
{
    if (! ds1_ac::circuitUsesOpampModel(circuitKind_))
        return;

    const auto modelFromSvg = ds1_ac::defaultOpampModelFromSvgView(svgView);
    if (modelFromSvg != opampModel_)
    {
        opampModel_ = modelFromSvg;
        if (onOpampModelChanged)
            onOpampModelChanged(opampModel_);
    }

    syncOpampOverlaySelectors();
}

void CircuitSchematicPanel::syncOpampOverlaySelectors()
{
    if (! ds1_ac::circuitUsesOpampModel(circuitKind_))
        return;

    const auto keys = svgView.findOverlayKeysWithPrefix("OPAMP_");
    if (keys.isEmpty())
        return;

    const int comboId = ds1_ac::opampModelComboId(opampModel_);
    for (const auto& key : keys)
        svgView.setOverlaySelectedId(key, comboId);

    svgView.resized();
}

void CircuitSchematicPanel::syncDiodeModelFromSvg()
{
    if (! ds1_ac::circuitUsesDiodeModel(circuitKind_))
        return;

    const auto keys = svgView.findOverlayKeysWithPrefix("DIODE_");
    nx_diode_model_t next = ds1_ac::defaultDiodeModelForCircuit(circuitKind_);
    if (! keys.isEmpty())
        next = ds1_ac::defaultDiodeModelFromSvgView(svgView);

    if (next != diodeModel_)
    {
        diodeModel_ = next;
        if (onDiodeModelChanged)
            onDiodeModelChanged(diodeModel_);
    }

    syncDiodeOverlaySelectors();
}

void CircuitSchematicPanel::syncDiodeOverlaySelectors()
{
    if (! ds1_ac::circuitUsesDiodeModel(circuitKind_))
        return;

    const auto keys = svgView.findOverlayKeysWithPrefix("DIODE_");
    if (keys.isEmpty())
        return;

    const int comboId = ds1_ac::diodeModelComboId(diodeModel_);
    for (const auto& key : keys)
        svgView.setOverlaySelectedId(key, comboId);

    svgView.resized();
}

void CircuitSchematicPanel::syncBjtModelFromSvg()
{
    if (! ds1_ac::circuitUsesBjtModel(circuitKind_))
        return;

    const auto keys = svgView.findOverlayKeysWithPrefix("BJT_NPN_");
    nx_bjt_npn_model_e next = ds1_ac::defaultBjtModel(circuitKind_);
    if (! keys.isEmpty())
        next = ds1_ac::defaultBjtModelFromSvgView(svgView);

    if (next != bjtModel_)
    {
        bjtModel_ = next;
        if (onBjtModelChanged)
            onBjtModelChanged(bjtModel_);
    }

    syncBjtOverlaySelectors();
}

void CircuitSchematicPanel::syncBjtOverlaySelectors()
{
    if (! ds1_ac::circuitUsesBjtModel(circuitKind_))
        return;

    const auto keys = svgView.findOverlayKeysWithPrefix("BJT_NPN_");
    if (keys.isEmpty())
        return;

    const int comboId = ds1_ac::bjtModelComboId(bjtModel_);
    for (const auto& key : keys)
        svgView.setOverlaySelectedId(key, comboId);

    svgView.resized();
}

void CircuitSchematicPanel::syncJfetModelFromSvg()
{
    if (! ds1_ac::circuitUsesJfetModel(circuitKind_))
        return;

    const auto keys = svgView.findOverlayKeysWithPrefix("JFET_");
    nx_jfet_n_model_e next = ds1_ac::defaultJfetModel(circuitKind_);
    if (! keys.isEmpty())
        next = ds1_ac::defaultJfetModelFromSvgView(svgView);

    if (next != jfetModel_)
    {
        jfetModel_ = next;
        if (onJfetModelChanged)
            onJfetModelChanged(jfetModel_);
    }

    syncJfetOverlaySelectors();
}

void CircuitSchematicPanel::syncJfetOverlaySelectors()
{
    if (! ds1_ac::circuitUsesJfetModel(circuitKind_))
        return;

    const auto keys = svgView.findOverlayKeysWithPrefix("JFET_");
    if (keys.isEmpty())
        return;

    const int comboId = ds1_ac::jfetModelComboId(jfetModel_);
    for (const auto& key : keys)
        svgView.setOverlaySelectedId(key, comboId);

    svgView.resized();
}

void CircuitSchematicPanel::handleOpampOverlaySelection(const juce::String& overlayKey, int selectedId)
{
    if (! overlayKey.startsWithIgnoreCase("OPAMP_"))
        return;

    const auto model = ds1_ac::opampModelFromComboId(selectedId);
    if (model == opampModel_)
        return;

    opampModel_ = model;
    syncOpampOverlaySelectors();

    if (onOpampModelChanged)
        onOpampModelChanged(model);
}

void CircuitSchematicPanel::handleDiodeOverlaySelection(const juce::String& overlayKey, int selectedId)
{
    if (! overlayKey.startsWithIgnoreCase("DIODE_"))
        return;

    const auto model = ds1_ac::diodeModelFromComboId(selectedId);
    if (model == diodeModel_)
        return;

    diodeModel_ = model;
    syncDiodeOverlaySelectors();

    if (onDiodeModelChanged)
        onDiodeModelChanged(model);
}

void CircuitSchematicPanel::handleBjtOverlaySelection(const juce::String& overlayKey, int selectedId)
{
    if (! overlayKey.startsWithIgnoreCase("BJT_NPN_") && ! overlayKey.startsWithIgnoreCase("BJT_PNP_"))
        return;

    const auto model = ds1_ac::bjtModelFromComboId(selectedId);
    if (model == bjtModel_)
        return;

    bjtModel_ = model;
    syncBjtOverlaySelectors();

    if (onBjtModelChanged)
        onBjtModelChanged(model);
}

void CircuitSchematicPanel::handleJfetOverlaySelection(const juce::String& overlayKey, int selectedId)
{
    if (! overlayKey.startsWithIgnoreCase("JFET_"))
        return;

    const auto model = ds1_ac::jfetModelFromComboId(selectedId);
    if (model == jfetModel_)
        return;

    jfetModel_ = model;
    syncJfetOverlaySelectors();

    if (onJfetModelChanged)
        onJfetModelChanged(model);
}

void CircuitSchematicPanel::handleOverlayTextChange(const juce::String& overlayKey, const juce::String& text)
{
    if (overlayKey.startsWithIgnoreCase("OPAMP_") || overlayKey.startsWithIgnoreCase("DIODE_")
        || overlayKey.startsWithIgnoreCase("BJT_NPN_") || overlayKey.startsWithIgnoreCase("BJT_PNP_")
        || overlayKey.startsWithIgnoreCase("JFET_"))
        return;

    const auto parsed = ds1_ac::SchematicComponentValues::parseLabelText(text);
    if (! parsed)
        return;

    auto key = overlayKey;
    const int colon = key.indexOfChar(':');
    if (colon >= 0)
        key = key.substring(colon + 1).trim();
    componentValues_.values[key] = *parsed;
    notifyComponentValuesChanged();
}

void CircuitSchematicPanel::notifyComponentValuesChanged()
{
    if (onComponentValuesChanged)
        onComponentValuesChanged(componentValues_);
}

void CircuitSchematicPanel::updateInternalToggleVisibility()
{
    const bool visible = operatorHasInternalFromRegistry(ds1_ac::circuitOperatorKey(circuitKind_));
    internalToggle.setVisible(visible);
    if (! visible)
    {
        showInternalSvg_ = false;
        internalToggle.setToggleState(false, juce::dontSendNotification);
    }
}

void CircuitSchematicPanel::syncSignalOutputDisplay()
{
    signalOutputInfo_ = {};
    signalOutputActiveLabel.setVisible(false);
    signalOutputOtherLabel.setVisible(false);
    if (! showInternalSvg_)
        return;

    signalOutputInfo_ = schematic_assets::loadCompositeStageSignalOutput(
        locateMudspRoot(), "ds1", juce::String(ds1_ac::circuitTopologyId(circuitKind_)));
    if (! signalOutputInfo_.hasOptions || signalOutputInfo_.activeLine.isEmpty())
        return;

    signalOutputActiveLabel.setVisible(true);
    signalOutputActiveLabel.setText(signalOutputInfo_.activeLine, juce::dontSendNotification);

    const bool showOther = signalOutputInfo_.otherLine.isNotEmpty();
    signalOutputOtherLabel.setVisible(showOther);
    if (showOther)
        signalOutputOtherLabel.setText(signalOutputInfo_.otherLine, juce::dontSendNotification);
}

void CircuitSchematicPanel::setCircuitKind(ds1_ac::CircuitKind circuitKind)
{
    if (circuitKind_ == circuitKind)
        return;

    circuitKind_ = circuitKind;
    showInternalSvg_ = false;
    internalToggle.setToggleState(false, juce::dontSendNotification);
    reloadSvg();
}

void CircuitSchematicPanel::applyTheme(const atom::ThemeColors& themeColors)
{
    themeColors_ = themeColors;
    titleLabel.refreshTheme();
    titleLabel.setColour(juce::Label::textColourId, themeColors.textSecondary);
    signalOutputActiveLabel.refreshTheme();
    signalOutputActiveLabel.setColour(juce::Label::textColourId, themeColors.accent);
    signalOutputOtherLabel.refreshTheme();
    signalOutputOtherLabel.setColour(juce::Label::textColourId, themeColors.textSecondary);
    internalToggle.sendLookAndFeelChange();
    svgView.sendLookAndFeelChange();
    repaint();
}

void CircuitSchematicPanel::reloadSvg()
{
    updateInternalToggleVisibility();
    syncSignalOutputDisplay();
    status_.clear();
    componentValues_ = {};
    loadSvgIntoView(svgView, resolveCircuitSvgFile(circuitKind_, showInternalSvg_), status_);

    auto title = juce::String("Stage schematic  |  ") + ds1_ac::circuitStageMenuLabel(circuitKind_);
    if (showInternalSvg_)
        title += "  (internal)";
    titleLabel.setText(title, juce::dontSendNotification);

    if (status_.isEmpty())
    {
        syncOpampModelFromSvg();
        syncDiodeModelFromSvg();
        syncBjtModelFromSvg();
        syncJfetModelFromSvg();
    }

    notifyComponentValuesChanged();

    resized();
    repaint();
}

void CircuitSchematicPanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour(themeColors_.backgroundSecondary);
    g.fillRoundedRectangle(bounds, 10.0f);
    g.setColour(themeColors_.border);
    g.drawRoundedRectangle(bounds.reduced(0.5f), 10.0f, 1.0f);

    if (status_.isNotEmpty())
    {
        g.setColour(themeColors_.textSecondary);
        g.setFont(13.0f);
        g.drawFittedText(status_, svgView.getBounds().reduced(12), juce::Justification::centred, 4);
    }
}

void CircuitSchematicPanel::resized()
{
    auto area = getLocalBounds().reduced(10, 8);

    auto stageHeader = area.removeFromTop(20);
    titleLabel.setBounds(stageHeader.removeFromLeft(juce::jmax(120, stageHeader.getWidth() - 104)));
    if (internalToggle.isVisible())
        internalToggle.setBounds(stageHeader.removeFromRight(96).withHeight(20));
    if (signalOutputActiveLabel.isVisible())
    {
        area.removeFromTop(2);
        signalOutputActiveLabel.setBounds(area.removeFromTop(18));
    }
    if (signalOutputOtherLabel.isVisible())
    {
        area.removeFromTop(2);
        signalOutputOtherLabel.setBounds(area.removeFromTop(16));
    }
    area.removeFromTop(4);
    svgView.setBounds(area);
    svgView.setVisible(status_.isEmpty());
}
