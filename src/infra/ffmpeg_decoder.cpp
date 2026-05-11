#include "ffmpeg_decoder.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}

namespace musicplayer {

void FfmpegDecoder::Deleter::operator()(AVFormatContext* p) const {
    if (p)
        avformat_close_input(&p);
}
void FfmpegDecoder::Deleter::operator()(AVCodecContext* p) const {
    if (p)
        avcodec_free_context(&p);
}
void FfmpegDecoder::Deleter::operator()(AVFrame* p) const {
    if (p)
        av_frame_free(&p);
}
void FfmpegDecoder::Deleter::operator()(AVPacket* p) const {
    if (p)
        av_packet_free(&p);
}
void FfmpegDecoder::Deleter::operator()(SwrContext* p) const {
    if (p)
        swr_free(&p);
}

FfmpegDecoder::FfmpegDecoder() {
    packet_.reset(av_packet_alloc());
    frame_.reset(av_frame_alloc());
}

FfmpegDecoder::~FfmpegDecoder() {
    FfmpegDecoder::close();
}

bool FfmpegDecoder::open(const std::string& filePath) {
    close();

    AVFormatContext* fmtCtx = nullptr;
    if (avformat_open_input(&fmtCtx, filePath.c_str(), nullptr, nullptr) < 0) {
        return false;
    }
    formatCtx_.reset(fmtCtx);

    if (avformat_find_stream_info(formatCtx_.get(), nullptr) < 0) {
        return false;
    }

    if (!findAudioStream())
        return false;
    if (!openCodec())
        return false;
    if (!initResampler())
        return false;

    eof_ = false;

    const AVCodecContext* cc = codecCtx_.get();
    const AVFormatContext* fc = formatCtx_.get();
    const AVStream* stream = fc->streams[streamIndex_];

    info_.duration = stream->duration > 0
                         ? static_cast<double>(stream->duration) * av_q2d(stream->time_base)
                         : static_cast<double>(fc->duration) / AV_TIME_BASE;
    info_.sampleRate = targetSampleRate_ > 0 ? targetSampleRate_ : cc->sample_rate;
    info_.channels = cc->ch_layout.nb_channels;
    info_.codecName = avcodec_get_name(cc->codec_id);
    info_.formatName = fc->iformat->name;
    info_.bitRate = static_cast<int>(cc->bit_rate / 1000);

    return true;
}

bool FfmpegDecoder::findAudioStream() {
    AVFormatContext* fc = formatCtx_.get();
    for (unsigned i = 0; i < fc->nb_streams; ++i) {
        if (fc->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            streamIndex_ = static_cast<int>(i);
            return true;
        }
    }
    return false;
}

bool FfmpegDecoder::openCodec() {
    AVFormatContext* fc = formatCtx_.get();
    AVCodecParameters* codecPar = fc->streams[streamIndex_]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecPar->codec_id);
    if (!codec)
        return false;

    AVCodecContext* cc = avcodec_alloc_context3(codec);
    if (!cc)
        return false;
    codecCtx_.reset(cc);

    if (avcodec_parameters_to_context(cc, codecPar) < 0)
        return false;
    if (avcodec_open2(cc, codec, nullptr) < 0)
        return false;

    return true;
}

bool FfmpegDecoder::initResampler() {
    AVCodecContext* cc = codecCtx_.get();
    AVChannelLayout outLayout;
    av_channel_layout_default(&outLayout, cc->ch_layout.nb_channels);

    const int outRate = targetSampleRate_ > 0 ? targetSampleRate_ : cc->sample_rate;

    SwrContext* swr = nullptr;
    int ret = swr_alloc_set_opts2(&swr, &outLayout, AV_SAMPLE_FMT_FLT, outRate, &cc->ch_layout,
                                  cc->sample_fmt, cc->sample_rate, 0, nullptr);
    av_channel_layout_uninit(&outLayout);
    if (ret < 0 || !swr)
        return false;

    swrCtx_.reset(swr);
    return swr_init(swrCtx_.get()) >= 0;
}

void FfmpegDecoder::setTargetSampleRate(int rate) {
    targetSampleRate_ = rate;
}

std::vector<float> FfmpegDecoder::decode(size_t maxFrames) {
    if (eof_ || !codecCtx_)
        return {};

    AVCodecContext* cc = codecCtx_.get();
    const int channels = cc->ch_layout.nb_channels;

    std::vector<float> output(maxFrames * static_cast<size_t>(channels));
    size_t framesWritten = 0;
    bool flushed = false;

    auto writeOutput = [&](const uint8_t** srcData, int srcSamples) {
        if (framesWritten >= maxFrames)
            return;
        const int wanted = static_cast<int>(maxFrames - framesWritten);
        uint8_t* dst[1] = {reinterpret_cast<uint8_t*>(
            output.data() + framesWritten * static_cast<size_t>(channels))};
        int got = swr_convert(swrCtx_.get(), dst, wanted, srcData, srcSamples);
        if (got > 0)
            framesWritten += static_cast<size_t>(got);
    };

    // Drain any samples still buffered inside swr from the previous call
    // (caused by capping the output last time). Pass nullptr input to flush.
    while (framesWritten < maxFrames) {
        const size_t before = framesWritten;
        writeOutput(nullptr, 0);
        if (framesWritten == before)
            break;
    }

    while (framesWritten < maxFrames) {
        int ret = avcodec_receive_frame(cc, frame_.get());
        if (ret == 0) {
            auto* srcData = const_cast<const uint8_t**>(frame_->data);
            writeOutput(srcData, frame_->nb_samples);
        } else if (ret == AVERROR(EAGAIN)) {
            if (flushed)
                break;
            if (av_read_frame(formatCtx_.get(), packet_.get()) < 0) {
                avcodec_send_packet(cc, nullptr);
                flushed = true;
                continue;
            }
            if (packet_->stream_index == streamIndex_) {
                avcodec_send_packet(cc, packet_.get());
            }
            av_packet_unref(packet_.get());
        } else {
            // AVERROR_EOF or fatal: drain swr's remaining output then stop.
            while (framesWritten < maxFrames) {
                const size_t before = framesWritten;
                writeOutput(nullptr, 0);
                if (framesWritten == before)
                    break;
            }
            eof_ = true;
            break;
        }
    }

    output.resize(framesWritten * static_cast<size_t>(channels));
    return output;
}

bool FfmpegDecoder::seek(double seconds) {
    if (!formatCtx_)
        return false;

    AVFormatContext* fc = formatCtx_.get();
    AVStream* stream = fc->streams[streamIndex_];
    int64_t target = av_rescale_q(static_cast<int64_t>(seconds * AV_TIME_BASE), AV_TIME_BASE_Q,
                                  stream->time_base);

    if (av_seek_frame(fc, streamIndex_, target, AVSEEK_FLAG_BACKWARD) < 0) {
        return false;
    }

    avcodec_flush_buffers(codecCtx_.get());
    eof_ = false;
    return true;
}

void FfmpegDecoder::close() {
    swrCtx_.reset();
    codecCtx_.reset();
    formatCtx_.reset();
    streamIndex_ = -1;
    eof_ = false;
    info_ = AudioDecoderInfo{};
}

AudioDecoderInfo FfmpegDecoder::info() const {
    return info_;
}

}  // namespace musicplayer
