#pragma once

#include <functional>

namespace musicplayer {

class IAudioOutput {
public:
    using DataCallback = std::function<void(float* buffer, int frameCount)>;

    virtual ~IAudioOutput() = default;

    // open: allocate native stream resources (does NOT begin pulling samples).
    virtual bool open(double sampleRate, int channels) = 0;

    // start: begin pulling samples via the data callback.
    // Valid from "opened-but-not-running" or "paused".
    virtual bool start() = 0;

    // pause: stop pulling samples but keep the stream open. resume() with start().
    virtual bool pause() = 0;

    // stop: stop pulling, release native stream. Re-open required to play again.
    virtual bool stop() = 0;

    virtual void setCallback(DataCallback callback) = 0;
};

}  // namespace musicplayer
