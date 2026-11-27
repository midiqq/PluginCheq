#pragma once

#include "dsp/AnalysisTypes.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace ui {

/**
 * Teenage Engineering / Precision Lab Maximized Transfer Function Canvas:
 *   - 100% full-canvas vertical maximization (no wasted subpanel margins)
 *   - Magnitude (dB) primary trace (Sky Cyan)
 *   - Coherence (gamma^2) overlaid directly on the canvas (Fine Amber Gold)
 *   - Calibrated 0.0 - 1.0 Coherence scale on the right margin inside the grid
 *   - Symmetrical inner-grid dB and Hz micro-scales without external margin loss
 *   - Interactive Hover HUD: [ Freq (Hz) | Mag (dB) | Coh (0..1) ] with pinned delta cursor
 *   - Strictly real-time safe: ZERO dynamic allocations in paint()
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

    static constexpr double kMinFreqHz = 20.0;
    static constexpr double kMaxFreqHz = 20000.0;

private:
    float freqToX(double freqHz, float left, float width) const noexcept;
    double xToFreq(float x, float left, float width) const noexcept;
    std::size_t freqToBin(const dsp::TransferFunctionFrame& f, double freqHz) const noexcept;

    void drawFrequencyGrid(juce::Graphics& g, juce::Rectangle<float> area) const;
    void drawMagnitudeScale(juce::Graphics& g, juce::Rectangle<float> area) const;
    void drawCoherenceScale(juce::Graphics& g, juce::Rectangle<float> area) const;
    void drawHoverHUD(juce::Graphics& g, juce::Rectangle<float> area) const;

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
    mutable juce::Path scratchPathMag_;
    mutable juce::Path scratchPathCoh_;
    mutable juce::Path scratchPathCohFill_;
    mutable juce::Path scratchPathPhase_;
    mutable juce::Path scratchPathH2_;
    mutable juce::Path scratchPathSnapA_;
    mutable juce::Path scratchPathSnapB_;
};

} // namespace ui
