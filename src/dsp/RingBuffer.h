#pragma once

#include <atomic>
#include <cstddef>
#include <cstring>
#include <vector>

namespace dsp {

/**
 * Lock-free Single-Producer Single-Consumer (SPSC) Ring Buffer.
 * Designed for real-time audio callback -> analysis worker thread transfer.
 * No allocations, no locks, no exceptions on push/pop.
 */
template <typename T>
class RingBuffer {
public:
    explicit RingBuffer(std::size_t capacityPowerOfTwo = 65536) {
        resize(capacityPowerOfTwo);
    }

    void resize(std::size_t requestedCapacity) {
        std::size_t cap = 1;
        while (cap < requestedCapacity) {
            cap <<= 1;
        }
        buffer_.assign(cap, T{});
        mask_ = cap - 1;
        writeIndex_.store(0, std::memory_order_relaxed);
        readIndex_.store(0, std::memory_order_relaxed);
    }

    void reset() noexcept {
        writeIndex_.store(0, std::memory_order_relaxed);
        readIndex_.store(0, std::memory_order_relaxed);
    }

    [[nodiscard]] std::size_t capacity() const noexcept {
        return buffer_.size();
    }

    [[nodiscard]] std::size_t availableToRead() const noexcept {
        const auto w = writeIndex_.load(std::memory_order_acquire);
        const auto r = readIndex_.load(std::memory_order_relaxed);
        return w - r;
    }

    [[nodiscard]] std::size_t availableToWrite() const noexcept {
        const auto w = writeIndex_.load(std::memory_order_relaxed);
        const auto r = readIndex_.load(std::memory_order_acquire);
        return buffer_.size() - (w - r);
    }

    /**
     * Push up to `count` elements. Returns actual number of elements pushed.
     * Safe to call from real-time audio thread.
     */
    std::size_t push(const T* data, std::size_t count) noexcept {
        const auto w = writeIndex_.load(std::memory_order_relaxed);
        const auto r = readIndex_.load(std::memory_order_acquire);
        const std::size_t freeSpace = buffer_.size() - (w - r);
        const std::size_t toWrite = (count < freeSpace) ? count : freeSpace;

        if (toWrite == 0) {
            return 0;
        }

        const std::size_t startPos = w & mask_;
        const std::size_t firstPart = buffer_.size() - startPos;
        if (toWrite <= firstPart) {
            std::memcpy(&buffer_[startPos], data, toWrite * sizeof(T));
        } else {
            std::memcpy(&buffer_[startPos], data, firstPart * sizeof(T));
            std::memcpy(&buffer_[0], data + firstPart, (toWrite - firstPart) * sizeof(T));
        }

        writeIndex_.store(w + toWrite, std::memory_order_release);
        return toWrite;
    }

    /**
     * Pop up to `count` elements. Returns actual number of elements popped.
     * Safe to call from consumer thread.
     */
    std::size_t pop(T* dest, std::size_t count) noexcept {
        const auto w = writeIndex_.load(std::memory_order_acquire);
        const auto r = readIndex_.load(std::memory_order_relaxed);
        const std::size_t available = w - r;
        const std::size_t toRead = (count < available) ? count : available;

        if (toRead == 0) {
            return 0;
        }

        const std::size_t startPos = r & mask_;
        const std::size_t firstPart = buffer_.size() - startPos;
        if (toRead <= firstPart) {
            std::memcpy(dest, &buffer_[startPos], toRead * sizeof(T));
        } else {
            std::memcpy(dest, &buffer_[startPos], firstPart * sizeof(T));
            std::memcpy(dest + firstPart, &buffer_[0], (toRead - firstPart) * sizeof(T));
        }

        readIndex_.store(r + toRead, std::memory_order_release);
        return toRead;
    }

    /**
     * Discard up to `count` elements from the read end.
     */
    std::size_t discard(std::size_t count) noexcept {
        const auto w = writeIndex_.load(std::memory_order_acquire);
        const auto r = readIndex_.load(std::memory_order_relaxed);
        const std::size_t available = w - r;
        const std::size_t toDiscard = (count < available) ? count : available;
        readIndex_.store(r + toDiscard, std::memory_order_release);
        return toDiscard;
    }

private:
    std::vector<T> buffer_;
    std::size_t mask_ = 0;

    alignas(64) std::atomic<std::size_t> writeIndex_{0};
    alignas(64) std::atomic<std::size_t> readIndex_{0};
};

} // namespace dsp
