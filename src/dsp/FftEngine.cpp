#include "FftEngine.h"

#include <cmath>
#include <stdexcept>

namespace dsp {

namespace {
constexpr double kPi = 3.1415926535897932384626433832795;

bool isPowerOfTwo(std::size_t n) noexcept {
    return n >= 2 && (n & (n - 1)) == 0;
}
} // namespace

void FftEngine::prepare(std::size_t fftSize) {
    if (!isPowerOfTwo(fftSize)) {
        throw std::invalid_argument("FFT size must be a power of two >= 2");
    }

    size_ = fftSize;
    log2Size_ = 0;
    for (std::size_t temp = size_; temp > 1; temp >>= 1) {
        ++log2Size_;
    }

    bitReversed_.resize(size_);
    for (std::size_t i = 0; i < size_; ++i) {
        std::size_t rev = 0;
        std::size_t val = i;
        for (std::size_t b = 0; b < log2Size_; ++b) {
            rev = (rev << 1) | (val & 1u);
            val >>= 1;
        }
        bitReversed_[i] = rev;
    }

    // Precompute twiddles for all stages: size/2 total unique twiddles
    twiddles_.resize(size_ / 2);
    for (std::size_t k = 0; k < size_ / 2; ++k) {
        const double angle = -2.0 * kPi * static_cast<double>(k) / static_cast<double>(size_);
        twiddles_[k] = std::complex<double>(std::cos(angle), std::sin(angle));
    }

    scratch_.resize(size_);
}

void FftEngine::forwardReal(const double* realInput, std::complex<double>* outSpectrum) {
    if (size_ == 0 || realInput == nullptr || outSpectrum == nullptr) {
        return;
    }

    // Load bit-reversed real input into complex scratch buffer
    for (std::size_t i = 0; i < size_; ++i) {
        scratch_[i] = std::complex<double>(realInput[bitReversed_[i]], 0.0);
    }

    // Iterative Cooley-Tukey butterflies
    for (std::size_t stage = 1; stage <= log2Size_; ++stage) {
        const std::size_t m = std::size_t{1} << stage;
        const std::size_t halfM = m >> 1;
        const std::size_t twiddleStep = size_ / m;

        for (std::size_t groupStart = 0; groupStart < size_; groupStart += m) {
            for (std::size_t j = 0; j < halfM; ++j) {
                const auto w = twiddles_[j * twiddleStep];
                const auto u = scratch_[groupStart + j];
                const auto t = w * scratch_[groupStart + j + halfM];
                scratch_[groupStart + j] = u + t;
                scratch_[groupStart + j + halfM] = u - t;
            }
        }
    }

    const std::size_t numBins = size_ / 2 + 1;
    for (std::size_t k = 0; k < numBins; ++k) {
        outSpectrum[k] = scratch_[k];
    }
}

void FftEngine::inverseRealFromHalfSpectrum(const std::complex<double>* halfSpectrum, double* outReal) {
    if (size_ == 0 || halfSpectrum == nullptr || outReal == nullptr) {
        return;
    }

    const std::size_t half = size_ / 2;

    // Reconstruct full conjugate-symmetric spectrum and take conjugate for IFFT via forward butterflies
    // Using: IFFT(X) = (1/N) * conj(FFT(conj(X)))
    for (std::size_t i = 0; i < size_; ++i) {
        const std::size_t k = bitReversed_[i];
        std::complex<double> val;
        if (k <= half) {
            val = std::conj(halfSpectrum[k]);
        } else {
            // Mirror from [1 .. half-1], un-conjugated because conj(conj(X[N-k])) = X[N-k]
            val = halfSpectrum[size_ - k];
        }
        scratch_[i] = val;
    }

    for (std::size_t stage = 1; stage <= log2Size_; ++stage) {
        const std::size_t m = std::size_t{1} << stage;
        const std::size_t halfM = m >> 1;
        const std::size_t twiddleStep = size_ / m;

        for (std::size_t groupStart = 0; groupStart < size_; groupStart += m) {
            for (std::size_t j = 0; j < halfM; ++j) {
                const auto w = twiddles_[j * twiddleStep];
                const auto u = scratch_[groupStart + j];
                const auto t = w * scratch_[groupStart + j + halfM];
                scratch_[groupStart + j] = u + t;
                scratch_[groupStart + j + halfM] = u - t;
            }
        }
    }

    const double invN = 1.0 / static_cast<double>(size_);
    for (std::size_t n = 0; n < size_; ++n) {
        outReal[n] = scratch_[n].real() * invN;
    }
}

} // namespace dsp
