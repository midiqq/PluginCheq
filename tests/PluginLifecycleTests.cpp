#include <catch2/catch_test_macros.hpp>

#include "plugin/PluginProcessor.h"
#include "ui/PluginEditor.h"

#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>

#include <atomic>
#include <cmath>
#include <memory>
#include <thread>

namespace {
struct ScopedJuceGuiInit {
    juce::ScopedJuceInitialiser_GUI init;
};
} // namespace

TEST_CASE("PluginLifecycle: Editor paint & hover before prepareToPlay, Ableton 4512 block re-prepare, and legacy XML restore", "[plugin][lifecycle]") {
    ScopedJuceGuiInit gui;

    auto proc = std::make_unique<plugin::DSPAnalysisProcessor>();

    // 1. Create Editor BEFORE prepareToPlay and paint empty/uninitialized frames safely
    std::unique_ptr<juce::AudioProcessorEditor> editor(proc->createEditor());
    REQUIRE(editor != nullptr);

    juce::Image canvas(juce::Image::ARGB, editor->getWidth(), editor->getHeight(), true);
    {
        juce::Graphics g(canvas);
        editor->paintEntireComponent(g, true);
    }

    // 2. Restore legacy state XML containing removed parameters (generatorType, generatorGainDb, routingMode=3)
    {
        juce::ValueTree legacyState("PARAMETERS");
        auto addParam = [&](const char* id, float val) {
            juce::ValueTree child("PARAM");
            child.setProperty("id", id, nullptr);
            child.setProperty("value", val, nullptr);
            legacyState.appendChild(child, nullptr);
        };
        addParam("routingMode", 3.0f); // Deleted InternalGenerator enum index
        addParam("generatorType", 2.0f);
        addParam("generatorGainDb", -12.0f);
        addParam("unwrapPhase", 1.0f);
        addParam("showH2", 1.0f);

        if (auto xml = legacyState.createXml()) {
            juce::MemoryBlock mb;
            juce::AudioProcessor::copyXmlToBinary(*xml, mb);
            proc->setStateInformation(mb.getData(), static_cast<int>(mb.getSize()));
        }
    }

    // 3. Ableton Live exact startup sequence: prepare(48000, 512) -> processBlock -> re-prepare(48000, 4512) -> large resize
    proc->prepareToPlay(48000.0, 512);

    juce::AudioBuffer<float> bufSmall(4, 512);
    bufSmall.clear();
    juce::MidiBuffer midi;
    proc->processBlock(bufSmall, midi);

    // Re-prepare with Ableton MME/DirectX 4512 buffer size while worker thread is already running
    proc->prepareToPlay(48000.0, 4512);

    std::atomic<bool> stopAudio{false};
    std::thread audioThread([&] {
        juce::AudioBuffer<float> buf(4, 4512);
        juce::MidiBuffer m;
        double phase = 0.0;
        while (!stopAudio.load(std::memory_order_relaxed)) {
            for (int ch = 0; ch < 4; ++ch) {
                float* ptr = buf.getWritePointer(ch);
                double p = phase;
                for (int i = 0; i < 4512; ++i) {
                    ptr[i] = static_cast<float>(0.5 * std::sin(p));
                    p += 2.0 * 3.141592653589793 * 1000.0 / 48000.0;
                }
            }
            phase += 0.37;
            proc->processBlock(buf, m);
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    });

    // Pump GUI timer ticks, paints, and resize events concurrently with streaming audio
    auto* typedEd = dynamic_cast<ui::PluginEditor*>(editor.get());
    REQUIRE(typedEd != nullptr);
    for (int iter = 0; iter < 25; ++iter) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        typedEd->tickForTesting();
        if (iter == 10) {
            editor->setSize(1280, 800);
        }
        juce::Graphics g(canvas);
        editor->paintEntireComponent(g, true);
    }

    stopAudio.store(true, std::memory_order_relaxed);
    audioThread.join();

    // Test extreme re-prepare that forces ring buffer resize while worker thread is running
    proc->prepareToPlay(96000.0, 20000);

    editor.reset();
    proc->releaseResources();
    proc.reset();
}

TEST_CASE("PluginLifecycle: Dual-instance TapSender -> PluginTapBus streaming + Editor rendering", "[plugin][interplugin]") {
    ScopedJuceGuiInit gui;

    plugin::DSPAnalysisProcessor sender;
    plugin::DSPAnalysisProcessor receiver;

    // Configure sender = TapSender (index 2), receiver = PluginTapBus (index 1)
    if (auto* p = sender.apvts().getParameter(plugin::ParamID::routingMode)) {
        p->setValueNotifyingHost(p->convertTo0to1(2.0f));
    }
    if (auto* p = receiver.apvts().getParameter(plugin::ParamID::routingMode)) {
        p->setValueNotifyingHost(p->convertTo0to1(1.0f));
    }

    sender.prepareToPlay(48000.0, 1024);
    receiver.prepareToPlay(48000.0, 1024);

    std::unique_ptr<juce::AudioProcessorEditor> senderEd(sender.createEditor());
    std::unique_ptr<juce::AudioProcessorEditor> receiverEd(receiver.createEditor());

    juce::AudioBuffer<float> audio(2, 1024);
    juce::MidiBuffer midi;
    for (int block = 0; block < 16; ++block) {
        for (int i = 0; i < 1024; ++i) {
            const float s = 0.4f * std::sin(2.0f * 3.14159265f * 440.0f * static_cast<float>(block * 1024 + i) / 48000.0f);
            audio.setSample(0, i, s);
            audio.setSample(1, i, s);
        }
        sender.processBlock(audio, midi);
        receiver.processBlock(audio, midi);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(40));
    if (auto* se = dynamic_cast<ui::PluginEditor*>(senderEd.get())) se->tickForTesting();
    if (auto* re = dynamic_cast<ui::PluginEditor*>(receiverEd.get())) re->tickForTesting();

    juce::Image img(juce::Image::ARGB, receiverEd->getWidth(), receiverEd->getHeight(), true);
    {
        juce::Graphics g(img);
        senderEd->paintEntireComponent(g, true);
        receiverEd->paintEntireComponent(g, true);
    }

    REQUIRE(receiver.isTapBusActive());

    receiverEd.reset();
    senderEd.reset();
    receiver.releaseResources();
    sender.releaseResources();
}
