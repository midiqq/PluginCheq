#include "ControlBarComponent.h"
#include "Theme.h"

#include <cmath>
#include <algorithm>
#include <array>

namespace ui {

namespace {
float linearToDb(float lin) noexcept {
    return (lin > 1e-6f) ? 20.0f * std::log10(lin) : -120.0f;
}
} // namespace

ControlBarComponent::ControlBarComponent(plugin::DSPAnalysisProcessor& processor)
    : processor_(processor) {
    auto& apvts = processor_.apvts();

    auto initCombo = [this](juce::ComboBox& box, const char* paramId) {
        if (auto* param = dynamic_cast<juce::AudioParameterChoice*>(processor_.apvts().getParameter(paramId))) {
            box.addItemList(param->choices, 1);
        }
        addChildComponent(box);
    };

    // --- TE PRIMARY ACTION STRIP (LEFT) ---
    // [⚡ ALIGN] (Teenage Engineering vibrant orange #FF5A00 accent)
    alignButton_.setComponentID("TE_ORANGE");
    alignButton_.setTooltip("Auto-detect and compensate latency delay (⚡ TE Orange)");
    alignButton_.onClick = [this] {
        if (onFindDelayClicked) onFindDelayClicked();
    };
    addAndMakeVisible(alignButton_);

    // [A / B] (Smart snapshot toggle & store)
    abButton_.setTooltip("Click to toggle/store snapshot A or B");
    abButton_.onClick = [this] {
        if (abState_ == 0) {
            abState_ = 1;
            if (onStoreSnapshotA) onStoreSnapshotA();
            if (onShowSnapshotAChanged) onShowSnapshotAChanged(true);
            if (onShowSnapshotBChanged) onShowSnapshotBChanged(false);
            showAButton_.setToggleState(true, juce::dontSendNotification);
            showBButton_.setToggleState(false, juce::dontSendNotification);
            abButton_.setButtonText("A");
        } else if (abState_ == 1) {
            abState_ = 2;
            if (onStoreSnapshotB) onStoreSnapshotB();
            if (onShowSnapshotAChanged) onShowSnapshotAChanged(false);
            if (onShowSnapshotBChanged) onShowSnapshotBChanged(true);
            showAButton_.setToggleState(false, juce::dontSendNotification);
            showBButton_.setToggleState(true, juce::dontSendNotification);
            abButton_.setButtonText("B");
        } else if (abState_ == 2) {
            abState_ = 3;
            if (onShowSnapshotAChanged) onShowSnapshotAChanged(true);
            if (onShowSnapshotBChanged) onShowSnapshotBChanged(true);
            showAButton_.setToggleState(true, juce::dontSendNotification);
            showBButton_.setToggleState(true, juce::dontSendNotification);
            abButton_.setButtonText("A+B");
        } else {
            abState_ = 0;
            if (onShowSnapshotAChanged) onShowSnapshotAChanged(false);
            if (onShowSnapshotBChanged) onShowSnapshotBChanged(false);
            showAButton_.setToggleState(false, juce::dontSendNotification);
            showBButton_.setToggleState(false, juce::dontSendNotification);
            abButton_.setButtonText("A / B");
        }
        abButton_.setToggleState(abState_ > 0, juce::dontSendNotification);
    };
    addAndMakeVisible(abButton_);

    // [FREEZE]
    freezeButton_.setClickingTogglesState(true);
    freezeButton_.onClick = [this] {
        if (onFreezeChanged) onFreezeChanged(freezeButton_.getToggleState());
    };
    addAndMakeVisible(freezeButton_);

    // [RESET]
    resetAvgButton_.onClick = [this] {
        processor_.worker().requestResetAveraging();
    };
    addAndMakeVisible(resetAvgButton_);

    // [SMOOTH] (cycles through fractional octave smoothing)
    smoothButton_.setTooltip("Cycle fractional octave smoothing");
    smoothButton_.onClick = [this] {
        auto* param = processor_.apvts().getParameter(plugin::ParamID::smoothingMode);
        if (auto* choice = dynamic_cast<juce::AudioParameterChoice*>(param)) {
            int next = (choice->getIndex() + 1) % static_cast<int>(choice->choices.size());
            param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(next)));
            updateSmoothButtonState();
        }
    };
    addAndMakeVisible(smoothButton_);
    updateSmoothButtonState();

    // [SWAP]
    swapButton_.setClickingTogglesState(true);
    swapAtt_ = std::make_unique<ButtonAttachment>(apvts, plugin::ParamID::swapRefMeas, swapButton_);
    addAndMakeVisible(swapButton_);

    // --- VIEW SELECTORS (CENTER TABS: [EQ] | [DYNAMICS] | [THD]) ---
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

    // --- RIGHT WIDGETS ---
    settingsButton_.setClickingTogglesState(true);
    settingsButton_.setTooltip("Open Advanced Settings Drawer");
    settingsButton_.onClick = [this] {
        if (onToggleAdvanced) onToggleAdvanced();
    };
    addAndMakeVisible(settingsButton_);

    helpBtn_.setTooltip("Keyboard Shortcuts & Guide");
    helpBtn_.onClick = [this] {
        if (onOpenHelp) onOpenHelp();
    };
    addChildComponent(helpBtn_);

    // --- ADVANCED DRAWER CONTROLS (STORED SAFELY IN DRAWER) ---
    initCombo(routingBox_, plugin::ParamID::routingMode);
    routingAtt_ = std::make_unique<ComboAttachment>(apvts, plugin::ParamID::routingMode, routingBox_);

    initCombo(tapChannelBox_, plugin::ParamID::tapChannel);
    tapChannelAtt_ = std::make_unique<ComboAttachment>(apvts, plugin::ParamID::tapChannel, tapChannelBox_);

    initCombo(smoothingBox_, plugin::ParamID::smoothingMode);
    smoothingAtt_ = std::make_unique<ComboAttachment>(apvts, plugin::ParamID::smoothingMode, smoothingBox_);

    initCombo(channelBox_, plugin::ParamID::channelMode);
    channelAtt_ = std::make_unique<ComboAttachment>(apvts, plugin::ParamID::channelMode, channelBox_);

    addChildComponent(storeAButton_);
    storeAButton_.onClick = [this] {
        abState_ = 1;
        abButton_.setButtonText("A");
        abButton_.setToggleState(true, juce::dontSendNotification);
        showAButton_.setToggleState(true, juce::dontSendNotification);
        if (onStoreSnapshotA) onStoreSnapshotA();
    };

    addChildComponent(showAButton_);
    showAButton_.onClick = [this] {
        if (onShowSnapshotAChanged) onShowSnapshotAChanged(showAButton_.getToggleState());
    };

    addChildComponent(storeBButton_);
    storeBButton_.onClick = [this] {
        abState_ = 2;
        abButton_.setButtonText("B");
        abButton_.setToggleState(true, juce::dontSendNotification);
        showBButton_.setToggleState(true, juce::dontSendNotification);
        if (onStoreSnapshotB) onStoreSnapshotB();
    };

    addChildComponent(showBButton_);
    showBButton_.onClick = [this] {
        if (onShowSnapshotBChanged) onShowSnapshotBChanged(showBButton_.getToggleState());
    };

    initCombo(fftSizeBox_, plugin::ParamID::fftSizeIndex);
    fftAtt_ = std::make_unique<ComboAttachment>(apvts, plugin::ParamID::fftSizeIndex, fftSizeBox_);

    initCombo(windowBox_, plugin::ParamID::windowType);
    windowAtt_ = std::make_unique<ComboAttachment>(apvts, plugin::ParamID::windowType, windowBox_);

    initCombo(avgModeBox_, plugin::ParamID::averagingMode);
    avgModeAtt_ = std::make_unique<ComboAttachment>(apvts, plugin::ParamID::averagingMode, avgModeBox_);

    alphaSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    alphaSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 36, 18);
    addChildComponent(alphaSlider_);
    addChildComponent(alphaLabel_);
    alphaLabel_.setColour(juce::Label::textColourId, Theme::textSecondary);
    alphaLabel_.setFont(juce::FontOptions(10.5f, juce::Font::plain));
    alphaAtt_ = std::make_unique<SliderAttachment>(apvts, plugin::ParamID::averagingAlpha, alphaSlider_);

    initCombo(magRangeBox_, plugin::ParamID::magRangeDb);
    magRangeAtt_ = std::make_unique<ComboAttachment>(apvts, plugin::ParamID::magRangeDb, magRangeBox_);

    addChildComponent(unwrapButton_);
    unwrapAtt_ = std::make_unique<ButtonAttachment>(apvts, plugin::ParamID::unwrapPhase, unwrapButton_);

    addChildComponent(showH2Button_);
    showH2Att_ = std::make_unique<ButtonAttachment>(apvts, plugin::ParamID::showH2, showH2Button_);

    addChildComponent(delayCompButton_);
    delayCompAtt_ = std::make_unique<ButtonAttachment>(apvts, plugin::ParamID::delayCompEnabled, delayCompButton_);

    delayMsSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    delayMsSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 46, 18);
    addChildComponent(delayMsSlider_);
    addChildComponent(delayMsLabel_);
    delayMsLabel_.setColour(juce::Label::textColourId, Theme::textSecondary);
    delayMsLabel_.setFont(juce::FontOptions(10.5f, juce::Font::plain));
    delayMsAtt_ = std::make_unique<SliderAttachment>(apvts, plugin::ParamID::delayCompMs, delayMsSlider_);
}

