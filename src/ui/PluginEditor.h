#pragma once

#include "ControlBarComponent.h"
#include "HelpOverlayComponent.h"
#include "PluginCheqLookAndFeel.h"
#include "TransferPlotComponent.h"
#include "DynamicsPlotComponent.h"
#include "HarmonicPlotComponent.h"
#include "plugin/PluginProcessor.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace ui {

class PluginEditor final : public juce::AudioProcessorEditor,
                           private juce::Timer {
public:
    explicit PluginEditor(plugin::DSPAnalysisProcessor& processor);
    ~PluginEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;

    PluginCheqLookAndFeel lookAndFeel_;
    plugin::DSPAnalysisProcessor& processor_;
    ControlBarComponent controlBar_;
    TransferPlotComponent plot_;
    DynamicsPlotComponent dynamicsPlot_;
    HarmonicPlotComponent harmonicPlot_;
    HelpOverlayComponent helpOverlay_;

    int currentView_ = 0; // 0 = Linear/EQ, 1 = Dynamics, 2 = Harmonics
    dsp::TransferFunctionFrame latestFrame_;
    bool isFrozen_ = false;
    bool isAdvancedOpen_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};

} // namespace ui
