#include "portaudio_output.h"

#include <cstring>
#include <memory>

extern "C" {
#include <portaudio.h>
}

namespace {

int paCallbackImpl(const void* /*input*/, void* output, unsigned long frameCount,
                   const PaStreamCallbackTimeInfo* /*timeInfo*/,
                   PaStreamCallbackFlags /*statusFlags*/, void* userData) {
    auto* self = static_cast<musicplayer::PortAudioOutput*>(userData);
    const auto& cb = self->callback();
    if (!cb) {
        auto* out = static_cast<float*>(output);
        auto totalSamples = static_cast<int>(frameCount) * self->channelCount();
        std::fill_n(out, totalSamples, 0.0f);
        return paContinue;
    }
    cb(static_cast<float*>(output), static_cast<int>(frameCount));
    return paContinue;
}

}  // namespace

namespace musicplayer {

struct PortAudioOutput::Context {
    PaStream* stream = nullptr;
};

PortAudioOutput::PortAudioOutput()
    : ctx_(std::make_unique<Context>()), initialized_(Pa_Initialize() == paNoError) {}

PortAudioOutput::~PortAudioOutput() {
    PortAudioOutput::stop();
    if (initialized_)
        Pa_Terminate();
}

bool PortAudioOutput::open(double sampleRate, int channels) {
    stop();

    sampleRate_ = sampleRate;
    channels_ = channels;

    PaStreamParameters params;
    std::memset(&params, 0, sizeof(params));
    params.device = Pa_GetDefaultOutputDevice();
    if (params.device == paNoDevice)
        return false;

    const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(params.device);
    params.channelCount = channels;
    params.sampleFormat = paFloat32;
    params.suggestedLatency = deviceInfo->defaultLowOutputLatency;
    params.hostApiSpecificStreamInfo = nullptr;

    PaError err = Pa_OpenStream(&ctx_->stream, nullptr, &params, sampleRate,
                                paFramesPerBufferUnspecified, paNoFlag, paCallbackImpl, this);
    if (err != paNoError) {
        ctx_->stream = nullptr;
        return false;
    }
    return true;
}

bool PortAudioOutput::start() {
    if (!ctx_->stream)
        return false;
    if (Pa_IsStreamActive(ctx_->stream) == 1)
        return true;
    return Pa_StartStream(ctx_->stream) == paNoError;
}

bool PortAudioOutput::pause() {
    if (!ctx_->stream)
        return true;
    if (Pa_IsStreamActive(ctx_->stream) != 1)
        return true;
    return Pa_StopStream(ctx_->stream) == paNoError;
}

bool PortAudioOutput::stop() {
    if (!ctx_->stream)
        return true;
    if (Pa_IsStreamActive(ctx_->stream) == 1)
        Pa_StopStream(ctx_->stream);
    Pa_CloseStream(ctx_->stream);
    ctx_->stream = nullptr;
    return true;
}

void PortAudioOutput::setCallback(DataCallback callback) {
    callback_ = std::move(callback);
}

}  // namespace musicplayer
