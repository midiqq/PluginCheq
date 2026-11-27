#include "PluginProcessor.h"
#include "ui/PluginEditor.h"

#include <algorithm>
#include <cmath>

namespace plugin {

namespace {
void extractChannel(const juce::AudioBuffer<float>& busBuffer,
                    dsp::ChannelMode mode,
                    int numSamples,
                    double* dest,
                    float& outPeak) noexcept {
    const int numCh = busBuffer.getNumChannels();
    if (numCh <= 0) {
        std::fill_n(dest, numSamples, 0.0);
        outPeak = 0.0f;
        return;
    }

    const float* chL = busBuffer.getReadPointer(0);
    const float* chR = (numCh > 1) ? busBuffer.getReadPointer(1) : chL;

    float peak = 0.0f;
    if (mode == dsp::ChannelMode::Left) {
        for (int i = 0; i < numSamples; ++i) {
            const float s = chL[i];
            dest[i] = static_cast<double>(s);
            peak = std::max(peak, std::abs(s));
        }
    } else if (mode == dsp::ChannelMode::Right) {
        for (int i = 0; i < numSamples; ++i) {
            const float s = chR[i];
            dest[i] = static_cast<double>(s);
            peak = std::max(peak, std::abs(s));
        }
    } else {
        for (int i = 0; i < numSamples; ++i) {
            const float s = 0.5f * (chL[i] + chR[i]);
            dest[i] = static_cast<double>(s);
            peak = std::max(peak, std::abs(s));
        }
    }
    outPeak = peak;
}
} // namespace

DSPAnalysisProcessor::DSPAnalysisProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Main Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Main Output", juce::AudioChannelSet::stereo(), true)
                         .withInput("Sidechain (Reference)", juce::AudioChannelSet::stereo(), false)),
      apvts_(*this, nullptr, "PARAMETERS", createParameterLayout()) {
    scratchMain_.assign(16384, 0.0);
    scratchSidechain_.assign(16384, 0.0);
    tapFloatScratch_.assign(16384, 0.0f);

    apvts_.addParameterListener(ParamID::fftSizeIndex, this);
    apvts_.addParameterListener(ParamID::windowType, this);
    apvts_.addParameterListener(ParamID::averagingMode, this);
    apvts_.addParameterListener(ParamID::averagingAlpha, this);
    apvts_.addParameterListener(ParamID::linearAvgCount, this);
    apvts_.addParameterListener(ParamID::smoothingMode, this);
    apvts_.addParameterListener(ParamID::delayCompEnabled, this);
    apvts_.addParameterListener(ParamID::delayCompMs, this);
    apvts_.addParameterListener(ParamID::routingMode, this);
    apvts_.addParameterListener(ParamID::tapChannel, this);
}

DSPAnalysisProcessor::~DSPAnalysisProcessor() {
    apvts_.removeParameterListener(ParamID::fftSizeIndex, this);
    apvts_.removeParameterListener(ParamID::windowType, this);
    apvts_.removeParameterListener(ParamID::averagingMode, this);
    apvts_.removeParameterListener(ParamID::averagingAlpha, this);
    apvts_.removeParameterListener(ParamID::linearAvgCount, this);
    apvts_.removeParameterListener(ParamID::smoothingMode, this);
    apvts_.removeParameterListener(ParamID::delayCompEnabled, this);
    apvts_.removeParameterListener(ParamID::delayCompMs, this);
    apvts_.removeParameterListener(ParamID::routingMode, this);
    apvts_.removeParameterListener(ParamID::tapChannel, this);

    if (registeredTapChannel_ >= 0) {
        dsp::InterPluginBus::getInstance().unregisterSender(registeredTapChannel_);
        registeredTapChannel_ = -1;
    }
}

bool DSPAnalysisProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    const auto mainIn = layouts.getMainInputChannelSet();
    const auto mainOut = layouts.getMainOutputChannelSet();

    if (mainIn != mainOut) {
        return false;
    }
    if (mainIn != juce::AudioChannelSet::mono() && mainIn != juce::AudioChannelSet::stereo()) {
        return false;
    }

    if (layouts.inputBuses.size() > 1) {
        const auto scIn = layouts.inputBuses[1];
        if (!scIn.isDisabled() &&
            scIn != juce::AudioChannelSet::mono() &&
            scIn != juce::AudioChannelSet::stereo()) {
            return false;
        }
    }

    return true;
}

void DSPAnalysisProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    currentSampleRate_.store(sampleRate, std::memory_order_relaxed);
    currentBlockSize_.store(samplesPerBlock, std::memory_order_relaxed);

    const std::size_t maxBlock = static_cast<std::size_t>(std::max(4096, samplesPerBlock * 2));
    scratchMain_.assign(maxBlock, 0.0);
    scratchSidechain_.assign(maxBlock, 0.0);
    tapFloatScratch_.assign(maxBlock, 0.0f);

    worker_.prepare(sampleRate, maxBlock);
    syncWorkerConfig();
}

void DSPAnalysisProcessor::releaseResources() {
    if (registeredTapChannel_ >= 0) {
        dsp::InterPluginBus::getInstance().unregisterSender(registeredTapChannel_);
        registeredTapChannel_ = -1;
    }
}

void DSPAnalysisProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    const int numSamples = buffer.getNumSamples();
    if (numSamples <= 0) {
        return;
    }

    auto mainBus = getBusBuffer(buffer, true, 0);
    const int safeSamples = std::min(numSamples, static_cast<int>(scratchMain_.size()));
    const auto chMode = static_cast<dsp::ChannelMode>(
        static_cast<int>(apvts_.getRawParameterValue(ParamID::channelMode)->load(std::memory_order_relaxed)));
    const bool swap =
        apvts_.getRawParameterValue(ParamID::swapRefMeas)->load(std::memory_order_relaxed) >= 0.5f;

    const auto routing = static_cast<dsp::RoutingMode>(
        static_cast<int>(apvts_.getRawParameterValue(ParamID::routingMode)->load(std::memory_order_relaxed)));
    currentRoutingMode_.store(static_cast<int>(routing), std::memory_order_relaxed);

    const int tapCh = static_cast<int>(
        apvts_.getRawParameterValue(ParamID::tapChannel)->load(std::memory_order_relaxed));

    // Extract main processed audio
    float mainPeak = 0.0f;
    extractChannel(mainBus, chMode, safeSamples, scratchMain_.data(), mainPeak);

    // MODE 1: Tap Sender (Pre-FX tap in Ableton or any DAW)
    if (routing == dsp::RoutingMode::TapSender) {
        if (registeredTapChannel_ != tapCh) {
            if (registeredTapChannel_ >= 0) dsp::InterPluginBus::getInstance().unregisterSender(registeredTapChannel_);
            registeredTapChannel_ = tapCh;
            dsp::InterPluginBus::getInstance().registerSender(tapCh, "TapSender", currentSampleRate_.load(std::memory_order_relaxed));
        }

        // Push current signal to inter-plugin bus as float
        for (int i = 0; i < safeSamples; ++i) {
            tapFloatScratch_[i] = static_cast<float>(scratchMain_[i]);
        }
        dsp::InterPluginBus::getInstance().pushSamples(tapCh, tapFloatScratch_.data(), static_cast<std::size_t>(safeSamples));

        refPeak_.store(mainPeak, std::memory_order_relaxed);
        measPeak_.store(0.0f, std::memory_order_relaxed);
        tapBusActive_.store(true, std::memory_order_relaxed);
        return; // Sender only passes audio through bit-accurately and broadcasts to receiver
    }

    // In analysis modes, unregister if we previously were sender
    if (registeredTapChannel_ >= 0) {
        dsp::InterPluginBus::getInstance().unregisterSender(registeredTapChannel_);
        registeredTapChannel_ = -1;
    }

    float refPeak = 0.0f;
    bool hasReference = false;

    // MODE 2: Plugin Tap Bus (Receiver mode - connects to Tap Sender before FX)
    if (routing == dsp::RoutingMode::PluginTapBus) {
        const bool senderActive = dsp::InterPluginBus::getInstance().isSenderActive(tapCh);
        tapBusActive_.store(senderActive, std::memory_order_relaxed);
        sidechainActive_.store(false, std::memory_order_relaxed);

        if (senderActive) {
            const std::size_t read = dsp::InterPluginBus::getInstance().pullSamples(
                tapCh, tapFloatScratch_.data(), static_cast<std::size_t>(safeSamples));

            for (std::size_t i = 0; i < read; ++i) {
                const double s = static_cast<double>(tapFloatScratch_[i]);
                scratchSidechain_[i] = s;
                refPeak = std::max(refPeak, static_cast<float>(std::abs(s)));
            }
            if (read < static_cast<std::size_t>(safeSamples)) {
                std::fill_n(scratchSidechain_.data() + read, safeSamples - read, 0.0);
            }
            hasReference = (read > 0);
        } else {
            std::fill_n(scratchSidechain_.data(), safeSamples, 0.0);
        }
    }
    // MODE 0: Standard DAW Sidechain Bus (Reaper, Cubase, Studio One)
    else {
        tapBusActive_.store(false, std::memory_order_relaxed);
        const bool scBusEnabled = (getBusCount(true) > 1) && getBus(true, 1)->isEnabled();
        const int scChannels = scBusEnabled ? getBusBuffer(buffer, true, 1).getNumChannels() : 0;
        const bool hasSidechain = scBusEnabled && (scChannels > 0);
        sidechainActive_.store(hasSidechain, std::memory_order_relaxed);

        if (hasSidechain) {
            auto scBus = getBusBuffer(buffer, true, 1);
            extractChannel(scBus, chMode, safeSamples, scratchSidechain_.data(), refPeak);
            hasReference = true;
        } else {
            std::fill_n(scratchSidechain_.data(), safeSamples, 0.0);
        }
    }

    // Default: Reference (scratchSidechain_) vs Measurement (scratchMain_)
    if (!swap) {
        refPeak_.store(refPeak, std::memory_order_relaxed);
        measPeak_.store(mainPeak, std::memory_order_relaxed);
        worker_.pushAudioBlock(scratchSidechain_.data(), scratchMain_.data(), static_cast<std::size_t>(safeSamples));
    } else {
        refPeak_.store(mainPeak, std::memory_order_relaxed);
        measPeak_.store(refPeak, std::memory_order_relaxed);
        worker_.pushAudioBlock(scratchMain_.data(), scratchSidechain_.data(), static_cast<std::size_t>(safeSamples));
    }
}

