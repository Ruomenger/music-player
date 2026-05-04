#include "audio_engine.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace musicplayer {

static constexpr size_t kRingBufferCapacity = 1 << 17;  // 131072 samples ≈ 3s @ 44100Hz
static constexpr size_t kDecodeChunkSize = 4096;

AudioEngine::AudioEngine() : ringBuffer_(kRingBufferCapacity) {}

AudioEngine::~AudioEngine() {
    stop();
}

void AudioEngine::setDecoder(std::unique_ptr<IAudioDecoder> decoder) {
    decoder_ = std::move(decoder);
}

void AudioEngine::setOutput(std::unique_ptr<IAudioOutput> output) {
    output_ = std::move(output);
}

bool AudioEngine::open(const std::string& filePath) {
    if (!decoder_)
        return false;
    if (!output_)
        return false;

    stop();

    if (!decoder_->open(filePath))
        return false;
    info_ = decoder_->info();

    if (!output_->open(info_.sampleRate, info_.channels)) {
        decoder_->close();
        return false;
    }

    output_->setCallback([this](float* buffer, int frameCount) -> int {
        if (state_.load(std::memory_order_acquire) == State::Paused) {
            std::fill_n(buffer, frameCount * info_.channels, 0.0f);
            return frameCount;
        }
        int totalSamples = frameCount * info_.channels;
        int read = static_cast<int>(ringBuffer_.read(buffer, totalSamples));
        if (read < totalSamples) {
            std::fill_n(buffer + read, totalSamples - read, 0.0f);
        }
        position_.store(position_.load(std::memory_order_acquire) +
                            static_cast<double>(frameCount) / info_.sampleRate,
                        std::memory_order_release);
        return frameCount;
    });

    return true;
}

void AudioEngine::play() {
    State expected = State::Stopped;
    if (state_.compare_exchange_strong(expected, State::Playing)) {
        ringBuffer_.clear();
        position_.store(0.0, std::memory_order_release);

        if (!output_->start()) {
            state_.store(State::Stopped, std::memory_order_release);
            return;
        }

        decodeThread_ = std::thread([this] { decodeLoop(); });
        return;
    }

    expected = State::Paused;
    if (state_.compare_exchange_strong(expected, State::Playing)) {
        state_.store(State::Playing, std::memory_order_release);
        output_->start();
    }
}

void AudioEngine::pause() {
    State expected = State::Playing;
    if (state_.compare_exchange_strong(expected, State::Paused)) {
        output_->stop();
    }
}

void AudioEngine::stop() {
    state_.store(State::Stopped, std::memory_order_release);

    if (output_)
        output_->stop();

    if (decodeThread_.joinable()) {
        decodeThread_.join();
    }

    ringBuffer_.clear();
    position_.store(0.0, std::memory_order_release);

    if (decoder_)
        decoder_->close();
}

void AudioEngine::seek(double seconds) {
    State prev = state_.load(std::memory_order_acquire);
    if (prev == State::Stopped)
        return;

    output_->stop();
    if (decodeThread_.joinable()) {
        decodeThread_.join();
    }

    ringBuffer_.clear();
    decoder_->seek(seconds);
    position_.store(seconds, std::memory_order_release);

    if (prev == State::Playing) {
        output_->start();
        decodeThread_ = std::thread([this] { decodeLoop(); });
    }
}

void AudioEngine::decodeLoop() {
    while (state_.load(std::memory_order_acquire) == State::Playing) {
        size_t free = ringBuffer_.freeSlots();
        if (free < kDecodeChunkSize * static_cast<size_t>(info_.channels)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        size_t framesToDecode =
            std::min(free / static_cast<size_t>(info_.channels), kDecodeChunkSize);
        auto samples = decoder_->decode(framesToDecode);
        if (samples.empty()) {
            break;
        }
        ringBuffer_.write(samples.data(), samples.size());
    }
}

}  // namespace musicplayer
