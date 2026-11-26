#include "ControlBarComponent.h"
#include "Theme.h"

#include <cmath>

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
        addAndMakeVisible(box);
    };

    // --- ROW 1: Routing & Primary Controls ---
    initCombo(routingBox_, plugin::ParamID::routingMode);
    routingAtt_ = std::make_unique<ComboAttachment>(apvts, plugin::ParamID::routingMode, routingBox_);

    initCombo(tapChannelBox_, plugin::ParamID::tapChannel);
    tapChannelAtt_ = std::make_unique<ComboAttachment>(apvts, plugin::ParamID::tapChannel, tapChannelBox_);

    addAndMakeVisible(swapButton_);
    swapButton_.setClickingTogglesState(true);
    swapAtt_ = std::make_unique<ButtonAttachment>(apvts, plugin::ParamID::swapRefMeas, swapButton_);

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

    // --- ROW 2: Smoothing, Channels, Generator, Snapshots ---
    initCombo(smoothingBox_, plugin::ParamID::smoothingMode);
    smoothingAtt_ = std::make_unique<ComboAttachment>(apvts, plugin::ParamID::smoothingMode, smoothingBox_);

    initCombo(channelBox_, plugin::ParamID::channelMode);
    channelAtt_ = std::make_unique<ComboAttachment>(apvts, plugin::ParamID::channelMode, channelBox_);

    initCombo(generatorBox_, plugin::ParamID::generatorType);
    generatorAtt_ = std::make_unique<ComboAttachment>(apvts, plugin::ParamID::generatorType, generatorBox_);

    genGainSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    genGainSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 44, 18);
    addAndMakeVisible(genGainSlider_);
    addAndMakeVisible(genGainLabel_);
    genGainLabel_.setColour(juce::Label::textColourId, Theme::textSecondary);
    genGainAtt_ = std::make_unique<SliderAttachment>(apvts, plugin::ParamID::generatorGainDb, genGainSlider_);

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

    // --- ROW 3 & 4: Advanced Drawer Controls ---
    initCombo(fftSizeBox_, plugin::ParamID::fftSizeIndex);
    fftAtt_ = std::make_unique<ComboAttachment>(apvts, plugin::ParamID::fftSizeIndex, fftSizeBox_);

    initCombo(windowBox_, plugin::ParamID::windowType);
    windowAtt_ = std::make_unique<ComboAttachment>(apvts, plugin::ParamID::windowType, windowBox_);

    initCombo(avgModeBox_, plugin::ParamID::averagingMode);
    avgModeAtt_ = std::make_unique<ComboAttachment>(apvts, plugin::ParamID::averagingMode, avgModeBox_);

    alphaSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    alphaSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 18);
    addChildComponent(alphaSlider_);
    addChildComponent(alphaLabel_);
    alphaLabel_.setColour(juce::Label::textColourId, Theme::textSecondary);
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
    delayMsSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 48, 18);
    addChildComponent(delayMsSlider_);
    addChildComponent(delayMsLabel_);
    delayMsLabel_.setColour(juce::Label::textColourId, Theme::textSecondary);
    delayMsAtt_ = std::make_unique<SliderAttachment>(apvts, plugin::ParamID::delayCompMs, delayMsSlider_);
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

    // Apple-style brand wordmark on top-left
    g.setColour(Theme::textPrimary);
    g.setFont(juce::FontOptions(15.5f, juce::Font::bold));
    g.drawText("PluginCheq", 14, 10, 110, 20, juce::Justification::centredLeft, false);

    g.setColour(Theme::textSecondary);
    g.setFont(juce::FontOptions(10.0f, juce::Font::plain));
    g.drawText("PRECISION DSP LAB", 14, 28, 110, 14, juce::Justification::centredLeft, false);

    // Routing status badge under wordmark
    juce::String routeBadge;
    juce::Colour badgeCol = Theme::textMuted;

    if (routingMode_ == 2) { // Tap Sender
        routeBadge = "SENDING TAP...";
        badgeCol = Theme::traceMagnitude;
    } else if (routingMode_ == 1) { // Plugin Tap Bus
        routeBadge = tapBusActive_ ? "TAP BUS: LOCKED" : "TAP BUS: SEARCHING";
        badgeCol = tapBusActive_ ? Theme::traceCoherence : Theme::meterClip;
    } else if (routingMode_ == 3) { // Internal Generator
        routeBadge = "INT. GENERATOR";
        badgeCol = Theme::traceH2;
    } else { // DAW Sidechain
        routeBadge = sidechainActive_ ? "SIDECHAIN: OK" : "NO SIDECHAIN";
        badgeCol = sidechainActive_ ? Theme::traceCoherence : Theme::textMuted;
    }

    g.setColour(badgeCol);
    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    g.drawText(routeBadge, 14, 46, 120, 14, juce::Justification::centredLeft, false);

    // Delay readout
    juce::String delayStr = delayEstimate_.valid
        ? ("Delay: " + juce::String(delayEstimate_.delayMs, 2) + " ms (" + juce::String(delayEstimate_.delaySamples, 1) + " spl)")
        : "Delay: --";
    g.setColour(delayEstimate_.valid ? Theme::textPrimary : Theme::textMuted);
    g.setFont(juce::FontOptions(10.5f, juce::Font::plain));
    g.drawText(delayStr, 14, 62, 126, 14, juce::Justification::centredLeft, false);

    // Meters on right side
    const float meterX = static_cast<float>(getWidth() - 256);
    const float refDb = linearToDb(refPeak_);
    const float measDb = linearToDb(measPeak_);

    g.setFont(10.5f);
    g.setColour(Theme::textPrimary);
    g.drawText("REF (Clean)", static_cast<int>(meterX), 8, 70, 14, juce::Justification::centredLeft, false);
    g.drawText("MEAS (Proc)", static_cast<int>(meterX), 26, 70, 14, juce::Justification::centredLeft, false);

    auto drawMeter = [&](float y, float db, juce::Colour col) {
        const float barX = meterX + 74.0f;
        const float barW = 100.0f;
        const float norm = std::clamp((db + 60.0f) / 60.0f, 0.0f, 1.0f);
        g.setColour(Theme::controlSurface);
        g.fillRoundedRectangle(barX, y + 2.0f, barW, 8.0f, 3.0f);
        g.setColour(db > -0.1f ? Theme::meterClip : col);
        g.fillRoundedRectangle(barX, y + 2.0f, barW * norm, 8.0f, 3.0f);

        g.setColour(Theme::textSecondary);
        g.drawText(db <= -119.0f ? "-inf" : juce::String(db, 1) + " dB",
                   static_cast<int>(barX + barW + 6.0f), static_cast<int>(y), 50, 14,
                   juce::Justification::centredLeft, false);
    };

    drawMeter(8.0f, refDb, Theme::meterRef);
    drawMeter(26.0f, measDb, Theme::meterMeas);

    // Host status info line
    const juce::String info = juce::String(sampleRate_ / 1000.0, 1) + " kHz | "
        + juce::String(blockSize_) + " spl";
    g.setColour(Theme::textSecondary);
    g.drawText(info, static_cast<int>(meterX), 46, 200, 14, juce::Justification::centredLeft, false);

    if (isAdvancedOpen_) {
        g.setColour(Theme::borderLightGray.withAlpha(0.4f));
        g.drawHorizontalLine(76, 12.0f, static_cast<float>(getWidth() - 12));
    }
}

