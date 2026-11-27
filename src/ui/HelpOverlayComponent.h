#pragma once

#include "Theme.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace ui {

class HelpOverlayComponent final : public juce::Component {
public:
    HelpOverlayComponent() {
        setOpaque(false);
        closeButton_.onClick = [this] { setVisible(false); };
        addAndMakeVisible(closeButton_);
    }

    void paint(juce::Graphics& g) override {
        // Dim background
        g.fillAll(juce::Colours::black.withAlpha(0.70f));

        const auto dialogBounds = getLocalBounds().reduced(60, 40).toFloat();
        g.setColour(Theme::panelBg);
        g.fillRoundedRectangle(dialogBounds, 12.0f);
        g.setColour(Theme::borderLightGray);
        g.drawRoundedRectangle(dialogBounds, 12.0f, 1.5f);

        auto content = dialogBounds.reduced(24, 20);

        // Header
        g.setColour(Theme::textPrimary);
        g.setFont(juce::FontOptions(19.0f, juce::Font::bold));
        g.drawText("Guía Rápida: ¿Cómo conectar y usar PluginCheq?", content.removeFromTop(28).toNearestInt(), juce::Justification::centredLeft);

        content.removeFromTop(8);

        auto drawSection = [&](const juce::String& title, const juce::String& desc) {
            g.setColour(Theme::traceMagnitude);
            g.setFont(juce::FontOptions(13.5f, juce::Font::bold));
            g.drawText(title, content.removeFromTop(18).toNearestInt(), juce::Justification::centredLeft);

            g.setColour(Theme::textPrimary);
            g.setFont(juce::FontOptions(12.0f, juce::Font::plain));
            g.drawFittedText(desc, content.removeFromTop(58).toNearestInt(), juce::Justification::topLeft, 3);
            content.removeFromTop(6);
        };

        drawSection("MÉTODO 1: EN ABLETON LIVE (Usando 2 PluginCheq en la misma pista)",
                    "1. Pon un PluginCheq AL PRINCIPIO de tu pista en modo: 'Tap Sender (Pre-FX)'.\n"
                    "2. En medio pon tus plugins de EQ, Compresión, Saturación, etc.\n"
                    "3. Pon un segundo PluginCheq AL FINAL de la pista en modo: 'Plugin Tap Bus (Ableton)'.\n"
                    "¡Listo! La señal limpia viaja sola por dentro. Presiona '⚡ Auto-Align Delay' para clavar la fase.");

        drawSection("MÉTODO 2: SIDECHAIN TRADICIONAL (1 solo PluginCheq al final)",
                    "Modo 'DAW Sidechain': Inserta un único PluginCheq al final de tu cadena.\n"
                    "En tu DAW, rutea la señal previa al procesamiento al bus Sidechain de PluginCheq.\n"
                    "Verás el indicador en verde 'SIDECHAIN: OK'.");

        drawSection("MÉTODO 3: MODO STANDALONE (Sin DAW, calibrando hardware o conversores)",
                    "Abre PluginCheq.exe con tu interfaz de audio configurada.\n"
                    "Envía la señal de referencia al canal de entrada correspondiente y el retorno medido.");

        drawSection("LOS 3 BOTONES CLAVE",
                    "• ⚡ Auto-Align Delay: Elimina el retardo de plugins/conversores y aplana la fase.\n"
                    "• Smoothing (1/12 Oct): Limpia los piquitos ruidosos y deja la curva suave como un EQ.\n"
                    "• Save A / Save B: Compara dos curvas o configuraciones distintas superpuestas.");
    }

    void resized() override {
        const auto dialogBounds = getLocalBounds().reduced(60, 40);
        closeButton_.setBounds(dialogBounds.getRight() - 76, dialogBounds.getY() + 16, 60, 24);
    }

private:
    juce::TextButton closeButton_{"Cerrar"};
};

} // namespace ui
