#pragma once

#include <cmath>
#include <complex>
#include <cstddef>
#include <random>
#include <vector>

namespace dsp::tests {

constexpr double kPi = 3.1415926535897932384626433832795;

struct Biquad {
    double b0 = 1.0, b1 = 0.0, b2 = 0.0;
    double a1 = 0.0, a2 = 0.0;
    double z1 = 0.0, z2 = 0.0;

    static Biquad makeButterworthLowPass(double sampleRate, double cutoffHz) {
        const double omega = 2.0 * kPi * cutoffHz / sampleRate;
        const double sinW = std::sin(omega);
        const double cosW = std::cos(omega);
        const double alpha = sinW / std::sqrt(2.0); // Q = 1/sqrt(2)
        const double a0 = 1.0 + alpha;

        Biquad bq;
        bq.b0 = ((1.0 - cosW) * 0.5) / a0;
        bq.b1 = (1.0 - cosW) / a0;
        bq.b2 = ((1.0 - cosW) * 0.5) / a0;
        bq.a1 = (-2.0 * cosW) / a0;
        bq.a2 = (1.0 - alpha) / a0;
        return bq;
    }

    static Biquad makeButterworthHighPass(double sampleRate, double cutoffHz) {
        const double omega = 2.0 * kPi * cutoffHz / sampleRate;
        const double sinW = std::sin(omega);
        const double cosW = std::cos(omega);
        const double alpha = sinW / std::sqrt(2.0);
        const double a0 = 1.0 + alpha;

        Biquad bq;
        bq.b0 = ((1.0 + cosW) * 0.5) / a0;
        bq.b1 = (-(1.0 + cosW)) / a0;
        bq.b2 = ((1.0 + cosW) * 0.5) / a0;
        bq.a1 = (-2.0 * cosW) / a0;
        bq.a2 = (1.0 - alpha) / a0;
        return bq;
    }

    double processSample(double x) noexcept {
        // Direct Form II Transposed
        const double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }

    std::complex<double> evaluateFrequencyResponse(double freqHz, double sampleRate) const noexcept {
        const double w = 2.0 * kPi * freqHz / sampleRate;
        const std::complex<double> zInv1(std::cos(-w), std::sin(-w));
        const std::complex<double> zInv2 = zInv1 * zInv1;

        const std::complex<double> num = b0 + b1 * zInv1 + b2 * zInv2;
        const std::complex<double> den = 1.0 + a1 * zInv1 + a2 * zInv2;
        return num / den;
    }
};

inline std::vector<double> generateWhiteNoise(std::size_t numSamples, std::uint32_t seed = 42, double amplitude = 0.5) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> dist(-amplitude, amplitude);
    std::vector<double> out(numSamples);
    for (auto& s : out) {
        s = dist(rng);
    }
    return out;
}

} // namespace dsp::tests
