#pragma once

#include "audio_decoder_info.h"

#include <cstddef>
#include <string>
#include <vector>

namespace musicplayer {

class IAudioDecoder {
public:
    IAudioDecoder() = default;
    virtual ~IAudioDecoder() = default;
    IAudioDecoder(const IAudioDecoder&) = delete;
    IAudioDecoder& operator=(const IAudioDecoder&) = delete;
    IAudioDecoder(IAudioDecoder&&) = delete;
    IAudioDecoder& operator=(IAudioDecoder&&) = delete;

    virtual bool open(const std::string& filePath) = 0;
    virtual std::vector<float> decode(size_t maxFrames) = 0;
    virtual bool seek(double seconds) = 0;
    virtual void close() = 0;
    [[nodiscard]] virtual AudioDecoderInfo info() const = 0;

    // Configure the desired output sample rate before open(). Pass 0 to use
    // the file's native rate (the default). After open(), info().sampleRate
    // reports the post-resampling rate, which is what callers should use to
    // configure the audio output.
    virtual void setTargetSampleRate(int rate) = 0;
};

}  // namespace musicplayer
