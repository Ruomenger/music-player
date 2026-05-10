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

class FfmpegDecoder final : public IAudioDecoder {
public:
    FfmpegDecoder();
    ~FfmpegDecoder() override;

    FfmpegDecoder(const FfmpegDecoder&) = delete;
    FfmpegDecoder& operator=(const FfmpegDecoder&) = delete;
    FfmpegDecoder(FfmpegDecoder&&) = delete;
    FfmpegDecoder& operator=(FfmpegDecoder&&) = delete;

    bool open(const std::string& filePath) override;
    std::vector<float> decode(size_t maxFrames) override;
    bool seek(double seconds) override;
    void close() override;
    [[nodiscard]] AudioDecoderInfo info() const override;

    // Configure the resampler's output sample rate. Pass 0 to disable rate
    // conversion (output rate == file's native rate, the default). Must be
    // called before open(); calling after open() has no effect on the active
    // resampler.
    void setTargetSampleRate(int rate);

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
    int targetSampleRate_ = 0;  // 0 = use file's native rate
    bool eof_ = false;
};

}  // namespace musicplayer
