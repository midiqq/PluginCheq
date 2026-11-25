#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "TestSystems.h"
#include "dsp/TransferFunctionEngine.h"

#include <cmath>
#include <vector>

using Catch::Matchers::WithinAbs;

namespace {

dsp::TransferFunctionFrame runEngineOnSignals(
    const std::vector<double>& ref,
    const std::vector<double>& meas,
    const dsp::TransferFunctionConfig& cfg,
    std::size_t skipInitialSamples = 0)
{
    dsp::TransferFunctionEngine engine;
    engine.configure(cfg);

    dsp::TransferFunctionFrame frame;
    const std::size_t hop = engine.hopSize();

    for (std::size_t pos = skipInitialSamples; pos + cfg.fftSize <= ref.size(); pos += hop) {
        engine.processFrame(ref.data() + pos, meas.data() + pos, frame);
    }
    return frame;
}

} // namespace

TEST_CASE("TransferFunction: Identity system produces 0 dB, 0 deg, coherence 1", "[tf][identity]") {
    constexpr double fs = 48000.0;
    constexpr std::size_t fftSize = 4096;
    const auto x = dsp::tests::generateWhiteNoise(fftSize * 8, 123);

    dsp::TransferFunctionConfig cfg;
    cfg.fftSize = fftSize;
    cfg.sampleRate = fs;
    cfg.windowType = dsp::WindowType::BlackmanHarris;
    cfg.averagingMode = dsp::AveragingMode::Linear;
    cfg.linearAverageCount = 12;

    const auto frame = runEngineOnSignals(x, x, cfg);

    for (std::size_t k = 2; k < frame.numBins - 2; ++k) {
        REQUIRE_THAT(frame.magnitudeDb[k], WithinAbs(0.0, 0.01));
        REQUIRE_THAT(frame.phaseWrappedDeg[k], WithinAbs(0.0, 0.1));
        REQUIRE(frame.coherence[k] > 0.9999);
    }
}

TEST_CASE("TransferFunction: Constant gain and attenuation", "[tf][gain]") {
    constexpr double fs = 48000.0;
    constexpr std::size_t fftSize = 2048;
    const auto x = dsp::tests::generateWhiteNoise(fftSize * 6, 77);

    dsp::TransferFunctionConfig cfg;
    cfg.fftSize = fftSize;
    cfg.sampleRate = fs;
    cfg.windowType = dsp::WindowType::Hann;
    cfg.averagingMode = dsp::AveragingMode::Exponential;
    cfg.exponentialAlpha = 0.25;

    SECTION("+6 dB gain") {
        const double gain = std::pow(10.0, 6.0 / 20.0);
        std::vector<double> y(x.size());
        for (std::size_t i = 0; i < x.size(); ++i) y[i] = x[i] * gain;

        const auto frame = runEngineOnSignals(x, y, cfg);
        for (std::size_t k = 2; k < frame.numBins - 2; ++k) {
            REQUIRE_THAT(frame.magnitudeDb[k], WithinAbs(6.0, 0.01));
            REQUIRE_THAT(frame.phaseWrappedDeg[k], WithinAbs(0.0, 0.1));
            REQUIRE(frame.coherence[k] > 0.9999);
        }
    }

    SECTION("-12 dB attenuation") {
        const double gain = std::pow(10.0, -12.0 / 20.0);
        std::vector<double> y(x.size());
        for (std::size_t i = 0; i < x.size(); ++i) y[i] = x[i] * gain;

        const auto frame = runEngineOnSignals(x, y, cfg);
        for (std::size_t k = 2; k < frame.numBins - 2; ++k) {
            REQUIRE_THAT(frame.magnitudeDb[k], WithinAbs(-12.0, 0.01));
            REQUIRE_THAT(frame.phaseWrappedDeg[k], WithinAbs(0.0, 0.1));
            REQUIRE(frame.coherence[k] > 0.9999);
        }
    }
}

TEST_CASE("TransferFunction: Polarity inversion yields +/-180 deg phase", "[tf][polarity]") {
    constexpr double fs = 48000.0;
    constexpr std::size_t fftSize = 2048;
    const auto x = dsp::tests::generateWhiteNoise(fftSize * 4, 99);
    std::vector<double> y(x.size());
    for (std::size_t i = 0; i < x.size(); ++i) y[i] = -x[i];

    dsp::TransferFunctionConfig cfg;
    cfg.fftSize = fftSize;
    cfg.sampleRate = fs;
    cfg.windowType = dsp::WindowType::Hann;
    cfg.averagingMode = dsp::AveragingMode::Linear;
    cfg.linearAverageCount = 8;

    const auto frame = runEngineOnSignals(x, y, cfg);
    for (std::size_t k = 2; k < frame.numBins - 2; ++k) {
        REQUIRE_THAT(frame.magnitudeDb[k], WithinAbs(0.0, 0.01));
        REQUIRE_THAT(std::abs(frame.phaseWrappedDeg[k]), WithinAbs(180.0, 0.1));
        REQUIRE(frame.coherence[k] > 0.9999);
    }
}

