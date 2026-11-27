#include "ControlBarComponent.h"
#include "Theme.h"

#include <cmath>
#include <algorithm>

namespace ui {

namespace {
float linearToDb(float lin) noexcept {
    return (lin > 1e-6f) ? 20.0f * std::log10(lin) : -120.0f;
}
} // namespace

ControlBarComponent::ControlBarComponent(plugin::DSPAnalysisProcessor& processor)
    : processor_(processor) {
    auto& apvts = processor_.apvts();

    auto initCombo = [this](juce::ComboBox& box, const char* paramId, bool visibleByDefault) {
        if (auto* param = dynamic_cast<juce::AudioParameterChoice*>(processor_.apvts().getParameter(paramId))) {
            box.addItemList(param->choices, 1);
        }
        if (visibleByDefault) {
            addAndMakeVisible(box);
        } else {
            addChildComponent(box);
        }
    };

    // --- VIEW SELECTORS (Apple Segmented Control) ---
    auto setupViewBtn = [this](juce::TextButton& btn, int idx) {
        btn.setClickingTogglesState(true);
        btn.setRadioGroupId(1001);
        btn.onClick = [this, idx] {
            currentView_ = idx;
            updateViewButtonStates();
            if (onViewChanged) onViewChanged(idx);
        };
        addAndMakeVisible(btn);
    };
    setupViewBtn(viewTransferBtn_, 0);
    setupViewBtn(viewDynamicsBtn_, 1);
    setupViewBtn(viewHarmonicsBtn_, 2);
    viewTransferBtn_.setToggleState(true, juce::dontSendNotification);

    // --- ROW 1: Primary Action Buttons ---
    addAndMakeVisible(findDelayButton_);
    findDelayButton_.onClick = [this] {
        if (onFindDelayClicked) onFindDelayClicked();
    };

    addAndMakeVisible(freezeButton_);
    freezeButton_.setClickingTogglesState(true);
    freezeButton_.onClick = [this] {
        if (onFreezeChanged) onFreezeChanged(freezeButton_.getToggleState());
    };

    addAndMakeVisible(resetAvgButton_);
    resetAvgButton_.onClick = [this] { processor_.worker().requestResetAveraging(); };

    addAndMakeVisible(advancedToggleBtn_);
    advancedToggleBtn_.setClickingTogglesState(true);
    advancedToggleBtn_.onClick = [this] {
        if (onToggleAdvanced) onToggleAdvanced();
    };

    addAndMakeVisible(helpBtn_);
    helpBtn_.onClick = [this] {
        if (onOpenHelp) onOpenHelp();
    };

    // --- ROW 2: Primary Measurement Controls ---
    initCombo(routingBox_, plugin::ParamID::routingMode, true);
    routingAtt_ = std::make_unique<ComboAttachment>(apvts, plugin::ParamID::routingMode, routingBox_);

    initCombo(tapChannelBox_, plugin::ParamID::tapChannel, true);
    tapChannelAtt_ = std::make_unique<ComboAttachment>(apvts, plugin::ParamID::tapChannel, tapChannelBox_);

    addAndMakeVisible(swapButton_);
    swapButton_.setClickingTogglesState(true);
    swapAtt_ = std::make_unique<ButtonAttachment>(apvts, plugin::ParamID::swapRefMeas, swapButton_);

    initCombo(smoothingBox_, plugin::ParamID::smoothingMode, true);
    smoothingAtt_ = std::make_unique<ComboAttachment>(apvts, plugin::ParamID::smoothingMode, smoothingBox_);

    initCombo(channelBox_, plugin::ParamID::channelMode, true);
    channelAtt_ = std::make_unique<ComboAttachment>(apvts, plugin::ParamID::channelMode, channelBox_);

    addAndMakeVisible(storeAButton_);
    storeAButton_.onClick = [this] {
        showAButton_.setToggleState(true, juce::dontSendNotification);
        if (onStoreSnapshotA) onStoreSnapshotA();
    };

    addAndMakeVisible(showAButton_);
    showAButton_.setToggleState(true, juce::dontSendNotification);
    showAButton_.onClick = [this] {
        if (onShowSnapshotAChanged) onShowSnapshotAChanged(showAButton_.getToggleState());
    };

    addAndMakeVisible(storeBButton_);
    storeBButton_.onClick = [this] {
        showBButton_.setToggleState(true, juce::dontSendNotification);
        if (onStoreSnapshotB) onStoreSnapshotB();
    };

    addAndMakeVisible(showBButton_);
    showBButton_.setToggleState(true, juce::dontSendNotification);
    showBButton_.onClick = [this] {
        if (onShowSnapshotBChanged) onShowSnapshotBChanged(showBButton_.getToggleState());
    };

    // --- ADVANCED DRAWER CONTROLS (Hidden until Advanced is toggled) ---
    initCombo(fftSizeBox_, plugin::ParamID::fftSizeIndex, false);
    fftAtt_ = std::make_unique<ComboAttachment>(apvts, plugin::ParamID::fftSizeIndex, fftSizeBox_);

    initCombo(windowBox_, plugin::ParamID::windowType, false);
    windowAtt_ = std::make_unique<ComboAttachment>(apvts, plugin::ParamID::windowType, windowBox_);

    initCombo(avgModeBox_, plugin::ParamID::averagingMode, false);
    avgModeAtt_ = std::make_unique<ComboAttachment>(apvts, plugin::ParamID::averagingMode, avgModeBox_);

    alphaSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    alphaSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 38, 18);
    addChildComponent(alphaSlider_);
    addChildComponent(alphaLabel_);
    alphaLabel_.setColour(juce::Label::textColourId, Theme::textSecondary);
    alphaLabel_.setFont(juce::FontOptions(11.0f, juce::Font::plain));
    alphaAtt_ = std::make_unique<SliderAttachment>(apvts, plugin::ParamID::averagingAlpha, alphaSlider_);

