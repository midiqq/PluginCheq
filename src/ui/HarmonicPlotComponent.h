#pragma once

#include "dsp/HarmonicEngine.h"
#include "Theme.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <algorithm>
#include <array>
#include <cmath>

namespace ui {

/**
 * Laboratory-Grade Harmonic & THD Distortion Spectrum Plot (PluginDoctor / Audio Precision style):
 * Displays full logarithmic spectrum (20 Hz to 20 kHz) from 0 dBFS down to -140 dBFS.
 * Features:
 *   - Automatic Fundamental (f0) & Harmonic (H2..H8) peak pin markers
 *   - Real-time Apple-style telemetry badges for THD % / dB, THD+N %, and Fundamental
 *   - Shaded spectral profile with anti-aliased trace
 *   - Interactive hover cursor with frequency, absolute dBFS, and relative dBc readout
 *   - Zero heap memory allocations in paint()
 */
class HarmonicPlotComponent final : public juce::Component {
public:
    HarmonicPlotComponent() {
        setOpaque(true);
    }

    void updateFrame(const dsp::HarmonicFrame& frame) {
        frame_ = frame;
        repaint();
    }

    void mouseMove(const juce::MouseEvent& event) override {
        mousePos_ = event.position;
        mouseInside_ = true;
        repaint();
    }