void ControlBarComponent::updateSmoothButtonState() {
    auto* param = processor_.apvts().getParameter(plugin::ParamID::smoothingMode);
    if (auto* choice = dynamic_cast<juce::AudioParameterChoice*>(param)) {
        const int idx = choice->getIndex();
        smoothButton_.setToggleState(idx > 0, juce::dontSendNotification);
    }
}

void ControlBarComponent::updateViewButtonStates() {
    viewTransferBtn_.setToggleState(currentView_ == 0, juce::dontSendNotification);
    viewDynamicsBtn_.setToggleState(currentView_ == 1, juce::dontSendNotification);
    viewHarmonicsBtn_.setToggleState(currentView_ == 2, juce::dontSendNotification);
    repaint();
}

void ControlBarComponent::setAdvancedOpen(bool isOpen) {
    isAdvancedOpen_ = isOpen;
    settingsButton_.setToggleState(isOpen, juce::dontSendNotification);

    routingBox_.setVisible(isOpen);
    tapChannelBox_.setVisible(isOpen);
    smoothingBox_.setVisible(isOpen);
    channelBox_.setVisible(isOpen);

    storeAButton_.setVisible(isOpen);
    showAButton_.setVisible(isOpen);
    storeBButton_.setVisible(isOpen);
    showBButton_.setVisible(isOpen);

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
    helpBtn_.setVisible(isOpen);

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
    updateSmoothButtonState();
    repaint();
}

