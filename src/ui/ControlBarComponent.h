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
    std::function<void()> onToggleAdvanced;
    std::function<void()> onOpenSettings;
    std::function<void()> onOpenHelp;
    std::function<void(int viewIndex)> onViewChanged;

    void updateStatus(float refPeak,
                      float measPeak,
                      double sampleRate,
                      int blockSize,
                      bool sidechainActive,
                      bool tapBusActive,
                      int routingMode,
                      const dsp::DelayEstimate& delayEstimate);

    void setAdvancedOpen(bool isOpen);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    plugin::DSPAnalysisProcessor& processor_;
    bool isAdvancedOpen_ = false;

    // --- VIEW SELECTORS (PluginDoctor Tabs) ---
    juce::TextButton viewTransferBtn_{"Linear / EQ"};
    juce::TextButton viewDynamicsBtn_{"Dynamics"};
    juce::TextButton viewHarmonicsBtn_{"Harmonics / THD"};
    int currentView_ = 0; // 0 = Transfer, 1 = Dynamics, 2 = Harmonics

    // --- ROW 1: Routing & Primary Workflow ---
    juce::ComboBox routingBox_;
    juce::ComboBox tapChannelBox_;
    juce::TextButton swapButton_{"⇄ Swap"};
    juce::TextButton findDelayButton_{"⚡ Auto-Align Delay"};
    juce::TextButton freezeButton_{"Freeze"};
    juce::TextButton resetAvgButton_{"Reset"};
    juce::TextButton advancedToggleBtn_{"⚙ Advanced"};
    juce::TextButton helpBtn_{"?"};

    // --- ROW 2 (Primary bar): Smoothing, Snapshots & Generator ---
    juce::ComboBox smoothingBox_;
    juce::ComboBox channelBox_;
    juce::ComboBox generatorBox_;
    juce::Slider genGainSlider_;
    juce::Label genGainLabel_{{}, "Gen:"};
    juce::TextButton storeAButton_{"Save A"};
    juce::ToggleButton showAButton_{"A"};
    juce::TextButton storeBButton_{"Save B"};
    juce::ToggleButton showBButton_{"B"};

    // --- ROW 3 & 4 (Advanced Settings Drawer) ---
    juce::ComboBox fftSizeBox_;
    juce::ComboBox windowBox_;
    juce::ComboBox avgModeBox_;
    juce::Slider alphaSlider_;
    juce::Label alphaLabel_{{}, "Alpha:"};
    juce::ComboBox magRangeBox_;
    juce::ToggleButton unwrapButton_{"Unwrap Phase"};
    juce::ToggleButton showH2Button_{"Show H2"};
    juce::ToggleButton delayCompButton_{"Comp Delay"};
    juce::Slider delayMsSlider_;
    juce::Label delayMsLabel_{{}, "Manual ms:"};

    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    std::unique_ptr<ComboAttachment> routingAtt_;
    std::unique_ptr<ComboAttachment> tapChannelAtt_;
    std::unique_ptr<ButtonAttachment> swapAtt_;
    std::unique_ptr<ComboAttachment> smoothingAtt_;
    std::unique_ptr<ComboAttachment> channelAtt_;
    std::unique_ptr<ComboAttachment> generatorAtt_;
    std::unique_ptr<SliderAttachment> genGainAtt_;

    std::unique_ptr<ComboAttachment> fftAtt_;
    std::unique_ptr<ComboAttachment> windowAtt_;
    std::unique_ptr<ComboAttachment> avgModeAtt_;
    std::unique_ptr<SliderAttachment> alphaAtt_;
    std::unique_ptr<ComboAttachment> magRangeAtt_;
    std::unique_ptr<ButtonAttachment> unwrapAtt_;
    std::unique_ptr<ButtonAttachment> showH2Att_;
    std::unique_ptr<ButtonAttachment> delayCompAtt_;
    std::unique_ptr<SliderAttachment> delayMsAtt_;

    float refPeak_ = 0.0f;
    float measPeak_ = 0.0f;
    double sampleRate_ = 48000.0;
    int blockSize_ = 512;
    bool sidechainActive_ = false;
    bool tapBusActive_ = false;
    int routingMode_ = 1;
    dsp::DelayEstimate delayEstimate_{};
};

} // namespace ui
