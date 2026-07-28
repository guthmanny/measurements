#pragma once

#include <atomic>
#include <cstdint>

#include <juce_atom_theme/juce_atom_theme.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "BjtCurveMath.h"

class BjtCurvePanel final : public juce::Component,
                            private juce::AsyncUpdater
{
public:
    BjtCurvePanel();
    ~BjtCurvePanel() override;

    void setModel(nx_bjt_npn_model_e model);
    void setCircuit(bjt_curve::CircuitKind circuit);
    void setCurveKind(bjt_curve::CurveKind kind);
    void setIbSweep(const bjt_curve::IbSweepParams& params);
    void setVceMaxVolts(float vceMaxVolts);
    void setVbeMaxVolts(float vbeMaxVolts);
    void setCurrentMaxAmps(float currentMaxAmps);
    void applyTheme(const atom::ThemeColors& themeColors);

    void paint(juce::Graphics& g) override;
    void paintOverChildren(juce::Graphics& g) override;
    void resized() override;

private:
    struct RebuildParams
    {
        nx_bjt_npn_model_e model{NX_BJT_NPN};
        bjt_curve::CircuitKind circuit{bjt_curve::CircuitKind::CommonEmitter};
        bjt_curve::CurveKind curveKind{bjt_curve::CurveKind::IcVsVbe};
        bjt_curve::IbSweepParams ibSweep{};
        float vceMaxVolts{bjt_curve::kDefaultMaxVce};
        float vbeMaxVolts{bjt_curve::kDefaultMaxVbe};
        float currentMaxAmps{bjt_curve::kDefaultMaxIcAmps};
        uint32_t generation{0};
    };

    struct RebuildResult
    {
        uint32_t generation{0};
        bjt_curve::CurveKind curveKind{bjt_curve::CurveKind::IcVsVbe};
        bjt_curve::CircuitKind circuit{bjt_curve::CircuitKind::CommonEmitter};
        std::vector<float> ibValues;
        std::vector<std::pair<float, float>> samples;
        std::vector<std::vector<std::pair<float, float>>> curveFamilies;
        bjt_curve::DcLoadLineOverlay dcOverlay;
        bjt_curve::AxisRange axisRange;
        bjt_curve::CurrentDisplayUnit currentDisplayUnit{bjt_curve::CurrentDisplayUnit::Microamps};
        juce::String title;
        juce::String xLabel;
        juce::String yLabel;
        bool useFamilyPlot{false};
        bool unsupportedOutputCurve{false};
    };

    void scheduleRebuild();
    void handleAsyncUpdate() override;
    static RebuildResult computeRebuild(const RebuildParams& params);
    void applyRebuildResult(RebuildResult&& result);
    void paintFamilyCurves(juce::Graphics& g);
    void paintTransferQPoint(juce::Graphics& g);
    void paintBusyOverlay(juce::Graphics& g);

    juce::Rectangle<float> getPlotArea() const noexcept;
    float dataToX(float x) const noexcept;
    float dataToY(float y) const noexcept;

    nx_bjt_npn_model_e model_{NX_BJT_NPN};
    bjt_curve::CircuitKind circuit_{bjt_curve::CircuitKind::CommonEmitter};
    bjt_curve::CurveKind curveKind_{bjt_curve::CurveKind::IcVsVbe};
    bjt_curve::IbSweepParams ibSweep_;
    float vceMaxVolts_{bjt_curve::kDefaultMaxVce};
    float vbeMaxVolts_{bjt_curve::kDefaultMaxVbe};
    float currentMaxAmps_{bjt_curve::kDefaultMaxIcAmps};
    atom::CurveControl::Style style_;

    atom::CurveControl curveControl{atom::CurveControl::Direction::Speedup};
    std::vector<std::pair<float, float>> samples_;
    std::vector<std::vector<std::pair<float, float>>> curveFamilies_;
    std::vector<float> ibValues_;
    bjt_curve::DcLoadLineOverlay dcOverlay_;
    bjt_curve::AxisRange axisRange_;
    bjt_curve::CurrentDisplayUnit currentDisplayUnit_{bjt_curve::CurrentDisplayUnit::Microamps};
    juce::String title_;
    juce::String xLabel_{"Vbe (V)"};
    juce::String yLabel_{"Ic (uA)"};

    std::atomic<uint32_t> rebuildGeneration_{0};
    std::atomic<bool> rebuildInFlight_{false};
    bool busy_{false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BjtCurvePanel)
};
