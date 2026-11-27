#pragma once

#include "dsp/DelayFinder.h"
#include "plugin/PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace ui {

/**
 * Teenage Engineering (OP-1 / TX-6) Ultra-Compact Control Bar & Settings Drawer:
 *   - Slim high-density primary bar (~38-40px height)
 *   - Left: TE tactile button strip: [⚡ ALIGN] [A / B] [FREEZE] [RESET] [SMOOTH] [SWAP]
 *   - Center: Compact segmented view tabs: [EQ] | [DYNAMICS] | [THD]
 *   - Right: Segmented LED meters (REF & MEAS), digital delay readout, minimal gear [⚙] button
 *   - Expandable Settings Drawer containing all secondary engine/display/routing parameters
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
    void updateSmoothButtonState();

    plugin::DSPAnalysisProcessor& processor_;
    bool isAdvancedOpen_ = false;

    // --- TE PRIMARY ACTION STRIP (LEFT) ---
    juce::TextButton alignButton_{juce::CharPointer_UTF8("\xe2\x9a\xa1 ALIGN")};
    juce::TextButton abButton_{"A / B"};
    juce::TextButton freezeButton_{"FREEZE"};
    juce::TextButton resetAvgButton_{"RESET"};
    juce::TextButton smoothButton_{"SMOOTH"};
    juce::TextButton swapButton_{"SWAP"};

    int abState_ = 0; // 0 = default, 1 = snap A active, 2 = snap B active, 3 = both

    // --- VIEW SELECTORS (CENTER TABS) ---
    juce::TextButton viewTransferBtn_{"EQ"};
    juce::TextButton viewDynamicsBtn_{"DYNAMICS"};
    juce::TextButton viewHarmonicsBtn_{"THD"};
    int currentView_ = 0; // 0 = Transfer/EQ, 1 = Dynamics, 2 = Harmonics/THD
    juce::Rectangle<int> segmentedCapsuleBounds_;

    // --- RIGHT WIDGETS ---
    juce::TextButton settingsButton_{juce::CharPointer_UTF8("\xe2\x9a\x99")};
    juce::TextButton helpBtn_{"?"};
    juce::Rectangle<int> delayBadgeBounds_;
    juce::Rectangle<int> metersBounds_;

    // --- ADVANCED SETTINGS DRAWER (SECONDARY CONTROLS) ---
    juce::ComboBox routingBox_;
    juce::ComboBox tapChannelBox_;
    juce::ComboBox smoothingBox_;
    juce::ComboBox channelBox_;

    juce::TextButton storeAButton_{"Store A"};
    juce::ToggleButton showAButton_{"Show A"};
    juce::TextButton storeBButton_{"Store B"};
    juce::ToggleButton showBButton_{"Show B"};

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
