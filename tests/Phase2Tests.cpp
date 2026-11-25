#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "TestSystems.h"
#include "dsp/DelayFinder.h"
#include "dsp/SpectrumSmoothing.h"
#include "dsp/TransferFunctionEngine.h"

#include <cmath>
#include <vector>

using Catch::Matchers::WithinAbs;

namespace {

std::vector<double> applyFractionalDelaySinc(const std::vector<double>& x, double delaySamples) {
    std::vector<double> y(x.size(), 0.0);
    constexpr int halfTap = 16;
    for (std::size_t n = 0; n < x.size(); ++n) {
        double sum = 0.0;
        const double center = static_cast<double>(n) - delaySamples;
        const int base = static_cast<int>(std::floor(center));
        for (int k = base - halfTap; k <= base + halfTap; ++k) {
            if (k < 0 || k >= static_cast<int>(x.size())) continue;
            const double t = center - static_cast<double>(k);
            const double sinc = (std::abs(t) < 1e-9)
                ? 1.0
                : std::sin(dsp::tests::kPi * t) / (dsp::tests::kPi * t);
            // Hann window on sinc kernel
            const double winArg = (static_cast<double>(k - base) + halfTap) / (2.0 * halfTap);
            const double win = 0.5 * (1.0 - std::cos(2.0 * dsp::tests::kPi * winArg));
            sum += x[static_cast<std::size_t>(k)] * sinc * win;
        }
        y[n] = sum;
    }
    return y;
}

} // namespace

TEST_CASE("DelayFinder: Detects positive, negative, and inverted-polarity delays", "[phase2][delayfinder]") {
    constexpr double fs = 48000.0;
    constexpr std::size_t fftSize = 4096;
    const auto x = dsp::tests::generateWhiteNoise(fftSize * 8, 555);

    auto measureDelay = [&](const std::vector<double>& ref, const std::vector<double>& meas) {
        dsp::TransferFunctionEngine engine;
        dsp::TransferFunctionConfig cfg;
        cfg.fftSize = fftSize;
        cfg.sampleRate = fs;
        cfg.windowType = dsp::WindowType::Hann;
        cfg.averagingMode = dsp::AveragingMode::Linear;
        cfg.linearAverageCount = 8;
        engine.configure(cfg);

        dsp::TransferFunctionFrame frame;
        const std::size_t hop = engine.hopSize();
        for (std::size_t pos = 256; pos + fftSize <= ref.size(); pos += hop) {
            engine.processFrame(ref.data() + pos, meas.data() + pos, frame);
        }
        return frame.delayEstimate;
    };

    SECTION("Positive delay (+16 samples)") {
        std::vector<double> y(x.size(), 0.0);
        for (std::size_t i = 16; i < x.size(); ++i) y[i] = x[i - 16];

        const auto est = measureDelay(x, y);
        REQUIRE(est.valid);
        REQUIRE_FALSE(est.polarityInverted);
        REQUIRE_THAT(est.delaySamples, WithinAbs(16.0, 0.05));
        REQUIRE_THAT(est.delayMs, WithinAbs(16.0 * 1000.0 / fs, 0.002));
    }

    SECTION("Negative delay (-12 samples)") {
        std::vector<double> y(x.size(), 0.0);
        for (std::size_t i = 0; i + 12 < x.size(); ++i) y[i] = x[i + 12];

        const auto est = measureDelay(x, y);
        REQUIRE(est.valid);
        REQUIRE_FALSE(est.polarityInverted);
        REQUIRE_THAT(est.delaySamples, WithinAbs(-12.0, 0.05));
    }

    SECTION("Polarity inversion with delay (+16 samples, inverted)") {
        std::vector<double> y(x.size(), 0.0);
        for (std::size_t i = 16; i < x.size(); ++i) y[i] = -x[i - 16];

        const auto est = measureDelay(x, y);
        REQUIRE(est.valid);
        REQUIRE(est.polarityInverted);
        REQUIRE_THAT(est.delaySamples, WithinAbs(16.0, 0.05));
    }

    SECTION("Sub-sample fractional delay (+10.35 samples)") {
        const auto y = applyFractionalDelaySinc(x, 10.35);
        const auto est = measureDelay(x, y);
        REQUIRE(est.valid);
        REQUIRE_FALSE(est.polarityInverted);
        REQUIRE_THAT(est.delaySamples, WithinAbs(10.35, 0.12));
    }
}

TEST_CASE("TransferFunction: Delay compensation flattens phase slope to 0 deg", "[phase2][compensation]") {
    constexpr double fs = 48000.0;
    constexpr std::size_t fftSize = 4096;
    constexpr std::size_t delaySamples = 16;

    const auto x = dsp::tests::generateWhiteNoise(fftSize * 8, 777);
    std::vector<double> y(x.size(), 0.0);
    for (std::size_t i = delaySamples; i < x.size(); ++i) {
        y[i] = x[i - delaySamples];
    }

    dsp::TransferFunctionConfig cfg;
    cfg.fftSize = fftSize;
    cfg.sampleRate = fs;
    cfg.windowType = dsp::WindowType::BlackmanHarris;
    cfg.averagingMode = dsp::AveragingMode::Linear;
    cfg.linearAverageCount = 16;
    cfg.delayCompensationSamples = static_cast<double>(delaySamples);

    dsp::TransferFunctionEngine engine;
    engine.configure(cfg);

    dsp::TransferFunctionFrame frame;
    for (std::size_t pos = 256; pos + fftSize <= x.size(); pos += engine.hopSize()) {
        engine.processFrame(x.data() + pos, y.data() + pos, frame);
    }

    for (std::size_t k = 10; k < frame.numBins / 2; ++k) {
        REQUIRE_THAT(frame.phaseUnwrappedDeg[k], WithinAbs(0.0, 1.0));
        REQUIRE_THAT(frame.magnitudeDb[k], WithinAbs(0.0, 0.05));
    }
}

TEST_CASE("SpectrumSmoothing: Preserves flat spectra and smooths narrowband peaks", "[phase2][smoothing]") {
    constexpr std::size_t numBins = 1025;
    constexpr double fs = 48000.0;
    const double binHz = fs / 2048.0;

    std::vector<double> freqs(numBins);
    std::vector<double> flat(numBins, -6.0);
    std::vector<double> spike(numBins, 0.0);

    for (std::size_t k = 0; k < numBins; ++k) {
        freqs[k] = static_cast<double>(k) * binHz;
    }
    spike[100] = 12.0; // narrowband spike around ~2343 Hz

    std::vector<double> outFlat;
    dsp::SpectrumSmoothing::apply(dsp::SmoothingMode::Octave3, freqs, flat, outFlat);
    for (std::size_t k = 0; k < numBins; ++k) {
        REQUIRE_THAT(outFlat[k], WithinAbs(-6.0, 1e-9));
    }

    std::vector<double> out12;
    std::vector<double> out3;
    dsp::SpectrumSmoothing::apply(dsp::SmoothingMode::Octave12, freqs, spike, out12);
    dsp::SpectrumSmoothing::apply(dsp::SmoothingMode::Octave3, freqs, spike, out3);

    REQUIRE(out12[100] < 12.0);
    REQUIRE(out3[100] < out12[100]);
    REQUIRE(out3[100] > 0.0);
}