void ControlBarComponent::paint(juce::Graphics& g) {
    // 1. Slim TE Top Bar Background
    g.setColour(Theme::barBg);
    g.fillRect(0, 0, getWidth(), 38);

    g.setColour(Theme::borderSubtle);
    g.drawHorizontalLine(37, 0.0f, static_cast<float>(getWidth()));

    // 2. Center Tabs Segmented Capsule Track
    if (!segmentedCapsuleBounds_.isEmpty()) {
        const auto capF = segmentedCapsuleBounds_.toFloat().reduced(0.5f);
        g.setColour(Theme::segmentedTrack);
        g.fillRoundedRectangle(capF, 4.0f);
        g.setColour(Theme::borderSubtle);
        g.drawRoundedRectangle(capF, 4.0f, 1.0f);
    }

    // 3. Digital Delay Readout Pill (TE Precision Telemetry)
    if (!delayBadgeBounds_.isEmpty()) {
        const auto badgeF = delayBadgeBounds_.toFloat().reduced(0.5f);
        g.setColour(Theme::controlSurface);
        g.fillRoundedRectangle(badgeF, 3.0f);
        g.setColour(Theme::borderSubtle);
        g.drawRoundedRectangle(badgeF, 3.0f, 1.0f);

        // Status LED dot
        const float dotX = badgeF.getX() + 6.0f;
        const float dotY = badgeF.getCentreY() - 2.5f;
        g.setColour(delayEstimate_.valid ? Theme::statusOk : Theme::textMuted);
        g.fillEllipse(dotX, dotY, 5.0f, 5.0f);

        // Digital delay readout
        juce::String delayStr = delayEstimate_.valid
            ? ("DELAY: " + juce::String(delayEstimate_.delayMs, 2) + " ms")
            : "DELAY: 0.00 ms";

        g.setColour(delayEstimate_.valid ? Theme::textPrimary : Theme::textSecondary);
        g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
        g.drawText(delayStr, static_cast<int>(dotX + 8.0f), static_cast<int>(badgeF.getY()),
                   static_cast<int>(badgeF.getWidth() - 14.0f), static_cast<int>(badgeF.getHeight()),
                   juce::Justification::centredLeft, false);
    }

    // 4. Compact Segmented LED Meters (REF & MEAS)
    if (!metersBounds_.isEmpty()) {
        static constexpr std::array<float, 10> kThresholds = {
            -48.0f, -36.0f, -24.0f, -18.0f, -12.0f, -8.0f, -5.0f, -3.0f, -1.0f, -0.2f
        };

        const float refDb = linearToDb(refPeak_);
        const float measDb = linearToDb(measPeak_);

        auto drawLedMeter = [&](float y, const char* name, float db) {
            // Label
            g.setColour(Theme::textSecondary);
            g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
            g.drawText(name, metersBounds_.getX(), static_cast<int>(y), 24, 12, juce::Justification::centredLeft, false);

            // 10 Segmented LED Bricks
            float segX = static_cast<float>(metersBounds_.getX() + 26);
            constexpr float segW = 5.0f;
            constexpr float segH = 7.0f;
            constexpr float gap = 2.0f;

            for (std::size_t i = 0; i < kThresholds.size(); ++i) {
                const bool isLit = (db >= kThresholds[i]);
                juce::Colour segCol = Theme::meterGreen;
                if (i >= 8) segCol = Theme::meterRed;
                else if (i >= 6) segCol = Theme::meterAmber;

                g.setColour(isLit ? segCol : Theme::meterOff);
                g.fillRoundedRectangle(segX, y + 2.5f, segW, segH, 1.0f);
                segX += (segW + gap);
            }

            // Numeric dB readout
            g.setColour(db > -0.2f ? Theme::meterRed : Theme::textSecondary);
            g.setFont(juce::FontOptions(8.5f, juce::Font::plain));
            juce::String valStr = (db <= -60.0f) ? "-inf" : (juce::String(static_cast<int>(std::round(db))) + " dB");
            g.drawText(valStr, static_cast<int>(segX + 2.0f), static_cast<int>(y), 32, 12, juce::Justification::centredLeft, false);
        };

        drawLedMeter(static_cast<float>(metersBounds_.getY() + 1), "REF", refDb);
        drawLedMeter(static_cast<float>(metersBounds_.getY() + 15), "MEAS", measDb);
    }

    // 5. Expandable Settings Drawer (Grouped Laboratory Parameter Deck)
    if (isAdvancedOpen_) {
        const int drawerTop = 38;
        const int drawerHeight = getHeight() - drawerTop;
        g.setColour(Theme::drawerBg);
        g.fillRect(0, drawerTop, getWidth(), drawerHeight);

        g.setColour(Theme::borderSubtle);
        g.drawHorizontalLine(getHeight() - 1, 0.0f, static_cast<float>(getWidth()));

        // Group headings with technical micro-typography
        g.setColour(Theme::textMuted);
        g.setFont(juce::FontOptions(9.0f, juce::Font::bold));

        g.drawText("ANALYSIS ENGINE", 14, drawerTop + 4, 180, 14, juce::Justification::centredLeft, false);
        g.drawText("ROUTING & CHANNELS", 370, drawerTop + 4, 160, 14, juce::Justification::centredLeft, false);
        g.drawText("DISPLAY & SNAPSHOTS", 600, drawerTop + 4, 180, 14, juce::Justification::centredLeft, false);
        g.drawText("TIME ALIGNMENT", 920, drawerTop + 4, 160, 14, juce::Justification::centredLeft, false);
    }
}