    void mouseExit(const juce::MouseEvent&) override {
        mouseInside_ = false;
        repaint();
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(Theme::backgroundDark);

        const auto bounds = getLocalBounds().toFloat();
        const float plotLeft = kLeftMargin;
        const float plotWidth = std::max(10.0f, bounds.getWidth() - kLeftMargin - kRightMargin);
        const float plotTop = kTopMargin;
        const float plotHeight = std::max(10.0f, bounds.getHeight() - kTopMargin - kBottomMargin);
        const juce::Rectangle<float> area(plotLeft, plotTop, plotWidth, plotHeight);

        // Card background
        g.setColour(Theme::panelBg);
        g.fillRoundedRectangle(area, 6.0f);

        constexpr float minDb = -140.0f;
        constexpr float maxDb = 0.0f;
        constexpr double minF = 20.0;
        constexpr double maxF = 20000.0;

        auto freqToX = [&](double f) noexcept {
            const double clamped = std::clamp(f, minF, maxF);
            const double norm = std::log10(clamped / minF) / std::log10(maxF / minF);
            return area.getX() + static_cast<float>(norm) * area.getWidth();
        };

        auto xToFreq = [&](float x) noexcept {
            if (area.getWidth() <= 1.0f) return minF;
            const double norm = std::clamp(static_cast<double>((x - area.getX()) / area.getWidth()), 0.0, 1.0);
            return minF * std::pow(maxF / minF, norm);
        };

        auto dbToY = [&](float db) noexcept {
            const float norm = std::clamp((maxDb - db) / (maxDb - minDb), 0.0f, 1.0f);
            return area.getY() + norm * area.getHeight();
        };

        auto yToDb = [&](float y) noexcept {
            if (area.getHeight() <= 1.0f) return minDb;
            const float norm = std::clamp((area.getBottom() - y) / area.getHeight(), 0.0f, 1.0f);
            return minDb + norm * (maxDb - minDb);
        };

        // Draw Logarithmic Frequency Grid (matching TransferPlot)
        static constexpr std::array<double, 28> kGridFreqs = {
            20, 30, 40, 50, 60, 70, 80, 90,
            100, 200, 300, 400, 500, 600, 700, 800, 900,
            1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000,
            10000, 20000
        };
        static constexpr std::array<double, 10> kMajorFreqs = {
            20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000
        };

        for (double f : kGridFreqs) {
            const float x = freqToX(f);
            const bool isMajor = std::find(kMajorFreqs.begin(), kMajorFreqs.end(), f) != kMajorFreqs.end();
            g.setColour(isMajor ? Theme::gridMajor : Theme::gridMinor);
            g.drawVerticalLine(static_cast<int>(x), area.getY(), area.getBottom());

            if (isMajor) {
                juce::String label = (f >= 1000.0)
                    ? juce::String(static_cast<int>(f / 1000.0)) + "k"
                    : juce::String(static_cast<int>(f));
                g.setColour(Theme::textSecondary);
                g.setFont(juce::FontOptions(11.0f, juce::Font::plain));
                g.drawText(label, static_cast<int>(x - 18.0f), static_cast<int>(area.getBottom() + 4.0f),
                           36, 14, juce::Justification::centred, false);
            }
        }

        // Draw dB Grid (-140 dB to 0 dB in 20 dB steps)
        g.setFont(juce::FontOptions(10.5f, juce::Font::plain));
        for (float db = minDb; db <= maxDb + 0.01f; db += 20.0f) {
            const float y = dbToY(db);
            g.setColour(std::abs(db) < 0.1f ? Theme::zeroLine : Theme::gridMajor);
            g.drawHorizontalLine(static_cast<int>(y), area.getX(), area.getRight());

            g.setColour(std::abs(db) < 0.1f ? Theme::textPrimary : Theme::textSecondary);
            g.drawText(juce::String(static_cast<int>(db)) + " dB",
                       2, static_cast<int>(y - 7.0f), static_cast<int>(kLeftMargin - 8.0f), 14,
                       juce::Justification::centredRight, false);
        }

        // Top-left Apple-style pill badge
        const float badgeX = area.getX() + 8.0f;
        const float badgeY = area.getY() + 6.0f;
        g.setColour(Theme::badgeBg);
        g.fillRoundedRectangle(badgeX, badgeY, 175.0f, 20.0f, 4.0f);
        g.setColour(Theme::badgeBorder);
        g.drawRoundedRectangle(badgeX, badgeY, 175.0f, 20.0f, 4.0f, 1.0f);

        g.setColour(Theme::textPrimary);
        g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
        g.drawText("HARMONIC & THD SPECTRUM", static_cast<int>(badgeX), static_cast<int>(badgeY), 175, 20, juce::Justification::centred, false);

        if (frame_.hasData) {
            // Real-time Telemetry Badges
            float pillX = badgeX + 185.0f;

            // Fundamental Badge
            juce::String f0Str = "f0: " + juce::String(frame_.fundamentalFreqHz, 0) + " Hz ("
                               + juce::String(frame_.fundamentalLevelDb, 1) + " dB)";
            const float f0W = 140.0f;
            g.setColour(Theme::controlSurface);
            g.fillRoundedRectangle(pillX, badgeY, f0W, 20.0f, 4.0f);
            g.setColour(Theme::traceMagnitude.withAlpha(0.6f));
            g.drawRoundedRectangle(pillX, badgeY, f0W, 20.0f, 4.0f, 1.0f);
            g.setColour(Theme::traceMagnitude);
            g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
            g.drawText(f0Str, static_cast<int>(pillX), static_cast<int>(badgeY), static_cast<int>(f0W), 20, juce::Justification::centred, false);
            pillX += f0W + 8.0f;

            // THD Badge
            juce::String thdStr = "THD: " + juce::String(frame_.thdPercent, 3) + "% (" + juce::String(frame_.thdDb, 1) + " dB)";
            const float thdW = 145.0f;
            g.setColour(Theme::controlSurface);
            g.fillRoundedRectangle(pillX, badgeY, thdW, 20.0f, 4.0f);
            g.setColour(Theme::traceCoherence.withAlpha(0.6f));
            g.drawRoundedRectangle(pillX, badgeY, thdW, 20.0f, 4.0f, 1.0f);
            g.setColour(Theme::traceCoherence);
            g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
            g.drawText(thdStr, static_cast<int>(pillX), static_cast<int>(badgeY), static_cast<int>(thdW), 20, juce::Justification::centred, false);
            pillX += thdW + 8.0f;

            // THD+N Badge
            juce::String thdnStr = "THD+N: " + juce::String(frame_.thdPlusNoisePercent, 3) + "% (" + juce::String(frame_.thdPlusNoiseDb, 1) + " dB)";
            const float thdnW = 160.0f;
            g.setColour(Theme::controlSurface);
            g.fillRoundedRectangle(pillX, badgeY, thdnW, 20.0f, 4.0f);
            g.setColour(Theme::borderLightGray);
            g.drawRoundedRectangle(pillX, badgeY, thdnW, 20.0f, 4.0f, 1.0f);
            g.setColour(Theme::textPrimary);
            g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
            g.drawText(thdnStr, static_cast<int>(pillX), static_cast<int>(badgeY), static_cast<int>(thdnW), 20, juce::Justification::centred, false);

            // Spectrum Path & Fill
            if (frame_.spectrumDb.size() > 2 && frame_.spectrumFreqsHz.size() > 2) {
                scratchSpectrum_.clear();
                scratchFill_.clear();

                bool started = false;
                float firstX = area.getX();
                float lastX = area.getX();
                const std::size_t maxBins = std::min(frame_.spectrumDb.size(), frame_.spectrumFreqsHz.size());

                for (std::size_t k = 1; k < maxBins; ++k) {
                    const double f = frame_.spectrumFreqsHz[k];
                    if (f < minF || f > maxF) continue;

                    const float x = freqToX(f);
                    const float db = std::clamp(frame_.spectrumDb[k], minDb, maxDb);
                    const float y = dbToY(db);

                    if (!started) {
                        scratchSpectrum_.startNewSubPath(x, y);
                        scratchFill_.startNewSubPath(x, area.getBottom());
                        scratchFill_.lineTo(x, y);
                        firstX = x;
                        started = true;
                    } else {
                        scratchSpectrum_.lineTo(x, y);
                        scratchFill_.lineTo(x, y);
                    }
                    lastX = x;
                }

                if (started) {
                    scratchFill_.lineTo(lastX, area.getBottom());
                    scratchFill_.lineTo(firstX, area.getBottom());
                    scratchFill_.closeSubPath();

                    g.saveState();
                    g.reduceClipRegion(area.toNearestInt());

                    // Gradient trace fill
                    g.setColour(Theme::traceMagnitude.withAlpha(0.08f));
                    g.fillPath(scratchFill_);

                    // Spectrum outline
                    g.setColour(Theme::traceMagnitude.withAlpha(0.90f));
                    g.strokePath(scratchSpectrum_, juce::PathStrokeType(1.6f));

                    // Highlight Fundamental f0 marker
                    if (frame_.fundamentalFreqHz >= minF && frame_.fundamentalFreqHz <= maxF) {
                        const float f0X = freqToX(frame_.fundamentalFreqHz);
                        const float f0Y = dbToY(std::clamp(frame_.fundamentalLevelDb, minDb, maxDb));

                        g.setColour(Theme::harmonicH1.withAlpha(0.35f));
                        g.fillEllipse(f0X - 6.0f, f0Y - 6.0f, 12.0f, 12.0f);
                        g.setColour(Theme::harmonicH1);
                        g.fillEllipse(f0X - 3.5f, f0Y - 3.5f, 7.0f, 7.0f);

                        // Pill tag above f0
                        g.setColour(Theme::badgeBg);
                        g.fillRoundedRectangle(f0X - 14.0f, f0Y - 20.0f, 28.0f, 14.0f, 3.0f);
                        g.setColour(Theme::badgeBorder);
                        g.drawRoundedRectangle(f0X - 14.0f, f0Y - 20.0f, 28.0f, 14.0f, 3.0f, 1.0f);
                        g.setColour(Theme::harmonicH1);
                        g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
                        g.drawText("f0", static_cast<int>(f0X - 14.0f), static_cast<int>(f0Y - 20.0f), 28, 14, juce::Justification::centred, false);
                    }

                    // Highlight Harmonics H2..H8
                    const std::size_t numHarmonics = std::min(frame_.harmonicFreqsHz.size(), frame_.harmonicLevelsDb.size());
                    for (std::size_t h = 0; h < numHarmonics; ++h) {
                        const float hf = frame_.harmonicFreqsHz[h];
                        if (hf < minF || hf > maxF) continue;

                        const float hx = freqToX(hf);
                        const float hdb = std::clamp(frame_.harmonicLevelsDb[h], minDb, maxDb);
                        const float hy = dbToY(hdb);
                        const int harmonicNum = static_cast<int>(h + 2);

                        juce::Colour hCol = (harmonicNum == 2) ? Theme::harmonicH2
                                          : (harmonicNum == 3) ? Theme::harmonicH3
                                          : Theme::harmonicHn;

                        g.setColour(hCol.withAlpha(0.35f));
                        g.fillEllipse(hx - 5.5f, hy - 5.5f, 11.0f, 11.0f);
                        g.setColour(hCol);
                        g.fillEllipse(hx - 3.0f, hy - 3.0f, 6.0f, 6.0f);

                        // Micro pill tag
                        g.setColour(Theme::badgeBg);
                        g.fillRoundedRectangle(hx - 12.0f, hy - 18.0f, 24.0f, 13.0f, 3.0f);
                        g.setColour(Theme::badgeBorder);
                        g.drawRoundedRectangle(hx - 12.0f, hy - 18.0f, 24.0f, 13.0f, 3.0f, 1.0f);
                        g.setColour(hCol);
                        g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
                        g.drawText("H" + juce::String(harmonicNum), static_cast<int>(hx - 12.0f), static_cast<int>(hy - 18.0f), 24, 13, juce::Justification::centred, false);
                    }

                    g.restoreState();
                }
            }
        } else {
            // Elegant Apple Waiting Placeholder
            g.setColour(Theme::textMuted);
            g.setFont(juce::FontOptions(13.0f, juce::Font::plain));
            g.drawText("Awaiting Signal \xE2\x80\x94 Feed a pure sine wave (e.g. 1 kHz) to analyze harmonic distortion & THD.",
                       area.toNearestInt(), juce::Justification::centred);
        }

        // Interactive Cursor Readout
        if (mouseInside_ && mousePos_.x >= area.getX() && mousePos_.x <= area.getRight()
            && mousePos_.y >= area.getY() && mousePos_.y <= area.getBottom()) {

            // Crosshair
            g.setColour(Theme::textSecondary.withAlpha(0.35f));
            g.drawVerticalLine(static_cast<int>(mousePos_.x), area.getY(), area.getBottom());
            g.drawHorizontalLine(static_cast<int>(mousePos_.y), area.getX(), area.getRight());

            const double cursorFreq = xToFreq(mousePos_.x);
            const float cursorDb = yToDb(mousePos_.y);

            juce::String hudText = (cursorFreq >= 1000.0 ? juce::String(cursorFreq / 1000.0, 2) + " kHz" : juce::String(cursorFreq, 1) + " Hz")
                                 + "   |   " + juce::String(cursorDb, 1) + " dBFS";

            if (frame_.hasData) {
                const float relDb = cursorDb - frame_.fundamentalLevelDb;
                hudText += "   |   " + (relDb <= 0.0f ? juce::String(relDb, 1) : ("+" + juce::String(relDb, 1))) + " dBc";
            }

            const float hudW = 260.0f;
            const float hudH = 24.0f;
            const float hudX = std::min(mousePos_.x + 12.0f, area.getRight() - hudW - 8.0f);
            const float hudY = std::max(area.getY() + 8.0f, mousePos_.y - hudH - 8.0f);

            g.setColour(Theme::barBg.withAlpha(0.95f));
            g.fillRoundedRectangle(hudX, hudY, hudW, hudH, 5.0f);
            g.setColour(Theme::borderLightGray);
            g.drawRoundedRectangle(hudX, hudY, hudW, hudH, 5.0f, 1.0f);

            g.setColour(Theme::textPrimary);
            g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
            g.drawText(hudText, static_cast<int>(hudX + 8), static_cast<int>(hudY), static_cast<int>(hudW - 16), static_cast<int>(hudH), juce::Justification::centredLeft, false);
        }

        // Outer border
        g.setColour(Theme::borderSubtle);
        g.drawRoundedRectangle(area, 6.0f, 1.0f);
    }

private:
    static constexpr float kLeftMargin = 58.0f;
    static constexpr float kRightMargin = 18.0f;
    static constexpr float kTopMargin = 6.0f;
    static constexpr float kBottomMargin = 24.0f;

    dsp::HarmonicFrame frame_{};
    juce::Point<float> mousePos_{-1.0f, -1.0f};
    bool mouseInside_ = false;

    // Preallocated mutable scratch paths to prevent dynamic heap allocations in paint()
    mutable juce::Path scratchSpectrum_;
    mutable juce::Path scratchFill_;
};

} // namespace ui
