#include "AnalysisWorker.h"

#include <algorithm>
#include <cstring>

namespace plugin {

AnalysisWorker::AnalysisWorker()
    : juce::Thread("DSPAnalysisWorker") {
    dsp::TransferFunctionConfig initial;
    engine_.configure(initial);
    pendingConfig_ = initial;

    refSlideBuf_.assign(initial.fftSize, 0.0);
    measSlideBuf_.assign(initial.fftSize, 0.0);
    hopScratchRef_.assign(engine_.hopSize(), 0.0);
    hopScratchMeas_.assign(engine_.hopSize(), 0.0);
}

AnalysisWorker::~AnalysisWorker() {
    signalThreadShouldExit();
    dataReadyEvent_.signal();
    stopThread(2000);
}

void AnalysisWorker::prepare(double sampleRate, std::size_t maxBlockSize) {
    const std::size_t minCap = std::max<std::size_t>(131072, maxBlockSize * 16);
    if (refRing_.capacity() < minCap) {
        refRing_.resize(minCap);
        measRing_.resize(minCap);
    }

    {
        const juce::SpinLock::ScopedLockType sl(configLock_);
        pendingConfig_.sampleRate = sampleRate;
        configDirty_.store(true, std::memory_order_release);
    }

    dynamicsEngine_.prepare(sampleRate);
    harmonicEngine_.prepare(engine_.config().fftSize, sampleRate);

    buffersResetRequested_.store(true, std::memory_order_release);

    if (!isThreadRunning()) {
        startThread(juce::Thread::Priority::normal);
    }
    dataReadyEvent_.signal();
}

void AnalysisWorker::requestConfigUpdate(const dsp::TransferFunctionConfig& newConfig) {
    {
        const juce::SpinLock::ScopedLockType sl(configLock_);
        pendingConfig_ = newConfig;
        configDirty_.store(true, std::memory_order_release);
    }
    dataReadyEvent_.signal();
}

void AnalysisWorker::requestResetAveraging() {
    resetRequested_.store(true, std::memory_order_release);
    dataReadyEvent_.signal();
}

void AnalysisWorker::pushAudioBlock(const double* refSamples,
                                    const double* measSamples,
                                    std::size_t numSamples) noexcept {
    if (numSamples == 0) {
        return;
    }
    // Keep both ring buffers synchronized by pushing the minimum free space available in both
    const std::size_t availRef = refRing_.availableToWrite();
    const std::size_t availMeas = measRing_.availableToWrite();
    const std::size_t canWrite = std::min({numSamples, availRef, availMeas});

    if (canWrite > 0) {
        refRing_.push(refSamples, canWrite);
        measRing_.push(measSamples, canWrite);
        dataReadyEvent_.signal();
    }
}

bool AnalysisWorker::fetchLatestFrame(dsp::TransferFunctionFrame& outFrame) {
    if (tripleBuffer_.fetchLatest()) {
        outFrame = tripleBuffer_.getReadBuffer();
        return true;
    }
    return false;
}

void AnalysisWorker::applyPendingConfig() {
    if (buffersResetRequested_.exchange(false, std::memory_order_acq_rel)) {
        refRing_.reset();
        measRing_.reset();
        std::fill(refSlideBuf_.begin(), refSlideBuf_.end(), 0.0);
        std::fill(measSlideBuf_.begin(), measSlideBuf_.end(), 0.0);
    }

    if (configDirty_.exchange(false, std::memory_order_acq_rel)) {
        dsp::TransferFunctionConfig cfg;
        {
            const juce::SpinLock::ScopedLockType sl(configLock_);
            cfg = pendingConfig_;
        }

        const bool fftSizeChanged = (cfg.fftSize != engine_.config().fftSize);
        engine_.configure(cfg);

        if (fftSizeChanged || refSlideBuf_.size() != cfg.fftSize) {
            refSlideBuf_.assign(cfg.fftSize, 0.0);
            measSlideBuf_.assign(cfg.fftSize, 0.0);
        }
        hopScratchRef_.assign(engine_.hopSize(), 0.0);
        hopScratchMeas_.assign(engine_.hopSize(), 0.0);
    }

    if (resetRequested_.exchange(false, std::memory_order_acq_rel)) {
        engine_.resetAveraging();
    }
}

void AnalysisWorker::run() {
    while (!threadShouldExit()) {
        applyPendingConfig();

        const std::size_t hop = engine_.hopSize();
        const std::size_t fftSize = engine_.config().fftSize;
        const std::size_t available = std::min(refRing_.availableToRead(), measRing_.availableToRead());

        if (available < hop) {
            dataReadyEvent_.wait(20);
            continue;
        }

        // Process all ready hops
        while (!threadShouldExit()) {
            if (configDirty_.load(std::memory_order_acquire) ||
                buffersResetRequested_.load(std::memory_order_acquire)) {
                break;
            }

            const std::size_t ready = std::min(refRing_.availableToRead(), measRing_.availableToRead());
            if (ready < hop) {
                break;
            }

            refRing_.pop(hopScratchRef_.data(), hop);
            measRing_.pop(hopScratchMeas_.data(), hop);

            // Shift sliding window left by `hop` and append new `hop` samples
            if (hop < fftSize) {
                const std::size_t keep = fftSize - hop;
                std::memmove(refSlideBuf_.data(), refSlideBuf_.data() + hop, keep * sizeof(double));
                std::memmove(measSlideBuf_.data(), measSlideBuf_.data() + hop, keep * sizeof(double));
                std::memcpy(refSlideBuf_.data() + keep, hopScratchRef_.data(), hop * sizeof(double));
                std::memcpy(measSlideBuf_.data() + keep, hopScratchMeas_.data(), hop * sizeof(double));
            } else {
                std::memcpy(refSlideBuf_.data(), hopScratchRef_.data() + (hop - fftSize), fftSize * sizeof(double));
                std::memcpy(measSlideBuf_.data(), hopScratchMeas_.data() + (hop - fftSize), fftSize * sizeof(double));
            }

            auto& writeFrame = tripleBuffer_.getWriteBuffer();
            engine_.processFrame(refSlideBuf_.data(), measSlideBuf_.data(), writeFrame);
            dynamicsEngine_.processBlock(refSlideBuf_.data(), measSlideBuf_.data(), fftSize, writeFrame.dynamicsFrame);
            harmonicEngine_.processBlock(measSlideBuf_.data(), fftSize, writeFrame.harmonicFrame);
            tripleBuffer_.publish();
        }
    }
}

} // namespace plugin
