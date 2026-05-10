#include "audio_engine.h"

#include <algorithm>

namespace musicplayer {

// Capacity is in samples (interleaved). 131072 samples = 65536 frames ≈ 1.5s @ 44.1 kHz stereo.
static constexpr size_t kRingBufferCapacity = 1 << 17;
static constexpr size_t kDecodeChunkFrames = 4096;

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

double AudioEngine::currentPosition() const {
    if (info_.sampleRate <= 0)
        return 0.0;
    const uint64_t total = seekFrameOffset_.load(std::memory_order_acquire) +
                           framesPlayed_.load(std::memory_order_acquire);
    return static_cast<double>(total) / static_cast<double>(info_.sampleRate);
}

bool AudioEngine::open(const std::string& filePath) {
    if (!decoder_ || !output_)
        return false;

    stop();

    if (!decoder_->open(filePath))
        return false;
    info_ = decoder_->info();

    if (!output_->open(info_.sampleRate, info_.channels)) {
        decoder_->close();
        return false;
    }

    output_->setCallback([this](float* buffer, int frameCount) {
        const int channels = info_.channels;
        const int totalSamples = frameCount * channels;

        if (state_.load(std::memory_order_acquire) != State::Playing) {
            std::fill_n(buffer, totalSamples, 0.0f);
            return;
        }

        const int read =
            static_cast<int>(ringBuffer_.read(buffer, static_cast<size_t>(totalSamples)));
        if (read < totalSamples) {
            std::fill_n(buffer + read, totalSamples - read, 0.0f);
        }
        // Advance position by wallclock frames, not just by frames consumed.
        // Underruns (zero-fill) still elapse on the audio device, so position should advance.
        framesPlayed_.fetch_add(static_cast<uint64_t>(frameCount), std::memory_order_acq_rel);
    });

    return true;
}

void AudioEngine::play() {
    State expected = State::Stopped;
    if (state_.compare_exchange_strong(expected, State::Playing)) {
        ringBuffer_.clear();
        framesPlayed_.store(0, std::memory_order_release);
        seekFrameOffset_.store(0, std::memory_order_release);

        startDecodeThread();
        if (!output_->start()) {
            stopDecodeThread();
            state_.store(State::Stopped, std::memory_order_release);
        }
        return;
    }

    expected = State::Paused;
    if (state_.compare_exchange_strong(expected, State::Playing)) {
        // Decode thread keeps running across pause; just resume the audio stream.
        output_->start();
    }
}

void AudioEngine::pause() {
    State expected = State::Playing;
    if (state_.compare_exchange_strong(expected, State::Paused)) {
        // Pause keeps the stream open and the decode thread alive (ring buffer holds samples).
        output_->pause();
    }
}

void AudioEngine::stop() {
    state_.store(State::Stopped, std::memory_order_release);

    if (output_)
        output_->stop();

    stopDecodeThread();

    ringBuffer_.clear();
    framesPlayed_.store(0, std::memory_order_release);
    seekFrameOffset_.store(0, std::memory_order_release);

    if (decoder_)
        decoder_->close();
}

void AudioEngine::seek(double seconds) {
    const State prev = state_.load(std::memory_order_acquire);
    if (prev == State::Stopped)
        return;

    // Bring the audio stream and decode thread to a quiescent state so it's safe to
    // mutate ring buffer, decoder position, and frame counters without races.
    output_->pause();
    stopDecodeThread();

    ringBuffer_.clear();
    decoder_->seek(seconds);

    const auto offsetFrames = static_cast<uint64_t>(std::max(0.0, seconds) * info_.sampleRate);
    seekFrameOffset_.store(offsetFrames, std::memory_order_release);
    framesPlayed_.store(0, std::memory_order_release);

    if (prev == State::Playing) {
        startDecodeThread();
        output_->start();
    }
    // If we were Paused, leave the stream paused; the next play() will resume it.
}

void AudioEngine::startDecodeThread() {
    stopDecodeThread();
    decodeRunning_.store(true, std::memory_order_release);
    decodeThread_ = std::thread([this] { decodeLoop(); });
}

void AudioEngine::stopDecodeThread() {
    decodeRunning_.store(false, std::memory_order_release);
    if (decodeThread_.joinable())
        decodeThread_.join();
}

void AudioEngine::decodeLoop() {
    std::vector<float> pending;
    while (decodeRunning_.load(std::memory_order_acquire)) {
        // Drain any leftover samples from a previous truncated write (defensive — current
        // FfmpegDecoder caps decode() output strictly, so this should not trigger).
        if (!pending.empty()) {
            const size_t written = ringBuffer_.write(pending.data(), pending.size());
            if (written < pending.size()) {
                pending.erase(pending.begin(), pending.begin() + static_cast<ptrdiff_t>(written));
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            pending.clear();
        }

        const size_t free = ringBuffer_.freeSlots();
        if (free < kDecodeChunkFrames * static_cast<size_t>(info_.channels)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        const size_t framesToDecode =
            std::min(free / static_cast<size_t>(info_.channels), kDecodeChunkFrames);
        auto samples = decoder_->decode(framesToDecode);
        if (samples.empty())
            break;

        const size_t written = ringBuffer_.write(samples.data(), samples.size());
        if (written < samples.size())
            pending.assign(samples.begin() + static_cast<ptrdiff_t>(written), samples.end());
    }
}

}  // namespace musicplayer
