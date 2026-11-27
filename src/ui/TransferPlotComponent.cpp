#include "TransferPlotComponent.h"
#include "Theme.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace ui {

TransferPlotComponent::TransferPlotComponent() {
    setOpaque(true);
}

void TransferPlotComponent::updateFrame(const dsp::TransferFunctionFrame& frame) {
    frame_ = frame;
    hasData_ = (frame_.numBins > 2
                && frame_.frequenciesHz.size() >= frame_.numBins
                && frame_.magnitudeDb.size() >= frame_.numBins
                && frame_.coherence.size() >= frame_.numBins);
    repaint();
}

void TransferPlotComponent::setDisplayOptions(bool unwrapPhase, bool showH2, float magRangeDb) {
    if (unwrapPhase_ != unwrapPhase || showH2_ != showH2 || magRangeDb_ != magRangeDb) {
        unwrapPhase_ = unwrapPhase;
        showH2_ = showH2;
        magRangeDb_ = magRangeDb;
        repaint();
    }
}

void TransferPlotComponent::setSnapshotA(const dsp::TransferFunctionFrame& frame) {
    snapshotA_ = frame;
    hasSnapshotA_ = (snapshotA_.numBins > 2);
    repaint();
}

void TransferPlotComponent::setSnapshotB(const dsp::TransferFunctionFrame& frame) {
    snapshotB_ = frame;
    hasSnapshotB_ = (snapshotB_.numBins > 2);
    repaint();
}

void TransferPlotComponent::setShowSnapshotA(bool show) {
    if (showSnapshotA_ != show) {
        showSnapshotA_ = show;
        repaint();
    }
}

void TransferPlotComponent::setShowSnapshotB(bool show) {
    if (showSnapshotB_ != show) {
        showSnapshotB_ = show;
        repaint();
    }
}

void TransferPlotComponent::mouseMove(const juce::MouseEvent& event) {
    mousePos_ = event.position;
    mouseInside_ = true;
    repaint();
}

void TransferPlotComponent::mouseDown(const juce::MouseEvent& event) {
    if (event.mods.isRightButtonDown()) {
        hasPinnedCursor_ = false;
        repaint();
        return;
    }

    const auto area = getLocalBounds().toFloat().reduced(2.0f);
    if (area.contains(event.position)) {
        const double clickedFreq = xToFreq(event.position.x, area.getX(), area.getWidth());
        if (hasPinnedCursor_ && std::abs(std::log2(clickedFreq / pinnedFreqHz_)) < 0.04) {
            hasPinnedCursor_ = false;
        } else {
            pinnedFreqHz_ = clickedFreq;
            hasPinnedCursor_ = true;
        }
        repaint();
    }
}

void TransferPlotComponent::mouseExit(const juce::MouseEvent&) {
    mouseInside_ = false;
    repaint();
}

float TransferPlotComponent::freqToX(double freqHz, float left, float width) const noexcept {
    const double clamped = std::clamp(freqHz, kMinFreqHz, kMaxFreqHz);
    const double norm = std::log10(clamped / kMinFreqHz) / std::log10(kMaxFreqHz / kMinFreqHz);
    return left + static_cast<float>(norm) * width;
}

double TransferPlotComponent::xToFreq(float x, float left, float width) const noexcept {
    if (width <= 1.0f) return kMinFreqHz;
    const double norm = std::clamp(static_cast<double>((x - left) / width), 0.0, 1.0);
    return kMinFreqHz * std::pow(kMaxFreqHz / kMinFreqHz, norm);
}

std::size_t TransferPlotComponent::freqToBin(const dsp::TransferFunctionFrame& f, double freqHz) const noexcept {
    if (f.numBins < 2 || f.fftSize == 0 || f.sampleRate <= 0.0 || f.magnitudeDb.empty()) return 0;
    const double binHz = f.sampleRate / static_cast<double>(f.fftSize);
    const std::size_t maxBin = std::min(f.numBins - 1, f.magnitudeDb.size() - 1);
    if (maxBin < 1) return 0;
    return std::clamp<std::size_t>(
        static_cast<std::size_t>(std::llround(freqHz / binHz)), 1, maxBin);
}

