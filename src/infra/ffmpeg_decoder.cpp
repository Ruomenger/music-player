#include "ffmpeg_decoder.h"

namespace musicplayer {

bool FfmpegDecoder::open(const std::string& /*filePath*/) {
    return false;
}
std::vector<float> FfmpegDecoder::decode(size_t /*maxFrames*/) {
    return {};
}
bool FfmpegDecoder::seek(double /*seconds*/) {
    return false;
}
void FfmpegDecoder::close() {}
AudioDecoderInfo FfmpegDecoder::info() const {
    return info_;
}

}  // namespace musicplayer
