#pragma once

#include <cstddef>
#include <vector>

namespace dsp {

enum class SmoothingMode : int {
    Off = 0,
    Octave24 = 1,
    Octave12 = 2,
    Octave6 = 3,
    Octave3 = 4,
    Octave1 = 5
};

/**
 * O(N) fractional-octave smoothing using prefix sums.
 * Smooths magnitude (dB), phase (degrees), and coherence [0, 1].
 */
class SpectrumSmoothing {
public:
    static double fractionDenominator(SmoothingMode mode) noexcept;

    static void apply(SmoothingMode mode,
                      const std::vector<double>& frequenciesHz,
                      const std::vector<double>& inputValues,
                      std::vector<double>& outSmoothed);
};

} // namespace dsp