void TransferPlotComponent::drawFrequencyGrid(juce::Graphics& g, juce::Rectangle<float> area) const {
    static constexpr std::array<double, 28> kGridFreqs = {
        20, 30, 40, 50, 60, 70, 80, 90,
        100, 200, 300, 400, 500, 600, 700, 800, 900,
        1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000,
        10000, 20000
    };
    static constexpr std::array<double, 10> kMajorFreqs = {
        20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000
    };

    const float left = area.getX();
    const float width = area.getWidth();
    const float bottom = area.getBottom();

    for (double f : kGridFreqs) {
        const float x = freqToX(f, left, width);
        const bool isMajor = std::find(kMajorFreqs.begin(), kMajorFreqs.end(), f) != kMajorFreqs.end();
        g.setColour(isMajor ? Theme::gridMajor : Theme::gridMinor);
        g.drawVerticalLine(static_cast<int>(x), area.getY(), bottom);

        if (isMajor) {
            juce::String label = (f >= 1000.0)
                ? juce::String(static_cast<int>(f / 1000.0)) + "k"
                : juce::String(static_cast<int>(f));

            g.setColour(Theme::textSecondary.withAlpha(0.60f));
            g.setFont(juce::FontOptions(9.5f, juce::Font::plain));
            // Render inside the grid near the bottom without taking external margins
            g.drawText(label, static_cast<int>(x - 18.0f), static_cast<int>(bottom - 16.0f),
                       36, 14, juce::Justification::centred, false);
        }
    }
}

void TransferPlotComponent::drawMagnitudeScale(juce::Graphics& g, juce::Rectangle<float> area) const {
    const float minDb = -magRangeDb_;
    const float maxDb = magRangeDb_;
    const float stepDb = (magRangeDb_ <= 12.0f) ? 3.0f : (magRangeDb_ <= 24.0f ? 6.0f : 12.0f);

    g.setFont(juce::FontOptions(9.5f, juce::Font::plain));

    for (float db = minDb; db <= maxDb + 0.01f; db += stepDb) {
        const float normY = (maxDb - db) / (maxDb - minDb);
        const float y = area.getY() + normY * area.getHeight();
        const bool isZero = std::abs(db) < 0.01f;

        g.setColour(isZero ? Theme::zeroLine : Theme::gridMajor);
        g.drawHorizontalLine(static_cast<int>(y), area.getX(), area.getRight());

        // Inner-grid dB labels rendered subtly on the left margin
        if (db > minDb + 0.1f && db < maxDb - 0.1f) {
            g.setColour(isZero ? Theme::textPrimary : Theme::textSecondary.withAlpha(0.70f));
            juce::String label = (db > 0.01f ? "+" : "") + juce::String(static_cast<int>(std::round(db))) + " dB";
            g.drawText(label, static_cast<int>(area.getX() + 6.0f), static_cast<int>(y - 7.0f),
                       46, 14, juce::Justification::centredLeft, false);
        }
    }
}

void TransferPlotComponent::drawCoherenceScale(juce::Graphics& g, juce::Rectangle<float> area) const {
    static constexpr std::array<float, 6> kCohSteps = {0.0f, 0.2f, 0.4f, 0.6f, 0.8f, 1.0f};

    g.setFont(juce::FontOptions(9.0f, juce::Font::plain));

    for (float c : kCohSteps) {
        const float normY = 1.0f - c;
        const float y = area.getY() + normY * area.getHeight();

        // Subtle tick mark on the right margin inside the grid
        g.setColour(Theme::traceH2.withAlpha(0.40f));
        g.drawHorizontalLine(static_cast<int>(y), area.getRight() - 6.0f, area.getRight());

        // Calibrated 0.0 - 1.0 coherence scale text rendered inside the right edge
        if (c > 0.01f && c < 0.99f) {
            g.setColour(Theme::traceH2.withAlpha(0.75f));
            g.drawText(juce::String(c, 1), static_cast<int>(area.getRight() - 26.0f), static_cast<int>(y - 7.0f),
                       22, 14, juce::Justification::centredRight, false);
        }
    }

    // Top-right coherence scale header badge
    g.setColour(Theme::traceH2.withAlpha(0.85f));
    g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
    g.drawText("COH (γ²)", static_cast<int>(area.getRight() - 54.0f), static_cast<int>(area.getY() + 8.0f),
               48, 14, juce::Justification::centredRight, false);
}

