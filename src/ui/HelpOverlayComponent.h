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
        g.fillAll(juce::Colours::black.withAlpha(0.72f));

        const auto dialogBounds = getLocalBounds().reduced(60, 40).toFloat();
        g.setColour(Theme::panelBg);
        g.fillRoundedRectangle(dialogBounds, 12.0f);
        g.setColour(Theme::borderLightGray);
        g.drawRoundedRectangle(dialogBounds, 12.0f, 1.5f);

        auto content = dialogBounds.reduced(28, 22);

        // Header (strictly UTF-8 decoded via fromUTF8 so accents never corrupt)
        g.setColour(Theme::textPrimary);
        g.setFont(juce::FontOptions(18.5f, juce::Font::bold));
        g.drawText(juce::String::fromUTF8("Guía Rápida: ¿Cómo conectar y usar PluginCheq?"),
                   content.removeFromTop(28.0f).toNearestInt(),
                   juce::Justification::centredLeft);

        content.removeFromTop(10.0f);

        auto drawSection = [&](const char* utf8Title, const char* utf8Desc, float descHeight) {
            g.setColour(Theme::traceMagnitude);
            g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
            g.drawText(juce::String::fromUTF8(utf8Title),
                       content.removeFromTop(20.0f).toNearestInt(),
                       juce::Justification::centredLeft);

            g.setColour(Theme::textPrimary);
            g.setFont(juce::FontOptions(12.0f, juce::Font::plain));
            g.drawFittedText(juce::String::fromUTF8(utf8Desc),
                             content.removeFromTop(descHeight).toNearestInt(),
                             juce::Justification::topLeft,
                             4);
            content.removeFromTop(10.0f);
        };

        drawSection("MÉTODO 1: EN ABLETON LIVE / FL STUDIO (2 instancias en la misma pista)",
                    "1. Inserta un primer PluginCheq ANTES de tus efectos en modo: 'Tap Sender (Pre-FX)'.\n"
                    "2. En medio coloca los plugins que quieras analizar (EQ, Compresor, Saturador).\n"
                    "3. Inserta un segundo PluginCheq AL FINAL de la pista en modo: 'Plugin Tap Bus (Ableton)'.\n"
                    "La señal limpia viaja internamente en tiempo real. Pulsa 'Auto-Align Delay' para compensar latencia.",
                    68.0f);

        drawSection("MÉTODO 2: SIDECHAIN TRADICIONAL (Reaper, Cubase, Studio One)",
                    "1. Inserta un único PluginCheq al final de la cadena en modo 'DAW Sidechain'.\n"
                    "2. Envía la señal limpia previa (Pre-FX) a la entrada Sidechain del plugin (verás 'SIDECHAIN: OK').",
                    44.0f);

        drawSection("CONTROLES Y VISTAS PRINCIPALES",
                    "- Linear / EQ: Magnitud (dB), Fase (envuelta o desenrollada) y Coherencia lineal.\n"
                    "- Dynamics & Harmonics / THD: Curva de compresión (Threshold/Ratio) y espectro armónico H2..H8.\n"
                    "- Auto-Align Delay, Smoothing (1/24 a 1/1 Oct) y Save A / Save B para comparar ajustes.",
                    58.0f);

        drawSection("NITIDEZ EN PANTALLAS ESCALADAS (ABLETON LIVE)",
                    "Haz click derecho sobre la barra superior del dispositivo PluginCheq en Ableton y desmarca\n"
                    "'Auto-scale plug-in window' para obtener renderizado vectorial 1:1 ultra nítido.",
                    40.0f);
    }

    void resized() override {
        const auto dialogBounds = getLocalBounds().reduced(60, 40);
        closeButton_.setBounds(dialogBounds.getRight() - 80, dialogBounds.getY() + 18, 62, 24);
    }

private:
    juce::TextButton closeButton_{"Cerrar"};
};

} // namespace ui
