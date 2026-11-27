#pragma once

#include "dsp/DynamicsEngine.h"
#include "Theme.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <algorithm>
#include <cmath>

namespace ui {

/**
 * Laboratory-Grade Dynamics Transfer Curve Plot (PluginDoctor / Audio Precision style):
 * Plots Input dBFS (-60 dB to 0 dB) vs Output dBFS (-60 dB to 0 dB).
 * Features:
 *   - 1:1 unity gain reference diagonal
 *   - Shaded gain-reduction compression zone
 *   - Threshold knee point indicator
 *   - Apple-style real-time telemetry badges (Threshold, Ratio)
 *   - Interactive crosshair cursor with In/Out/GR readout
 *   - Zero heap memory allocations in paint()
 */
class DynamicsPlotComponent final : public juce::Component {
public:
    DynamicsPlotComponent() {
        setOpaque(true);
    }

    void updateFrame(const dsp::DynamicsFrame& frame) {
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

        constexpr float minDb = -60.0f;
        constexpr float maxDb = 0.0f;

        auto dbToX = [&](float db) noexcept {
            const float norm = std::clamp((db - minDb) / (maxDb - minDb), 0.0f, 1.0f);
            return area.getX() + norm * area.getWidth();
        };

        auto dbToY = [&](float db) noexcept {
            const float norm = std::clamp((maxDb - db) / (maxDb - minDb), 0.0f, 1.0f);
            return area.getY() + norm * area.getHeight();
        };

        auto xToDb = [&](float x) noexcept {
            if (area.getWidth() <= 1.0f) return minDb;
            const float norm = std::clamp((x - area.getX()) / area.getWidth(), 0.0f, 1.0f);
            return minDb + norm * (maxDb - minDb);
        };

        auto yToDb = [&](float y) noexcept {
            if (area.getHeight() <= 1.0f) return minDb;
            const float norm = std::clamp((area.getBottom() - y) / area.getHeight(), 0.0f, 1.0f);
            return minDb + norm * (maxDb - minDb);
        };

        // Draw dB Grid (-60 dB to 0 dB, 10 dB steps)
        g.setFont(juce::FontOptions(10.5f, juce::Font::plain));

        for (float db = minDb; db <= maxDb + 0.01f; db += 10.0f) {
            const float x = dbToX(db);
            const float y = dbToY(db);

            // Vertical & Horizontal Grid lines
            const bool isEdge = (std::abs(db - minDb) < 0.1f || std::abs(db - maxDb) < 0.1f);
            g.setColour(isEdge ? Theme::borderSubtle : Theme::gridMinor);
            g.drawVerticalLine(static_cast<int>(x), area.getY(), area.getBottom());
            g.drawHorizontalLine(static_cast<int>(y), area.getX(), area.getRight());

            // Y-axis label (Output dBFS)
            g.setColour(std::abs(db) < 0.1f ? Theme::textPrimary : Theme::textSecondary);
            g.drawText(juce::String(static_cast<int>(db)) + " dB",
                       2, static_cast<int>(y - 7.0f), static_cast<int>(kLeftMargin - 8.0f), 14,
                       juce::Justification::centredRight, false);

            // X-axis label (Input dBFS)
            g.drawText(juce::String(static_cast<int>(db)),
                       static_cast<int>(x - 18.0f), static_cast<int>(area.getBottom() + 4.0f), 36, 14,
                       juce::Justification::centred, false);
        }

        // X-axis axis label
        g.setColour(Theme::textMuted);
        g.setFont(juce::FontOptions(10.0f, juce::Font::plain));
        g.drawText("INPUT (dBFS)", static_cast<int>(area.getRight() - 90.0f), static_cast<int>(area.getBottom() + 4.0f), 90, 14, juce::Justification::centredRight, false);

        // 1:1 Unity Gain Reference Diagonal Line
        g.setColour(Theme::zeroLine.withAlpha(0.65f));
        g.drawLine(area.getX(), area.getBottom(), area.getRight(), area.getY(), 1.0f);

        // Top-left Apple-style pill badge
        const float badgeX = area.getX() + 8.0f;
        const float badgeY = area.getY() + 6.0f;
        g.setColour(Theme::badgeBg);
        g.fillRoundedRectangle(badgeX, badgeY, 180.0f, 20.0f, 4.0f);
        g.setColour(Theme::badgeBorder);
        g.drawRoundedRectangle(badgeX, badgeY, 180.0f, 20.0f, 4.0f, 1.0f);

        g.setColour(Theme::textPrimary);
        g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
        g.drawText("DYNAMICS TRANSFER CURVE", static_cast<int>(badgeX), static_cast<int>(badgeY), 180, 20, juce::Justification::centred, false);

        if (frame_.hasData && frame_.inputLevelsDb.size() > 1) {
            // Real-time Telemetry Badges on top right / center
            float pillX = badgeX + 190.0f;

            // Threshold Badge
            const juce::String threshStr = "THR: " + juce::String(frame_.estimatedThresholdDb, 1) + " dBFS";
            const float threshW = 110.0f;
            g.setColour(Theme::controlSurface);
            g.fillRoundedRectangle(pillX, badgeY, threshW, 20.0f, 4.0f);
            g.setColour(Theme::traceH2.withAlpha(0.6f));
            g.drawRoundedRectangle(pillX, badgeY, threshW, 20.0f, 4.0f, 1.0f);
            g.setColour(Theme::traceH2);
            g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
            g.drawText(threshStr, static_cast<int>(pillX), static_cast<int>(badgeY), static_cast<int>(threshW), 20, juce::Justification::centred, false);
            pillX += threshW + 8.0f;

            // Ratio Badge
            const juce::String ratioStr = "RATIO: " + juce::String(frame_.estimatedRatio, 1) + ":1";
            const float ratioW = 95.0f;
            g.setColour(Theme::controlSurface);
            g.fillRoundedRectangle(pillX, badgeY, ratioW, 20.0f, 4.0f);
            g.setColour(Theme::traceMagnitude.withAlpha(0.6f));
            g.drawRoundedRectangle(pillX, badgeY, ratioW, 20.0f, 4.0f, 1.0f);
            g.setColour(Theme::traceMagnitude);
            g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
            g.drawText(ratioStr, static_cast<int>(pillX), static_cast<int>(badgeY), static_cast<int>(ratioW), 20, juce::Justification::centred, false);

            // Build curve & shaded paths into preallocated scratch objects
            scratchCurve_.clear();
            scratchFill_.clear();
            scratchGrArea_.clear();

            bool started = false;
            float firstX = area.getX();
            float lastX = area.getX();

            for (std::size_t i = 0; i < frame_.inputLevelsDb.size(); ++i) {
                const float inDb = std::clamp(frame_.inputLevelsDb[i], minDb, maxDb);
                const float outDb = std::clamp(frame_.outputLevelsDb[i], minDb, maxDb);

                const float x = dbToX(inDb);
                const float y = dbToY(outDb);

                if (!started) {
                    scratchCurve_.startNewSubPath(x, y);
                    scratchFill_.startNewSubPath(x, area.getBottom());
                    scratchFill_.lineTo(x, y);

                    // Gain reduction area starts at unity diagonal (x, dbToY(inDb))
                    scratchGrArea_.startNewSubPath(x, dbToY(inDb));
                    scratchGrArea_.lineTo(x, y);

                    firstX = x;
                    started = true;
                } else {
                    scratchCurve_.lineTo(x, y);
                    scratchFill_.lineTo(x, y);
                    scratchGrArea_.lineTo(x, y);
                }
                lastX = x;
            }

            if (started) {
                // Close fill under curve
                scratchFill_.lineTo(lastX, area.getBottom());
                scratchFill_.lineTo(firstX, area.getBottom());
                scratchFill_.closeSubPath();

                // Close compression gain reduction area back along unity diagonal
                for (int i = static_cast<int>(frame_.inputLevelsDb.size()) - 1; i >= 0; --i) {
                    const float inDb = std::clamp(frame_.inputLevelsDb[static_cast<std::size_t>(i)], minDb, maxDb);
                    scratchGrArea_.lineTo(dbToX(inDb), dbToY(inDb));
                }
                scratchGrArea_.closeSubPath();

                g.saveState();
                g.reduceClipRegion(area.toNearestInt());

                // Subtle gain reduction compression shade (Amber/Coral tint)
                g.setColour(Theme::traceH2.withAlpha(0.10f));
                g.fillPath(scratchGrArea_);

                // Subtle gradient trace fill underneath
                g.setColour(Theme::traceMagnitude.withAlpha(0.08f));
                g.fillPath(scratchFill_);

                // Measured transfer curve trace
                g.setColour(Theme::traceMagnitude);
                g.strokePath(scratchCurve_, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

                // Threshold Knee marker
                if (frame_.estimatedThresholdDb >= minDb && frame_.estimatedThresholdDb <= maxDb) {
                    const float kx = dbToX(frame_.estimatedThresholdDb);
                    // Find closest output level for threshold
                    float ky = dbToY(frame_.estimatedThresholdDb);
                    for (std::size_t i = 0; i < frame_.inputLevelsDb.size(); ++i) {
                        if (frame_.inputLevelsDb[i] >= frame_.estimatedThresholdDb) {
                            ky = dbToY(frame_.outputLevelsDb[i]);
                            break;
                        }
                    }

                    // Glow circle
                    g.setColour(Theme::traceH2.withAlpha(0.35f));
                    g.fillEllipse(kx - 6.0f, ky - 6.0f, 12.0f, 12.0f);
                    g.setColour(Theme::traceH2);
                    g.fillEllipse(kx - 3.5f, ky - 3.5f, 7.0f, 7.0f);
                }

                g.restoreState();
            }
        } else {
            // Elegant Apple Waiting Placeholder
            g.setColour(Theme::textMuted);
            g.setFont(juce::FontOptions(13.0f, juce::Font::plain));
            g.drawText("Awaiting Signal \xE2\x80\x94 Feed dynamic audio or test ramp through DUT to map compression curve.",
                       area.toNearestInt(), juce::Justification::centred);
        }

        // Interactive Cursor Readout
        if (mouseInside_ && mousePos_.x >= area.getX() && mousePos_.x <= area.getRight()
            && mousePos_.y >= area.getY() && mousePos_.y <= area.getBottom()) {

            // Crosshair lines
            g.setColour(Theme::textSecondary.withAlpha(0.35f));
            g.drawVerticalLine(static_cast<int>(mousePos_.x), area.getY(), area.getBottom());
            g.drawHorizontalLine(static_cast<int>(mousePos_.y), area.getX(), area.getRight());

            const float cursorInDb = xToDb(mousePos_.x);
            const float cursorOutDb = yToDb(mousePos_.y);
            const float gainChangeDb = cursorOutDb - cursorInDb;

            juce::String grText = (gainChangeDb <= 0.0f)
                ? ("GR: " + juce::String(-gainChangeDb, 1) + " dB")
                : ("Boost: +" + juce::String(gainChangeDb, 1) + " dB");

            const juce::String hudText = "In: " + juce::String(cursorInDb, 1) + " dBFS   |   Out: "
                                       + juce::String(cursorOutDb, 1) + " dBFS   |   " + grText;

            const float hudW = 280.0f;
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

    dsp::DynamicsFrame frame_{};
    juce::Point<float> mousePos_{-1.0f, -1.0f};
    bool mouseInside_ = false;

    // Preallocated mutable scratch paths to prevent dynamic heap allocations in paint()
    mutable juce::Path scratchCurve_;
    mutable juce::Path scratchFill_;
    mutable juce::Path scratchGrArea_;
};

} // namespace ui
