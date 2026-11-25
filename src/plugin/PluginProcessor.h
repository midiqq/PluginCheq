#pragma once

#include "AnalysisWorker.h"
#include "Parameters.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>
#include <vector>

namespace plugin {

class DSPAnalysisProcessor final : public juce::AudioProcessor,
                                   private juce::AudioProcessorValueTreeState::Listener {
public:
    DSPAnalysisProcessor();
    ~DSPAnalysisProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& apvts() noexcept { return apvts_; }
    AnalysisWorker& worker() noexcept { return worker_; }

    float getRefPeakLinear() const noexcept { return refPeak_.load(std::memory_order_relaxed); }
    float getMeasPeakLinear() const noexcept { return measPeak_.load(std::memory_order_relaxed); }
    double getCurrentSampleRate() const noexcept { return currentSampleRate_.load(std::memory_order_relaxed); }
    int getCurrentBlockSize() const noexcept { return currentBlockSize_.load(std::memory_order_relaxed); }
    bool isSidechainActive() const noexcept { return sidechainActive_.load(std::memory_order_relaxed); }

private:
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void syncWorkerConfig();

    juce::AudioProcessorValueTreeState apvts_;
    AnalysisWorker worker_;

    // Pre-allocated scratch buffers for audio callback (zero allocs in processBlock)
    std::vector<double> scratchMain_;
    std::vector<double> scratchSidechain_;

    std::atomic<float> refPeak_{0.0f};
    std::atomic<float> measPeak_{0.0f};
    std::atomic<double> currentSampleRate_{48000.0};
    std::atomic<int> currentBlockSize_{512};
    std::atomic<bool> sidechainActive_{false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DSPAnalysisProcessor)
};

} // namespace plugin
