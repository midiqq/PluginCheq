#include "DelayFinder.h"

#include <algorithm>
#include <cmath>

namespace dsp {

namespace {
constexpr double kEpsilon = 1e-18;
} // namespace

void DelayFinder::prepare(std::size_t fftSize) {
    fftSize_ = fftSize;
    fft_.prepare(fftSize_);
    const std::size_t numBins = fftSize_ / 2 + 1;
    phatSpec_.assign(numBins, {0.0, 0.0});
    corrPhat_.assign(fftSize_, 0.0);
    corrLinear_.assign(fftSize_, 0.0);
}

DelayEstimate DelayFinder::estimate(const std::vector<std::complex<double>>& gxy,
                                    double sampleRate) const {
    DelayEstimate result;
    if (fftSize_ == 0 || gxy.size() != (fftSize_ / 2 + 1) || sampleRate <= 0.0) {
        return result;
    }

    // 1. Compute GCC-PHAT spectrum (whitened phase for sharp peak)
    double totalMag = 0.0;
    for (std::size_t k = 0; k < gxy.size(); ++k) {
        const double mag = std::abs(gxy[k]);
        totalMag += mag;
        if (mag > kEpsilon) {
            phatSpec_[k] = gxy[k] / mag;
        } else {
            phatSpec_[k] = {0.0, 0.0};
        }
    }

    if (totalMag <= kEpsilon) {
        return result;
    }

    // 2. Inverse FFT of both GCC-PHAT (peak localization) and linear Gxy (polarity sign)
    fft_.inverseRealFromHalfSpectrum(phatSpec_.data(), corrPhat_.data());
    fft_.inverseRealFromHalfSpectrum(gxy.data(), corrLinear_.data());

    // 3. Find maximum absolute peak in GCC-PHAT correlation
    std::size_t peakIdx = 0;
    double peakAbs = -1.0;
    double sumSq = 0.0;

    for (std::size_t i = 0; i < fftSize_; ++i) {
        const double v = corrPhat_[i];
        sumSq += v * v;
        const double a = std::abs(v);
        if (a > peakAbs) {
            peakAbs = a;
            peakIdx = i;
        }
    }

    const double rms = std::sqrt(sumSq / static_cast<double>(fftSize_));
    if (rms <= kEpsilon) {
        return result;
    }

    // 4. Sub-sample peak refinement via windowed sinc interpolation + golden-section search
    auto evalInterpAbs = [&](double delta) {
        constexpr int halfTap = 8;
        constexpr double kPi = 3.1415926535897932384626433832795;
        double sum = 0.0;
        for (int m = -halfTap; m <= halfTap; ++m) {
            const std::size_t idx = (peakIdx + fftSize_ + static_cast<std::size_t>(m)) % fftSize_;
            const double x = delta - static_cast<double>(m);
            const double sinc = (std::abs(x) < 1e-9) ? 1.0 : std::sin(kPi * x) / (kPi * x);
            const double win = 0.5 * (1.0 + std::cos(kPi * x / static_cast<double>(halfTap + 1)));
            sum += corrPhat_[idx] * sinc * win;
        }
        return std::abs(sum);
    };

    double lo = -0.5;
    double hi = 0.5;
    constexpr double kInvPhi = 0.6180339887498949; // (sqrt(5) - 1) / 2
    double c = hi - kInvPhi * (hi - lo);
    double d = lo + kInvPhi * (hi - lo);
    double fc = evalInterpAbs(c);
    double fd = evalInterpAbs(d);

    for (int iter = 0; iter < 18; ++iter) {
        if (fc < fd) {
            lo = c;
            c = d;
            fc = fd;
            d = lo + kInvPhi * (hi - lo);
            fd = evalInterpAbs(d);
        } else {
            hi = d;
            d = c;
            fd = fc;
            c = hi - kInvPhi * (hi - lo);
            fc = evalInterpAbs(c);
        }
    }
    const double subSampleOffset = 0.5 * (lo + hi);

    // Map circular index [0..N-1] to signed lag [-N/2 .. N/2 - 1]
    double signedLag = static_cast<double>(peakIdx);
    if (peakIdx >= fftSize_ / 2) {
        signedLag -= static_cast<double>(fftSize_);
    }
    signedLag += subSampleOffset;

    // 5. Polarity detection from unweighted cross-correlation around peakIdx
    const double linPeak = corrLinear_[peakIdx];
    result.polarityInverted = (linPeak < 0.0);
    result.delaySamples = signedLag;
    result.delayMs = (signedLag * 1000.0) / sampleRate;
    result.confidence = peakAbs / rms;
    result.valid = (result.confidence > 2.5);

    return result;
}

} // namespace dsp
