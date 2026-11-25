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

    addAndMakeVisible(swapButton_);
    swapButton_.setClickingTogglesState(true);
    swapAtt_ = std::make_unique<ButtonAttachment>(apvts, plugin::ParamID::swapRefMeas, swapButton_);

    addAndMakeVisible(resetAvgButton_);
    resetAvgButton_.onClick = [this] { processor_.worker().requestResetAveraging(); };

    addAndMakeVisible(freezeButton_);
    freezeButton_.setClickingTogglesState(true);
    freezeButton_.onClick = [this] {
        if (onFreezeChanged) onFreezeChanged(freezeButton_.getToggleState());
    };

    auto initCombo = [this](juce::ComboBox& box, const char* paramId) {
        if (auto* param = dynamic_cast<juce::AudioParameterChoice*>(processor_.apvts().getParameter(paramId))) {
            box.addItemList(param->choices, 1);
        }
        addAndMakeVisible(box);
    };

    initCombo(channelBox_, plugin::ParamID::channelMode);
    initCombo(fftSizeBox_, plugin::ParamID::fftSizeIndex);
    initCombo(windowBox_, plugin::ParamID::windowType);
    initCombo(avgModeBox_, plugin::ParamID::averagingMode);
    initCombo(smoothingBox_, plugin::ParamID::smoothingMode);
    initCombo(magRangeBox_, plugin::ParamID::magRangeDb);

    channelAtt_   = std::make_unique<ComboAttachment>(apvts, plugin::ParamID::channelMode, channelBox_);
    fftAtt_       = std::make_unique<ComboAttachment>(apvts, plugin::ParamID::fftSizeIndex, fftSizeBox_);
    windowAtt_    = std::make_unique<ComboAttachment>(apvts, plugin::ParamID::windowType, windowBox_);
    avgModeAtt_   = std::make_unique<ComboAttachment>(apvts, plugin::ParamID::averagingMode, avgModeBox_);
    smoothingAtt_ = std::make_unique<ComboAttachment>(apvts, plugin::ParamID::smoothingMode, smoothingBox_);
    magRangeAtt_  = std::make_unique<ComboAttachment>(apvts, plugin::ParamID::magRangeDb, magRangeBox_);

    alphaSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    alphaSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 20);
    addAndMakeVisible(alphaSlider_);
    addAndMakeVisible(alphaLabel_);
    alphaLabel_.setColour(juce::Label::textColourId, Theme::textSecondary);
    alphaAtt_ = std::make_unique<SliderAttachment>(apvts, plugin::ParamID::averagingAlpha, alphaSlider_);

    addAndMakeVisible(unwrapButton_);
    unwrapAtt_ = std::make_unique<ButtonAttachment>(apvts, plugin::ParamID::unwrapPhase, unwrapButton_);

    addAndMakeVisible(showH2Button_);
    showH2Att_ = std::make_unique<ButtonAttachment>(apvts, plugin::ParamID::showH2, showH2Button_);

    // Delay Finder & Compensation
    addAndMakeVisible(findDelayButton_);
    findDelayButton_.onClick = [this] {
        if (onFindDelayClicked) onFindDelayClicked();
    };

    addAndMakeVisible(delayCompButton_);
    delayCompAtt_ = std::make_unique<ButtonAttachment>(apvts, plugin::ParamID::delayCompEnabled, delayCompButton_);

    addAndMakeVisible(zeroDelayButton_);
    zeroDelayButton_.onClick = [this] {
        if (auto* p = processor_.apvts().getParameter(plugin::ParamID::delayCompMs)) {
            p->setValueNotifyingHost(p->convertTo0to1(0.0f));
        }
        if (auto* pOn = processor_.apvts().getParameter(plugin::ParamID::delayCompEnabled)) {
            pOn->setValueNotifyingHost(0.0f);
        }
    };

    // A/B Snapshots
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
}

void ControlBarComponent::updateStatus(float refPeak,
                                       float measPeak,
                                       double sampleRate,
                                       int blockSize,
                                       bool sidechainActive,
                                       const dsp::DelayEstimate& delayEstimate) {
    refPeak_ = refPeak;
    measPeak_ = measPeak;
    sampleRate_ = sampleRate;
    blockSize_ = blockSize;
    sidechainActive_ = sidechainActive;
    delayEstimate_ = delayEstimate;
    repaint();
}

