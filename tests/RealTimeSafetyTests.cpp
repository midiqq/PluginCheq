#include <catch2/catch_test_macros.hpp>

#include "dsp/RingBuffer.h"
#include "dsp/TripleBuffer.h"

#include <atomic>
#include <numeric>
#include <thread>
#include <vector>

TEST_CASE("RingBuffer: SPSC concurrent push/pop preserves exact sample stream", "[rt][ringbuffer]") {
    constexpr std::size_t totalSamples = 200000;
    dsp::RingBuffer<double> rb(8192);

    std::vector<double> received;
    received.reserve(totalSamples);

    std::atomic<bool> done{false};

    std::thread producer([&]() {
        std::size_t sent = 0;
        std::vector<double> chunk(128);
        while (sent < totalSamples) {
            const std::size_t n = std::min<std::size_t>(chunk.size(), totalSamples - sent);
            for (std::size_t i = 0; i < n; ++i) {
                chunk[i] = static_cast<double>(sent + i);
            }
            const std::size_t pushed = rb.push(chunk.data(), n);
            sent += pushed;
            if (pushed == 0) {
                std::this_thread::yield();
            }
        }
        done.store(true, std::memory_order_release);
    });

    std::thread consumer([&]() {
        std::vector<double> chunk(256);
        while (received.size() < totalSamples) {
            const std::size_t popped = rb.pop(chunk.data(), chunk.size());
            if (popped > 0) {
                received.insert(received.end(), chunk.begin(), chunk.begin() + static_cast<std::ptrdiff_t>(popped));
            } else if (!done.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
        }
    });

    producer.join();
    consumer.join();

    REQUIRE(received.size() == totalSamples);
    for (std::size_t i = 0; i < totalSamples; ++i) {
        REQUIRE(received[i] == static_cast<double>(i));
    }
}

TEST_CASE("TripleBuffer: Always delivers monotonically newer frames without tearing", "[rt][triplebuffer]") {
    struct Frame {
        std::uint64_t seq = 0;
        std::uint64_t check = 0;
    };

    dsp::TripleBuffer<Frame> tb;
    std::atomic<bool> stop{false};

    std::thread writer([&]() {
        for (std::uint64_t i = 1; i <= 50000; ++i) {
            auto& buf = tb.getWriteBuffer();
            buf.seq = i;
            buf.check = i * 7 + 13;
            tb.publish();
        }
        stop.store(true, std::memory_order_release);
    });

    std::uint64_t lastSeq = 0;
    while (!stop.load(std::memory_order_acquire)) {
        if (tb.fetchLatest()) {
            const auto& f = tb.getReadBuffer();
            REQUIRE(f.check == f.seq * 7 + 13);
            REQUIRE(f.seq > lastSeq);
            lastSeq = f.seq;
        }
    }

    writer.join();
    if (tb.fetchLatest()) {
        const auto& f = tb.getReadBuffer();
        REQUIRE(f.check == f.seq * 7 + 13);
        REQUIRE(f.seq >= lastSeq);
    }
}
