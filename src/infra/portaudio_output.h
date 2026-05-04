#pragma once

#include "iaudio_output.h"

#include <memory>

namespace musicplayer {

class PortAudioOutput : public IAudioOutput {
public:
    PortAudioOutput();
    ~PortAudioOutput() override;

    PortAudioOutput(const PortAudioOutput&) = delete;
    PortAudioOutput& operator=(const PortAudioOutput&) = delete;

    bool open(double sampleRate, int channels) override;
    bool start() override;
    bool pause() override;
    bool stop() override;
    void setCallback(DataCallback callback) override;

    const DataCallback& callback() const { return callback_; }
    int channelCount() const { return channels_; }

private:
    struct Context;
    std::unique_ptr<Context> ctx_;
    DataCallback callback_;
    double sampleRate_ = 44100.0;
    int channels_ = 2;
    bool initialized_ = false;
};

}  // namespace musicplayer