TEST_CASE("TransferFunction: Butterworth 2nd-order LPF and HPF match analytical H(z)", "[tf][biquad]") {
    constexpr double fs = 48000.0;
    constexpr double fc = 1000.0;
    constexpr std::size_t fftSize = 8192;
    const auto x = dsp::tests::generateWhiteNoise(fftSize * 16, 314);

    dsp::TransferFunctionConfig cfg;
    cfg.fftSize = fftSize;
    cfg.sampleRate = fs;
    cfg.windowType = dsp::WindowType::BlackmanHarris;
    cfg.averagingMode = dsp::AveragingMode::Linear;
    cfg.linearAverageCount = 32;

    SECTION("Butterworth Low-Pass 1 kHz") {
        auto lpf = dsp::tests::Biquad::makeButterworthLowPass(fs, fc);
        std::vector<double> y(x.size());
        for (std::size_t i = 0; i < x.size(); ++i) y[i] = lpf.processSample(x[i]);

        // Skip initial transient (4096 samples)
        const auto frame = runEngineOnSignals(x, y, cfg, 4096);

        // Validate from 100 Hz to 10 kHz where filter SNR is high
        for (std::size_t k = 1; k < frame.numBins; ++k) {
            const double f = frame.frequenciesHz[k];
            if (f < 100.0 || f > 10000.0) continue;

            const auto exactH = lpf.evaluateFrequencyResponse(f, fs);
            const double exactMagDb = 20.0 * std::log10(std::abs(exactH));
            const double exactPhaseDeg = std::atan2(exactH.imag(), exactH.real()) * (180.0 / dsp::tests::kPi);

            REQUIRE_THAT(frame.magnitudeDb[k], WithinAbs(exactMagDb, 0.25));
            REQUIRE_THAT(frame.phaseWrappedDeg[k], WithinAbs(exactPhaseDeg, 2.0));
            REQUIRE(frame.coherence[k] > 0.98);
        }
    }

    SECTION("Butterworth High-Pass 1 kHz") {
        auto hpf = dsp::tests::Biquad::makeButterworthHighPass(fs, fc);
        std::vector<double> y(x.size());
        for (std::size_t i = 0; i < x.size(); ++i) y[i] = hpf.processSample(x[i]);

        const auto frame = runEngineOnSignals(x, y, cfg, 4096);

        for (std::size_t k = 1; k < frame.numBins; ++k) {
            const double f = frame.frequenciesHz[k];
            if (f < 200.0 || f > 18000.0) continue;

            const auto exactH = hpf.evaluateFrequencyResponse(f, fs);
            const double exactMagDb = 20.0 * std::log10(std::abs(exactH));

            REQUIRE_THAT(frame.magnitudeDb[k], WithinAbs(exactMagDb, 0.25));
            REQUIRE(frame.coherence[k] > 0.98);
        }
    }
}

TEST_CASE("TransferFunction: Known integer delay produces linear phase slope", "[tf][delay]") {
    constexpr double fs = 48000.0;
    constexpr std::size_t fftSize = 4096;
    constexpr std::size_t delaySamples = 16;

    const auto x = dsp::tests::generateWhiteNoise(fftSize * 12, 2026);
    std::vector<double> y(x.size(), 0.0);
    for (std::size_t i = delaySamples; i < x.size(); ++i) {
        y[i] = x[i - delaySamples];
    }

    dsp::TransferFunctionConfig cfg;
    cfg.fftSize = fftSize;
    cfg.sampleRate = fs;
    cfg.windowType = dsp::WindowType::BlackmanHarris;
    cfg.averagingMode = dsp::AveragingMode::Linear;
    cfg.linearAverageCount = 24;

    const auto frame = runEngineOnSignals(x, y, cfg, 256);

    // Expected unwrapped phase at frequency f: -360 * f * (D / fs)
    const double expectedSlopeDegPerHz = -360.0 * static_cast<double>(delaySamples) / fs;

    for (std::size_t k = 10; k < frame.numBins / 2; ++k) {
        const double f = frame.frequenciesHz[k];
        const double expectedPhase = expectedSlopeDegPerHz * f;
        REQUIRE_THAT(frame.phaseUnwrappedDeg[k], WithinAbs(expectedPhase, 3.0));
        REQUIRE(frame.coherence[k] > 0.97);
    }
}

TEST_CASE("TransferFunction: Uncorrelated output noise reduces coherence while H1 stays unbiased", "[tf][noise]") {
    constexpr double fs = 48000.0;
    constexpr std::size_t fftSize = 2048;

    const auto x = dsp::tests::generateWhiteNoise(fftSize * 32, 101, 0.5);
    const auto noise = dsp::tests::generateWhiteNoise(fftSize * 32, 202, 0.5); // equal power -> SNR = 0 dB -> coherence ~ 0.5

    std::vector<double> y(x.size());
    for (std::size_t i = 0; i < x.size(); ++i) {
        y[i] = x[i] + noise[i];
    }

    dsp::TransferFunctionConfig cfg;
    cfg.fftSize = fftSize;
    cfg.sampleRate = fs;
    cfg.windowType = dsp::WindowType::Hann;
    cfg.averagingMode = dsp::AveragingMode::Linear;
    cfg.linearAverageCount = 64;

    const auto frame = runEngineOnSignals(x, y, cfg);

    double meanCoh = 0.0;
    double meanH1Db = 0.0;
    double meanH2Db = 0.0;
    std::size_t count = 0;

    for (std::size_t k = 10; k < frame.numBins - 10; ++k) {
        meanCoh += frame.coherence[k];
        meanH1Db += frame.magnitudeDb[k];
        meanH2Db += frame.h2MagnitudeDb[k];
        ++count;
    }

    meanCoh /= static_cast<double>(count);
    meanH1Db /= static_cast<double>(count);
    meanH2Db /= static_cast<double>(count);

    // At 0 dB output noise SNR, theoretical coherence = 0.5, H1 = 0 dB, H2 = +3.01 dB
    REQUIRE_THAT(meanCoh, WithinAbs(0.5, 0.06));
    REQUIRE_THAT(meanH1Db, WithinAbs(0.0, 0.3));
    REQUIRE(meanH2Db > meanH1Db + 2.0);
}