    initCombo(magRangeBox_, plugin::ParamID::magRangeDb, false);
    magRangeAtt_ = std::make_unique<ComboAttachment>(apvts, plugin::ParamID::magRangeDb, magRangeBox_);

    addChildComponent(unwrapButton_);
    unwrapAtt_ = std::make_unique<ButtonAttachment>(apvts, plugin::ParamID::unwrapPhase, unwrapButton_);

    addChildComponent(showH2Button_);
    showH2Att_ = std::make_unique<ButtonAttachment>(apvts, plugin::ParamID::showH2, showH2Button_);

    addChildComponent(delayCompButton_);
    delayCompAtt_ = std::make_unique<ButtonAttachment>(apvts, plugin::ParamID::delayCompEnabled, delayCompButton_);

    delayMsSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    delayMsSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 48, 18);
    addChildComponent(delayMsSlider_);
    addChildComponent(delayMsLabel_);
    delayMsLabel_.setColour(juce::Label::textColourId, Theme::textSecondary);
    delayMsLabel_.setFont(juce::FontOptions(11.0f, juce::Font::plain));
    delayMsAtt_ = std::make_unique<SliderAttachment>(apvts, plugin::ParamID::delayCompMs, delayMsSlider_);
}

void ControlBarComponent::updateViewButtonStates() {
    viewTransferBtn_.setToggleState(currentView_ == 0, juce::dontSendNotification);
    viewDynamicsBtn_.setToggleState(currentView_ == 1, juce::dontSendNotification);
    viewHarmonicsBtn_.setToggleState(currentView_ == 2, juce::dontSendNotification);
    repaint();
}

void ControlBarComponent::setAdvancedOpen(bool isOpen) {
    isAdvancedOpen_ = isOpen;
    advancedToggleBtn_.setToggleState(isOpen, juce::dontSendNotification);

    fftSizeBox_.setVisible(isOpen);
    windowBox_.setVisible(isOpen);
    avgModeBox_.setVisible(isOpen);
    alphaSlider_.setVisible(isOpen);
    alphaLabel_.setVisible(isOpen);
    magRangeBox_.setVisible(isOpen);
    unwrapButton_.setVisible(isOpen);
    showH2Button_.setVisible(isOpen);
    delayCompButton_.setVisible(isOpen);
    delayMsSlider_.setVisible(isOpen);
    delayMsLabel_.setVisible(isOpen);

    resized();
    repaint();
}

