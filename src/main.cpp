#include "ffmpeg_decoder.h"
#include "audio_engine.h"
#include "portaudio_output.h"
#include "ui/main_window.h"

#include <QApplication>
#include <iostream>
#include <string>
#include <thread>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Music Player");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("musicplayer");

    if (argc > 2 && std::string(argv[1]) == "--play") {
        std::string filePath = argv[2];
        std::cout << "Playing: " << filePath << std::endl;

        musicplayer::AudioEngine engine;
        engine.setDecoder(std::make_unique<musicplayer::FfmpegDecoder>());
        engine.setOutput(std::make_unique<musicplayer::PortAudioOutput>());

        if (!engine.open(filePath)) {
            std::cerr << "Failed to open: " << filePath << std::endl;
            return 1;
        }

        auto info = engine.info();
        std::cout << "Codec: " << info.codecName << ", Format: " << info.formatName << ", "
                  << info.sampleRate << "Hz, " << info.channels << "ch, "
                  << static_cast<int>(info.duration) << "s" << std::endl;

        engine.play();
        std::cout << "Playing... Press Enter to stop." << std::endl;

        double lastPos = -1.0;
        while (engine.state() == musicplayer::AudioEngine::State::Playing) {
            double pos = engine.currentPosition();
            int posInt = static_cast<int>(pos);
            if (posInt != static_cast<int>(lastPos)) {
                int dur = static_cast<int>(info.duration);
                int pct = dur > 0 ? posInt * 100 / dur : 0;
                std::cout << "\r  [" << posInt / 60 << ":" << (posInt % 60 < 10 ? "0" : "")
                          << posInt % 60 << " / " << dur / 60 << ":"
                          << (dur % 60 < 10 ? "0" : "") << dur % 60 << "] " << pct << "%"
                          << std::flush;
                lastPos = pos;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        std::cout << std::endl;

        engine.stop();
        std::cout << "Stopped." << std::endl;
        return 0;
    }

    musicplayer::MainWindow window;
    window.show();
    return app.exec();
}