void ControlBarComponent::paint(juce::Graphics& g) {
    g.fillAll(Theme::barBg);
    g.setColour(Theme::borderSubtle);
    g.drawHorizontalLine(getHeight() - 1, 0.0f, static_cast<float>(getWidth()));

    // Apple-style minimalist brand wordmark on top-left
    g.setColour(Theme::textPrimary);
    g.setFont(juce::FontOptions(15.5f, juce::Font::bold));
    g.drawText("PluginCheq", 14, 12, 110, 22, juce::Justification::centredLeft, false);

    g.setColour(Theme::textSecondary);
    g.setFont(juce::FontOptions(10.5f, juce::Font::plain));
    g.drawText("TRANSFER LAB", 14, 34, 110, 14, juce::Justification::centredLeft, false);

    // Delay Finder live readout under wordmark
    juce::String delayText = delayEstimate_.valid
        ? (juce::String(delayEstimate_.delayMs, 2) + " ms (" + juce::String(delayEstimate_.delaySamples, 1) + " spl)")
        : "No Lock";
    g.setColour(delayEstimate_.valid ? Theme::textPrimary : Theme::textMuted);
    g.setFont(juce::FontOptions(10.5f, juce::Font::plain));
    g.drawText(delayText, 14, 56, 112, 16, juce::Justification::centredLeft, false);

    if (delayEstimate_.valid && delayEstimate_.polarityInverted) {
        g.setColour(Theme::meterClip);
        g.drawText("POL: INV", 14, 74, 110, 14, juce::Justification::centredLeft, false);
    } else {
        g.setColour(Theme::textMuted);
        g.drawText("POL: +", 14, 74, 110, 14, juce::Justification::centredLeft, false);
    }

    // Draw Ref & Meas signal meters on the right side
    const float meterX = static_cast<float>(getWidth() - 252);
    const float refDb = linearToDb(refPeak_);
    const float measDb = linearToDb(measPeak_);

    g.setFont(11.0f);
    g.setColour(Theme::textPrimary);
    g.drawText("REF", static_cast<int>(meterX), 12, 36, 16, juce::Justification::centredLeft, false);
    g.drawText("MEAS", static_cast<int>(meterX), 34, 40, 16, juce::Justification::centredLeft, false);

    auto drawMeterBar = [&](float y, float db, juce::Colour col) {
        const float barX = meterX + 44.0f;
        const float barW = 110.0f;
        const float norm = std::clamp((db + 60.0f) / 60.0f, 0.0f, 1.0f);
        g.setColour(Theme::controlSurface);
        g.fillRoundedRectangle(barX, y + 3.0f, barW, 8.0f, 3.0f);
        g.setColour(db > -0.1f ? Theme::meterClip : col);
        g.fillRoundedRectangle(barX, y + 3.0f, barW * norm, 8.0f, 3.0f);

        g.setColour(Theme::textSecondary);
        g.drawText(db <= -119.0f ? "-inf" : juce::String(db, 1) + " dB",
                   static_cast<int>(barX + barW + 6.0f), static_cast<int>(y), 54, 16,
                   juce::Justification::centredLeft, false);
    };

    drawMeterBar(12.0f, refDb, Theme::meterRef);
    drawMeterBar(34.0f, measDb, Theme::meterMeas);

    const float compMs = processor_.apvts().getRawParameterValue(plugin::ParamID::delayCompMs)->load();
    const bool compOn = processor_.apvts().getRawParameterValue(plugin::ParamID::delayCompEnabled)->load() >= 0.5f;
    const juce::String compStr = compOn ? ("Comp: " + juce::String(compMs, 2) + " ms") : "Comp: Off";

    const juce::String info = juce::String(sampleRate_ / 1000.0, 1) + " kHz | "
        + juce::String(blockSize_) + " spl | SC: " + (sidechainActive_ ? "ON" : "OFF");

    g.setColour(sidechainActive_ ? Theme::traceCoherence : Theme::textSecondary);
    g.setFont(11.0f);
    g.drawText(info, static_cast<int>(meterX), 56, 240, 16, juce::Justification::centredLeft, false);

    g.setColour(compOn ? Theme::traceMagnitude : Theme::textMuted);
    g.drawText(compStr, static_cast<int>(meterX), 74, 240, 16, juce::Justification::centredLeft, false);
}

void ControlBarComponent::resized() {
    auto area = getLocalBounds().reduced(12, 8);
    area.removeFromLeft(122);  // reserved for PluginCheq wordmark + delay readout
    area.removeFromRight(258); // reserved for meters & host status

    auto row1 = area.removeFromTop(26);
    swapButton_.setBounds(row1.removeFromLeft(118));
    row1.removeFromLeft(6);
    resetAvgButton_.setBounds(row1.removeFromLeft(78));
    row1.removeFromLeft(6);
    freezeButton_.setBounds(row1.removeFromLeft(66));
    row1.removeFromLeft(8);
    channelBox_.setBounds(row1.removeFromLeft(90));
    row1.removeFromLeft(6);
    fftSizeBox_.setBounds(row1.removeFromLeft(80));
    row1.removeFromLeft(6);
    windowBox_.setBounds(row1.removeFromLeft(120));

    area.removeFromTop(6);
    auto row2 = area.removeFromTop(24);
    avgModeBox_.setBounds(row2.removeFromLeft(105));
    row2.removeFromLeft(6);
    alphaLabel_.setBounds(row2.removeFromLeft(36));
    alphaSlider_.setBounds(row2.removeFromLeft(126));
    row2.removeFromLeft(6);
    smoothingBox_.setBounds(row2.removeFromLeft(102));
    row2.removeFromLeft(6);
    magRangeBox_.setBounds(row2.removeFromLeft(88));
    row2.removeFromLeft(6);
    unwrapButton_.setBounds(row2.removeFromLeft(72));
    row2.removeFromLeft(4);
    showH2Button_.setBounds(row2.removeFromLeft(46));

    area.removeFromTop(6);
    auto row3 = area.removeFromTop(24);
    findDelayButton_.setBounds(row3.removeFromLeft(84));
    row3.removeFromLeft(6);
    delayCompButton_.setBounds(row3.removeFromLeft(64));
    row3.removeFromLeft(4);
    zeroDelayButton_.setBounds(row3.removeFromLeft(50));
    row3.removeFromLeft(16);
    storeAButton_.setBounds(row3.removeFromLeft(62));
    row3.removeFromLeft(4);
    showAButton_.setBounds(row3.removeFromLeft(36));
    row3.removeFromLeft(8);
    storeBButton_.setBounds(row3.removeFromLeft(62));
    row3.removeFromLeft(4);
    showBButton_.setBounds(row3.removeFromLeft(36));
}

} // namespace ui
