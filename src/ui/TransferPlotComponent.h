#pragma once

#include "dsp/AnalysisTypes.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace ui {

/**
 * High-Precision Multi-Panel Transfer Function Graph:
 *   1. Magnitude (dB) [H1 + optional H2 + A/B Snapshots]
 *   2. Phase (degrees, wrapped or unwrapped + A/B Snapshots)
 *   3. Coherence (gamma^2 in [0, 1] + A/B Snapshots)
 * Features Apple dark styling, interactive delta cursor, and zero heap allocations in paint().
 */
class TransferPlotComponent final : public juce::Component {
public:
    TransferPlotComponent();

    void updateFrame(const dsp::TransferFunctionFrame& frame);
    void setDisplayOptions(bool unwrapPhase, bool showH2, float magRangeDb);

    void setSnapshotA(const dsp::TransferFunctionFrame& frame);
    void setSnapshotB(const dsp::TransferFunctionFrame& frame);
    void setShowSnapshotA(bool show);
    void setShowSnapshotB(bool show);

    void paint(juce::Graphics& g) override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;

    // Common measurement graph layout constants across all views
    static constexpr double kMinFreqHz = 20.0;
    static constexpr double kMaxFreqHz = 20000.0;
    static constexpr float kLeftMargin = 58.0f;
    static constexpr float kRightMargin = 18.0f;

private:
    float freqToX(double freqHz, float left, float width) const noexcept;
    double xToFreq(float x, float left, float width) const noexcept;
    std::size_t freqToBin(const dsp::TransferFunctionFrame& f, double freqHz) const noexcept;

    void drawFrequencyGrid(juce::Graphics& g, juce::Rectangle<float> area, bool drawLabels) const;
    void drawMagnitudePanel(juce::Graphics& g, juce::Rectangle<float> area) const;
    void drawPhasePanel(juce::Graphics& g, juce::Rectangle<float> area) const;
    void drawCoherencePanel(juce::Graphics& g, juce::Rectangle<float> area) const;
    void drawCursorReadout(juce::Graphics& g, juce::Rectangle<float> fullPlotArea) const;

    dsp::TransferFunctionFrame frame_;
    dsp::TransferFunctionFrame snapshotA_;
    dsp::TransferFunctionFrame snapshotB_;
    bool hasData_ = false;
    bool hasSnapshotA_ = false;
    bool hasSnapshotB_ = false;
    bool showSnapshotA_ = true;
    bool showSnapshotB_ = true;

    bool unwrapPhase_ = false;
    bool showH2_ = false;
    float magRangeDb_ = 24.0f;

    juce::Point<float> mousePos_{-1.0f, -1.0f};
    bool mouseInside_ = false;
    bool hasPinnedCursor_ = false;
    double pinnedFreqHz_ = 1000.0;

    // Preallocated mutable scratch paths to prevent dynamic heap reallocations in paint()
    mutable juce::Path scratchPathMain_;
    mutable juce::Path scratchPathH2_;
    mutable juce::Path scratchPathSnapA_;
    mutable juce::Path scratchPathSnapB_;
    mutable juce::Path scratchPathFill_;
};

} // namespace ui
