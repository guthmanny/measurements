#pragma once

#include <functional>

#include <juce_atom_theme/juce_atom_theme.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "Ds1OpampAcMath.h"
#include "SchematicComponentValues.h"
#include "SchematicSignalOutput.h"

class CircuitSchematicPanel final : public juce::Component
{
public:
    CircuitSchematicPanel();

    void setPedalKind(ds1_ac::PedalKind pedalKind);
    void setCircuitKind(ds1_ac::CircuitKind circuitKind);
    void setSelection(ds1_ac::PedalKind pedalKind, ds1_ac::CircuitKind circuitKind);
    void setOpampModel(nx_opamp_model_e model);
    void setDiodeModel(nx_diode_model_t model);
    void setBjtModel(nx_bjt_npn_model_e model);
    void setJfetModel(nx_jfet_n_model_e model);
    nx_opamp_model_e getOpampModel() const noexcept { return opampModel_; }
    nx_diode_model_t getDiodeModel() const noexcept { return diodeModel_; }
    nx_bjt_npn_model_e getBjtModel() const noexcept { return bjtModel_; }
    nx_jfet_n_model_e getJfetModel() const noexcept { return jfetModel_; }
    void applyTheme(const atom::ThemeColors& themeColors);

    std::function<void(nx_opamp_model_e model)> onOpampModelChanged;
    std::function<void(nx_diode_model_t model)> onDiodeModelChanged;
    std::function<void(nx_bjt_npn_model_e model)> onBjtModelChanged;
    std::function<void(nx_jfet_n_model_e model)> onJfetModelChanged;
    std::function<void(const ds1_ac::SchematicComponentValues& values)> onComponentValuesChanged;

    const ds1_ac::SchematicComponentValues& getComponentValues() const noexcept { return componentValues_; }

    /** Panel width that keeps the loaded SVG's aspect ratio for the given height. */
    int preferredWidthForHeight(int panelHeight) const noexcept;

    std::function<void()> onPreferredSizeChanged;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void reloadSvg();
    void updateInternalToggleVisibility();
    void syncSignalOutputDisplay();
    void syncOpampModelFromSvg();
    void syncOpampOverlaySelectors();
    void syncDiodeModelFromSvg();
    void syncDiodeOverlaySelectors();
    void syncBjtModelFromSvg();
    void syncBjtOverlaySelectors();
    void syncJfetModelFromSvg();
    void syncJfetOverlaySelectors();
    void handleOpampOverlaySelection(const juce::String& overlayKey, int selectedId);
    void handleDiodeOverlaySelection(const juce::String& overlayKey, int selectedId);
    void handleBjtOverlaySelection(const juce::String& overlayKey, int selectedId);
    void handleJfetOverlaySelection(const juce::String& overlayKey, int selectedId);
    void handleOverlayTextChange(const juce::String& overlayKey, const juce::String& text);
    void notifyComponentValuesChanged();
    void applyOperatorValuesToOverlays();
    int chromeHeight() const noexcept;

    ds1_ac::PedalKind pedalKind_{ds1_ac::PedalKind::Ds1};
    ds1_ac::CircuitKind circuitKind_{ds1_ac::CircuitKind::BjtFollower};
    nx_opamp_model_e opampModel_{NX_OPAMP_BA728};
    nx_diode_model_t diodeModel_{NX_DIODE_1N4148};
    nx_bjt_npn_model_e bjtModel_{NX_BJT_2N3904};
    nx_jfet_n_model_e jfetModel_{NX_JFET_2N5457};
    ds1_ac::SchematicComponentValues componentValues_;
    atom::ThemeColors themeColors_;
    juce::String status_;

    atom::Label titleLabel;
    atom::Label signalOutputActiveLabel;
    atom::Label signalOutputOtherLabel;
    atom::ToggleButton internalToggle { "internalToggle", "Internal" };
    atom::SvgView svgView;
    bool showInternalSvg_ = false;
    schematic_assets::SignalOutputInfo signalOutputInfo_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CircuitSchematicPanel)
};
