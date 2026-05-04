#pragma once

#include "iaudio_decoder.h"

#include <memory>
#include <string>
#include <vector>

struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct SwrContext;

namespace musicplayer {

class FfmpegDecoder : public IAudioDecoder {
public:
    FfmpegDecoder();
    ~FfmpegDecoder() override;

    FfmpegDecoder(const FfmpegDecoder&) = delete;
    FfmpegDecoder& operator=(const FfmpegDecoder&) = delete;

    bool open(const std::string& filePath) override;
    std::vector<float> decode(size_t maxFrames) override;
    bool seek(double seconds) override;
    void close() override;
    AudioDecoderInfo info() const override;

private:
    bool findAudioStream();
    bool openCodec();
    bool initResampler();

    struct Deleter {
        void operator()(AVFormatContext* p) const;
        void operator()(AVCodecContext* p) const;
        void operator()(AVFrame* p) const;
        void operator()(AVPacket* p) const;
        void operator()(SwrContext* p) const;
    };

    template<typename T>
    using UniquePtr = std::unique_ptr<T, Deleter>;

    UniquePtr<AVFormatContext> formatCtx_;
    UniquePtr<AVCodecContext> codecCtx_;
    UniquePtr<AVFrame> frame_;
    UniquePtr<AVPacket> packet_;
    UniquePtr<SwrContext> swrCtx_;

    AudioDecoderInfo info_;
    int streamIndex_ = -1;
    bool eof_ = false;
};

}  // namespace musicplayer
