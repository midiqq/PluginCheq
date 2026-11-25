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
    hasData_ = (frame_.numBins > 2);
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

    const float plotLeft = kLeftMargin;
    const float plotWidth = std::max(10.0f, static_cast<float>(getWidth()) - kLeftMargin - kRightMargin);
    if (event.position.x >= plotLeft && event.position.x <= plotLeft + plotWidth) {
        const double clickedFreq = xToFreq(event.position.x, plotLeft, plotWidth);
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
    if (f.numBins < 2 || f.fftSize == 0 || f.sampleRate <= 0.0) return 0;
    const double binHz = f.sampleRate / static_cast<double>(f.fftSize);
    return std::clamp<std::size_t>(
        static_cast<std::size_t>(std::llround(freqHz / binHz)), 1, f.numBins - 1);
}

void TransferPlotComponent::drawFrequencyGrid(juce::Graphics& g,
                                              juce::Rectangle<float> area,
                                              bool drawLabels) const {
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

    for (double f : kGridFreqs) {
        const float x = freqToX(f, left, width);
        const bool isMajor = std::find(kMajorFreqs.begin(), kMajorFreqs.end(), f) != kMajorFreqs.end();
        g.setColour(isMajor ? Theme::gridMajor : Theme::gridMinor);
        g.drawVerticalLine(static_cast<int>(x), area.getY(), area.getBottom());

        if (drawLabels && isMajor) {
            juce::String label = (f >= 1000.0)
                ? juce::String(static_cast<int>(f / 1000.0)) + "k"
                : juce::String(static_cast<int>(f));
            g.setColour(Theme::textSecondary);
            g.setFont(11.0f);
            g.drawText(label, static_cast<int>(x - 18.0f), static_cast<int>(area.getBottom() + 2.0f),
                       36, 14, juce::Justification::centred, false);
        }
    }
}

