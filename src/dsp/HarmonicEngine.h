#pragma once

#include "FftEngine.h"
#include <vector>
#include <cmath>
#include <complex>
#include <cstddef>
#include <algorithm>

namespace dsp {

struct HarmonicFrame {
    float fundamentalFreqHz = 1000.0f;
    float fundamentalLevelDb = 0.0f;
    float thdPercent = 0.0f;
    float thdDb = -120.0f;
    float thdPlusNoisePercent = 0.0f;
    float thdPlusNoiseDb = -120.0f;

    // Harmonic levels H2..H8 in dBFS or dBc (dB relative to carrier)
    std::vector<float> harmonicFreqsHz;
    std::vector<float> harmonicLevelsDb;
    std::vector<float> harmonicRelativeDb; // dBc

    // Spectrum magnitude bins for graphical display
    std::vector<float> spectrumFreqsHz;
    std::vector<float> spectrumDb;
    bool hasData = false;
};

/**
 * Harmonic Distortion & THD/THD+N Engine.
 * Identifies fundamental peak, measures harmonics H2..H8, computes THD and THD+N.
 */
class HarmonicEngine {
public:
    void prepare(std::size_t fftSize, double sampleRate) {
        fftSize_ = fftSize > 0 ? fftSize : 4096;
        sampleRate_ = sampleRate > 1000.0 ? sampleRate : 48000.0;
        fft_.prepare(fftSize_);
        windowed_.assign(fftSize_, 0.0);
        spec_.assign(fftSize_ / 2 + 1, {0.0, 0.0});
    }

    void processBlock(const double* meas, std::size_t numSamples, HarmonicFrame& frame) {
        if (!meas || numSamples < fftSize_) return;

        // Apply Hann window
        const double twoPi = 6.28318530717958647692;
        const double denom = static_cast<double>(fftSize_ - 1);
        for (std::size_t i = 0; i < fftSize_; ++i) {
            const double w = 0.5 * (1.0 - std::cos((twoPi * i) / denom));
            windowed_[i] = meas[i] * w;
        }

        fft_.forwardReal(windowed_.data(), spec_.data());

        const std::size_t numBins = fftSize_ / 2 + 1;
        const double binHz = sampleRate_ / static_cast<double>(fftSize_);

        frame.spectrumFreqsHz.resize(numBins);
        frame.spectrumDb.resize(numBins);

        std::size_t peakBin = 1;
        double peakPower = 0.0;
        double totalPower = 0.0;

        for (std::size_t k = 1; k < numBins; ++k) {
            const double mag = std::abs(spec_[k]);
            const double power = mag * mag;
            totalPower += power;

            frame.spectrumFreqsHz[k] = static_cast<float>(k * binHz);
            frame.spectrumDb[k] = static_cast<float>(20.0 * std::log10(std::max(1e-6, mag / (fftSize_ * 0.25))));

            if (power > peakPower) {
                peakPower = power;
                peakBin = k;
            }
        }

        frame.fundamentalFreqHz = static_cast<float>(peakBin * binHz);
        frame.fundamentalLevelDb = frame.spectrumDb[peakBin];

        // Measure harmonics H2 through H8
        frame.harmonicFreqsHz.clear();
        frame.harmonicLevelsDb.clear();
        frame.harmonicRelativeDb.clear();

        double harmonicPowerSum = 0.0;
        for (int h = 2; h <= 8; ++h) {
            const std::size_t hBin = peakBin * h;
            if (hBin >= numBins) break;

            // Search local peak in +/- 2 bins around expected harmonic
            std::size_t bestBin = hBin;
            double bestMag = 0.0;
            for (int offset = -2; offset <= 2; ++offset) {
                const int testBin = static_cast<int>(hBin) + offset;
                if (testBin >= 1 && testBin < static_cast<int>(numBins)) {
                    const double m = std::abs(spec_[testBin]);
                    if (m > bestMag) {
                        bestMag = m;
                        bestBin = static_cast<std::size_t>(testBin);
                    }
                }
            }

            const double hPower = bestMag * bestMag;
            harmonicPowerSum += hPower;

            const float hLevel = frame.spectrumDb[bestBin];
            frame.harmonicFreqsHz.push_back(static_cast<float>(bestBin * binHz));
            frame.harmonicLevelsDb.push_back(hLevel);
            frame.harmonicRelativeDb.push_back(hLevel - frame.fundamentalLevelDb);
        }

        // THD % = sqrt(harmonicPowerSum / fundamentalPower) * 100
        if (peakPower > 1e-12) {
            const double thdRatio = std::sqrt(harmonicPowerSum / peakPower);
            frame.thdPercent = static_cast<float>(thdRatio * 100.0);
            frame.thdDb = static_cast<float>(20.0 * std::log10(std::max(1e-6, thdRatio)));

            // THD+N
            const double noiseAndHarmonics = std::max(0.0, totalPower - peakPower);
            const double thdnRatio = std::sqrt(noiseAndHarmonics / peakPower);
            frame.thdPlusNoisePercent = static_cast<float>(thdnRatio * 100.0);
            frame.thdPlusNoiseDb = static_cast<float>(20.0 * std::log10(std::max(1e-6, thdnRatio)));
        } else {
            frame.thdPercent = 0.0f;
            frame.thdDb = -120.0f;
            frame.thdPlusNoisePercent = 0.0f;
            frame.thdPlusNoiseDb = -120.0f;
        }

        frame.hasData = true;
    }

private:
    std::size_t fftSize_ = 4096;
    double sampleRate_ = 48000.0;
    FftEngine fft_;
    std::vector<double> windowed_;
    std::vector<std::complex<double>> spec_;
};

} // namespace dsp
