#pragma once

#include <functional>

namespace musicplayer {

class IAudioOutput {
public:
    using DataCallback = std::function<void(float* buffer, int frameCount)>;

    IAudioOutput() = default;
    virtual ~IAudioOutput() = default;
    IAudioOutput(const IAudioOutput&) = delete;
    IAudioOutput& operator=(const IAudioOutput&) = delete;
    IAudioOutput(IAudioOutput&&) = delete;
    IAudioOutput& operator=(IAudioOutput&&) = delete;

    // open: allocate native stream resources (does NOT begin pulling samples).
    virtual bool open(double sampleRate, int channels) = 0;

    // defaultSampleRate: the audio device's preferred output rate (Hz). 0 if
    // no device is available. Callers can use this to drive decoder-side
    // resampling so the output never has to negotiate an unsupported rate.
    [[nodiscard]] virtual double defaultSampleRate() const = 0;

    // start: begin pulling samples via the data callback.
    // Valid from "opened-but-not-running" or "paused".
    virtual bool start() = 0;

    // pause: stop pulling samples but keep the stream open. resume() with start().
    virtual bool pause() = 0;

    // stop: stop pulling, release native stream. Re-open required to play again.
    virtual bool stop() = 0;

    // Set the data callback. MUST NOT be called while the stream is running:
    // implementations are not required to provide an atomic swap, and the
    // realtime audio thread reads the callback without synchronization.
    // Safe call sites: before start(), or after a stop()/pause() that has
    // actually halted the stream.
    virtual void setCallback(DataCallback callback) = 0;
};

}  // namespace musicplayer
