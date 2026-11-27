#pragma once

#include "DelayFinder.h"
#include "SpectrumSmoothing.h"
#include "DynamicsEngine.h"
#include "HarmonicEngine.h"

#include <complex>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace dsp {

enum class WindowType : int {
    Hann = 0,
    BlackmanHarris = 1,
    Rectangular = 2
};

enum class AveragingMode : int {
    Exponential = 0,
    Linear = 1
};

enum class ChannelMode : int {
    Left = 0,
    Right = 1,
    Mid = 2
};

enum class RoutingMode : int {
    SidechainInput = 0,   // Standard DAW sidechain bus (Reaper, Cubase, Studio One)
    PluginTapBus = 1,     // Direct inter-plugin tap (Receiver mode for Ableton Live, FL Studio)
    TapSender = 2         // Instance operates as sender tap before FX chain
};

struct TransferFunctionConfig {
    std::size_t fftSize = 4096;
    double sampleRate = 48000.0;
    WindowType windowType = WindowType::BlackmanHarris;
    AveragingMode averagingMode = AveragingMode::Exponential;
    double exponentialAlpha = 0.15;   // (0, 1] — smaller = smoother/slower
    std::size_t linearAverageCount = 16;
    double hopFraction = 0.25;        // 75% overlap by default
    double delayCompensationSamples = 0.0; // Analysis-only delay compensation
    SmoothingMode smoothingMode = SmoothingMode::Off;
};

struct TransferFunctionFrame {
    double sampleRate = 48000.0;
    std::size_t fftSize = 0;
    std::size_t numBins = 0;       // fftSize / 2 + 1
    std::uint64_t frameCounter = 0;

    DelayEstimate delayEstimate{};
    DynamicsFrame dynamicsFrame{};
    HarmonicFrame harmonicFrame{};

    // Frequency vector [0 .. fs/2]
    std::vector<double> frequenciesHz;

    // H1 = Gxy / Gxx (primary estimator, unbiased by output noise)
    std::vector<double> magnitudeDb;
    std::vector<double> phaseWrappedDeg;
    std::vector<double> phaseUnwrappedDeg;

    // H2 = Gyy / Gyx (secondary estimator, unbiased by input noise)
    std::vector<double> h2MagnitudeDb;

    // Ordinary squared coherence gamma^2 in [0, 1]
    std::vector<double> coherence;

    // Auto-spectra & cross-spectrum (power-normalized)
    std::vector<double> gxx;
    std::vector<double> gyy;
    std::vector<std::complex<double>> gxy;

    void resize(std::size_t newFftSize) {
        fftSize = newFftSize;
        numBins = newFftSize / 2 + 1;
        frequenciesHz.assign(numBins, 0.0);
        magnitudeDb.assign(numBins, 0.0);
        phaseWrappedDeg.assign(numBins, 0.0);
        phaseUnwrappedDeg.assign(numBins, 0.0);
        h2MagnitudeDb.assign(numBins, 0.0);
        coherence.assign(numBins, 0.0);
        gxx.assign(numBins, 0.0);
        gyy.assign(numBins, 0.0);
        gxy.assign(numBins, std::complex<double>{0.0, 0.0});
    }
};

} // namespace dsp
