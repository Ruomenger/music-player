#pragma once

#include "audio_decoder_info.h"
#include "iaudio_decoder.h"
#include "iaudio_output.h"
#include "ring_buffer.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <thread>

namespace musicplayer {

class AudioEngine {
public:
    enum class State { Stopped, Playing, Paused };

    AudioEngine();
    ~AudioEngine();

    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;

    void setDecoder(std::unique_ptr<IAudioDecoder> decoder);
    void setOutput(std::unique_ptr<IAudioOutput> output);

    bool open(const std::string& filePath);
    void play();
    void pause();
    void stop();
    void seek(double seconds);

    State state() const { return state_.load(std::memory_order_acquire); }
    double currentPosition() const { return position_.load(std::memory_order_acquire); }
    double duration() const { return info_.duration; }
    const AudioDecoderInfo& info() const { return info_; }

private:
    void decodeLoop();
    static constexpr double positionPollInterval = 0.25;

    std::unique_ptr<IAudioDecoder> decoder_;
    std::unique_ptr<IAudioOutput> output_;
    RingBuffer<float> ringBuffer_;
    AudioDecoderInfo info_;

    std::thread decodeThread_;
    std::atomic<State> state_{State::Stopped};
    std::atomic<double> position_{0.0};
};

}  // namespace musicplayer
