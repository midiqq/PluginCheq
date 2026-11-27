#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "TestSystems.h"
#include "dsp/DynamicsEngine.h"
#include "dsp/HarmonicEngine.h"

#include <vector>
#include <cmath>

using Catch::Matchers::WithinAbs;

TEST_CASE("DynamicsEngine: Correctly detects compression curve and ratio", "[dynamics]") {
    dsp::DynamicsEngine dyn;
    dyn.prepare(48000.0);

    constexpr std::size_t numSamples = 48000; // 1 second of audio
    std::vector<double> ref(numSamples);
    std::vector<double> meas(numSamples);

    // Generate amplitude ramp from 0.001 (-60 dB) to 1.0 (0 dB)
    for (std::size_t i = 0; i < numSamples; ++i) {
        const double frac = static_cast<double>(i) / static_cast<double>(numSamples);
        const double amp = 0.001 + frac * 0.999;
        const double carrier = std::sin(2.0 * 3.141592653589793 * 1000.0 * i / 48000.0);
        ref[i] = carrier * amp;

        // Apply 4:1 compressor above -12 dB (amp ~ 0.251)
        double outAmp = amp;
        constexpr double thresh = 0.2511886; // -12 dB
        if (amp > thresh) {
            const double overDb = 20.0 * std::log10(amp / thresh);
            const double compOverDb = overDb / 4.0; // 4:1 ratio
            outAmp = thresh * std::pow(10.0, compOverDb / 20.0);
        }
        meas[i] = carrier * outAmp;
    }

    dsp::DynamicsFrame frame;
    dyn.processBlock(ref.data(), meas.data(), numSamples, frame);

    REQUIRE(frame.hasData);
    REQUIRE(frame.inputLevelsDb.size() >= 8);
    REQUIRE(frame.estimatedRatio > 1.5f);
}

TEST_CASE("HarmonicEngine: Accurately identifies sine fundamental and harmonic distortion", "[harmonics]") {
    dsp::HarmonicEngine harm;
    constexpr std::size_t fftSize = 4096;
    constexpr double fs = 48000.0;
    harm.prepare(fftSize, fs);

    std::vector<double> meas(fftSize);
    // Generate 1 kHz fundamental at -6 dBFS + 2 kHz H2 at -26 dBFS (10% distortion / -20 dBc)
    constexpr double twoPi = 6.28318530717958647692;
    for (std::size_t i = 0; i < fftSize; ++i) {
        const double f1 = std::sin(twoPi * 1000.0 * i / fs) * 0.5; // -6 dBFS
        const double h2 = std::sin(twoPi * 2000.0 * i / fs) * 0.05; // -26 dBFS
        meas[i] = f1 + h2;
    }

    dsp::HarmonicFrame frame;
    harm.processBlock(meas.data(), fftSize, frame);

    REQUIRE(frame.hasData);
    REQUIRE_THAT(frame.fundamentalFreqHz, WithinAbs(1000.0, 5.0));
    REQUIRE_THAT(frame.fundamentalLevelDb, WithinAbs(-6.0, 0.5));
    REQUIRE(frame.thdPercent > 8.0f);
    REQUIRE(frame.thdPercent < 12.0f);
    REQUIRE(frame.harmonicRelativeDb.size() >= 1);
    REQUIRE_THAT(frame.harmonicRelativeDb[0], WithinAbs(-20.0, 1.0));
}