void ControlBarComponent::updateStatus(float refPeak,
                                       float measPeak,
                                       double sampleRate,
                                       int blockSize,
                                       bool sidechainActive,
                                       bool tapBusActive,
                                       int routingMode,
                                       const dsp::DelayEstimate& delayEstimate) {
    refPeak_ = refPeak;
    measPeak_ = measPeak;
    sampleRate_ = sampleRate;
    blockSize_ = blockSize;
    sidechainActive_ = sidechainActive;
    tapBusActive_ = tapBusActive;
    routingMode_ = routingMode;
    delayEstimate_ = delayEstimate;
    repaint();
}

void ControlBarComponent::paint(juce::Graphics& g) {
    g.fillAll(Theme::barBg);
    g.setColour(Theme::borderSubtle);
    g.drawHorizontalLine(getHeight() - 1, 0.0f, static_cast<float>(getWidth()));

    // --- LEFT: Brand Wordmark & Dynamic Telemetry ---
    g.setColour(Theme::textPrimary);
    g.setFont(juce::FontOptions(15.0f, juce::Font::bold));
    g.drawText("PluginCheq", 14, 7, 112, 18, juce::Justification::centredLeft, false);

    g.setColour(Theme::textSecondary);
    g.setFont(juce::FontOptions(9.5f, juce::Font::plain));
    g.drawText("PRECISION DSP LAB", 14, 24, 112, 12, juce::Justification::centredLeft, false);

    // Dynamic routing status pill badge
    juce::String routeBadge;
    juce::Colour badgeCol = Theme::textMuted;

    if (routingMode_ == 2) { // Tap Sender
        routeBadge = "SENDING TAP...";
        badgeCol = Theme::traceMagnitude;
    } else if (routingMode_ == 1) { // Plugin Tap Bus
        routeBadge = tapBusActive_ ? "TAP: LOCKED" : "TAP: SEARCHING";
        badgeCol = tapBusActive_ ? Theme::statusOk : Theme::statusSearching;
    } else { // DAW Sidechain
        routeBadge = sidechainActive_ ? "SIDECHAIN: OK" : "NO SIDECHAIN";
        badgeCol = sidechainActive_ ? Theme::statusOk : Theme::textMuted;
    }

    const float routePillY = 39.0f;
    g.setColour(Theme::controlSurface);
    g.fillRoundedRectangle(14.0f, routePillY, 112.0f, 15.0f, 3.0f);
    g.setColour(badgeCol.withAlpha(0.6f));
    g.drawRoundedRectangle(14.0f, routePillY, 112.0f, 15.0f, 3.0f, 1.0f);
    g.setColour(badgeCol);
    g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
    g.drawText(routeBadge, 14, static_cast<int>(routePillY), 112, 15, juce::Justification::centred, false);

    // Delay status pill
    const float delayPillY = 56.0f;
    juce::String delayStr = delayEstimate_.valid
        ? (juce::String(delayEstimate_.delayMs, 2) + " ms (" + juce::String(delayEstimate_.delaySamples, 1) + " spl)")
        : "Delay: --";
    g.setColour(Theme::controlSurface);
    g.fillRoundedRectangle(14.0f, delayPillY, 112.0f, 15.0f, 3.0f);
    g.setColour(Theme::borderSubtle);
    g.drawRoundedRectangle(14.0f, delayPillY, 112.0f, 15.0f, 3.0f, 1.0f);
    g.setColour(delayEstimate_.valid ? Theme::statusOk : Theme::textMuted);
    g.fillEllipse(18.0f, delayPillY + 4.5f, 6.0f, 6.0f);
    g.setColour(delayEstimate_.valid ? Theme::textPrimary : Theme::textMuted);
    g.setFont(juce::FontOptions(9.0f, juce::Font::plain));
    g.drawText(delayStr, 27, static_cast<int>(delayPillY), 97, 15, juce::Justification::centredLeft, false);

    // --- SEGMENTED TABS RECESSED CAPSULE CONTAINER ---
    if (!segmentedCapsuleBounds_.isEmpty()) {
        const auto capF = segmentedCapsuleBounds_.toFloat().reduced(0.5f);
        g.setColour(Theme::segmentedTrack);
        g.fillRoundedRectangle(capF, 6.0f);
        g.setColour(Theme::borderSubtle);
        g.drawRoundedRectangle(capF, 6.0f, 1.0f);
    }

    // --- RIGHT: Precision Dual Meters & Host Telemetry ---
    const float meterAreaW = 224.0f;
    const float meterX = static_cast<float>(getWidth()) - meterAreaW - 12.0f;
    const float refDb = linearToDb(refPeak_);
    const float measDb = linearToDb(measPeak_);

    g.setFont(juce::FontOptions(10.0f, juce::Font::plain));
    g.setColour(Theme::textSecondary);
    g.drawText("REF", static_cast<int>(meterX), 8, 36, 14, juce::Justification::centredLeft, false);
    g.drawText("MEAS", static_cast<int>(meterX), 25, 36, 14, juce::Justification::centredLeft, false);

    auto drawMeterBar = [&](float y, float db, juce::Colour col) {
        const float barX = meterX + 38.0f;
        const float barW = 110.0f;
        const float norm = std::clamp((db + 60.0f) / 60.0f, 0.0f, 1.0f);

        g.setColour(Theme::controlSurface);
        g.fillRoundedRectangle(barX, y + 2.0f, barW, 7.0f, 2.5f);

        if (norm > 0.005f) {
            g.setColour(db > -0.1f ? Theme::meterClip : col);
            g.fillRoundedRectangle(barX, y + 2.0f, barW * norm, 7.0f, 2.5f);
        }

        if (db > -0.1f) {
            g.setColour(Theme::meterClip);
            g.fillEllipse(barX + barW - 6.0f, y + 2.5f, 6.0f, 6.0f);
        }

        g.setColour(db > -0.1f ? Theme::meterClip : Theme::textSecondary);
        g.setFont(juce::FontOptions(10.0f, juce::Font::plain));
        g.drawText(db <= -119.0f ? "-inf" : (juce::String(db, 1) + " dB"),
                   static_cast<int>(barX + barW + 6.0f), static_cast<int>(y), 56, 14,
                   juce::Justification::centredLeft, false);
    };

    drawMeterBar(8.0f, refDb, Theme::meterRef);
    drawMeterBar(25.0f, measDb, Theme::meterMeas);

    const juce::String info = juce::String(sampleRate_ / 1000.0, 1) + " kHz  |  "
        + juce::String(blockSize_) + " spl";
    g.setColour(Theme::textMuted);
    g.setFont(juce::FontOptions(10.0f, juce::Font::plain));
    g.drawText(info, static_cast<int>(meterX), 44, 200, 14, juce::Justification::centredLeft, false);

    // Divider line & aligned group headings for Advanced Drawer if expanded
    if (isAdvancedOpen_) {
        g.setColour(Theme::borderSubtle);
        g.drawHorizontalLine(76, 12.0f, static_cast<float>(getWidth() - 12));

        g.setColour(Theme::textMuted);
        g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
        g.drawText("ANALYSIS ENGINE", drawerSec1X_, 81, 160, 14, juce::Justification::centredLeft, false);
        g.drawText("DISPLAY SCALING", drawerSec2X_, 81, 160, 14, juce::Justification::centredLeft, false);
        g.drawText("TIME ALIGNMENT", drawerSec3X_, 81, 160, 14, juce::Justification::centredLeft, false);
    }
}

