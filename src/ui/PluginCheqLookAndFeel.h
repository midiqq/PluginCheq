#pragma once

#include "Theme.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <algorithm>

namespace ui {

/**
 * Minimalist Apple-inspired Pro Dark LookAndFeel for PluginCheq.
 * Features flat precision surfaces, 1px micro-borders, elegant pill buttons,
 * crisp custom chevrons, and refined typography.
 */
class PluginCheqLookAndFeel final : public juce::LookAndFeel_V4 {
public:
    PluginCheqLookAndFeel() {
        setColour(juce::ResizableWindow::backgroundColourId, Theme::backgroundDark);
        setColour(juce::PopupMenu::backgroundColourId, Theme::panelBg);
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
        setColour(juce::Slider::textBoxHighlightColourId, Theme::controlHover);
    }

    void drawButtonBackground(juce::Graphics& g,
                              juce::Button& button,
                              const juce::Colour&,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override {
        const auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
        const bool toggled = button.getToggleState();
        const bool isOrange = (button.getComponentID() == "TE_ORANGE");

        juce::Colour bg = toggled ? Theme::controlActive : Theme::controlSurface;
        if (isOrange) {
            bg = Theme::teOrange;
            if (shouldDrawButtonAsDown) bg = bg.darker(0.25f);
            else if (shouldDrawButtonAsHighlighted) bg = bg.brighter(0.12f);
        } else if (!toggled) {
            if (shouldDrawButtonAsDown) {
                bg = Theme::controlHover.darker(0.15f);
            } else if (shouldDrawButtonAsHighlighted) {
                bg = Theme::controlHover;
            }
        }

        constexpr float cornerRadius = 4.0f;
        g.setColour(bg);
        g.fillRoundedRectangle(bounds, cornerRadius);

        juce::Colour outline = toggled ? Theme::controlActive : (shouldDrawButtonAsHighlighted ? Theme::borderActive : Theme::borderLightGray);
        if (isOrange) {
            outline = shouldDrawButtonAsHighlighted ? juce::Colours::white.withAlpha(0.7f) : Theme::teOrange.darker(0.2f);
        }
        g.setColour(outline);
        g.drawRoundedRectangle(bounds, cornerRadius, 1.0f);
    }

    void drawButtonText(juce::Graphics& g,
                        juce::TextButton& button,
                        bool,
                        bool) override {
        const bool toggled = button.getToggleState();
        const bool isOrange = (button.getComponentID() == "TE_ORANGE");
        g.setColour(isOrange ? juce::Colours::white : (toggled ? Theme::controlActiveTxt : Theme::textPrimary));
        g.setFont(juce::FontOptions(11.0f, (toggled || isOrange) ? juce::Font::bold : juce::Font::plain));
        g.drawFittedText(button.getButtonText(),
                         button.getLocalBounds().reduced(4, 2),
                         juce::Justification::centred, 1);
    }

    void drawToggleButton(juce::Graphics& g,
                          juce::ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted,
                          bool shouldDrawButtonAsDown) override {
        const auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
        const bool toggled = button.getToggleState();

        juce::Colour bg = toggled ? Theme::controlActive : Theme::controlSurface;
        if (!toggled) {
            if (shouldDrawButtonAsDown) {
                bg = Theme::controlHover.darker(0.15f);
            } else if (shouldDrawButtonAsHighlighted) {
                bg = Theme::controlHover;
            }
        }

        constexpr float cornerRadius = 5.0f;
        g.setColour(bg);
        g.fillRoundedRectangle(bounds, cornerRadius);

        g.setColour(toggled ? Theme::controlActive : (shouldDrawButtonAsHighlighted ? Theme::borderActive : Theme::borderLightGray));
        g.drawRoundedRectangle(bounds, cornerRadius, 1.0f);

        g.setColour(toggled ? Theme::controlActiveTxt : (shouldDrawButtonAsHighlighted ? Theme::textPrimary : Theme::textSecondary));
        g.setFont(juce::FontOptions(11.5f, toggled ? juce::Font::bold : juce::Font::plain));
        g.drawFittedText(button.getButtonText(),
                         button.getLocalBounds().reduced(6, 2),
                         juce::Justification::centred, 1);
    }

    void drawComboBox(juce::Graphics& g,
                      int width,
                      int height,
                      bool isButtonDown,
                      int,
                      int,
                      int,
                      int,
                      juce::ComboBox& box) override {
        const juce::Rectangle<float> bounds(0.5f, 0.5f, static_cast<float>(width) - 1.0f, static_cast<float>(height) - 1.0f);
        constexpr float cornerRadius = 5.0f;

        const bool isHighlighted = box.isMouseOver(true) || isButtonDown;
        g.setColour(isHighlighted ? Theme::controlHover : Theme::controlSurface);
        g.fillRoundedRectangle(bounds, cornerRadius);

        g.setColour(isHighlighted ? Theme::borderActive : Theme::borderLightGray);
        g.drawRoundedRectangle(bounds, cornerRadius, 1.0f);

        // Apple-style minimalist chevron
        const float arrowX = static_cast<float>(width) - 14.0f;
        const float arrowY = static_cast<float>(height) * 0.5f - 2.0f;
        juce::Path p;
        p.startNewSubPath(arrowX - 3.5f, arrowY);
        p.lineTo(arrowX, arrowY + 3.5f);
        p.lineTo(arrowX + 3.5f, arrowY);

        g.setColour(box.isEnabled() ? (isHighlighted ? Theme::textPrimary : Theme::textSecondary) : Theme::textMuted);
        g.strokePath(p, juce::PathStrokeType(1.3f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    void positionComboBoxText(juce::ComboBox& box, juce::Label& label) override {
        label.setBounds(4, 1, box.getWidth() - 22, box.getHeight() - 2);
        label.setFont(juce::FontOptions(11.0f, juce::Font::plain));
        label.setJustificationType(juce::Justification::centredLeft);
        label.setColour(juce::Label::textColourId, box.isEnabled() ? Theme::textPrimary : Theme::textMuted);
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
                          juce::Slider& slider) override {
        const float trackY = static_cast<float>(y) + static_cast<float>(height) * 0.5f;
        const float left = static_cast<float>(x) + 4.0f;
        const float right = static_cast<float>(x + width) - 4.0f;
        const float trackH = 4.0f;

        // Recessed track background
        g.setColour(Theme::controlSurface);
        g.fillRoundedRectangle(left, trackY - trackH * 0.5f, right - left, trackH, 2.0f);

        // Active track highlight (Sky Cyan / Slate)
        const float activeW = std::clamp(sliderPos - left, 0.0f, right - left);
        g.setColour(Theme::traceMagnitude.withAlpha(0.65f));
        g.fillRoundedRectangle(left, trackY - trackH * 0.5f, activeW, trackH, 2.0f);

        // Circular thumb handle
        const float thumbRadius = slider.isMouseOverOrDragging() ? 6.0f : 5.0f;
        g.setColour(Theme::backgroundDark);
        g.fillEllipse(sliderPos - thumbRadius - 0.5f, trackY - thumbRadius - 0.5f, (thumbRadius + 0.5f) * 2.0f, (thumbRadius + 0.5f) * 2.0f);
        g.setColour(Theme::textPrimary);
        g.fillEllipse(sliderPos - thumbRadius, trackY - thumbRadius, thumbRadius * 2.0f, thumbRadius * 2.0f);
    }

    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override {
        const juce::Rectangle<float> bounds(0.5f, 0.5f, static_cast<float>(width) - 1.0f, static_cast<float>(height) - 1.0f);
        g.setColour(Theme::panelBg);
        g.fillRoundedRectangle(bounds, 6.0f);
        g.setColour(Theme::borderLightGray);
        g.drawRoundedRectangle(bounds, 6.0f, 1.0f);
    }

    void drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                           bool isSeparator, bool isActive, bool isHighlighted,
                           bool isTicked, bool hasSubMenu, const juce::String& text,
                           const juce::String& shortcutKeyText,
                           const juce::Drawable* icon, const juce::Colour* textColour) override {
        if (isSeparator) {
            auto r = area.reduced(5, 0);
            g.setColour(Theme::borderSubtle);
            g.drawHorizontalLine(r.getCentreY(), static_cast<float>(r.getX()), static_cast<float>(r.getRight()));
            return;
        }

        if (isHighlighted && isActive) {
            g.setColour(Theme::controlHover);
            g.fillRoundedRectangle(area.reduced(2, 1).toFloat(), 4.0f);
        }

        juce::Colour textCol = Theme::textPrimary;
        if (!isActive) {
            textCol = Theme::textMuted;
        } else if (textColour != nullptr) {
            textCol = *textColour;
        }

        g.setColour(textCol);
        g.setFont(juce::FontOptions(12.0f, isTicked ? juce::Font::bold : juce::Font::plain));

        auto textBounds = area.reduced(10, 0);
        if (isTicked) {
            g.drawText(juce::CharPointer_UTF8("\xE2\x9C\x93 "), textBounds.removeFromLeft(16), juce::Justification::centredLeft, false);
        }
        g.drawFittedText(text, textBounds, juce::Justification::centredLeft, 1);
    }
};

} // namespace ui
