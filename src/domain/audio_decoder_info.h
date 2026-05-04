#pragma once

#include <cstdint>
#include <string>

namespace musicplayer {

struct AudioDecoderInfo {
    double duration = 0.0;
    int sampleRate = 44100;
    int channels = 2;
    std::string codecName;
    std::string formatName;
    int bitRate = 0;
};

}  // namespace musicplayer