void DSPAnalysisProcessor::parameterChanged(const juce::String&, float) {
    syncWorkerConfig();
}

void DSPAnalysisProcessor::syncWorkerConfig() {
    dsp::TransferFunctionConfig cfg;
    cfg.sampleRate = currentSampleRate_.load(std::memory_order_relaxed);
    cfg.fftSize = fftSizeFromIndex(
        static_cast<int>(apvts_.getRawParameterValue(ParamID::fftSizeIndex)->load()));
    cfg.windowType = static_cast<dsp::WindowType>(
        static_cast<int>(apvts_.getRawParameterValue(ParamID::windowType)->load()));
    cfg.averagingMode = static_cast<dsp::AveragingMode>(
        static_cast<int>(apvts_.getRawParameterValue(ParamID::averagingMode)->load()));
    cfg.exponentialAlpha = static_cast<double>(
        apvts_.getRawParameterValue(ParamID::averagingAlpha)->load());
    cfg.linearAverageCount = static_cast<std::size_t>(
        apvts_.getRawParameterValue(ParamID::linearAvgCount)->load());
    cfg.hopFraction = 0.25;
    cfg.smoothingMode = static_cast<dsp::SmoothingMode>(
        static_cast<int>(apvts_.getRawParameterValue(ParamID::smoothingMode)->load()));

    const bool delayCompOn =
        apvts_.getRawParameterValue(ParamID::delayCompEnabled)->load() >= 0.5f;
    const double delayMs = static_cast<double>(
        apvts_.getRawParameterValue(ParamID::delayCompMs)->load());
    cfg.delayCompensationSamples = delayCompOn ? (delayMs * cfg.sampleRate / 1000.0) : 0.0;

    worker_.requestConfigUpdate(cfg);
}

void DSPAnalysisProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = apvts_.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void DSPAnalysisProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts_.state.getType())) {
        apvts_.replaceState(juce::ValueTree::fromXml(*xmlState));
    }
}

juce::AudioProcessorEditor* DSPAnalysisProcessor::createEditor() {
    return new ui::PluginEditor(*this);
}

} // namespace plugin

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new plugin::DSPAnalysisProcessor();
}
