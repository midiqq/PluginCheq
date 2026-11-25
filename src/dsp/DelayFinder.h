#pragma once

#include "FftEngine.h"

#include <complex>
#include <cstddef>
#include <vector>

namespace dsp {

struct DelayEstimate {
    double delaySamples = 0.0;     // Positive => Measurement lags Reference
    double delayMs = 0.0;
    bool polarityInverted = false;
    double confidence = 0.0;       // Peak-to-RMS ratio or normalized correlation
    bool valid = false;
};

/**
 * Estimates time delay (with sub-sample parabolic interpolation) and polarity
 * inversion between Reference and Measurement using cross-correlation (GCC-PHAT
 * for sharp delay localization + unweighted cross-correlation for polarity).
 */
class DelayFinder {
public:
    void prepare(std::size_t fftSize);

    DelayEstimate estimate(const std::vector<std::complex<double>>& gxy,
                           double sampleRate) const;

private:
    std::size_t fftSize_ = 0;
    mutable FftEngine fft_;
    mutable std::vector<std::complex<double>> phatSpec_;
    mutable std::vector<double> corrPhat_;
    mutable std::vector<double> corrLinear_;
};

} // namespace dsp
