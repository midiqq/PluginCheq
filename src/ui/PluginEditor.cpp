#include "PluginEditor.h"
#include "Theme.h"

namespace ui {

PluginEditor::PluginEditor(plugin::DSPAnalysisProcessor& processor)
    : AudioProcessorEditor(&processor),
      processor_(processor),
      controlBar_(processor) {
    setLookAndFeel(&lookAndFeel_);
    addAndMakeVisible(controlBar_);
    addAndMakeVisible(plot_);

    controlBar_.onFreezeChanged = [this](bool frozen) {
        isFrozen_ = frozen;
    };
    controlBar_.onStoreSnapshotA = [this] {
        plot_.setSnapshotA(latestFrame_);
    };
    controlBar_.onStoreSnapshotB = [this] {
        plot_.setSnapshotB(latestFrame_);
    };
    controlBar_.onShowSnapshotAChanged = [this](bool show) {
        plot_.setShowSnapshotA(show);
    };
    controlBar_.onShowSnapshotBChanged = [this](bool show) {
        plot_.setShowSnapshotB(show);
    };
    controlBar_.onFindDelayClicked = [this] {
        if (latestFrame_.delayEstimate.valid) {
            const float detectedMs = static_cast<float>(latestFrame_.delayEstimate.delayMs);
            if (auto* pMs = processor_.apvts().getParameter(plugin::ParamID::delayCompMs)) {
                pMs->setValueNotifyingHost(pMs->convertTo0to1(detectedMs));
            }
            if (auto* pOn = processor_.apvts().getParameter(plugin::ParamID::delayCompEnabled)) {
                pOn->setValueNotifyingHost(1.0f);
            }
        }
    };

    setResizable(true, true);
    setResizeLimits(880, 580, 2400, 1600);
    setSize(1120, 720);

    startTimerHz(30);
}

PluginEditor::~PluginEditor() {
    stopTimer();
    setLookAndFeel(nullptr);
}

void PluginEditor::timerCallback() {
    if (processor_.worker().fetchLatestFrame(latestFrame_) && !isFrozen_) {
        plot_.updateFrame(latestFrame_);
    }

    const bool unwrap =
        processor_.apvts().getRawParameterValue(plugin::ParamID::unwrapPhase)->load() >= 0.5f;
    const bool showH2 =
        processor_.apvts().getRawParameterValue(plugin::ParamID::showH2)->load() >= 0.5f;
    const float magRange = plugin::magRangeFromIndex(
        static_cast<int>(processor_.apvts().getRawParameterValue(plugin::ParamID::magRangeDb)->load()));

    plot_.setDisplayOptions(unwrap, showH2, magRange);

    controlBar_.updateStatus(
        processor_.getRefPeakLinear(),
        processor_.getMeasPeakLinear(),
        processor_.getCurrentSampleRate(),
        processor_.getCurrentBlockSize(),
        processor_.isSidechainActive(),
        latestFrame_.delayEstimate);
}

void PluginEditor::paint(juce::Graphics& g) {
    g.fillAll(Theme::backgroundDark);
}

void PluginEditor::resized() {
    auto area = getLocalBounds();
    controlBar_.setBounds(area.removeFromTop(102));
    plot_.setBounds(area.reduced(8, 6));
}

} // namespace ui
