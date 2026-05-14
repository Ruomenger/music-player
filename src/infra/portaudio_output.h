#pragma once

#include "iaudio_output.h"

#include <memory>
#include <string>
#include <vector>

namespace musicplayer {

struct AudioOutputDeviceInfo {
    std::string name;
    bool isDefault = false;
};

class PortAudioOutput final : public IAudioOutput {
public:
    PortAudioOutput();
    ~PortAudioOutput() override;

    PortAudioOutput(const PortAudioOutput&) = delete;
    PortAudioOutput& operator=(const PortAudioOutput&) = delete;
    PortAudioOutput(PortAudioOutput&&) = delete;
    PortAudioOutput& operator=(PortAudioOutput&&) = delete;

    bool open(double sampleRate, int channels) override;
    bool start() override;
    bool pause() override;
    bool stop() override;
    void setCallback(DataCallback callback) override;
    [[nodiscard]] double defaultSampleRate() const override;

    [[nodiscard]] const DataCallback& callback() const { return callback_; }
    [[nodiscard]] int channelCount() const { return channels_; }

    // Enumerate every device that can output audio (positive maxOutputChannels).
    // The default device is marked isDefault=true. Pa_Initialize is invoked
    // and balanced internally so callers don't need a live PortAudioOutput.
    [[nodiscard]] static std::vector<AudioOutputDeviceInfo> enumerateOutputDevices();

    // Pin a specific output device by name. Empty string (the default) means
    // "use Pa_GetDefaultOutputDevice() at open() time". Match is exact on
    // PaDeviceInfo::name; on mismatch we fall back to the default with a
    // qWarning rather than refusing to open.
    void setPreferredDevice(std::string deviceName);
    [[nodiscard]] const std::string& preferredDevice() const { return preferredDevice_; }

private:
    struct Context;
    std::unique_ptr<Context> ctx_;
    DataCallback callback_;
    double sampleRate_ = 44100.0;
    int channels_ = 2;
    bool initialized_ = false;
    std::string preferredDevice_;
};

}  // namespace musicplayer
