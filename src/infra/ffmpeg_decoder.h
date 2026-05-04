#pragma once

#include <string>
#include <vector>

namespace musicplayer {

struct AudioDecoderInfo {
    double duration = 0.0;
    int sampleRate = 44100;
    int channels = 2;
};

class FfmpegDecoder {
public:
    bool open(const std::string& filePath);
    std::vector<float> decode(size_t maxFrames);
    bool seek(double seconds);
    void close();
    AudioDecoderInfo info() const;

private:
    AudioDecoderInfo info_;
};

}  // namespace musicplayer