void TransferPlotComponent::drawHoverHUD(juce::Graphics& g, juce::Rectangle<float> area) const {
    if (!hasData_ || frame_.magnitudeDb.empty() || frame_.coherence.empty()) return;

    if (hasPinnedCursor_) {
        const float px = freqToX(pinnedFreqHz_, area.getX(), area.getWidth());
        g.setColour(Theme::traceSnapshotA.withAlpha(0.75f));
        g.drawVerticalLine(static_cast<int>(px), area.getY(), area.getBottom());
    }

    if (!mouseInside_) return;
    if (mousePos_.x < area.getX() || mousePos_.x > area.getRight()) return;

    // Crosshair hairline
    g.setColour(Theme::textSecondary.withAlpha(0.35f));
    g.drawVerticalLine(static_cast<int>(mousePos_.x), area.getY(), area.getBottom());

    const double freq = xToFreq(mousePos_.x, area.getX(), area.getWidth());
    const std::size_t bin = freqToBin(frame_, freq);
    if (bin >= frame_.magnitudeDb.size() || bin >= frame_.coherence.size()) return;

    const double mag = frame_.magnitudeDb[bin];
    const double coh = std::clamp(frame_.coherence[bin], 0.0, 1.0);

    // Micro-badge format: 'Freq (Hz) | Mag (dB) | Coh (0..1)'
    juce::String freqStr = (freq >= 1000.0)
        ? (juce::String(freq / 1000.0, 2) + " kHz")
        : (juce::String(static_cast<int>(std::round(freq))) + " Hz");

    juce::String magStr = (mag >= 0.0 ? "+" : "") + juce::String(mag, 2) + " dB";
    juce::String cohStr = "Coh " + juce::String(coh, 3);

    juce::String deltaStr;
    if (hasPinnedCursor_) {
        const std::size_t pinBin = freqToBin(frame_, pinnedFreqHz_);
        if (pinBin < frame_.magnitudeDb.size()) {
            const double dMag = mag - frame_.magnitudeDb[pinBin];
            deltaStr = juce::String("  [Δ ") + (dMag >= 0.0 ? "+" : "") + juce::String(dMag, 2) + " dB]";
        }
    }

    const float hudW = hasPinnedCursor_ ? 310.0f : 230.0f;
    const float hudH = 22.0f;
    const float hudX = std::clamp(mousePos_.x + 12.0f, area.getX() + 6.0f, area.getRight() - hudW - 6.0f);
    const float hudY = std::clamp(mousePos_.y - 28.0f, area.getY() + 6.0f, area.getBottom() - hudH - 20.0f);

    const juce::Rectangle<float> hudBounds(hudX, hudY, hudW, hudH);

    // TE Dark Capsule micro-badge
    g.setColour(Theme::barBg.withAlpha(0.95f));
    g.fillRoundedRectangle(hudBounds, 4.0f);
    g.setColour(Theme::borderLightGray);
    g.drawRoundedRectangle(hudBounds, 4.0f, 1.0f);

    // Multi-color micro-typography inside the badge
    const float textY = hudY + 2.0f;
    float curX = hudX + 8.0f;

    g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
    g.setColour(Theme::textPrimary);
    g.drawText(freqStr, static_cast<int>(curX), static_cast<int>(textY), 58, 18, juce::Justification::centredLeft, false);
    curX += 60.0f;

    g.setColour(Theme::textDim);
    g.drawText("|", static_cast<int>(curX), static_cast<int>(textY), 10, 18, juce::Justification::centred, false);
    curX += 12.0f;

    g.setColour(Theme::traceMagnitude);
    g.drawText(magStr, static_cast<int>(curX), static_cast<int>(textY), 62, 18, juce::Justification::centredLeft, false);
    curX += 64.0f;

    g.setColour(Theme::textDim);
    g.drawText("|", static_cast<int>(curX), static_cast<int>(textY), 10, 18, juce::Justification::centred, false);
    curX += 12.0f;

    g.setColour(Theme::traceH2);
    g.drawText(cohStr, static_cast<int>(curX), static_cast<int>(textY), 58, 18, juce::Justification::centredLeft, false);
    curX += 60.0f;

    if (hasPinnedCursor_ && deltaStr.isNotEmpty()) {
        g.setColour(Theme::traceSnapshotA);
        g.drawText(deltaStr, static_cast<int>(curX), static_cast<int>(textY), 76, 18, juce::Justification::centredLeft, false);
    }
}

