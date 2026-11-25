#include "WindowFunctions.h"

#include <cmath>
#include <numeric>

namespace dsp {

namespace {
constexpr double kPi = 3.1415926535897932384626433832795;
} // namespace

void WindowFunctions::generate(WindowType type, std::size_t size, std::vector<double>& outWindow) {
    outWindow.resize(size);
    if (size == 0) {
        return;
    }
    if (size == 1) {
        outWindow[0] = 1.0;
        return;
    }

    const double denom = static_cast<double>(size - 1);

    switch (type) {
        case WindowType::Rectangular: {
            std::fill(outWindow.begin(), outWindow.end(), 1.0);
            break;
        }
        case WindowType::Hann: {
            for (std::size_t n = 0; n < size; ++n) {
                const double phase = (2.0 * kPi * static_cast<double>(n)) / denom;
                outWindow[n] = 0.5 * (1.0 - std::cos(phase));
            }
            break;
        }
        case WindowType::BlackmanHarris: {
            // 4-term Blackman-Harris (-92 dB sidelobes)
            constexpr double a0 = 0.35875;
            constexpr double a1 = 0.48829;
            constexpr double a2 = 0.14128;
            constexpr double a3 = 0.01168;
            for (std::size_t n = 0; n < size; ++n) {
                const double phase = (2.0 * kPi * static_cast<double>(n)) / denom;
                outWindow[n] = a0
                             - a1 * std::cos(phase)
                             + a2 * std::cos(2.0 * phase)
                             - a3 * std::cos(3.0 * phase);
            }
            break;
        }
    }
}

double WindowFunctions::coherentGain(const std::vector<double>& window) noexcept {
    if (window.empty()) {
        return 1.0;
    }
    const double sum = std::accumulate(window.begin(), window.end(), 0.0);
    return sum / static_cast<double>(window.size());
}

double WindowFunctions::energyGain(const std::vector<double>& window) noexcept {
    if (window.empty()) {
        return 1.0;
    }
    double sumSq = 0.0;
    for (double v : window) {
        sumSq += v * v;
    }
    return sumSq / static_cast<double>(window.size());
}

} // namespace dsp
