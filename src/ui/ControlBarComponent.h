#pragma once

#include "dsp/DelayFinder.h"
#include "plugin/PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace ui {

class ControlBarComponent final : public juce::Component {
public:
    explicit ControlBarComponent(plugin::DSPAnalysisProcessor& processor);

    std::function<void(bool frozen)> onFreezeChanged;
    std::function<void()> onStoreSnapshotA;
    std::function<void()> onStoreSnapshotB;
    std::function<void(bool show)> onShowSnapshotAChanged;
    std::function<void(bool show)> onShowSnapshotBChanged;
    std::function<void()> onFindDelayClicked;

    void updateStatus(float refPeak,
                      float measPeak,
                      double sampleRate,
                      int blockSize,
                      bool sidechainActive,
                      const dsp::DelayEstimate& delayEstimate);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    plugin::DSPAnalysisProcessor& processor_;

    // Row 1: Routing & FFT
    juce::TextButton swapButton_{"Swap Ref / Meas"};
    juce::TextButton resetAvgButton_{"Reset Avg"};
    juce::TextButton freezeButton_{"Freeze"};
    juce::ComboBox channelBox_;
    juce::ComboBox fftSizeBox_;
    juce::ComboBox windowBox_;

    // Row 2: Averaging, Smoothing & Display
    juce::ComboBox avgModeBox_;
    juce::Slider alphaSlider_;
    juce::Label alphaLabel_{{}, "Alpha"};
    juce::ComboBox smoothingBox_;
    juce::ComboBox magRangeBox_;
    juce::ToggleButton unwrapButton_{"Unwrap"};
    juce::ToggleButton showH2Button_{"H2"};

    // Row 3: Delay Finder & A/B Snapshots
    juce::TextButton findDelayButton_{"Find Delay"};
    juce::ToggleButton delayCompButton_{"Comp"};
    juce::TextButton zeroDelayButton_{"0 ms"};
    juce::TextButton storeAButton_{"Save A"};
    juce::ToggleButton showAButton_{"A"};
    juce::TextButton storeBButton_{"Save B"};
    juce::ToggleButton showBButton_{"B"};

    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    std::unique_ptr<ButtonAttachment> swapAtt_;
    std::unique_ptr<ComboAttachment> channelAtt_;
    std::unique_ptr<ComboAttachment> fftAtt_;
    std::unique_ptr<ComboAttachment> windowAtt_;
    std::unique_ptr<ComboAttachment> avgModeAtt_;
    std::unique_ptr<SliderAttachment> alphaAtt_;
    std::unique_ptr<ComboAttachment> smoothingAtt_;
    std::unique_ptr<ComboAttachment> magRangeAtt_;
    std::unique_ptr<ButtonAttachment> unwrapAtt_;
    std::unique_ptr<ButtonAttachment> showH2Att_;
    std::unique_ptr<ButtonAttachment> delayCompAtt_;

    float refPeak_ = 0.0f;
    float measPeak_ = 0.0f;
    double sampleRate_ = 48000.0;
    int blockSize_ = 512;
    bool sidechainActive_ = false;
    dsp::DelayEstimate delayEstimate_{};
};

} // namespace ui
