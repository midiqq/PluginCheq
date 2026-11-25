#pragma once

#include <array>
#include <atomic>
#include <cstdint>

namespace dsp {

/**
 * Lock-free Triple Buffer for passing completed analysis frames
 * from the Analysis Worker Thread to the GUI Thread without blocking.
 *
 * State encoding in `sharedState_`:
 *   bits [0..1]: index of the latest ready buffer (0, 1, 2)
 *   bit  [2]   : dirty flag (1 = new frame available since last consumer read)
 */
template <typename T>
class TripleBuffer {
public:
    TripleBuffer() = default;

    explicit TripleBuffer(const T& initialValue) {
        for (auto& buf : buffers_) {
            buf = initialValue;
        }
    }

    /**
     * Producer: returns a mutable reference to the current back buffer.
     */
    T& getWriteBuffer() noexcept {
        return buffers_[writeIdx_];
    }

    /**
     * Producer: publishes the current back buffer and swaps it with the shared slot.
     */
    void publish() noexcept {
        std::uint8_t prev = sharedState_.load(std::memory_order_relaxed);
        std::uint8_t desired = static_cast<std::uint8_t>((writeIdx_ & 0x03u) | 0x04u);

        while (!sharedState_.compare_exchange_weak(
            prev, desired, std::memory_order_acq_rel, std::memory_order_relaxed)) {
        }

        writeIdx_ = prev & 0x03u;
    }

    /**
     * Consumer: checks if a new frame was published; if so, swaps it into the read slot
     * and returns true. Always safe to read getReadBuffer() afterwards.
     */
    bool fetchLatest() noexcept {
        std::uint8_t current = sharedState_.load(std::memory_order_acquire);
        if ((current & 0x04u) == 0) {
            return false;
        }

        std::uint8_t desired = static_cast<std::uint8_t>(readIdx_ & 0x03u); // dirty=0
        while (!sharedState_.compare_exchange_weak(
            current, desired, std::memory_order_acq_rel, std::memory_order_relaxed)) {
            if ((current & 0x04u) == 0) {
                return false;
            }
        }

        readIdx_ = current & 0x03u;
        return true;
    }

    [[nodiscard]] const T& getReadBuffer() const noexcept {
        return buffers_[readIdx_];
    }

private:
    std::array<T, 3> buffers_{};
    std::uint8_t writeIdx_ = 0;
    std::uint8_t readIdx_ = 2;
    std::atomic<std::uint8_t> sharedState_{1}; // middle slot initially
};

} // namespace dsp
