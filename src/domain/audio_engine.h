#pragma once

#include "audio_decoder_info.h"
#include "iaudio_decoder.h"
#include "iaudio_output.h"
#include "ring_buffer.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>

namespace musicplayer {

class AudioEngine {
public:
    enum class State : std::uint8_t { Stopped, Playing, Paused };

    AudioEngine();
    ~AudioEngine();

    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;
    AudioEngine(AudioEngine&&) = delete;
    AudioEngine& operator=(AudioEngine&&) = delete;

    void setDecoder(std::unique_ptr<IAudioDecoder> decoder);
    void setOutput(std::unique_ptr<IAudioOutput> output);

    bool open(const std::string& filePath);
    void play();
    void pause();
    void stop();
    void seek(double seconds);

    [[nodiscard]] State state() const { return state_.load(std::memory_order_acquire); }
    [[nodiscard]] double currentPosition() const;
    [[nodiscard]] double duration() const { return info_.duration; }
    [[nodiscard]] const AudioDecoderInfo& info() const { return info_; }

private:
    void decodeLoop();
    void startDecodeThread();
    void stopDecodeThread();

    std::unique_ptr<IAudioDecoder> decoder_;
    std::unique_ptr<IAudioOutput> output_;
    RingBuffer<float> ringBuffer_;
    AudioDecoderInfo info_;

    std::thread decodeThread_;
    std::atomic<State> state_{State::Stopped};
    std::atomic<bool> decodeRunning_{false};

    // Set by the decode thread when the decoder reports end-of-stream. The
    // audio callback consults this once the ring buffer drains: if the buffer
    // is empty and EOF is reached, it transitions Playing -> Stopped without
    // calling any blocking output API (PortAudio forbids that from a callback).
    std::atomic<bool> eofReached_{false};

    // Position tracking: callback only does fetch_add on framesPlayed_ (true atomic);
    // seek/start mutate both fields while the stream is paused (no race with callback).
    // Read side: position = (seekFrameOffset_ + framesPlayed_) / sampleRate.
    std::atomic<uint64_t> framesPlayed_{0};
    std::atomic<uint64_t> seekFrameOffset_{0};
};

}  // namespace musicplayer
