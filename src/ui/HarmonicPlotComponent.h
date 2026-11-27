#pragma once

#include "dsp/HarmonicEngine.h"
#include "Theme.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace ui {

/**
 * Harmonic & THD Analysis Plot Component (PluginDoctor style):
 * Displays full harmonic distortion spectrum (H1 fundamental, H2..H8 harmonics),
 * alongside THD % and THD+N dB readouts.
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

    void paint(juce::Graphics& g) override {
        g.fillAll(Theme::backgroundDark);

        const auto area = getLocalBounds().reduced(24, 16).toFloat();
        g.setColour(Theme::panelBg);
        g.fillRoundedRectangle(area, 8.0f);
        g.setColour(Theme::borderSubtle);
        g.drawRoundedRectangle(area, 8.0f, 1.0f);

        // Header & THD Badges
        g.setColour(Theme::textPrimary);
        g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
        g.drawText("HARMONIC DISTORTION & THD SPECTRUM", static_cast<int>(area.getX() + 12), static_cast<int>(area.getY() + 8), 300, 16, juce::Justification::centredLeft);

        if (frame_.hasData) {
            juce::String thdStr = "Fundamental: " + juce::String(frame_.fundamentalFreqHz, 0) + " Hz (" + juce::String(frame_.fundamentalLevelDb, 1) + " dBFS)  |  "
                                + "THD: " + juce::String(frame_.thdPercent, 2) + "% (" + juce::String(frame_.thdDb, 1) + " dB)  |  "
                                + "THD+N: " + juce::String(frame_.thdPlusNoisePercent, 2) + "%";

            g.setColour(Theme::traceCoherence);
            g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
            g.drawText(thdStr, static_cast<int>(area.getX() + 12), static_cast<int>(area.getY() + 26), 600, 16, juce::Justification::centredLeft);

            // Draw Logarithmic Spectrum from 20 Hz to 20 kHz
            constexpr float minDb = -120.0f;
            constexpr float maxDb = 0.0f;
            constexpr double minF = 20.0;
            constexpr double maxF = 20000.0;

            auto freqToX = [&](double f) {
                const double clamped = std::clamp(f, minF, maxF);
                const double norm = std::log10(clamped / minF) / std::log10(maxF / minF);
                return area.getX() + static_cast<float>(norm) * area.getWidth();
            };

            // Grid lines
            g.setFont(juce::FontOptions(10.5f, juce::Font::plain));
            for (float db = minDb; db <= maxDb; db += 20.0f) {
                const float normY = (maxDb - db) / (maxDb - minDb);
                const float y = area.getY() + 46.0f + normY * (area.getHeight() - 50.0f);
                g.setColour(Theme::gridMinor);
                g.drawHorizontalLine(static_cast<int>(y), area.getX(), area.getRight());
                g.setColour(Theme::textSecondary);
                g.drawText(juce::String(static_cast<int>(db)) + " dB", static_cast<int>(area.getX() + 4), static_cast<int>(y - 7), 50, 14, juce::Justification::centredLeft);
            }

            // Spectrum Path
            if (frame_.spectrumDb.size() > 2) {
                juce::Path p;
                bool started = false;
                for (std::size_t k = 1; k < frame_.spectrumDb.size(); ++k) {
                    const double f = frame_.spectrumFreqsHz[k];
                    if (f < minF || f > maxF) continue;

                    const float x = freqToX(f);
                    const float db = std::clamp(frame_.spectrumDb[k], minDb, maxDb);
                    const float normY = (maxDb - db) / (maxDb - minDb);
                    const float y = area.getY() + 46.0f + normY * (area.getHeight() - 50.0f);

                    if (!started) {
                        p.startNewSubPath(x, y);
                        started = true;
                    } else {
                        p.lineTo(x, y);
                    }
                }

                g.saveState();
                g.reduceClipRegion(area.toNearestInt());
                g.setColour(Theme::traceMagnitude.withAlpha(0.85f));
                g.strokePath(p, juce::PathStrokeType(1.5f));

                // Highlight harmonics H2..H8
                for (std::size_t h = 0; h < frame_.harmonicFreqsHz.size(); ++h) {
                    const float hf = frame_.harmonicFreqsHz[h];
                    if (hf < minF || hf > maxF) continue;
                    const float hx = freqToX(hf);
                    const float hdb = std::clamp(frame_.harmonicLevelsDb[h], minDb, maxDb);
                    const float hnormY = (maxDb - hdb) / (maxDb - minDb);
                    const float hy = area.getY() + 46.0f + hnormY * (area.getHeight() - 50.0f);

                    g.setColour(Theme::meterClip);
                    g.fillEllipse(hx - 3.5f, hy - 3.5f, 7.0f, 7.0f);
                    g.setColour(Theme::textPrimary);
                    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
                    g.drawText("H" + juce::String(static_cast<int>(h + 2)), static_cast<int>(hx - 10), static_cast<int>(hy - 16), 20, 12, juce::Justification::centred);
                }
                g.restoreState();
            }
        } else {
            g.setColour(Theme::textMuted);
            g.setFont(juce::FontOptions(13.0f, juce::Font::plain));
            g.drawText("Esperando señal senoidal o de prueba en la medición...", area.toNearestInt(), juce::Justification::centred);
        }
    }

private:
    dsp::HarmonicFrame frame_{};
};

} // namespace ui
