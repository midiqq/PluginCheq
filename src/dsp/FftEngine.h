#pragma once

#include <complex>
#include <cstddef>
#include <vector>

namespace dsp {

/**
 * Iterative Radix-2 Cooley-Tukey FFT engine in double precision.
 * Precomputes twiddle factors and bit-reversal indices on prepare()
 * so forward() performs zero heap allocations.
 */
class FftEngine {
public:
    FftEngine() = default;

    void prepare(std::size_t fftSize);

    [[nodiscard]] std::size_t size() const noexcept { return size_; }

    /**
     * Computes forward DFT of `realInput` (length == size()) and writes
     * the non-redundant half-spectrum [0 .. size()/2] to `outSpectrum`
     * (length == size()/2 + 1).
     */
    void forwardReal(const double* realInput, std::complex<double>* outSpectrum);

    /**
     * Computes inverse DFT from a conjugate-symmetric half-spectrum [0 .. size()/2]
     * and writes the real time-domain result (normalized by 1/size()) to `outReal`
     * (length == size()).
     */
    void inverseRealFromHalfSpectrum(const std::complex<double>* halfSpectrum, double* outReal);

private:
    std::size_t size_ = 0;
    std::size_t log2Size_ = 0;
    std::vector<std::size_t> bitReversed_;
    std::vector<std::complex<double>> twiddles_;
    std::vector<std::complex<double>> scratch_;
};

} // namespace dsp
