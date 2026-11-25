#pragma once

#include "dsp/AnalysisTypes.h"
#include "dsp/RingBuffer.h"
#include "dsp/TransferFunctionEngine.h"
#include "dsp/TripleBuffer.h"

#include <juce_core/juce_core.h>

#include <atomic>
#include <cstddef>
#include <vector>

namespace plugin {

/**
 * Background analysis thread that consumes lock-free SPSC ring buffers
 * from the real-time audio callback and publishes TransferFunctionFrames
 * into a lock-free TripleBuffer for the GUI.
 */
class AnalysisWorker final : public juce::Thread {
public:
    AnalysisWorker();
    ~AnalysisWorker() override;

    void prepare(double sampleRate, std::size_t maxBlockSize);
    void requestConfigUpdate(const dsp::TransferFunctionConfig& newConfig);
    void requestResetAveraging();

    /**
     * Real-time safe push from audio thread.
     * Precondition: refSamples and measSamples have `numSamples` elements.
     */
    void pushAudioBlock(const double* refSamples, const double* measSamples, std::size_t numSamples) noexcept;

    /**
     * GUI thread: fetches latest completed frame if available.
     */
    bool fetchLatestFrame(dsp::TransferFunctionFrame& outFrame);

    void run() override;

private:
    void applyPendingConfig();

    dsp::RingBuffer<double> refRing_{131072};
    dsp::RingBuffer<double> measRing_{131072};

    dsp::TransferFunctionEngine engine_;
    dsp::TripleBuffer<dsp::TransferFunctionFrame> tripleBuffer_;

    std::vector<double> refSlideBuf_;
    std::vector<double> measSlideBuf_;
    std::vector<double> hopScratchRef_;
    std::vector<double> hopScratchMeas_;

    // Pending config protected by a lightweight spin/lock (only touched on worker & message thread, NEVER audio thread)
    juce::SpinLock configLock_;
    dsp::TransferFunctionConfig pendingConfig_{};
    std::atomic<bool> configDirty_{false};
    std::atomic<bool> resetRequested_{false};
    std::atomic<bool> buffersResetRequested_{false};

    juce::WaitableEvent dataReadyEvent_;
};

} // namespace plugin
