#include "audio_engine.h"
#include "ffmpeg_decoder.h"
#include "portaudio_output.h"
#include "ui/main_window.h"

#include <QApplication>
#include <iostream>
#include <string>
#include <thread>

namespace {

int runCli(const std::string& filePath) {
    std::cout << "Playing: " << filePath << '\n';

    musicplayer::AudioEngine engine;
    engine.setDecoder(std::make_unique<musicplayer::FfmpegDecoder>());
    engine.setOutput(std::make_unique<musicplayer::PortAudioOutput>());

    if (!engine.open(filePath)) {
        std::cerr << "Failed to open: " << filePath << '\n';
        return 1;
    }

    auto info = engine.info();
    std::cout << "Codec: " << info.codecName << ", Format: " << info.formatName << ", "
              << info.sampleRate << "Hz, " << info.channels << "ch, "
              << static_cast<int>(info.duration) << "s\n";

    engine.play();
    std::cout << "Playing...\n";

    double lastPos = -1.0;
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        if (engine.state() != musicplayer::AudioEngine::State::Playing)
            break;
        double pos = engine.currentPosition();
        int posSec = static_cast<int>(pos);
        if (posSec != static_cast<int>(lastPos)) {
            int durSec = static_cast<int>(info.duration);
            std::cout << "\r  pos=" << pos << " sec [" << posSec / 60 << ":"
                      << (posSec % 60 < 10 ? "0" : "") << posSec % 60 << " / " << durSec / 60 << ":"
                      << (durSec % 60 < 10 ? "0" : "") << durSec % 60 << "] "
                      << (durSec > 0 ? posSec * 100 / durSec : 0) << "%" << std::flush;
            lastPos = pos;
        }
    }
    std::cout << "\nDone.\n";
    std::cout << "Stopped.\n";
    return 0;
}

}  // namespace

int main(int argc, char* argv[]) {
    try {
        QApplication app(argc, argv);
        QApplication::setApplicationName("Music Player");
        QApplication::setApplicationVersion("0.1.0");
        QApplication::setOrganizationName("musicplayer");

        if (argc > 2 && std::string(argv[1]) == "--play") {
            return runCli(argv[2]);
        }

        musicplayer::MainWindow window;
        window.show();
        return QApplication::exec();
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << '\n';
        return 1;
    } catch (...) {
        std::cerr << "Fatal: unknown exception\n";
        return 1;
    }
}
