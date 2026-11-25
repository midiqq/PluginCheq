#include "SpectrumSmoothing.h"

#include <algorithm>
#include <cmath>

namespace dsp {

double SpectrumSmoothing::fractionDenominator(SmoothingMode mode) noexcept {
    switch (mode) {
        case SmoothingMode::Octave24: return 24.0;
        case SmoothingMode::Octave12: return 12.0;
        case SmoothingMode::Octave6:  return 6.0;
        case SmoothingMode::Octave3:  return 3.0;
        case SmoothingMode::Octave1:  return 1.0;
        case SmoothingMode::Off:
        default:                      return 0.0;
    }
}

void SpectrumSmoothing::apply(SmoothingMode mode,
                              const std::vector<double>& frequenciesHz,
                              const std::vector<double>& inputValues,
                              std::vector<double>& outSmoothed) {
    const std::size_t n = inputValues.size();
    if (mode == SmoothingMode::Off || n < 3 || frequenciesHz.size() != n) {
        outSmoothed = inputValues;
        return;
    }

    const double denom = fractionDenominator(mode);
    const double halfFactor = std::pow(2.0, 1.0 / (2.0 * denom));
    const double invHalfFactor = 1.0 / halfFactor;

    // Build prefix sum array (index 0 = 0.0, prefix[i+1] = sum of inputValues[0..i])
    std::vector<double> prefix(n + 1, 0.0);
    for (std::size_t i = 0; i < n; ++i) {
        prefix[i + 1] = prefix[i] + inputValues[i];
    }

    outSmoothed.resize(n);
    outSmoothed[0] = inputValues[0]; // DC bin unchanged

    std::size_t lo = 1;
    std::size_t hi = 1;

    for (std::size_t k = 1; k < n; ++k) {
        const double f = frequenciesHz[k];
        const double fLow = f * invHalfFactor;
        const double fHigh = f * halfFactor;

        while (lo < k && frequenciesHz[lo] < fLow) {
            ++lo;
        }
        while (hi + 1 < n && frequenciesHz[hi + 1] <= fHigh) {
            ++hi;
        }
        if (hi < k) {
            hi = k;
        }

        const std::size_t count = (hi >= lo) ? (hi - lo + 1) : 1;
        outSmoothed[k] = (prefix[hi + 1] - prefix[lo]) / static_cast<double>(count);
    }
}

} // namespace dsp
