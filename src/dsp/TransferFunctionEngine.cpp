#include "TransferFunctionEngine.h"
#include "WindowFunctions.h"

#include <algorithm>
#include <cmath>

namespace dsp {

namespace {
constexpr double kEpsilon = 1e-24;
constexpr double kMinMagnitude = 1e-12;
constexpr double kPi = 3.1415926535897932384626433832795;
constexpr double kRadToDeg = 180.0 / kPi;
} // namespace

void TransferFunctionEngine::configure(const TransferFunctionConfig& config) {
    config_ = config;
    if (config_.fftSize < 64) {
        config_.fftSize = 64;
    }
    config_.exponentialAlpha = std::clamp(config_.exponentialAlpha, 0.005, 1.0);
    config_.linearAverageCount = std::max<std::size_t>(1, config_.linearAverageCount);
    config_.hopFraction = std::clamp(config_.hopFraction, 0.0625, 1.0);

    hopSize_ = std::max<std::size_t>(
        1, static_cast<std::size_t>(static_cast<double>(config_.fftSize) * config_.hopFraction));

    fft_.prepare(config_.fftSize);
    delayFinder_.prepare(config_.fftSize);
    WindowFunctions::generate(config_.windowType, config_.fftSize, window_);

    const std::size_t numBins = config_.fftSize / 2 + 1;
    windowedRef_.assign(config_.fftSize, 0.0);
    windowedMeas_.assign(config_.fftSize, 0.0);
    specRef_.assign(numBins, {0.0, 0.0});
    specMeas_.assign(numBins, {0.0, 0.0});

    avgGxx_.assign(numBins, 0.0);
    avgGyy_.assign(numBins, 0.0);
    avgGxy_.assign(numBins, {0.0, 0.0});

    histGxx_.assign(config_.linearAverageCount, std::vector<double>(numBins, 0.0));
    histGyy_.assign(config_.linearAverageCount, std::vector<double>(numBins, 0.0));
    histGxy_.assign(config_.linearAverageCount, std::vector<std::complex<double>>(numBins, {0.0, 0.0}));

    resetAveraging();
}

void TransferFunctionEngine::resetAveraging() {
    frameCount_ = 0;
    histWriteIdx_ = 0;
    histFilled_ = 0;
    std::fill(avgGxx_.begin(), avgGxx_.end(), 0.0);
    std::fill(avgGyy_.begin(), avgGyy_.end(), 0.0);
    std::fill(avgGxy_.begin(), avgGxy_.end(), std::complex<double>{0.0, 0.0});

    for (auto& v : histGxx_) std::fill(v.begin(), v.end(), 0.0);
    for (auto& v : histGyy_) std::fill(v.begin(), v.end(), 0.0);
    for (auto& v : histGxy_) std::fill(v.begin(), v.end(), std::complex<double>{0.0, 0.0});
}

void TransferFunctionEngine::processFrame(const double* referenceFrame,
                                          const double* measurementFrame,
                                          TransferFunctionFrame& outFrame) {
    const std::size_t N = config_.fftSize;
    const std::size_t numBins = N / 2 + 1;

    if (outFrame.numBins != numBins || outFrame.fftSize != N || outFrame.frequenciesHz.size() != numBins) {
        outFrame.resize(N);
    }
    outFrame.sampleRate = config_.sampleRate;

    // 1. Apply analysis window
    for (std::size_t n = 0; n < N; ++n) {
        const double w = window_[n];
        windowedRef_[n] = referenceFrame[n] * w;
        windowedMeas_[n] = measurementFrame[n] * w;
    }

    // 2. Compute forward FFTs
    fft_.forwardReal(windowedRef_.data(), specRef_.data());
    fft_.forwardReal(windowedMeas_.data(), specMeas_.data());

    // 3. Accumulate auto-spectra and cross-spectrum
    const double binHz = config_.sampleRate / static_cast<double>(N);

    if (config_.averagingMode == AveragingMode::Exponential) {
        const double alpha = (frameCount_ == 0) ? 1.0 : config_.exponentialAlpha;
        const double oneMinusAlpha = 1.0 - alpha;

        for (std::size_t k = 0; k < numBins; ++k) {
            const auto& X = specRef_[k];
            const auto& Y = specMeas_[k];

            const double sxx = std::norm(X);
            const double syy = std::norm(Y);
            const std::complex<double> sxy = Y * std::conj(X);

            avgGxx_[k] = alpha * sxx + oneMinusAlpha * avgGxx_[k];
            avgGyy_[k] = alpha * syy + oneMinusAlpha * avgGyy_[k];
            avgGxy_[k] = alpha * sxy + oneMinusAlpha * avgGxy_[k];
        }
    } else {
        // Linear sliding block average over up to linearAverageCount frames
        const std::size_t M = config_.linearAverageCount;
        const bool bufferFull = (histFilled_ == M);
        const std::size_t newCount = bufferFull ? M : (histFilled_ + 1);

        auto& slotGxx = histGxx_[histWriteIdx_];
        auto& slotGyy = histGyy_[histWriteIdx_];
        auto& slotGxy = histGxy_[histWriteIdx_];

        for (std::size_t k = 0; k < numBins; ++k) {
            const auto& X = specRef_[k];
            const auto& Y = specMeas_[k];

            const double sxx = std::norm(X);
            const double syy = std::norm(Y);
            const std::complex<double> sxy = Y * std::conj(X);

            if (bufferFull) {
                const double sumGxx = avgGxx_[k] * static_cast<double>(M) - slotGxx[k] + sxx;
                const double sumGyy = avgGyy_[k] * static_cast<double>(M) - slotGyy[k] + syy;
                const auto sumGxy = avgGxy_[k] * static_cast<double>(M) - slotGxy[k] + sxy;

                avgGxx_[k] = std::max(0.0, sumGxx / static_cast<double>(M));
                avgGyy_[k] = std::max(0.0, sumGyy / static_cast<double>(M));
                avgGxy_[k] = sumGxy / static_cast<double>(M);
            } else {
                const double prevN = static_cast<double>(histFilled_);
                const double currN = static_cast<double>(newCount);
                avgGxx_[k] = (avgGxx_[k] * prevN + sxx) / currN;
                avgGyy_[k] = (avgGyy_[k] * prevN + syy) / currN;
                avgGxy_[k] = (avgGxy_[k] * prevN + sxy) / currN;
            }

            slotGxx[k] = sxx;
            slotGyy[k] = syy;
            slotGxy[k] = sxy;
        }

        histWriteIdx_ = (histWriteIdx_ + 1) % M;
        if (histFilled_ < M) {
            ++histFilled_;
        }

        // Recompute exact average when wrapping to index 0 to eliminate floating-point drift
        if (bufferFull && histWriteIdx_ == 0) {
            const double invM = 1.0 / static_cast<double>(M);
            for (std::size_t k = 0; k < numBins; ++k) {
                double sumX = 0.0;
                double sumY = 0.0;
                std::complex<double> sumXY{0.0, 0.0};
                for (std::size_t m = 0; m < M; ++m) {
                    sumX += histGxx_[m][k];
                    sumY += histGyy_[m][k];
                    sumXY += histGxy_[m][k];
                }
                avgGxx_[k] = sumX * invM;
                avgGyy_[k] = sumY * invM;
                avgGxy_[k] = sumXY * invM;
            }
        }
    }

    ++frameCount_;
    outFrame.frameCounter = frameCount_;

    // 4. Compute Delay Estimate from averaged cross-spectrum
    outFrame.delayEstimate = delayFinder_.estimate(avgGxy_, config_.sampleRate);

    // 5. Compute H1, H2, Magnitude, Wrapped Phase, and Squared Coherence
    const double delayComp = config_.delayCompensationSamples;
    const bool applyDelayComp = (std::abs(delayComp) > 1e-9);
    const double twoPiOverN = (2.0 * kPi) / static_cast<double>(N);

    for (std::size_t k = 0; k < numBins; ++k) {
        outFrame.frequenciesHz[k] = static_cast<double>(k) * binHz;

        const double gxx = avgGxx_[k];
        const double gyy = avgGyy_[k];
        const std::complex<double> gxy = avgGxy_[k];

        outFrame.gxx[k] = gxx;
        outFrame.gyy[k] = gyy;
        outFrame.gxy[k] = gxy;

        std::complex<double> phaseComp{1.0, 0.0};
        if (applyDelayComp) {
            const double angle = twoPiOverN * static_cast<double>(k) * delayComp;
            phaseComp = std::complex<double>(std::cos(angle), std::sin(angle));
        }

        // H1 = (Gxy / Gxx) * e^{+j * 2*pi*k*D/N}
        const std::complex<double> h1 = (gxy / (gxx + kEpsilon)) * phaseComp;
        const double h1Mag = std::abs(h1);
        outFrame.magnitudeDb[k] = 20.0 * std::log10(std::max(h1Mag, kMinMagnitude));
        outFrame.phaseWrappedDeg[k] = std::atan2(h1.imag(), h1.real()) * kRadToDeg;

        // H2 = (Gyy / Gyx) * e^{+j * 2*pi*k*D/N}
        const std::complex<double> gyx = std::conj(gxy);
        const std::complex<double> h2 = (gyy / (gyx + std::complex<double>{kEpsilon, 0.0})) * phaseComp;
        const double h2Mag = std::abs(h2);
        outFrame.h2MagnitudeDb[k] = 20.0 * std::log10(std::max(h2Mag, kMinMagnitude));

        // Coherence gamma^2 = |Gxy|^2 / (Gxx * Gyy)
        const double denom = gxx * gyy + kEpsilon;
        const double coh = std::norm(gxy) / denom;
        outFrame.coherence[k] = std::clamp(coh, 0.0, 1.0);
    }

    // 6. Compute Unwrapped Phase
    unwrapPhaseDegrees(outFrame.phaseWrappedDeg, outFrame.phaseUnwrappedDeg);

    // 7. Apply Fractional-Octave Smoothing if enabled
    if (config_.smoothingMode != SmoothingMode::Off) {
        SpectrumSmoothing::apply(config_.smoothingMode, outFrame.frequenciesHz, outFrame.magnitudeDb, smoothScratch_);
        outFrame.magnitudeDb = smoothScratch_;

        SpectrumSmoothing::apply(config_.smoothingMode, outFrame.frequenciesHz, outFrame.h2MagnitudeDb, smoothScratch_);
        outFrame.h2MagnitudeDb = smoothScratch_;

        SpectrumSmoothing::apply(config_.smoothingMode, outFrame.frequenciesHz, outFrame.coherence, smoothScratch_);
        outFrame.coherence = smoothScratch_;

        SpectrumSmoothing::apply(config_.smoothingMode, outFrame.frequenciesHz, outFrame.phaseUnwrappedDeg, smoothScratch_);
        outFrame.phaseUnwrappedDeg = smoothScratch_;
    }
}

void TransferFunctionEngine::unwrapPhaseDegrees(const std::vector<double>& wrappedDeg,
                                                std::vector<double>& outUnwrappedDeg) {
    const std::size_t n = wrappedDeg.size();
    outUnwrappedDeg.resize(n);
    if (n == 0) {
        return;
    }

    outUnwrappedDeg[0] = wrappedDeg[0];
    double offset = 0.0;

    for (std::size_t i = 1; i < n; ++i) {
        double diff = wrappedDeg[i] - wrappedDeg[i - 1];
        while (diff > 180.0) {
            diff -= 360.0;
            offset -= 360.0;
        }
        while (diff < -180.0) {
            diff += 360.0;
            offset += 360.0;
        }
        outUnwrappedDeg[i] = wrappedDeg[i] + offset;
    }
}

} // namespace dsp
