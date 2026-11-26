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
        g.fillAll(juce::Colours::black.withAlpha(0.65f));

        const auto dialogBounds = getLocalBounds().reduced(80, 50).toFloat();
        g.setColour(Theme::panelBg);
        g.fillRoundedRectangle(dialogBounds, 12.0f);
        g.setColour(Theme::borderLightGray);
        g.drawRoundedRectangle(dialogBounds, 12.0f, 1.5f);

        auto content = dialogBounds.reduced(24, 20);

        // Header
        g.setColour(Theme::textPrimary);
        g.setFont(juce::FontOptions(20.0f, juce::Font::bold));
        g.drawText("PluginCheq - Guía Rápida de Enrutamiento y Uso", content.removeFromTop(30).toNearestInt(), juce::Justification::centredLeft);

        g.setColour(Theme::textSecondary);
        g.setFont(juce::FontOptions(13.0f, juce::Font::plain));
        content.removeFromTop(10);

        auto drawSection = [&](const juce::String& title, const juce::String& desc) {
            g.setColour(Theme::traceMagnitude);
            g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
            g.drawText(title, content.removeFromTop(20).toNearestInt(), juce::Justification::centredLeft);

            g.setColour(Theme::textPrimary);
            g.setFont(juce::FontOptions(12.5f, juce::Font::plain));
            g.drawFittedText(desc, content.removeFromTop(50).toNearestInt(), juce::Justification::topLeft, 3);
            content.removeFromTop(8);
        };

        drawSection("1. ENRUTAMIENTO EN ABLETON LIVE (Plugin Tap Bus - Recomendado)",
                    "Inserta PluginCheq antes de tus plugins en modo 'Tap Sender (Pre-FX)'.\n"
                    "Inserta una segunda instancia al final en modo 'Plugin Tap Bus' (Receiver).\n"
                    "¡Listo! La señal limpia viaja internamente sin necesidad de configurar sidechain en el DAW.");

        drawSection("2. ENRUTAMIENTO ESTÁNDAR POR SIDECHAIN (Reaper, Cubase, Studio One)",
                    "Modo 'DAW Sidechain': Inserta PluginCheq al final de tu pista o en el Master.\n"
                    "Envía la señal limpia (pre-FX) a la entrada Sidechain del plugin.");

        drawSection("3. VERSIÓN STANDALONE Y PRUEBAS SIN DAW (Internal Generator)",
                    "Activa 'Internal Generator' (Ruido Rosa, Blanco o Sweep).\n"
                    "El plugin inyecta el estímulo de prueba directamente para calibrar hardware, cables o probar el analizador.");

        drawSection("4. BORROSIDAD EN ABLETON LIVE (High-DPI / Pantalla 4K)",
                    "Si la interfaz se ve borrosa en Windows, haz click derecho sobre la barra superior del plugin en el rack de Ableton y DESMARCA 'Auto-scale plug-in window'. Se verá 100% nítido.");
    }

    void resized() override {
        const auto dialogBounds = getLocalBounds().reduced(80, 50);
        closeButton_.setBounds(dialogBounds.getRight() - 70, dialogBounds.getY() + 16, 50, 24);
    }

private:
    juce::TextButton closeButton_{"Cerrar"};
};

} // namespace ui
