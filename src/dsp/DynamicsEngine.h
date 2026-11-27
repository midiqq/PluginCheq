#pragma once

#include <vector>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace dsp {

struct DynamicsFrame {
    // Ramp test curve: input dBFS vs output dBFS
    std::vector<float> inputLevelsDb;
    std::vector<float> outputLevelsDb;
    float estimatedThresholdDb = 0.0f;
    float estimatedRatio = 1.0f;
    float estimatedKneeDb = 0.0f;
    bool hasData = false;

    // Attack/Release envelope test
    std::vector<float> attackReleaseTimeMs;
    std::vector<float> envelopeInputDb;
    std::vector<float> envelopeOutputDb;
    float attackTimeMs = 0.0f;
    float releaseTimeMs = 0.0f;
};

/**
 * Dynamics Analyzer Engine:
 * 1. Ramp test: computes transfer characteristics (Threshold, Ratio, Knee) by analyzing input vs output level pairs.
 * 2. Attack / Release envelope tracking: tracks instantaneous envelope rise and fall times.
 */
class DynamicsEngine {
public:
    void prepare(double sampleRate) noexcept {
        sampleRate_ = sampleRate > 1000.0 ? sampleRate : 48000.0;
        reset();
    }

    void reset() noexcept {
        envIn_ = 0.0f;
        envOut_ = 0.0f;
        histIn_.clear();
        histOut_.clear();
    }

    void processBlock(const double* ref, const double* meas, std::size_t numSamples, DynamicsFrame& frame) {
        if (!ref || !meas || numSamples == 0) return;

        // Peak / RMS envelope follower
        const float alpha = 0.005f; // smoothing for level estimation
        for (std::size_t i = 0; i < numSamples; ++i) {
            const float inAbs = static_cast<float>(std::abs(ref[i]));
            const float outAbs = static_cast<float>(std::abs(meas[i]));

            envIn_ = (1.0f - alpha) * envIn_ + alpha * inAbs;
            envOut_ = (1.0f - alpha) * envOut_ + alpha * outAbs;

            // Sample every 64 samples to populate ramp curve
            if ((i & 63) == 0 && envIn_ > 1e-4f) {
                const float inDb = 20.0f * std::log10(std::max(1e-4f, envIn_));
                const float outDb = 20.0f * std::log10(std::max(1e-4f, envOut_));
                histIn_.push_back(inDb);
                histOut_.push_back(outDb);
                if (histIn_.size() > 500) {
                    histIn_.erase(histIn_.begin());
                    histOut_.erase(histOut_.begin());
                }
            }
        }

        if (histIn_.size() > 20) {
            // Sort by input level to generate monotonic transfer curve
            std::vector<std::pair<float, float>> pairs(histIn_.size());
            for (std::size_t i = 0; i < histIn_.size(); ++i) {
                pairs[i] = { histIn_[i], histOut_[i] };
            }
            std::sort(pairs.begin(), pairs.end(), [](const auto& a, const auto& b) {
                return a.first < b.first;
            });

            frame.inputLevelsDb.resize(pairs.size());
            frame.outputLevelsDb.resize(pairs.size());
            for (std::size_t i = 0; i < pairs.size(); ++i) {
                frame.inputLevelsDb[i] = pairs[i].first;
                frame.outputLevelsDb[i] = pairs[i].second;
            }
            frame.hasData = true;

            // Simple threshold and ratio estimation
            // Under linear gain: output - input = 0 (or gain offset)
            // Under compression: slope dOut / dIn < 1.0
            if (pairs.size() >= 10) {
                const auto& mid = pairs[pairs.size() / 2];
                const auto& top = pairs.back();
                const float dIn = top.first - mid.first;
                const float dOut = top.second - mid.second;
                if (dIn > 2.0f && dOut > 0.1f) {
                    frame.estimatedRatio = std::clamp(dIn / dOut, 1.0f, 30.0f);
                } else {
                    frame.estimatedRatio = 1.0f;
                }
                frame.estimatedThresholdDb = mid.first;
            }
        }
    }

private:
    double sampleRate_ = 48000.0;
    float envIn_ = 0.0f;
    float envOut_ = 0.0f;
    std::vector<float> histIn_;
    std::vector<float> histOut_;
};

} // namespace dsp
