#pragma once

#include "audio_decoder_info.h"

#include <cstddef>
#include <string>
#include <vector>

namespace musicplayer {

class IAudioDecoder {
public:
    virtual ~IAudioDecoder() = default;

    virtual bool open(const std::string& filePath) = 0;
    virtual std::vector<float> decode(size_t maxFrames) = 0;
    virtual bool seek(double seconds) = 0;
    virtual void close() = 0;
    virtual AudioDecoderInfo info() const = 0;
};

}  // namespace musicplayer
