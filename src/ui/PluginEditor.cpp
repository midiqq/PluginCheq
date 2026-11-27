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
    addChildComponent(dynamicsPlot_);
    addChildComponent(harmonicPlot_);
    addChildComponent(helpOverlay_);

    controlBar_.onViewChanged = [this](int viewIndex) {
        currentView_ = viewIndex;
        plot_.setVisible(currentView_ == 0);
        dynamicsPlot_.setVisible(currentView_ == 1);
        harmonicPlot_.setVisible(currentView_ == 2);
        resized();
    };

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
    controlBar_.onToggleAdvanced = [this] {
        isAdvancedOpen_ = !isAdvancedOpen_;
        controlBar_.setAdvancedOpen(isAdvancedOpen_);
        resized();
    };
    controlBar_.onOpenHelp = [this] {
        helpOverlay_.setVisible(true);
        helpOverlay_.toFront(true);
    };

    setResizable(true, true);
    setResizeLimits(920, 600, 2400, 1600);
    setSize(1140, 740);

    startTimerHz(30);
}

PluginEditor::~PluginEditor() {
    stopTimer();
    setLookAndFeel(nullptr);
}

void PluginEditor::timerCallback() {
    if (processor_.worker().fetchLatestFrame(latestFrame_) && !isFrozen_) {
        plot_.updateFrame(latestFrame_);
        dynamicsPlot_.updateFrame(latestFrame_.dynamicsFrame);
        harmonicPlot_.updateFrame(latestFrame_.harmonicFrame);
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
        processor_.isTapBusActive(),
        processor_.getCurrentRoutingMode(),
        latestFrame_.delayEstimate);
}

void PluginEditor::paint(juce::Graphics& g) {
    g.fillAll(Theme::backgroundDark);
}

void PluginEditor::resized() {
    auto area = getLocalBounds();
    const int barHeight = isAdvancedOpen_ ? 138 : 74;
    controlBar_.setBounds(area.removeFromTop(barHeight));
    const auto plotArea = area.reduced(8, 6);
    plot_.setBounds(plotArea);
    dynamicsPlot_.setBounds(plotArea);
    harmonicPlot_.setBounds(plotArea);
    helpOverlay_.setBounds(getLocalBounds());
}

} // namespace ui
