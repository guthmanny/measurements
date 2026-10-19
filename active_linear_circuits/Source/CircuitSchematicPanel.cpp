#include "CircuitSchematicPanel.h"

#include "SchematicComponentApply.h"

#include <optional>
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

std::optional<double> findComponentValue(const ds1_ac::SchematicComponentValues& values, const juce::String& name)
{
    auto it = values.values.find(name);
    if (it != values.values.end())
        return it->second;

    for (const auto& entry : values.values)
    {
        if (entry.first.equalsIgnoreCase(name))
            return entry.second;
    }

    return std::nullopt;
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
    controller_.attach(svgView);
    controller_.setBinding(&overlayBinding);
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

CircuitSchematicPanel::~CircuitSchematicPanel()
{
    controller_.setBinding(nullptr);
}

void CircuitSchematicPanel::setOpampModel(nx_opamp_model_e model)
{
    opampModel_ = model;
    controller_.refresh();
}

void CircuitSchematicPanel::setDiodeModel(nx_diode_model_t model)
{
    diodeModel_ = model;
    controller_.refresh();
}

void CircuitSchematicPanel::setBjtModel(nx_bjt_npn_model_e model)
{
    bjtModel_ = model;
    controller_.refresh();
}

void CircuitSchematicPanel::setJfetModel(nx_jfet_n_model_e model)
{
    jfetModel_ = model;
    controller_.refresh();
}

std::optional<atom::schematic::OverlayValue> CircuitSchematicPanel::OverlayBinding::read(const juce::String& overlayKey)
{
    const auto kind = atom::schematic::deviceKindFromOverlay(overlayKey);
    if (kind == atom::schematic::DeviceKind::Opamp)
    {
        atom::schematic::OverlayValue value;
        value.kind = atom::schematic::ValueKind::Device;
        value.enumCode = static_cast<int>(owner.opampModel_);
        value.comboId = ds1_ac::opampModelComboId(owner.opampModel_);
        return value;
    }
    if (kind == atom::schematic::DeviceKind::Diode)
    {
        atom::schematic::OverlayValue value;
        value.kind = atom::schematic::ValueKind::Device;
        value.enumCode = static_cast<int>(owner.diodeModel_);
        value.comboId = ds1_ac::diodeModelComboId(owner.diodeModel_);
        return value;
    }
    if (kind == atom::schematic::DeviceKind::BjtNpn || kind == atom::schematic::DeviceKind::BjtPnp)
    {
        atom::schematic::OverlayValue value;
        value.kind = atom::schematic::ValueKind::Device;
        value.enumCode = static_cast<int>(owner.bjtModel_);
        value.comboId = ds1_ac::bjtModelComboId(owner.bjtModel_);
        return value;
    }
    if (kind == atom::schematic::DeviceKind::Jfet)
    {
        atom::schematic::OverlayValue value;
        value.kind = atom::schematic::ValueKind::Device;
        value.enumCode = static_cast<int>(owner.jfetModel_);
        value.comboId = ds1_ac::jfetModelComboId(owner.jfetModel_);
        return value;
    }
    if (kind == atom::schematic::DeviceKind::Switch)
    {
        const auto canonical = ds1_ac::switchCanonicalName(overlayKey);
        const auto stored = findComponentValue(owner.componentValues_, canonical);
        const auto fallback = canonical.equalsIgnoreCase("SW1") ? 1 : 0;
        const auto position = static_cast<uint8_t>(stored ? juce::jlimit(0, 255, juce::roundToInt(*stored))
                                                         : fallback);
        atom::schematic::OverlayValue value;
        value.kind = atom::schematic::ValueKind::Switch;
        value.leaf = canonical;
        value.numeric = position;
        value.comboId = ds1_ac::switchComboId(owner.circuitKind_, overlayKey, position);
        return value;
    }

    auto component = atom::schematic::canonicalLeaf(overlayKey);
    if (component.isEmpty() || atom::schematic::isDeviceOrNameOverlay(component))
        return std::nullopt;

    auto lookupValue = [&](const juce::String& name)
    {
        auto found = owner.componentValues_.values.find(name);
        if (found != owner.componentValues_.values.end())
            return found;

        for (auto candidate = owner.componentValues_.values.begin();
             candidate != owner.componentValues_.values.end(); ++candidate)
        {
            if (candidate->first.equalsIgnoreCase(name))
                return candidate;
        }
        return owner.componentValues_.values.end();
    };

    auto it = lookupValue(component);
    if (it == owner.componentValues_.values.end())
    {
        const int dot = component.lastIndexOfChar('.');
        if (dot >= 0)
            it = lookupValue(component.substring(dot + 1));
    }
    if (it == owner.componentValues_.values.end())
        return std::nullopt;

    auto taperIt = owner.componentValues_.potTapers.find(it->first);
    if (taperIt == owner.componentValues_.potTapers.end())
    {
        for (auto candidate = owner.componentValues_.potTapers.begin();
             candidate != owner.componentValues_.potTapers.end(); ++candidate)
        {
            if (candidate->first.equalsIgnoreCase(it->first))
            {
                taperIt = candidate;
                break;
            }
        }
    }

    atom::schematic::OverlayValue value;
    value.kind = atom::schematic::ValueKind::Component;
    value.leaf = it->first;
    value.numeric = it->second;
    if (taperIt != owner.componentValues_.potTapers.end())
        value.potTaper = static_cast<int>(taperIt->second);
    return value;
}

bool CircuitSchematicPanel::OverlayBinding::write(const juce::String& overlayKey,
                                                 const atom::schematic::OverlayValue& value)
{
    if (value.kind == atom::schematic::ValueKind::Device)
    {
        const auto kind = atom::schematic::deviceKindFromOverlay(overlayKey);
        if (kind == atom::schematic::DeviceKind::Opamp)
        {
            const auto model = ds1_ac::opampModelFromComboId(value.comboId);
            if (model == owner.opampModel_)
                return false;
            owner.opampModel_ = model;
            if (owner.onOpampModelChanged)
                owner.onOpampModelChanged(model);
            return true;
        }
        if (kind == atom::schematic::DeviceKind::Diode)
        {
            const auto model = ds1_ac::diodeModelFromComboId(value.comboId);
            if (model == owner.diodeModel_)
                return false;
            owner.diodeModel_ = model;
            if (owner.onDiodeModelChanged)
                owner.onDiodeModelChanged(model);
            return true;
        }
        if (kind == atom::schematic::DeviceKind::BjtNpn || kind == atom::schematic::DeviceKind::BjtPnp)
        {
            const auto model = ds1_ac::bjtModelFromComboId(value.comboId);
            if (model == owner.bjtModel_)
                return false;
            owner.bjtModel_ = model;
            if (owner.onBjtModelChanged)
                owner.onBjtModelChanged(model);
            return true;
        }
        if (kind == atom::schematic::DeviceKind::Jfet)
        {
            const auto model = ds1_ac::jfetModelFromComboId(value.comboId);
            if (model == owner.jfetModel_)
                return false;
            owner.jfetModel_ = model;
            if (owner.onJfetModelChanged)
                owner.onJfetModelChanged(model);
            return true;
        }
        return false;
    }

    if (value.kind == atom::schematic::ValueKind::Switch)
    {
        const auto canonical = ds1_ac::switchCanonicalName(overlayKey);
        const auto position = ds1_ac::switchPositionFromComboId(owner.circuitKind_, overlayKey, value.comboId);
        const auto previous = findComponentValue(owner.componentValues_, canonical);
        if (previous && juce::roundToInt(*previous) == static_cast<int>(position))
            return false;

        owner.componentValues_.values[canonical] = static_cast<double>(position);
        owner.notifyComponentValuesChanged();
        return true;
    }

    auto key = atom::schematic::canonicalLeaf(overlayKey);
    if (key.isEmpty())
        return false;

    owner.componentValues_.values[key] = value.numeric;
    if (value.potTaper >= 0 && value.potTaper < NX_POT_TAPER_COUNT)
        owner.componentValues_.potTapers[key] = static_cast<nx_pot_taper_e>(value.potTaper);
    owner.notifyComponentValuesChanged();
    return true;
}

bool CircuitSchematicPanel::OverlayBinding::configureSelector(const juce::String& overlayKey, juce::ComboBox& combo)
{
    if (ds1_ac::isSwitchOverlayKey(overlayKey))
    {
        ds1_ac::populateSwitchCombo(combo, overlayKey, owner.circuitKind_);
        return true;
    }
    if (overlayKey.startsWithIgnoreCase("BJT_PNP_"))
    {
        atom::schematic::populateDeviceCombo("BJT_NPN_", combo);
        return true;
    }
    return false;
}

void CircuitSchematicPanel::notifyComponentValuesChanged()
{
    if (onComponentValuesChanged)
        onComponentValuesChanged(componentValues_);
}

void CircuitSchematicPanel::applyOperatorValuesToOverlays()
{
    componentValues_ = ds1_ac::readStageComponentValues(pedalKind_, circuitKind_, useCompositeConfig_);
    const auto models = ds1_ac::readStageDeviceModels(pedalKind_, circuitKind_, useCompositeConfig_);

    if (models.hasOpamp)
        opampModel_ = models.opamp;
    if (models.hasDiode)
        diodeModel_ = models.diode;
    if (models.hasBjt)
        bjtModel_ = models.bjt;
    if (models.hasJfet)
        jfetModel_ = models.jfet;

    controller_.refresh();
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

    if (! useCompositeConfig_)
        return;

    signalOutputInfo_ = schematic_assets::loadCompositeStageSignalOutput(
        locateMudspRoot(),
        ds1_ac::pedalCatalogKey(pedalKind_),
        juce::String(ds1_ac::circuitTopologyId(pedalKind_, circuitKind_)));
    if (! signalOutputInfo_.hasOptions || signalOutputInfo_.activeLine.isEmpty())
        return;

    signalOutputActiveLabel.setVisible(true);
    signalOutputActiveLabel.setText(signalOutputInfo_.activeLine, juce::dontSendNotification);

    const bool showOther = signalOutputInfo_.otherLine.isNotEmpty();
    signalOutputOtherLabel.setVisible(showOther);
    if (showOther)
        signalOutputOtherLabel.setText(signalOutputInfo_.otherLine, juce::dontSendNotification);
}

void CircuitSchematicPanel::setPedalKind(ds1_ac::PedalKind pedalKind)
{
    setSelection(pedalKind, circuitKind_, useCompositeConfig_);
}

void CircuitSchematicPanel::setCircuitKind(ds1_ac::CircuitKind circuitKind)
{
    setSelection(pedalKind_, circuitKind, useCompositeConfig_);
}

void CircuitSchematicPanel::setSelection(ds1_ac::PedalKind pedalKind,
                                        ds1_ac::CircuitKind circuitKind,
                                        bool useCompositeConfig)
{
    if (pedalKind_ == pedalKind && circuitKind_ == circuitKind && useCompositeConfig_ == useCompositeConfig)
        return;

    pedalKind_ = pedalKind;
    circuitKind_ = circuitKind;
    useCompositeConfig_ = useCompositeConfig;
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

    juce::String title;
    if (useCompositeConfig_)
        title = juce::String("Stage schematic  |  ") + ds1_ac::compositeDisplayName(pedalKind_)
                + "  |  " + ds1_ac::circuitStageMenuLabel(circuitKind_);
    else
        title = juce::String("Operator schematic  |  ") + ds1_ac::circuitOperatorKey(circuitKind_)
                + " — " + ds1_ac::circuitStageLabel(circuitKind_);
    if (showInternalSvg_)
        title += "  (internal)";
    titleLabel.setText(title, juce::dontSendNotification);

    if (status_.isEmpty())
    {
        applyOperatorValuesToOverlays();
        if (onOpampModelChanged && ds1_ac::circuitUsesOpampModel(circuitKind_))
            onOpampModelChanged(opampModel_);
        if (onDiodeModelChanged && ds1_ac::circuitUsesDiodeModel(circuitKind_))
            onDiodeModelChanged(diodeModel_);
        if (onBjtModelChanged && ds1_ac::circuitUsesBjtModel(circuitKind_))
            onBjtModelChanged(bjtModel_);
        if (onJfetModelChanged && ds1_ac::circuitUsesJfetModel(circuitKind_))
            onJfetModelChanged(jfetModel_);
    }

    notifyComponentValuesChanged();

    resized();
    if (onPreferredSizeChanged)
        onPreferredSizeChanged();
    repaint();
}

int CircuitSchematicPanel::chromeHeight() const noexcept
{
    int height = 16 + 20 + 4;
    if (signalOutputActiveLabel.isVisible())
        height += 20;
    if (signalOutputOtherLabel.isVisible())
        height += 18;
    return height;
}

int CircuitSchematicPanel::preferredWidthForHeight(int panelHeight) const noexcept
{
    const auto natural = svgView.getSvgNaturalBounds();
    if (natural.isEmpty() || natural.getHeight() <= 0.0f)
        return 280;

    const int svgHeight = juce::jmax(1, panelHeight - chromeHeight());
    const int svgWidth = juce::roundToInt(static_cast<float>(svgHeight) * natural.getWidth() / natural.getHeight());
    return juce::jmax(120, svgWidth + 20);
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