void ControlBarComponent::resized() {
    auto area = getLocalBounds().reduced(10, 6);
    area.removeFromLeft(134);  // logo and status
    area.removeFromRight(260); // meters

    // ROW 1: Routing & Main Action Buttons
    auto row1 = area.removeFromTop(28);
    routingBox_.setBounds(row1.removeFromLeft(150));
    row1.removeFromLeft(6);
    tapChannelBox_.setBounds(row1.removeFromLeft(74));
    row1.removeFromLeft(6);
    swapButton_.setBounds(row1.removeFromLeft(72));
    row1.removeFromLeft(8);
    findDelayButton_.setBounds(row1.removeFromLeft(136));
    row1.removeFromLeft(6);
    freezeButton_.setBounds(row1.removeFromLeft(62));
    row1.removeFromLeft(6);
    resetAvgButton_.setBounds(row1.removeFromLeft(54));
    row1.removeFromLeft(8);
    advancedToggleBtn_.setBounds(row1.removeFromLeft(90));
    row1.removeFromLeft(6);
    helpBtn_.setBounds(row1.removeFromLeft(28));

    area.removeFromTop(6);

    // ROW 2: Primary Controls (Smoothing, Generator, Snapshots)
    auto row2 = area.removeFromTop(26);
    smoothingBox_.setBounds(row2.removeFromLeft(108));
    row2.removeFromLeft(6);
    channelBox_.setBounds(row2.removeFromLeft(88));
    row2.removeFromLeft(8);
    generatorBox_.setBounds(row2.removeFromLeft(108));
    row2.removeFromLeft(4);
    genGainLabel_.setBounds(row2.removeFromLeft(30));
    genGainSlider_.setBounds(row2.removeFromLeft(84));
    row2.removeFromLeft(12);
    storeAButton_.setBounds(row2.removeFromLeft(56));
    row2.removeFromLeft(4);
    showAButton_.setBounds(row2.removeFromLeft(32));
    row2.removeFromLeft(6);
    storeBButton_.setBounds(row2.removeFromLeft(56));
    row2.removeFromLeft(4);
    showBButton_.setBounds(row2.removeFromLeft(32));

    // If Advanced Drawer is Open
    if (isAdvancedOpen_) {
        area.removeFromTop(12);
        auto row3 = area.removeFromTop(24);
        fftSizeBox_.setBounds(row3.removeFromLeft(80));
        row3.removeFromLeft(6);
        windowBox_.setBounds(row3.removeFromLeft(115));
        row3.removeFromLeft(6);
        avgModeBox_.setBounds(row3.removeFromLeft(95));
        row3.removeFromLeft(4);
        alphaLabel_.setBounds(row3.removeFromLeft(38));
        alphaSlider_.setBounds(row3.removeFromLeft(90));
        row3.removeFromLeft(8);
        magRangeBox_.setBounds(row3.removeFromLeft(82));
        row3.removeFromLeft(6);
        unwrapButton_.setBounds(row3.removeFromLeft(100));
        row3.removeFromLeft(4);
        showH2Button_.setBounds(row3.removeFromLeft(74));

        area.removeFromTop(6);
        auto row4 = area.removeFromTop(24);
        delayCompButton_.setBounds(row4.removeFromLeft(95));
        row4.removeFromLeft(6);
        delayMsLabel_.setBounds(row4.removeFromLeft(70));
        delayMsSlider_.setBounds(row4.removeFromLeft(130));
    }
}

} // namespace ui
