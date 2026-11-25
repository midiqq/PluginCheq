#pragma once

#include "AnalysisTypes.h"
#include "DelayFinder.h"
#include "FftEngine.h"
#include "SpectrumSmoothing.h"

#include <complex>
#include <cstddef>
#include <vector>

namespace dsp {

/**
 * Computes two-channel Transfer Function (H1, H2), Phase (wrapped & unwrapped),
 * and Ordinary Squared Coherence from Reference (X) and Measurement (Y) frames.
 */
class TransferFunctionEngine {
public:
    TransferFunctionEngine() = default;

    void configure(const TransferFunctionConfig& config);
    void resetAveraging();

    [[nodiscard]] const TransferFunctionConfig& config() const noexcept { return config_; }
    [[nodiscard]] std::size_t hopSize() const noexcept { return hopSize_; }

    /**
     * Processes one pair of time-domain frames of length `config().fftSize`.
     * Writes the updated averaged transfer function into `outFrame`.
     */
    void processFrame(const double* referenceFrame,
                      const double* measurementFrame,
                      TransferFunctionFrame& outFrame);

    /**
     * Unwraps a wrapped phase vector (in degrees) into an unwrapped phase vector (in degrees).
     */
    static void unwrapPhaseDegrees(const std::vector<double>& wrappedDeg,
                                   std::vector<double>& outUnwrappedDeg);

private:
    TransferFunctionConfig config_{};
    std::size_t hopSize_ = 1024;
    std::uint64_t frameCount_ = 0;

    FftEngine fft_;
    DelayFinder delayFinder_;
    std::vector<double> window_;
    std::vector<double> smoothScratch_;
    std::vector<double> windowedRef_;
    std::vector<double> windowedMeas_;
    std::vector<std::complex<double>> specRef_;
    std::vector<std::complex<double>> specMeas_;

    // Accumulated/averaged spectra
    std::vector<double> avgGxx_;
    std::vector<double> avgGyy_;
    std::vector<std::complex<double>> avgGxy_;

    // Circular history for linear averaging
    std::vector<std::vector<double>> histGxx_;
    std::vector<std::vector<double>> histGyy_;
    std::vector<std::vector<std::complex<double>>> histGxy_;
    std::size_t histWriteIdx_ = 0;
    std::size_t histFilled_ = 0;
};

} // namespace dsp
