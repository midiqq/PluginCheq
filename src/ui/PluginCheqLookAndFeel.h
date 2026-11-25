#pragma once

#include "Theme.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace ui {

/**
 * Minimalist Apple-inspired Dark + Light-Gray LookAndFeel for PluginCheq.
 * Flat surfaces, 1px hairline borders, pill buttons, clean sans-serif typography.
 */
class PluginCheqLookAndFeel final : public juce::LookAndFeel_V4 {
public:
    PluginCheqLookAndFeel() {
        setColour(juce::ResizableWindow::backgroundColourId, Theme::backgroundDark);
        setColour(juce::PopupMenu::backgroundColourId, Theme::barBg);
        setColour(juce::PopupMenu::textColourId, Theme::textPrimary);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, Theme::controlHover);
        setColour(juce::PopupMenu::highlightedTextColourId, Theme::textPrimary);
        setColour(juce::ComboBox::backgroundColourId, Theme::controlSurface);
        setColour(juce::ComboBox::textColourId, Theme::textPrimary);
        setColour(juce::ComboBox::outlineColourId, Theme::borderLightGray);
        setColour(juce::ComboBox::arrowColourId, Theme::textSecondary);
        setColour(juce::Slider::textBoxTextColourId, Theme::textPrimary);
        setColour(juce::Slider::textBoxBackgroundColourId, Theme::controlSurface);
        setColour(juce::Slider::textBoxOutlineColourId, Theme::borderSubtle);
    }

    void drawButtonBackground(juce::Graphics& g,
                              juce::Button& button,
                              const juce::Colour&,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override {
        const auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
        const bool toggled = button.getToggleState();

        juce::Colour bg = toggled ? Theme::controlActive : Theme::controlSurface;
        if (!toggled && (shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted)) {
            bg = Theme::controlHover;
        }

        g.setColour(bg);
        g.fillRoundedRectangle(bounds, 6.0f);

        g.setColour(toggled ? Theme::controlActive : Theme::borderLightGray);
        g.drawRoundedRectangle(bounds, 6.0f, 1.0f);
    }

    void drawButtonText(juce::Graphics& g,
                        juce::TextButton& button,
                        bool,
                        bool) override {
        const bool toggled = button.getToggleState();
        g.setColour(toggled ? Theme::controlActiveTxt : Theme::textPrimary);
        g.setFont(juce::FontOptions(12.0f, juce::Font::plain));
        g.drawFittedText(button.getButtonText(),
                         button.getLocalBounds().reduced(6, 2),
                         juce::Justification::centred, 1);
    }

    void drawToggleButton(juce::Graphics& g,
                          juce::ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted,
                          bool) override {
        const auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
        const bool toggled = button.getToggleState();

        g.setColour(toggled ? Theme::controlActive : (shouldDrawButtonAsHighlighted ? Theme::controlHover : Theme::controlSurface));
        g.fillRoundedRectangle(bounds, 6.0f);
        g.setColour(toggled ? Theme::controlActive : Theme::borderLightGray);
        g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

        g.setColour(toggled ? Theme::controlActiveTxt : Theme::textSecondary);
        g.setFont(juce::FontOptions(11.5f, juce::Font::plain));
        g.drawFittedText(button.getButtonText(),
                         button.getLocalBounds().reduced(6, 2),
                         juce::Justification::centred, 1);
    }

    void drawComboBox(juce::Graphics& g,
                      int width,
                      int height,
                      bool,
                      int,
                      int,
                      int,
                      int,
                      juce::ComboBox& box) override {
        const juce::Rectangle<float> bounds(0.5f, 0.5f, static_cast<float>(width) - 1.0f, static_cast<float>(height) - 1.0f);
        g.setColour(Theme::controlSurface);
        g.fillRoundedRectangle(bounds, 6.0f);
        g.setColour(Theme::borderLightGray);
        g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

        // Minimalist chevron
        const float arrowX = static_cast<float>(width) - 14.0f;
        const float arrowY = static_cast<float>(height) * 0.5f - 2.0f;
        juce::Path p;
        p.startNewSubPath(arrowX - 3.5f, arrowY);
        p.lineTo(arrowX, arrowY + 3.5f);
        p.lineTo(arrowX + 3.5f, arrowY);
        g.setColour(box.isEnabled() ? Theme::textSecondary : Theme::textMuted);
        g.strokePath(p, juce::PathStrokeType(1.3f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    void drawLinearSlider(juce::Graphics& g,
                          int x,
                          int y,
                          int width,
                          int height,
                          float sliderPos,
                          float,
                          float,
                          juce::Slider::SliderStyle,
                          juce::Slider&) override {
        const float trackY = static_cast<float>(y) + static_cast<float>(height) * 0.5f;
        const float left = static_cast<float>(x) + 4.0f;
        const float right = static_cast<float>(x + width) - 4.0f;

        g.setColour(Theme::controlSurface);
        g.fillRoundedRectangle(left, trackY - 2.0f, right - left, 4.0f, 2.0f);

        g.setColour(Theme::textSecondary);
        g.fillRoundedRectangle(left, trackY - 2.0f, std::max(0.0f, sliderPos - left), 4.0f, 2.0f);

        g.setColour(Theme::textPrimary);
        g.fillEllipse(sliderPos - 5.5f, trackY - 5.5f, 11.0f, 11.0f);
    }
};

} // namespace ui
