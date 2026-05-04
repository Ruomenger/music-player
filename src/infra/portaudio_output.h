#pragma once

#include <functional>
#include <memory>

namespace musicplayer {

class PortAudioOutput {
public:
    using DataCallback = std::function<int(float*, int)>;

    PortAudioOutput();
    ~PortAudioOutput();

    bool init(double sampleRate, int channels);
    bool start();
    bool stop();
    bool pause();
    void setDataCallback(DataCallback callback);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace musicplayer
