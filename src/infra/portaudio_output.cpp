#include "portaudio_output.h"

namespace musicplayer {

struct PortAudioOutput::Impl { };

PortAudioOutput::PortAudioOutput() : impl_(std::make_unique<Impl>()) {}
PortAudioOutput::~PortAudioOutput() = default;

bool PortAudioOutput::init(double /*sampleRate*/, int /*channels*/) { return false; }
bool PortAudioOutput::start() { return false; }
bool PortAudioOutput::stop() { return false; }
bool PortAudioOutput::pause() { return false; }
void PortAudioOutput::setDataCallback(DataCallback /*callback*/) {}

} // namespace musicplayer
