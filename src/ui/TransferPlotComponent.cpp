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
                && frame_.phaseWrappedDeg.size() >= frame_.numBins
                && frame_.phaseUnwrappedDeg.size() >= frame_.numBins
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
    if (f.numBins < 2 || f.fftSize == 0 || f.sampleRate <= 0.0 || f.magnitudeDb.empty()) return 0;
    const double binHz = f.sampleRate / static_cast<double>(f.fftSize);
    const std::size_t maxBin = std::min(f.numBins - 1, f.magnitudeDb.size() - 1);
    if (maxBin < 1) return 0;
    return std::clamp<std::size_t>(
        static_cast<std::size_t>(std::llround(freqHz / binHz)), 1, maxBin);
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
            g.setFont(juce::FontOptions(11.0f, juce::Font::plain));
            g.drawText(label, static_cast<int>(x - 18.0f), static_cast<int>(area.getBottom() + 4.0f),
                       36, 14, juce::Justification::centred, false);
        }
    }
}

void TransferPlotComponent::drawMagnitudePanel(juce::Graphics& g, juce::Rectangle<float> area) const {
    g.setColour(Theme::panelBg);
    g.fillRoundedRectangle(area, 6.0f);
    drawFrequencyGrid(g, area, false);

    const float minDb = -magRangeDb_;
    const float maxDb = magRangeDb_;
    const float stepDb = (magRangeDb_ <= 12.0f) ? 3.0f : (magRangeDb_ <= 24.0f ? 6.0f : 12.0f);

    g.setFont(juce::FontOptions(10.5f, juce::Font::plain));
    for (float db = minDb; db <= maxDb + 0.01f; db += stepDb) {
        const float normY = (maxDb - db) / (maxDb - minDb);
        const float y = area.getY() + normY * area.getHeight();
        g.setColour(std::abs(db) < 0.01f ? Theme::zeroLine : Theme::gridMajor);
        g.drawHorizontalLine(static_cast<int>(y), area.getX(), area.getRight());

        // Draw labels with safe insets to avoid colliding with adjacent panels
        if (db > minDb + 0.1f) {
            float labelY = y - 7.0f;
            if (std::abs(db - maxDb) < 0.1f) labelY = area.getY() + 2.0f;

            g.setColour(std::abs(db) < 0.01f ? Theme::textPrimary : Theme::textSecondary);
            g.drawText(juce::String(db, 0) + " dB",
                       2, static_cast<int>(labelY), static_cast<int>(kLeftMargin - 8.0f), 14,
                       juce::Justification::centredRight, false);
        }
    }

    // Top-left Apple-style pill badge & legend
    const float badgeX = area.getX() + 8.0f;
    const float badgeY = area.getY() + 6.0f;
    g.setColour(Theme::badgeBg);
    g.fillRoundedRectangle(badgeX, badgeY, 110.0f, 18.0f, 4.0f);
    g.setColour(Theme::badgeBorder);
    g.drawRoundedRectangle(badgeX, badgeY, 110.0f, 18.0f, 4.0f, 1.0f);

    g.setColour(Theme::textPrimary);
    g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
    g.drawText("MAGNITUDE (dB)", static_cast<int>(badgeX), static_cast<int>(badgeY), 110, 18, juce::Justification::centred, false);

    // Legend items
    float legX = badgeX + 118.0f;
    auto drawLegendDot = [&](juce::Colour col, const char* name) {
        g.setColour(col);
        g.fillEllipse(legX, badgeY + 5.0f, 6.0f, 6.0f);
        legX += 10.0f;
        g.setColour(Theme::textSecondary);
        g.setFont(juce::FontOptions(10.0f, juce::Font::plain));
        g.drawText(name, static_cast<int>(legX), static_cast<int>(badgeY), 45, 18, juce::Justification::centredLeft, false);
        legX += 50.0f;
    };

    drawLegendDot(Theme::traceMagnitude, "H1 (Meas)");
    if (showH2_) drawLegendDot(Theme::traceH2, "H2");
    if (hasSnapshotA_ && showSnapshotA_) drawLegendDot(Theme::traceSnapshotA, "Snap A");
    if (hasSnapshotB_ && showSnapshotB_) drawLegendDot(Theme::traceSnapshotB, "Snap B");

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

    g.saveState();
    g.reduceClipRegion(area.toNearestInt());

    if (hasSnapshotA_ && showSnapshotA_) {
        buildMagPath(scratchPathSnapA_, snapshotA_, snapshotA_.magnitudeDb);
        g.setColour(Theme::traceSnapshotA.withAlpha(0.55f));
        g.strokePath(scratchPathSnapA_, juce::PathStrokeType(1.3f));
    }
    if (hasSnapshotB_ && showSnapshotB_) {
        buildMagPath(scratchPathSnapB_, snapshotB_, snapshotB_.magnitudeDb);
        g.setColour(Theme::traceSnapshotB.withAlpha(0.65f));
        g.strokePath(scratchPathSnapB_, juce::PathStrokeType(1.3f));
    }
    if (hasData_) {
        if (showH2_) {
            buildMagPath(scratchPathH2_, frame_, frame_.h2MagnitudeDb);
            g.setColour(Theme::traceH2.withAlpha(0.80f));
            g.strokePath(scratchPathH2_, juce::PathStrokeType(1.25f));
        }
        buildMagPath(scratchPathMain_, frame_, frame_.magnitudeDb);
        g.setColour(Theme::traceMagnitude);
        g.strokePath(scratchPathMain_, juce::PathStrokeType(1.85f));
    }

    g.restoreState();

    g.setColour(Theme::borderSubtle);
    g.drawRoundedRectangle(area, 6.0f, 1.0f);
}