void ControlBarComponent::resized() {
    auto topArea = juce::Rectangle<int>(0, 0, getWidth(), 74).reduced(10, 6);
    topArea.removeFromLeft(128);  // Space for brand wordmark & delay/routing badges
    topArea.removeFromRight(232); // Space for precision meters & host telemetry

    // --- ROW 1: Segmented View Selector + Primary Action Buttons ---
    auto row1 = topArea.removeFromTop(28);

    constexpr int segW1 = 94;
    constexpr int segW2 = 82;
    constexpr int segW3 = 114;
    segmentedCapsuleBounds_ = juce::Rectangle<int>(row1.getX(), row1.getY(), segW1 + segW2 + segW3 + 4, 28);

    auto segArea = row1.removeFromLeft(segW1 + segW2 + segW3 + 4).reduced(2, 2);
    viewTransferBtn_.setBounds(segArea.removeFromLeft(segW1));
    viewDynamicsBtn_.setBounds(segArea.removeFromLeft(segW2));
    viewHarmonicsBtn_.setBounds(segArea.removeFromLeft(segW3));

    row1.removeFromLeft(12);

    findDelayButton_.setBounds(row1.removeFromLeft(126));
    row1.removeFromLeft(6);
    freezeButton_.setBounds(row1.removeFromLeft(54));
    row1.removeFromLeft(5);
    resetAvgButton_.setBounds(row1.removeFromLeft(48));
    row1.removeFromLeft(8);
    advancedToggleBtn_.setBounds(row1.removeFromLeft(86));
    row1.removeFromLeft(6);
    helpBtn_.setBounds(row1.removeFromLeft(26));

    topArea.removeFromTop(8);

    // --- ROW 2: Primary Measurement Bar ---
    auto row2 = topArea.removeFromTop(26);
    routingBox_.setBounds(row2.removeFromLeft(152));
    row2.removeFromLeft(6);
    tapChannelBox_.setBounds(row2.removeFromLeft(64));
    row2.removeFromLeft(6);
    swapButton_.setBounds(row2.removeFromLeft(58));
    row2.removeFromLeft(10);
    smoothingBox_.setBounds(row2.removeFromLeft(104));
    row2.removeFromLeft(6);
    channelBox_.setBounds(row2.removeFromLeft(86));
    row2.removeFromLeft(12);
    storeAButton_.setBounds(row2.removeFromLeft(52));
    row2.removeFromLeft(3);
    showAButton_.setBounds(row2.removeFromLeft(26));
    row2.removeFromLeft(6);
    storeBButton_.setBounds(row2.removeFromLeft(52));
    row2.removeFromLeft(3);
    showBButton_.setBounds(row2.removeFromLeft(26));

    // --- ADVANCED SETTINGS DRAWER (Full Width below Y=76 so headings & controls never collide) ---
    if (isAdvancedOpen_) {
        auto drawerRow = juce::Rectangle<int>(16, 98, getWidth() - 32, 26);

        // Section 1: Analysis Engine
        drawerSec1X_ = drawerRow.getX();
        fftSizeBox_.setBounds(drawerRow.removeFromLeft(78));
        drawerRow.removeFromLeft(6);
        windowBox_.setBounds(drawerRow.removeFromLeft(132));
        drawerRow.removeFromLeft(6);
        avgModeBox_.setBounds(drawerRow.removeFromLeft(92));
        drawerRow.removeFromLeft(6);
        alphaLabel_.setBounds(drawerRow.removeFromLeft(42));
        alphaSlider_.setBounds(drawerRow.removeFromLeft(86));

        drawerRow.removeFromLeft(22);

        // Section 2: Display Scaling
        drawerSec2X_ = drawerRow.getX();
        magRangeBox_.setBounds(drawerRow.removeFromLeft(94));
        drawerRow.removeFromLeft(6);
        unwrapButton_.setBounds(drawerRow.removeFromLeft(96));
        drawerRow.removeFromLeft(6);
        showH2Button_.setBounds(drawerRow.removeFromLeft(72));

        drawerRow.removeFromLeft(22);

        // Section 3: Time Alignment
        drawerSec3X_ = drawerRow.getX();
        delayCompButton_.setBounds(drawerRow.removeFromLeft(90));
        drawerRow.removeFromLeft(6);
        delayMsLabel_.setBounds(drawerRow.removeFromLeft(58));
        delayMsSlider_.setBounds(drawerRow.removeFromLeft(108));
    }
}

} // namespace ui
