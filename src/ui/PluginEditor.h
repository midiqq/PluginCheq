#pragma once

#include "ControlBarComponent.h"
#include "PluginCheqLookAndFeel.h"
#include "TransferPlotComponent.h"
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

    dsp::TransferFunctionFrame latestFrame_;
    bool isFrozen_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};

} // namespace ui