void TransferPlotComponent::drawPhasePanel(juce::Graphics& g, juce::Rectangle<float> area) const {
    g.setColour(Theme::panelBg);
    g.fillRoundedRectangle(area, 6.0f);
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

    g.setFont(juce::FontOptions(10.5f, juce::Font::plain));
    for (int step = 0; step <= 4; ++step) {
        const float frac = static_cast<float>(step) / 4.0f;
        const float deg = maxDeg - frac * (maxDeg - minDeg);
        const float y = area.getY() + frac * area.getHeight();
        g.setColour(std::abs(deg) < 0.5f ? Theme::zeroLine : Theme::gridMajor);
        g.drawHorizontalLine(static_cast<int>(y), area.getX(), area.getRight());

        // Inset top and bottom labels so they stay strictly within phase panel
        float labelY = y - 7.0f;
        if (step == 0) labelY = area.getY() + 2.0f;
        else if (step == 4) labelY = area.getBottom() - 14.0f;

        g.setColour(std::abs(deg) < 0.5f ? Theme::textPrimary : Theme::textSecondary);
        g.drawText(juce::String(static_cast<int>(std::round(deg))) + juce::String::charToString(0x00B0),
                   2, static_cast<int>(labelY), static_cast<int>(kLeftMargin - 8.0f), 14,
                   juce::Justification::centredRight, false);
    }

    // Top-left pill badge
    const float badgeX = area.getX() + 8.0f;
    const float badgeY = area.getY() + 6.0f;
    g.setColour(Theme::badgeBg);
    g.fillRoundedRectangle(badgeX, badgeY, 130.0f, 18.0f, 4.0f);
    g.setColour(Theme::badgeBorder);
    g.drawRoundedRectangle(badgeX, badgeY, 130.0f, 18.0f, 4.0f, 1.0f);

    g.setColour(Theme::textPrimary);
    g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
    g.drawText(unwrapPhase_ ? "PHASE (UNWRAPPED)" : "PHASE (WRAPPED)",
               static_cast<int>(badgeX), static_cast<int>(badgeY), 130, 18, juce::Justification::centred, false);

    auto buildPhasePath = [&](juce::Path& p, const dsp::TransferFunctionFrame& f) {
        p.clear();
        const auto& phaseVec = unwrapPhase_ ? f.phaseUnwrappedDeg : f.phaseWrappedDeg;
        if (f.numBins < 2 || f.frequenciesHz.empty() || phaseVec.empty()) return;
        const std::size_t maxK = std::min({f.numBins, f.frequenciesHz.size(), phaseVec.size()});
        bool started = false;
        float prevDeg = 0.0f;

        for (std::size_t k = 1; k < maxK; ++k) {
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
    };

    g.saveState();
    g.reduceClipRegion(area.toNearestInt());

    if (hasSnapshotA_ && showSnapshotA_) {
        buildPhasePath(scratchPathSnapA_, snapshotA_);
        g.setColour(Theme::traceSnapshotA.withAlpha(0.50f));
        g.strokePath(scratchPathSnapA_, juce::PathStrokeType(1.2f));
    }
    if (hasSnapshotB_ && showSnapshotB_) {
        buildPhasePath(scratchPathSnapB_, snapshotB_);
        g.setColour(Theme::traceSnapshotB.withAlpha(0.60f));
        g.strokePath(scratchPathSnapB_, juce::PathStrokeType(1.2f));
    }
    if (hasData_) {
        buildPhasePath(scratchPathMain_, frame_);
        g.setColour(Theme::tracePhase);
        g.strokePath(scratchPathMain_, juce::PathStrokeType(1.6f));
    }

    g.restoreState();

    g.setColour(Theme::borderSubtle);
    g.drawRoundedRectangle(area, 6.0f, 1.0f);
}

void TransferPlotComponent::drawCoherencePanel(juce::Graphics& g, juce::Rectangle<float> area) const {
    g.setColour(Theme::panelBg);
    g.fillRoundedRectangle(area, 6.0f);
    drawFrequencyGrid(g, area, true);

    g.setFont(juce::FontOptions(10.5f, juce::Font::plain));
    for (float c : {0.0f, 0.25f, 0.5f, 0.75f, 1.0f}) {
        const float y = area.getY() + (1.0f - c) * area.getHeight();
        g.setColour(Theme::gridMajor);
        g.drawHorizontalLine(static_cast<int>(y), area.getX(), area.getRight());

        float labelY = y - 7.0f;
        if (c >= 0.99f) labelY = area.getY() + 2.0f;
        else if (c <= 0.01f) labelY = area.getBottom() - 14.0f;

        g.setColour(Theme::textSecondary);
        g.drawText(juce::String(c, 2),
                   2, static_cast<int>(labelY), static_cast<int>(kLeftMargin - 8.0f), 14,
                   juce::Justification::centredRight, false);
    }

    // Top-left pill badge
    const float badgeX = area.getX() + 8.0f;
    const float badgeY = area.getY() + 6.0f;
    g.setColour(Theme::badgeBg);
    g.fillRoundedRectangle(badgeX, badgeY, 110.0f, 18.0f, 4.0f);
    g.setColour(Theme::badgeBorder);
    g.drawRoundedRectangle(badgeX, badgeY, 110.0f, 18.0f, 4.0f, 1.0f);

    g.setColour(Theme::textPrimary);
    g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
    g.drawText("COHERENCE", static_cast<int>(badgeX), static_cast<int>(badgeY), 110, 18, juce::Justification::centred, false);

    g.saveState();
    g.reduceClipRegion(area.toNearestInt());

    auto drawCohTrace = [&](juce::Path& strokePath, juce::Path& fillPath, const dsp::TransferFunctionFrame& f, juce::Colour col, bool fill) {
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
                g.setColour(col.withAlpha(0.12f));
                g.fillPath(fillPath);
            }
            g.setColour(col);
            g.strokePath(strokePath, juce::PathStrokeType(fill ? 1.6f : 1.2f));
        }
    };

    if (hasSnapshotA_ && showSnapshotA_) {
        drawCohTrace(scratchPathSnapA_, scratchPathFill_, snapshotA_, Theme::traceSnapshotA.withAlpha(0.5f), false);
    }
    if (hasSnapshotB_ && showSnapshotB_) {
        drawCohTrace(scratchPathSnapB_, scratchPathFill_, snapshotB_, Theme::traceSnapshotB.withAlpha(0.6f), false);
    }
    if (hasData_) {
        drawCohTrace(scratchPathMain_, scratchPathFill_, frame_, Theme::traceCoherence, true);
    }

    g.restoreState();

    g.setColour(Theme::borderSubtle);
    g.drawRoundedRectangle(area, 6.0f, 1.0f);
}