void ControlBarComponent::resized() {
    constexpr int barH = 38;

    // --- LEFT: Compact TE Action Strip ---
    int leftX = 8;
    constexpr int btnH = 26;
    constexpr int btnY = (barH - btnH) / 2;

    alignButton_.setBounds(leftX, btnY, 68, btnH);
    leftX += 72;

    abButton_.setBounds(leftX, btnY, 44, btnH);
    leftX += 48;

    freezeButton_.setBounds(leftX, btnY, 56, btnH);
    leftX += 60;

    resetAvgButton_.setBounds(leftX, btnY, 48, btnH);
    leftX += 52;

    smoothButton_.setBounds(leftX, btnY, 58, btnH);
    leftX += 62;

    swapButton_.setBounds(leftX, btnY, 48, btnH);

    // --- CENTER: Compact View Selector Tabs [EQ] | [DYNAMICS] | [THD] ---
    constexpr int segW1 = 44;
    constexpr int segW2 = 74;
    constexpr int segW3 = 46;
    constexpr int totalSegW = segW1 + segW2 + segW3 + 4;
    const int segX = (getWidth() - totalSegW) / 2;

    segmentedCapsuleBounds_ = juce::Rectangle<int>(segX, btnY, totalSegW, btnH);
    auto segArea = segmentedCapsuleBounds_.reduced(2, 2);
    viewTransferBtn_.setBounds(segArea.removeFromLeft(segW1));
    viewDynamicsBtn_.setBounds(segArea.removeFromLeft(segW2));
    viewHarmonicsBtn_.setBounds(segArea.removeFromLeft(segW3));

    // --- RIGHT: Settings Gear, Segmented LED Meters, Digital Delay ---
    int rightX = getWidth() - 8;

    // Gear [⚙] button
    rightX -= 26;
    settingsButton_.setBounds(rightX, btnY, 26, btnH);

    // Segmented LED Meters
    rightX -= 138;
    metersBounds_ = juce::Rectangle<int>(rightX, 4, 132, 30);

    // Digital Delay Readout Pill
    rightX -= 108;
    delayBadgeBounds_ = juce::Rectangle<int>(rightX, (barH - 22) / 2, 102, 22);

    // --- EXPANDABLE ADVANCED SETTINGS DRAWER ---
    if (isAdvancedOpen_) {
        constexpr int drawerRowY = 38 + 22;
        constexpr int ctlH = 24;

        // Section 1: Analysis Engine
        int ctlX = 14;
        fftSizeBox_.setBounds(ctlX, drawerRowY, 68, ctlH);
        ctlX += 72;
        windowBox_.setBounds(ctlX, drawerRowY, 96, ctlH);
        ctlX += 100;
        avgModeBox_.setBounds(ctlX, drawerRowY, 82, ctlH);
        ctlX += 86;
        alphaLabel_.setBounds(ctlX, drawerRowY, 36, ctlH);
        ctlX += 38;
        alphaSlider_.setBounds(ctlX, drawerRowY, 56, ctlH);

        // Section 2: Routing & Channels
        ctlX = 370;
        routingBox_.setBounds(ctlX, drawerRowY, 108, ctlH);
        ctlX += 112;
        tapChannelBox_.setBounds(ctlX, drawerRowY, 48, ctlH);
        ctlX += 52;
        smoothingBox_.setBounds(ctlX, drawerRowY, 84, ctlH);
        ctlX += 88;
        channelBox_.setBounds(ctlX, drawerRowY, 60, ctlH);

        // Section 3: Display & Snapshots
        ctlX = 744;
        magRangeBox_.setBounds(ctlX, drawerRowY, 68, ctlH);
        ctlX += 72;
        unwrapButton_.setBounds(ctlX, drawerRowY, 74, ctlH);
        ctlX += 78;
        showH2Button_.setBounds(ctlX, drawerRowY, 54, ctlH);
        ctlX += 58;
        storeAButton_.setBounds(ctlX, drawerRowY, 46, ctlH);
        ctlX += 48;
        showAButton_.setBounds(ctlX, drawerRowY, 26, ctlH);
        ctlX += 30;
        storeBButton_.setBounds(ctlX, drawerRowY, 46, ctlH);
        ctlX += 48;
        showBButton_.setBounds(ctlX, drawerRowY, 26, ctlH);

        // Section 4: Time Alignment & Help
        ctlX = 998;
        delayCompButton_.setBounds(ctlX, drawerRowY, 76, ctlH);
        ctlX += 80;
        delayMsLabel_.setBounds(ctlX, drawerRowY, 38, ctlH);
        ctlX += 40;
        delayMsSlider_.setBounds(ctlX, drawerRowY, 74, ctlH);
        ctlX += 78;
        helpBtn_.setBounds(ctlX, drawerRowY, 26, ctlH);
    }
}

} // namespace ui