void TransferPlotComponent::paint(juce::Graphics& g) {
    g.fillAll(Theme::backgroundDark);

    const auto area = getLocalBounds().toFloat().reduced(2.0f);
    if (area.getWidth() < 20.0f || area.getHeight() < 20.0f) return;

    // 1. Recessed full-canvas panel background
    g.setColour(Theme::panelBg);
    g.fillRoundedRectangle(area, 4.0f);

    // 2. Inner-grid Frequency & Amplitude Scales (Zero exterior margins)
    drawFrequencyGrid(g, area);
    drawMagnitudeScale(g, area);
    drawCoherenceScale(g, area);

    // 3. Top-Left Legend Badges
    const float badgeY = area.getY() + 7.0f;
    float legX = area.getX() + 10.0f;

    auto drawLegendPill = [&](juce::Colour col, const char* name, int w) {
        g.setColour(Theme::badgeBg);
        g.fillRoundedRectangle(legX, badgeY, static_cast<float>(w), 17.0f, 3.0f);
        g.setColour(Theme::badgeBorder);
        g.drawRoundedRectangle(legX, badgeY, static_cast<float>(w), 17.0f, 3.0f, 1.0f);

        g.setColour(col);
        g.fillEllipse(legX + 5.0f, badgeY + 5.5f, 6.0f, 6.0f);

        g.setColour(Theme::textPrimary);
        g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
        g.drawText(name, static_cast<int>(legX + 15.0f), static_cast<int>(badgeY), w - 17, 17, juce::Justification::centredLeft, false);
        legX += static_cast<float>(w + 6);
    };

    drawLegendPill(Theme::traceMagnitude, "MAG (H1)", 76);
    drawLegendPill(Theme::traceH2, "COHERENCE", 84);
    if (showH2_) drawLegendPill(Theme::traceH2.darker(0.2f), "H2", 42);
    if (hasSnapshotA_ && showSnapshotA_) drawLegendPill(Theme::traceSnapshotA, "SNAP A", 64);
    if (hasSnapshotB_ && showSnapshotB_) drawLegendPill(Theme::traceSnapshotB, "SNAP B", 64);

    const float minDb = -magRangeDb_;
    const float maxDb = magRangeDb_;

    // Helper lambdas to populate scratch paths without dynamic allocation
    auto buildMagPath = [&](juce::Path& p, const dsp::TransferFunctionFrame& f, const std::vector<double>& dbValues) {
        p.clear();
        if (f.numBins < 2 || f.frequenciesHz.empty() || dbValues.empty()) return;
        const std::size_t maxK = std::min({f.numBins, f.frequenciesHz.size(), dbValues.size()});
        bool started = false;
        for (std::size_t k = 1; k < maxK; ++k) {
            const double freq = f.frequenciesHz[k];
            if (freq < kMinFreqHz || freq > kMaxFreqHz) continue;

            const float x = freqToX(freq, area.getX(), area.getWidth());
            const float db = std::clamp(static_cast<float>(dbValues[k]), minDb, maxDb);
            const float normY = (maxDb - db) / (maxDb - minDb);
            const float y = area.getY() + normY * area.getHeight();

            if (!started) {
                p.startNewSubPath(x, y);
                started = true;
            } else {
                p.lineTo(x, y);
            }
        }
    };

    auto buildCohPath = [&](juce::Path& strokePath, juce::Path& fillPath, const dsp::TransferFunctionFrame& f) {
        strokePath.clear();
        fillPath.clear();
        if (f.numBins < 2 || f.frequenciesHz.empty() || f.coherence.empty()) return;
        const std::size_t maxK = std::min({f.numBins, f.frequenciesHz.size(), f.coherence.size()});
        bool started = false;
        float firstX = area.getX();
        float lastX = area.getX();

        for (std::size_t k = 1; k < maxK; ++k) {
            const double freq = f.frequenciesHz[k];
            if (freq < kMinFreqHz || freq > kMaxFreqHz) continue;

            const float x = freqToX(freq, area.getX(), area.getWidth());
            const float coh = std::clamp(static_cast<float>(f.coherence[k]), 0.0f, 1.0f);
            const float normY = 1.0f - coh;
            const float y = area.getY() + normY * area.getHeight();

            if (!started) {
                strokePath.startNewSubPath(x, y);
                fillPath.startNewSubPath(x, area.getBottom());
                fillPath.lineTo(x, y);
                firstX = x;
                started = true;
            } else {
                strokePath.lineTo(x, y);
                fillPath.lineTo(x, y);
            }
            lastX = x;
        }

        if (started) {
            fillPath.lineTo(lastX, area.getBottom());
            fillPath.lineTo(firstX, area.getBottom());
            fillPath.closeSubPath();
        }
    };

    g.saveState();
    g.reduceClipRegion(area.toNearestInt());

    // 4. Overlaid COHERENCE Curve (Directly on canvas with calibrated right scale)
    if (hasData_) {
        buildCohPath(scratchPathCoh_, scratchPathCohFill_, frame_);

        // Subtle amber underfill for depth
        g.setColour(Theme::traceH2.withAlpha(0.06f));
        g.fillPath(scratchPathCohFill_);

        // Fine golden / amber coherence stroke
        g.setColour(Theme::traceH2.withAlpha(0.85f));
        g.strokePath(scratchPathCoh_, juce::PathStrokeType(1.3f));
    }

    // 5. Phase trace (if unwrapping is enabled or selected)
    if (unwrapPhase_ && hasData_ && !frame_.phaseUnwrappedDeg.empty() && !frame_.frequenciesHz.empty()) {
        scratchPathPhase_.clear();
        const std::size_t maxK = std::min({frame_.numBins, frame_.frequenciesHz.size(), frame_.phaseUnwrappedDeg.size()});
        bool started = false;
        for (std::size_t k = 1; k < maxK; ++k) {
            const double freq = frame_.frequenciesHz[k];
            if (freq < kMinFreqHz || freq > kMaxFreqHz) continue;

            const float x = freqToX(freq, area.getX(), area.getWidth());
            const float deg = static_cast<float>(frame_.phaseUnwrappedDeg[k]);
            const float normY = std::clamp((180.0f - deg) / 360.0f, 0.0f, 1.0f);
            const float y = area.getY() + normY * area.getHeight();

            if (!started) {
                scratchPathPhase_.startNewSubPath(x, y);
                started = true;
            } else {
                scratchPathPhase_.lineTo(x, y);
            }
        }
        g.setColour(Theme::tracePhase.withAlpha(0.40f));
        g.strokePath(scratchPathPhase_, juce::PathStrokeType(1.2f));
    }

    // 6. Snapshots A and B
    if (hasSnapshotA_ && showSnapshotA_ && !snapshotA_.magnitudeDb.empty()) {
        buildMagPath(scratchPathSnapA_, snapshotA_, snapshotA_.magnitudeDb);
        g.setColour(Theme::traceSnapshotA.withAlpha(0.55f));
        g.strokePath(scratchPathSnapA_, juce::PathStrokeType(1.3f));
    }
    if (hasSnapshotB_ && showSnapshotB_ && !snapshotB_.magnitudeDb.empty()) {
        buildMagPath(scratchPathSnapB_, snapshotB_, snapshotB_.magnitudeDb);
        g.setColour(Theme::traceSnapshotB.withAlpha(0.65f));
        g.strokePath(scratchPathSnapB_, juce::PathStrokeType(1.3f));
    }

    // 7. Optional H2 curve
    if (hasData_ && showH2_ && !frame_.h2MagnitudeDb.empty()) {
        buildMagPath(scratchPathH2_, frame_, frame_.h2MagnitudeDb);
        g.setColour(Theme::traceH2.darker(0.2f).withAlpha(0.75f));
        g.strokePath(scratchPathH2_, juce::PathStrokeType(1.2f));
    }

    // 8. Main Magnitude Curve (Sky Cyan, crisp 2.0f stroke)
    if (hasData_) {
        buildMagPath(scratchPathMag_, frame_, frame_.magnitudeDb);
        g.setColour(Theme::traceMagnitude);
        g.strokePath(scratchPathMag_, juce::PathStrokeType(2.0f));
    }

    g.restoreState();

    // 9. Precision Outer Border
    g.setColour(Theme::borderSubtle);
    g.drawRoundedRectangle(area, 4.0f, 1.0f);

    // 10. Interactive Hover HUD
    drawHoverHUD(g, area);
}

} // namespace ui
