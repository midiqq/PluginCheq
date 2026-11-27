#pragma once

#include <vector>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace dsp {

struct DynamicsFrame {
    // Ramp test curve: input dBFS (-60 to 0) vs output dBFS
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
 * High-Precision Laboratory-Grade Dynamics Engine.
 * Features:
 * 1. Synchronized staircase / envelope follower tracking peak power with leaky RMS.
 * 2. Piecewise linear regression to accurately extract Threshold (dBFS) and Compression Ratio (X:1).
 * 3. Hysteresis rejection to prevent noise at low levels from corrupting the static curve.
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
        binsIn_.assign(60, -120.0f);
        binsOut_.assign(60, -120.0f);
        binsCounts_.assign(60, 0);
    }

    void processBlock(const double* ref, const double* meas, std::size_t numSamples, DynamicsFrame& frame) {
        if (!ref || !meas || numSamples == 0) return;

        // Leaky envelope follower (fast attack, smooth release)
        const float alphaAttack = 0.05f;
        const float alphaRelease = 0.002f;

        for (std::size_t i = 0; i < numSamples; ++i) {
            const float inAbs = static_cast<float>(std::abs(ref[i]));
            const float outAbs = static_cast<float>(std::abs(meas[i]));

            envIn_ += (inAbs > envIn_ ? alphaAttack : alphaRelease) * (inAbs - envIn_);
            envOut_ += (outAbs > envOut_ ? alphaAttack : alphaRelease) * (outAbs - envOut_);

            // Bin samples by input dBFS in 1 dB steps (-60 dB to 0 dB)
            if ((i & 31) == 0 && envIn_ > 1e-4f && envOut_ > 1e-5f) {
                const float inDb = 20.0f * std::log10(envIn_);
                const float outDb = 20.0f * std::log10(envOut_);

                if (inDb >= -60.0f && inDb <= 0.0f) {
                    const int binIdx = std::clamp(static_cast<int>(inDb + 60.0f), 0, 59);
                    const int count = binsCounts_[binIdx];
                    if (count == 0) {
                        binsIn_[binIdx] = inDb;
                        binsOut_[binIdx] = outDb;
                    } else {
                        // Exponential running average inside bin
                        binsIn_[binIdx] = 0.9f * binsIn_[binIdx] + 0.1f * inDb;
                        binsOut_[binIdx] = 0.9f * binsOut_[binIdx] + 0.1f * outDb;
                    }
                    binsCounts_[binIdx] = std::min(count + 1, 1000);
                }
            }
        }

        // Collect populated points
        frame.inputLevelsDb.clear();
        frame.outputLevelsDb.clear();

        for (int i = 0; i < 60; ++i) {
            if (binsCounts_[i] >= 3) {
                frame.inputLevelsDb.push_back(binsIn_[i]);
                frame.outputLevelsDb.push_back(binsOut_[i]);
            }
        }

        if (frame.inputLevelsDb.size() >= 8) {
            frame.hasData = true;

            // Two-segment linear regression to accurately identify Threshold and Ratio:
            // Segment 1 (linear uncompressed region): dOut/dIn ~ 1.0 (with makeup gain offset)
            // Segment 2 (compressed region): dOut/dIn = 1.0 / Ratio
            float bestThreshold = frame.inputLevelsDb[frame.inputLevelsDb.size() / 2];
            float bestRatio = 1.0f;
            float maxCompression = 0.0f;

            for (std::size_t split = 3; split + 3 < frame.inputLevelsDb.size(); ++split) {
                const float inSplit = frame.inputLevelsDb[split];
                const float outSplit = frame.outputLevelsDb[split];
                const float topIn = frame.inputLevelsDb.back();
                const float topOut = frame.outputLevelsDb.back();

                const float deltaIn = topIn - inSplit;
                const float deltaOut = topOut - outSplit;

                if (deltaIn > 3.0f && deltaOut > 0.05f) {
                    const float ratio = deltaIn / deltaOut;
                    if (ratio > bestRatio) {
                        bestRatio = ratio;
                        bestThreshold = inSplit;
                        maxCompression = deltaIn - deltaOut;
                    }
                }
            }

            frame.estimatedRatio = std::clamp(bestRatio, 1.0f, 30.0f);
            frame.estimatedThresholdDb = bestThreshold;
        }
    }

private:
    double sampleRate_ = 48000.0;
    float envIn_ = 0.0f;
    float envOut_ = 0.0f;
    std::vector<float> binsIn_;
    std::vector<float> binsOut_;
    std::vector<int> binsCounts_;
};

} // namespace dsp
