#pragma once

#include <atomic>
#include <array>
#include <cstddef>
#include <cstring>
#include <cstdint>

namespace dsp {

/**
 * High-performance lock-free inter-instance audio bus.
 * Allows Instance 1 (Sender / Tap before plugin chain) to stream reference audio
 * directly to Instance 2 (Receiver / PluginCheq main analyzer after plugin chain).
 * Completely bypasses DAW sidechain limitations (perfect for Ableton Live, FL Studio, etc.).
 */
class InterPluginBus {
public:
    static constexpr std::size_t kMaxChannels = 8;
    static constexpr std::size_t kRingCapacity = 65536; // ~1.36s at 48kHz
    static constexpr std::size_t kMask = kRingCapacity - 1;

    struct ChannelSlot {
        char name[32]{"Default"};
        std::atomic<bool> inUse{false};
        std::atomic<uint32_t> sampleRate{48000};
        alignas(64) std::atomic<std::size_t> writeIdx{0};
        alignas(64) std::atomic<std::size_t> readIdx{0};
        std::array<float, kRingCapacity> buffer{};
    };

    static InterPluginBus& getInstance() noexcept {
        static InterPluginBus instance;
        return instance;
    }

    void registerSender(int channelId, const char* name, double sampleRate) noexcept {
        if (channelId < 0 || channelId >= static_cast<int>(kMaxChannels)) return;
        auto& slot = channels_[channelId];
        if (name) {
            std::strncpy(slot.name, name, sizeof(slot.name) - 1);
            slot.name[sizeof(slot.name) - 1] = '\0';
        }
        slot.sampleRate.store(static_cast<uint32_t>(sampleRate), std::memory_order_relaxed);
        slot.writeIdx.store(0, std::memory_order_relaxed);
        slot.readIdx.store(0, std::memory_order_relaxed);
        slot.inUse.store(true, std::memory_order_release);
    }

    void unregisterSender(int channelId) noexcept {
        if (channelId < 0 || channelId >= static_cast<int>(kMaxChannels)) return;
        channels_[channelId].inUse.store(false, std::memory_order_release);
    }

    bool isSenderActive(int channelId) const noexcept {
        if (channelId < 0 || channelId >= static_cast<int>(kMaxChannels)) return false;
        return channels_[channelId].inUse.load(std::memory_order_acquire);
    }

    const char* getChannelName(int channelId) const noexcept {
        if (channelId < 0 || channelId >= static_cast<int>(kMaxChannels)) return "";
        return channels_[channelId].name;
    }

    void pushSamples(int channelId, const float* data, std::size_t numSamples) noexcept {
        if (channelId < 0 || channelId >= static_cast<int>(kMaxChannels) || !data || numSamples == 0) return;
        auto& slot = channels_[channelId];
        if (!slot.inUse.load(std::memory_order_relaxed)) return;

        const std::size_t w = slot.writeIdx.load(std::memory_order_relaxed);
        const std::size_t r = slot.readIdx.load(std::memory_order_acquire);
        const std::size_t freeSpace = kRingCapacity - (w - r);
        const std::size_t toWrite = std::min(numSamples, freeSpace);

        if (toWrite == 0) return;

        const std::size_t startPos = w & kMask;
        const std::size_t firstPart = kRingCapacity - startPos;
        if (toWrite <= firstPart) {
            std::memcpy(&slot.buffer[startPos], data, toWrite * sizeof(float));
        } else {
            std::memcpy(&slot.buffer[startPos], data, firstPart * sizeof(float));
            std::memcpy(&slot.buffer[0], data + firstPart, (toWrite - firstPart) * sizeof(float));
        }

        slot.writeIdx.store(w + toWrite, std::memory_order_release);
    }

    std::size_t pullSamples(int channelId, float* dest, std::size_t numSamples) noexcept {
        if (channelId < 0 || channelId >= static_cast<int>(kMaxChannels) || !dest || numSamples == 0) return 0;
        auto& slot = channels_[channelId];
        if (!slot.inUse.load(std::memory_order_relaxed)) return 0;

        const std::size_t w = slot.writeIdx.load(std::memory_order_acquire);
        const std::size_t r = slot.readIdx.load(std::memory_order_relaxed);
        const std::size_t available = w - r;
        const std::size_t toRead = std::min(numSamples, available);

        if (toRead == 0) return 0;

        const std::size_t startPos = r & kMask;
        const std::size_t firstPart = kRingCapacity - startPos;
        if (toRead <= firstPart) {
            std::memcpy(dest, &slot.buffer[startPos], toRead * sizeof(float));
        } else {
            std::memcpy(dest, &slot.buffer[startPos], firstPart * sizeof(float));
            std::memcpy(dest + firstPart, &slot.buffer[0], (toRead - firstPart) * sizeof(float));
        }

        slot.readIdx.store(r + toRead, std::memory_order_release);
        return toRead;
    }

private:
    InterPluginBus() = default;
    std::array<ChannelSlot, kMaxChannels> channels_{};
};

} // namespace dsp
