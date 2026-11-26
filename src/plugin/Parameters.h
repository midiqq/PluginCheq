#pragma once

#include "dsp/AnalysisTypes.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace plugin {

namespace ParamID {
inline constexpr const char* routingMode      = "routingMode";
inline constexpr const char* tapChannel       = "tapChannel";
inline constexpr const char* generatorType    = "generatorType";
inline constexpr const char* generatorGainDb  = "generatorGainDb";
inline constexpr const char* swapRefMeas      = "swapRefMeas";
inline constexpr const char* channelMode      = "channelMode";
inline constexpr const char* fftSizeIndex     = "fftSizeIndex";
inline constexpr const char* windowType       = "windowType";
inline constexpr const char* averagingMode    = "averagingMode";
inline constexpr const char* averagingAlpha   = "averagingAlpha";
inline constexpr const char* linearAvgCount   = "linearAvgCount";
inline constexpr const char* unwrapPhase      = "unwrapPhase";
inline constexpr const char* showH2           = "showH2";
inline constexpr const char* magRangeDb       = "magRangeDb";
inline constexpr const char* smoothingMode    = "smoothingMode";
inline constexpr const char* delayCompEnabled = "delayCompEnabled";
inline constexpr const char* delayCompMs      = "delayCompMs";
} // namespace ParamID

inline std::size_t fftSizeFromIndex(int idx) noexcept {
    switch (idx) {
        case 0: return 1024;
        case 1: return 2048;
        case 2: return 4096;
        case 3: return 8192;
        case 4: return 16384;
        default: return 4096;
    }
}

inline float magRangeFromIndex(int idx) noexcept {
    switch (idx) {
        case 0: return 12.0f;
        case 1: return 24.0f;
        case 2: return 48.0f;
        default: return 24.0f;
    }
}

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Routing Mode
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ParamID::routingMode, 1},
        "Routing Mode",
        juce::StringArray{"DAW Sidechain", "Plugin Tap Bus (Ableton)", "Tap Sender (Pre-FX)", "Internal Generator"},
        1)); // Default to Plugin Tap Bus for seamless Ableton / universal experience

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ParamID::tapChannel, 1},
        "Tap Bus Channel",
        juce::StringArray{"Bus 1", "Bus 2", "Bus 3", "Bus 4"},
        0));

    // Internal Generator
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ParamID::generatorType, 1},
        "Generator",
        juce::StringArray{"Off", "Pink Noise", "White Noise", "Sine Sweep", "Multitone"},
        0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamID::generatorGainDb, 1},
        "Gen Gain (dB)",
        juce::NormalisableRange<float>(-48.0f, 0.0f, 0.5f),
        -18.0f));

    // Core Controls
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ParamID::swapRefMeas, 1},
        "Swap Ref / Meas",
        false));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ParamID::channelMode, 1},
        "Channel",
        juce::StringArray{"Left", "Right", "Mid (L+R)"},
        2));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ParamID::fftSizeIndex, 1},
        "FFT Size",
        juce::StringArray{"1024", "2048", "4096", "8192", "16384"},
        2));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ParamID::windowType, 1},
        "Window",
        juce::StringArray{"Hann", "Blackman-Harris", "Rectangular"},
        1));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ParamID::averagingMode, 1},
        "Averaging Mode",
        juce::StringArray{"Exponential", "Linear"},
        0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamID::averagingAlpha, 1},
        "Exp Alpha",
        juce::NormalisableRange<float>(0.02f, 1.0f, 0.01f, 0.5f),
        0.15f));

    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{ParamID::linearAvgCount, 1},
        "Linear Frames",
        1, 64, 16));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ParamID::unwrapPhase, 1},
        "Unwrap Phase",
        false));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ParamID::showH2, 1},
        "Show H2",
        false));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ParamID::magRangeDb, 1},
        "Magnitude Range",
        juce::StringArray{"+/- 12 dB", "+/- 24 dB", "+/- 48 dB"},
        1));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ParamID::smoothingMode, 1},
        "Smoothing",
        juce::StringArray{"Smooth: Off", "1/24 Oct", "1/12 Oct", "1/6 Oct", "1/3 Oct", "1/1 Oct"},
        0));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ParamID::delayCompEnabled, 1},
        "Delay Compensation",
        false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamID::delayCompMs, 1},
        "Delay Comp (ms)",
        juce::NormalisableRange<float>(-200.0f, 200.0f, 0.01f),
        0.0f));

    return {params.begin(), params.end()};
}

} // namespace plugin
