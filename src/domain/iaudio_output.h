#pragma once

#include <functional>

namespace musicplayer {

class IAudioOutput {
public:
    using DataCallback = std::function<int(float* buffer, int frameCount)>;

    virtual ~IAudioOutput() = default;

    virtual bool open(double sampleRate, int channels) = 0;
    virtual bool start() = 0;
    virtual bool stop() = 0;
    virtual void setCallback(DataCallback callback) = 0;
};

}  // namespace musicplayer
