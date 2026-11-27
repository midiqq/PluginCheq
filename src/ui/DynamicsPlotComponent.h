#pragma once

#include "dsp/DynamicsEngine.h"
#include "Theme.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace ui {

/**
 * Dynamics Plot Component (PluginDoctor style):
 * Plots Input dBFS (-60 dB to 0 dB) vs Output dBFS (-60 dB to 0 dB).
 * Shows unity 1:1 gain diagonal, measured transfer curve, detected threshold and ratio.
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

    void paint(juce::Graphics& g) override {
        g.fillAll(Theme::backgroundDark);

        const auto area = getLocalBounds().reduced(24, 16).toFloat();
        g.setColour(Theme::panelBg);
        g.fillRoundedRectangle(area, 8.0f);
        g.setColour(Theme::borderSubtle);
        g.drawRoundedRectangle(area, 8.0f, 1.0f);

        // Draw dB Grid (-60 dB to 0 dB)
        constexpr float minDb = -60.0f;
        constexpr float maxDb = 0.0f;

        g.setFont(juce::FontOptions(11.0f, juce::Font::plain));

        for (float db = minDb; db <= maxDb; db += 10.0f) {
            const float normX = (db - minDb) / (maxDb - minDb);
            const float normY = (maxDb - db) / (maxDb - minDb);
            const float x = area.getX() + normX * area.getWidth();
            const float y = area.getY() + normY * area.getHeight();

            // Grid lines
            g.setColour(Theme::gridMinor);
            g.drawVerticalLine(static_cast<int>(x), area.getY(), area.getBottom());
            g.drawHorizontalLine(static_cast<int>(y), area.getX(), area.getRight());

            // Labels
            g.setColour(Theme::textSecondary);
            g.drawText(juce::String(static_cast<int>(db)), static_cast<int>(x - 14), static_cast<int>(area.getBottom() + 2), 28, 14, juce::Justification::centred);
            g.drawText(juce::String(static_cast<int>(db)), static_cast<int>(area.getX() - 30), static_cast<int>(y - 7), 26, 14, juce::Justification::centredRight);
        }

        // 1:1 Unity gain reference line (diagonal)
        g.setColour(Theme::zeroLine.withAlpha(0.6f));
        g.drawLine(area.getX(), area.getBottom(), area.getRight(), area.getY(), 1.0f);

        // Title and metrics badge
        g.setColour(Theme::textPrimary);
        g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
        g.drawText("DYNAMICS TRANSFER CURVE (RAMP)", static_cast<int>(area.getX() + 12), static_cast<int>(area.getY() + 8), 260, 16, juce::Justification::centredLeft);

        if (frame_.hasData) {
            juce::String metrics = "Threshold: " + juce::String(frame_.estimatedThresholdDb, 1) + " dBFS  |  Ratio: "
                                 + juce::String(frame_.estimatedRatio, 1) + ":1";
            g.setColour(Theme::traceH2);
            g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
            g.drawText(metrics, static_cast<int>(area.getX() + 12), static_cast<int>(area.getY() + 26), 340, 16, juce::Justification::centredLeft);

            // Draw measured curve
            if (frame_.inputLevelsDb.size() > 1) {
                juce::Path curve;
                bool started = false;

                for (std::size_t i = 0; i < frame_.inputLevelsDb.size(); ++i) {
                    const float inDb = std::clamp(frame_.inputLevelsDb[i], minDb, maxDb);
                    const float outDb = std::clamp(frame_.outputLevelsDb[i], minDb, maxDb);

                    const float normX = (inDb - minDb) / (maxDb - minDb);
                    const float normY = (maxDb - outDb) / (maxDb - minDb);
                    const float x = area.getX() + normX * area.getWidth();
                    const float y = area.getY() + normY * area.getHeight();

                    if (!started) {
                        curve.startNewSubPath(x, y);
                        started = true;
                    } else {
                        curve.lineTo(x, y);
                    }
                }

                g.saveState();
                g.reduceClipRegion(area.toNearestInt());
                g.setColour(Theme::traceMagnitude);
                g.strokePath(curve, juce::PathStrokeType(2.2f));
                g.restoreState();
            }
        } else {
            g.setColour(Theme::textMuted);
            g.setFont(juce::FontOptions(13.0f, juce::Font::plain));
            g.drawText("Reproduce audio a través de la cadena para medir la curva dinámica...", area.toNearestInt(), juce::Justification::centred);
        }
    }

private:
    dsp::DynamicsFrame frame_{};
};

} // namespace ui
