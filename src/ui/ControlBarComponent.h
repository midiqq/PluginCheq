#pragma once

#include "dsp/DelayFinder.h"
#include "plugin/PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace ui {

/**
 * Precision Laboratory Control Bar:
 *   - Apple-style segmented tabs: [ Linear / EQ ] [ Dynamics ] [ Harmonics / THD ]
 *   - Brand wordmark & dynamic telemetry badge (Tap Bus / Sidechain / Delay Lock)
 *   - Primary measurement action buttons (Auto-Align, Freeze, Reset, Advanced Drawer)
 *   - Signal routing, smoothing, channel mode, internal test generator, and A/B snapshots
 *   - Expandable Advanced Drawer with grouped Analysis Engine & Display options
 *   - Precision dual peak meters with clip detection
 */
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
    bool isAdvancedOpen() const noexcept { return isAdvancedOpen_; }

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void updateViewButtonStates();

    plugin::DSPAnalysisProcessor& processor_;
    bool isAdvancedOpen_ = false;

    // --- VIEW SELECTORS (Apple Segmented Tab Capsule) ---
    juce::TextButton viewTransferBtn_{"Linear / EQ"};
    juce::TextButton viewDynamicsBtn_{"Dynamics"};
    juce::TextButton viewHarmonicsBtn_{"Harmonics / THD"};
    int currentView_ = 0; // 0 = Transfer, 1 = Dynamics, 2 = Harmonics
    juce::Rectangle<int> segmentedCapsuleBounds_;

    // --- ROW 1: Primary Action Buttons ---
    juce::TextButton findDelayButton_{"Auto-Align Delay"};
    juce::TextButton freezeButton_{"Freeze"};
    juce::TextButton resetAvgButton_{"Reset"};
    juce::TextButton advancedToggleBtn_{"Advanced"};
    juce::TextButton helpBtn_{"?"};

    // --- ROW 2: Primary Measurement Bar (Routing, Smoothing, Generator, Snapshots) ---
    juce::ComboBox routingBox_;
    juce::ComboBox tapChannelBox_;
    juce::TextButton swapButton_{"Swap"};
    juce::ComboBox smoothingBox_;
    juce::ComboBox channelBox_;
    juce::ComboBox generatorBox_;
    juce::Slider genGainSlider_;
    juce::Label genGainLabel_{{}, "Gen:"};
    juce::TextButton storeAButton_{"Save A"};
    juce::ToggleButton showAButton_{"A"};
    juce::TextButton storeBButton_{"Save B"};
    juce::ToggleButton showBButton_{"B"};

    // --- ADVANCED SETTINGS DRAWER ---
    // Section 1: FFT Engine
    juce::ComboBox fftSizeBox_;
    juce::ComboBox windowBox_;
    juce::ComboBox avgModeBox_;
    juce::Slider alphaSlider_;
    juce::Label alphaLabel_{{}, "Alpha:"};

    // Section 2: Display & Traces
    juce::ComboBox magRangeBox_;
    juce::ToggleButton unwrapButton_{"Unwrap Phase"};
    juce::ToggleButton showH2Button_{"Show H2"};

    // Section 3: Time Alignment
    juce::ToggleButton delayCompButton_{"Comp Delay"};
    juce::Slider delayMsSlider_;
    juce::Label delayMsLabel_{{}, "Delay ms:"};

    // Attachments
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

    // Telemetry state
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