void TransferPlotComponent::drawMagnitudePanel(juce::Graphics& g, juce::Rectangle<float> area) const {
    g.setColour(Theme::panelBg);
    g.fillRoundedRectangle(area, 8.0f);
    drawFrequencyGrid(g, area, false);

    const float minDb = -magRangeDb_;
    const float maxDb = magRangeDb_;
    const float stepDb = (magRangeDb_ <= 12.0f) ? 3.0f : (magRangeDb_ <= 24.0f ? 6.0f : 12.0f);

    g.setFont(11.0f);
    for (float db = minDb; db <= maxDb + 0.01f; db += stepDb) {
        const float normY = (maxDb - db) / (maxDb - minDb);
        const float y = area.getY() + normY * area.getHeight();
        g.setColour(std::abs(db) < 0.01f ? Theme::zeroLine : Theme::gridMajor);
        g.drawHorizontalLine(static_cast<int>(y), area.getX(), area.getRight());

        g.setColour(Theme::textSecondary);
        g.drawText(juce::String(db, 0) + " dB",
                   4, static_cast<int>(y - 7.0f), static_cast<int>(kLeftMargin - 10.0f), 14,
                   juce::Justification::centredRight, false);
    }

    g.setColour(Theme::textPrimary);
    g.setFont(12.0f);
    g.drawText("MAGNITUDE (H1)", static_cast<int>(area.getX() + 10), static_cast<int>(area.getY() + 6),
               140, 16, juce::Justification::centredLeft, false);

    auto buildMagPath = [&](const dsp::TransferFunctionFrame& f, const std::vector<double>& dbValues) {
        juce::Path p;
        bool started = false;
        for (std::size_t k = 1; k < f.numBins; ++k) {
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
        return p;
    };

    g.saveState();
    g.reduceClipRegion(area.toNearestInt());

    if (hasSnapshotA_ && showSnapshotA_) {
        g.setColour(Theme::traceSnapshotA.withAlpha(0.55f));
        g.strokePath(buildMagPath(snapshotA_, snapshotA_.magnitudeDb), juce::PathStrokeType(1.3f));
    }
    if (hasSnapshotB_ && showSnapshotB_) {
        g.setColour(Theme::traceSnapshotB.withAlpha(0.65f));
        g.strokePath(buildMagPath(snapshotB_, snapshotB_.magnitudeDb), juce::PathStrokeType(1.3f));
    }
    if (hasData_) {
        if (showH2_) {
            g.setColour(Theme::traceH2.withAlpha(0.75f));
            g.strokePath(buildMagPath(frame_, frame_.h2MagnitudeDb), juce::PathStrokeType(1.25f));
        }
        g.setColour(Theme::traceMagnitude);
        g.strokePath(buildMagPath(frame_, frame_.magnitudeDb), juce::PathStrokeType(1.75f));
    }

    g.restoreState();

    g.setColour(Theme::borderSubtle);
    g.drawRoundedRectangle(area, 8.0f, 1.0f);
}

void TransferPlotComponent::drawPhasePanel(juce::Graphics& g, juce::Rectangle<float> area) const {
    g.setColour(Theme::panelBg);
    g.fillRoundedRectangle(area, 8.0f);
    drawFrequencyGrid(g, area, false);

    float minDeg = -180.0f;
    float maxDeg = 180.0f;

    if (unwrapPhase_ && hasData_) {
        double pMin = 0.0, pMax = 0.0;
        for (std::size_t k = 1; k < frame_.numBins; ++k) {
            const double f = frame_.frequenciesHz[k];
            if (f < kMinFreqHz || f > kMaxFreqHz) continue;
            pMin = std::min(pMin, frame_.phaseUnwrappedDeg[k]);
            pMax = std::max(pMax, frame_.phaseUnwrappedDeg[k]);
        }
        minDeg = std::min(-180.0f, static_cast<float>(std::floor(pMin / 180.0) * 180.0));
        maxDeg = std::max(180.0f, static_cast<float>(std::ceil(pMax / 180.0) * 180.0));
    }

    g.setFont(11.0f);
    for (int step = 0; step <= 4; ++step) {
        const float frac = static_cast<float>(step) / 4.0f;
        const float deg = maxDeg - frac * (maxDeg - minDeg);
        const float y = area.getY() + frac * area.getHeight();
        g.setColour(std::abs(deg) < 0.5f ? Theme::zeroLine : Theme::gridMajor);
        g.drawHorizontalLine(static_cast<int>(y), area.getX(), area.getRight());

        g.setColour(Theme::textSecondary);
        g.drawText(juce::String(static_cast<int>(std::round(deg))) + juce::String::charToString(0x00B0),
                   4, static_cast<int>(y - 7.0f), static_cast<int>(kLeftMargin - 10.0f), 14,
                   juce::Justification::centredRight, false);
    }

    g.setColour(Theme::textPrimary);
    g.setFont(12.0f);
    g.drawText(unwrapPhase_ ? "PHASE (UNWRAPPED)" : "PHASE (WRAPPED)",
               static_cast<int>(area.getX() + 10), static_cast<int>(area.getY() + 6),
               160, 16, juce::Justification::centredLeft, false);

    auto buildPhasePath = [&](const dsp::TransferFunctionFrame& f) {
        const auto& phaseVec = unwrapPhase_ ? f.phaseUnwrappedDeg : f.phaseWrappedDeg;
        juce::Path p;
        bool started = false;
        float prevDeg = 0.0f;

        for (std::size_t k = 1; k < f.numBins; ++k) {
            const double freq = f.frequenciesHz[k];
            if (freq < kMinFreqHz || freq > kMaxFreqHz) continue;

            const float x = freqToX(freq, area.getX(), area.getWidth());
            const float deg = static_cast<float>(phaseVec[k]);
            const float normY = (maxDeg - deg) / std::max(1.0f, maxDeg - minDeg);
            const float y = area.getY() + normY * area.getHeight();

            if (!started || (!unwrapPhase_ && std::abs(deg - prevDeg) > 180.0f)) {
                p.startNewSubPath(x, y);
                started = true;
            } else {
                p.lineTo(x, y);
            }
            prevDeg = deg;
        }
        return p;
    };

    g.saveState();
    g.reduceClipRegion(area.toNearestInt());

    if (hasSnapshotA_ && showSnapshotA_) {
        g.setColour(Theme::traceSnapshotA.withAlpha(0.50f));
        g.strokePath(buildPhasePath(snapshotA_), juce::PathStrokeType(1.2f));
    }
    if (hasSnapshotB_ && showSnapshotB_) {
        g.setColour(Theme::traceSnapshotB.withAlpha(0.60f));
        g.strokePath(buildPhasePath(snapshotB_), juce::PathStrokeType(1.2f));
    }
    if (hasData_) {
        g.setColour(Theme::tracePhase);
        g.strokePath(buildPhasePath(frame_), juce::PathStrokeType(1.5f));
    }

    g.restoreState();

    g.setColour(Theme::borderSubtle);
    g.drawRoundedRectangle(area, 8.0f, 1.0f);
}

void TransferPlotComponent::drawCoherencePanel(juce::Graphics& g, juce::Rectangle<float> area) const {
    g.setColour(Theme::panelBg);
    g.fillRoundedRectangle(area, 8.0f);
    drawFrequencyGrid(g, area, true);

    g.setFont(11.0f);
    for (float c : {0.0f, 0.25f, 0.5f, 0.75f, 1.0f}) {
        const float y = area.getY() + (1.0f - c) * area.getHeight();
        g.setColour(Theme::gridMajor);
        g.drawHorizontalLine(static_cast<int>(y), area.getX(), area.getRight());
        g.setColour(Theme::textSecondary);
        g.drawText(juce::String(c, 2),
                   4, static_cast<int>(y - 7.0f), static_cast<int>(kLeftMargin - 10.0f), 14,
                   juce::Justification::centredRight, false);
    }

    g.setColour(Theme::textPrimary);
    g.setFont(12.0f);
    g.drawText("COHERENCE", static_cast<int>(area.getX() + 10), static_cast<int>(area.getY() + 6),
               120, 16, juce::Justification::centredLeft, false);

    g.saveState();
    g.reduceClipRegion(area.toNearestInt());

    auto drawCohTrace = [&](const dsp::TransferFunctionFrame& f, juce::Colour col, bool fill) {
        juce::Path strokePath;
        juce::Path fillPath;
        bool started = false;
        float firstX = area.getX();
        float lastX = area.getX();

        for (std::size_t k = 1; k < f.numBins; ++k) {
            const double freq = f.frequenciesHz[k];
            if (freq < kMinFreqHz || freq > kMaxFreqHz) continue;

            const float x = freqToX(freq, area.getX(), area.getWidth());
            const float coh = std::clamp(static_cast<float>(f.coherence[k]), 0.0f, 1.0f);
            const float y = area.getY() + (1.0f - coh) * area.getHeight();

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
            if (fill) {
                fillPath.lineTo(lastX, area.getBottom());
                fillPath.lineTo(firstX, area.getBottom());
                fillPath.closeSubPath();
                g.setColour(col.withAlpha(0.14f));
                g.fillPath(fillPath);
            }
            g.setColour(col);
            g.strokePath(strokePath, juce::PathStrokeType(fill ? 1.5f : 1.15f));
        }
    };

    if (hasSnapshotA_ && showSnapshotA_) {
        drawCohTrace(snapshotA_, Theme::traceSnapshotA.withAlpha(0.5f), false);
    }
    if (hasSnapshotB_ && showSnapshotB_) {
        drawCohTrace(snapshotB_, Theme::traceSnapshotB.withAlpha(0.6f), false);
    }
    if (hasData_) {
        drawCohTrace(frame_, Theme::traceCoherence, true);
    }

    g.restoreState();

    g.setColour(Theme::borderSubtle);
    g.drawRoundedRectangle(area, 8.0f, 1.0f);
}

void TransferPlotComponent::drawCursorReadout(juce::Graphics& g, juce::Rectangle<float> fullPlotArea) const {
    if (!hasData_) return;

    if (hasPinnedCursor_) {
        const float px = freqToX(pinnedFreqHz_, fullPlotArea.getX(), fullPlotArea.getWidth());
        g.setColour(Theme::traceSnapshotA.withAlpha(0.75f));
        g.drawVerticalLine(static_cast<int>(px), fullPlotArea.getY(), fullPlotArea.getBottom());
    }

    if (!mouseInside_) return;
    if (mousePos_.x < fullPlotArea.getX() || mousePos_.x > fullPlotArea.getRight()) return;

    g.setColour(Theme::textSecondary.withAlpha(0.55f));
    g.drawVerticalLine(static_cast<int>(mousePos_.x), fullPlotArea.getY(), fullPlotArea.getBottom());

    const double freq = xToFreq(mousePos_.x, fullPlotArea.getX(), fullPlotArea.getWidth());
    const std::size_t bin = freqToBin(frame_, freq);

    const double mag = frame_.magnitudeDb[bin];
    const double ph = unwrapPhase_ ? frame_.phaseUnwrappedDeg[bin] : frame_.phaseWrappedDeg[bin];
    const double coh = frame_.coherence[bin];

    juce::String text = juce::String(freq, freq >= 1000.0 ? 0 : 1) + " Hz  |  "
        + juce::String(mag, 2) + " dB  |  "
        + juce::String(ph, 1) + juce::String::charToString(0x00B0) + "  |  Coh: "
        + juce::String(coh, 3);

    if (hasPinnedCursor_) {
        const std::size_t pinBin = freqToBin(frame_, pinnedFreqHz_);
        const double dMag = mag - frame_.magnitudeDb[pinBin];
        const double pinPh = unwrapPhase_ ? frame_.phaseUnwrappedDeg[pinBin] : frame_.phaseWrappedDeg[pinBin];
        const double dPh = ph - pinPh;
        text += "   [" + juce::String::charToString(0x0394) + " "
            + juce::String(dMag, 2) + " dB, "
            + juce::String(dPh, 1) + juce::String::charToString(0x00B0) + "]";
    }

    const float boxW = hasPinnedCursor_ ? 450.0f : 315.0f;
    const float boxH = 24.0f;
    const float boxX = std::min(mousePos_.x + 12.0f, fullPlotArea.getRight() - boxW - 6.0f);
    const float boxY = fullPlotArea.getY() + 6.0f;

    g.setColour(Theme::barBg.withAlpha(0.94f));
    g.fillRoundedRectangle(boxX, boxY, boxW, boxH, 6.0f);
    g.setColour(Theme::borderLightGray);
    g.drawRoundedRectangle(boxX, boxY, boxW, boxH, 6.0f, 1.0f);

    g.setColour(Theme::textPrimary);
    g.setFont(11.5f);
    g.drawText(text, static_cast<int>(boxX + 8), static_cast<int>(boxY),
               static_cast<int>(boxW - 16), static_cast<int>(boxH),
               juce::Justification::centredLeft, false);
}

void TransferPlotComponent::paint(juce::Graphics& g) {
    g.fillAll(Theme::backgroundDark);

    const auto bounds = getLocalBounds().toFloat();
    const float plotLeft = kLeftMargin;
    const float plotWidth = std::max(10.0f, bounds.getWidth() - kLeftMargin - kRightMargin);
    const float totalHeight = std::max(60.0f, bounds.getHeight() - 26.0f);

    constexpr float gap = 6.0f;
    const float usableH = totalHeight - 2.0f * gap;
    const float magH = usableH * 0.48f;
    const float phaseH = usableH * 0.30f;
    const float cohH = usableH * 0.22f;

    const juce::Rectangle<float> magArea(plotLeft, 4.0f, plotWidth, magH);
    const juce::Rectangle<float> phaseArea(plotLeft, magArea.getBottom() + gap, plotWidth, phaseH);
    const juce::Rectangle<float> cohArea(plotLeft, phaseArea.getBottom() + gap, plotWidth, cohH);

    drawMagnitudePanel(g, magArea);
    drawPhasePanel(g, phaseArea);
    drawCoherencePanel(g, cohArea);

    const juce::Rectangle<float> fullPlotArea(plotLeft, magArea.getY(), plotWidth, cohArea.getBottom() - magArea.getY());
    drawCursorReadout(g, fullPlotArea);
}

} // namespace ui