void TransferPlotComponent::drawCursorReadout(juce::Graphics& g, juce::Rectangle<float> fullPlotArea) const {
    if (!hasData_) return;

    if (hasPinnedCursor_) {
        const float px = freqToX(pinnedFreqHz_, fullPlotArea.getX(), fullPlotArea.getWidth());
        g.setColour(Theme::traceSnapshotA.withAlpha(0.70f));
        g.drawVerticalLine(static_cast<int>(px), fullPlotArea.getY(), fullPlotArea.getBottom());
    }

    if (!mouseInside_) return;
    if (mousePos_.x < fullPlotArea.getX() || mousePos_.x > fullPlotArea.getRight()) return;

    // Subtle crosshair hairline
    g.setColour(Theme::textSecondary.withAlpha(0.40f));
    g.drawVerticalLine(static_cast<int>(mousePos_.x), fullPlotArea.getY(), fullPlotArea.getBottom());

    const double freq = xToFreq(mousePos_.x, fullPlotArea.getX(), fullPlotArea.getWidth());
    const std::size_t bin = freqToBin(frame_, freq);
    const auto& phaseVec = unwrapPhase_ ? frame_.phaseUnwrappedDeg : frame_.phaseWrappedDeg;
    if (bin >= frame_.magnitudeDb.size() || bin >= phaseVec.size() || bin >= frame_.coherence.size()) return;

    const double mag = frame_.magnitudeDb[bin];
    const double ph = phaseVec[bin];
    const double coh = frame_.coherence[bin];

    juce::String text = juce::String(freq, freq >= 1000.0 ? 0 : 1) + " Hz   |   "
        + juce::String(mag, 2) + " dB   |   "
        + juce::String(ph, 1) + " deg   |   Coh: "
        + juce::String(coh, 3);

    if (hasPinnedCursor_) {
        const std::size_t pinBin = freqToBin(frame_, pinnedFreqHz_);
        if (pinBin < frame_.magnitudeDb.size() && pinBin < phaseVec.size()) {
            const double dMag = mag - frame_.magnitudeDb[pinBin];
            const double pinPh = phaseVec[pinBin];
            const double dPh = ph - pinPh;
            text += "   [Delta "
                + juce::String(dMag, 2) + " dB, "
                + juce::String(dPh, 1) + " deg]";
        }
    }

    const float boxW = hasPinnedCursor_ ? 460.0f : 320.0f;
    const float boxH = 26.0f;
    const float boxX = std::min(mousePos_.x + 14.0f, fullPlotArea.getRight() - boxW - 8.0f);
    const float boxY = fullPlotArea.getY() + 8.0f;

    g.setColour(Theme::barBg.withAlpha(0.95f));
    g.fillRoundedRectangle(boxX, boxY, boxW, boxH, 6.0f);
    g.setColour(Theme::borderLightGray);
    g.drawRoundedRectangle(boxX, boxY, boxW, boxH, 6.0f, 1.0f);

    g.setColour(Theme::textPrimary);
    g.setFont(juce::FontOptions(11.5f, juce::Font::bold));
    g.drawText(text, static_cast<int>(boxX + 10), static_cast<int>(boxY),
               static_cast<int>(boxW - 20), static_cast<int>(boxH),
               juce::Justification::centredLeft, false);
}

void TransferPlotComponent::paint(juce::Graphics& g) {
    g.fillAll(Theme::backgroundDark);

    const auto bounds = getLocalBounds().toFloat();
    const float plotLeft = kLeftMargin;
    const float plotWidth = std::max(10.0f, bounds.getWidth() - kLeftMargin - kRightMargin);
    const float totalHeight = std::max(60.0f, bounds.getHeight() - 26.0f);

    constexpr float gap = 14.0f;
    const float usableH = totalHeight - 2.0f * gap;
    const float magH = usableH * 0.46f;
    const float phaseH = usableH * 0.30f;
    const float cohH = usableH * 0.24f;

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
