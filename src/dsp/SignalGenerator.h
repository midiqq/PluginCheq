#pragma once

#include <cmath>
#include <cstddef>
#include <random>
#include <vector>

namespace dsp {

enum class GeneratorType : int {
    Off = 0,
    PinkNoise = 1,
    WhiteNoise = 2,
    SineSweep = 3,
    Multitone = 4
};

/**
 * Real-time test signal generator for transfer function analysis in DAWs and Standalone mode.
 * Pink Noise (Paul Kellet's filter), White Noise, Logarithmic Sine Sweep, Multitone.
 */
class SignalGenerator {
public:
    void prepare(double sampleRate) noexcept {
        sampleRate_ = (sampleRate > 1000.0) ? sampleRate : 48000.0;
        phase_ = 0.0;
        b0_ = b1_ = b2_ = b3_ = b4_ = b5_ = b6_ = 0.0;
    }

    void generate(GeneratorType type, float gainDb, float* buffer, std::size_t numSamples) noexcept {
        if (type == GeneratorType::Off || !buffer || numSamples == 0) return;

        const float amplitude = std::pow(10.0f, gainDb / 20.0f);

        switch (type) {
            case GeneratorType::WhiteNoise: {
                for (std::size_t i = 0; i < numSamples; ++i) {
                    const float r = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 2.0f - 1.0f;
                    buffer[i] = r * amplitude;
                }
                break;
            }
            case GeneratorType::PinkNoise: {
                // Paul Kellet's filtered pink noise algorithm
                for (std::size_t i = 0; i < numSamples; ++i) {
                    const float white = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 2.0f - 1.0f;
                    b0_ = 0.99886 * b0_ + white * 0.0555179;
                    b1_ = 0.99332 * b1_ + white * 0.0750759;
                    b2_ = 0.96900 * b2_ + white * 0.1538520;
                    b3_ = 0.86650 * b3_ + white * 0.3104856;
                    b4_ = 0.55000 * b4_ + white * 0.5329522;
                    b5_ = -0.7616 * b5_ - white * 0.0168980;
                    const float pink = static_cast<float>(b0_ + b1_ + b2_ + b3_ + b4_ + b5_ + b6_ + white * 0.5362) * 0.11f;
                    b6_ = white * 0.115926;
                    buffer[i] = pink * amplitude;
                }
                break;
            }
            case GeneratorType::SineSweep: {
                // Logarithmic recurring sine sweep 20 Hz to 20 kHz over 2 seconds
                const double sweepDuration = 2.0;
                const double f0 = 20.0;
                const double f1 = 20000.0;
                const double k = (sweepDuration * f0) / std::log(f1 / f0);

                for (std::size_t i = 0; i < numSamples; ++i) {
                    const double t = phase_ / sampleRate_;
                    const double sweepTime = std::fmod(t, sweepDuration);
                    const double instFreq = f0 * std::pow(f1 / f0, sweepTime / sweepDuration);
                    const double sample = std::sin(2.0 * 3.141592653589793 * instFreq * sweepTime);
                    buffer[i] = static_cast<float>(sample) * amplitude;
                    phase_ += 1.0;
                    if (phase_ >= sweepDuration * sampleRate_) phase_ = 0.0;
                }
                break;
            }
            case GeneratorType::Multitone: {
                // 31 standard 1/3-octave ISO frequencies with pseudo-random phases to avoid high crest factor
                static constexpr double freqs[31] = {
                    20, 25, 31.5, 40, 50, 63, 80, 100, 125, 160, 200, 250, 315, 400, 500,
                    630, 800, 1000, 1250, 1600, 2000, 2500, 3150, 4000, 5000, 6300, 8000, 10000, 12500, 16000, 20000
                };
                for (std::size_t i = 0; i < numSamples; ++i) {
                    double val = 0.0;
                    for (int m = 0; m < 31; ++m) {
                        const double p = (2.0 * 3.141592653589793 * freqs[m] * phase_) / sampleRate_ + (m * 1.6180339887);
                        val += std::sin(p);
                    }
                    buffer[i] = static_cast<float>(val * (1.0 / 31.0)) * amplitude;
                    phase_ += 1.0;
                }
                break;
            }
            default:
                break;
        }
    }

private:
    double sampleRate_ = 48000.0;
    double phase_ = 0.0;
    double b0_ = 0.0, b1_ = 0.0, b2_ = 0.0, b3_ = 0.0, b4_ = 0.0, b5_ = 0.0, b6_ = 0.0;
};

} // namespace dsp
