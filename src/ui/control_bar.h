#pragma once

#include <QWidget>

#include "song_info.h"

class QLabel;
class QPushButton;
class QSlider;

namespace musicplayer {

// Transport controls: prev / play-pause / stop / next / mode buttons +
// progress slider + volume slider + time labels. The widget is decoupled
// from PlayerController — it only emits *Requested signals and consumes
// set*() slots, so MainWindow can wire it both ways.
class ControlBar : public QWidget {
    Q_OBJECT
public:
    explicit ControlBar(QWidget* parent = nullptr);

public slots:
    // Reflect external state in the UI without re-emitting *Requested signals.
    // The internal setters block signal emission for the duration of the
    // assignment to keep the loop closed.
    void setPlayingState(bool playing);
    void setPosition(double currentSec, double totalSec);
    void setVolume(double volume);  // 0.0 - 1.0
    void setPlayMode(PlayMode mode);

signals:
    void playRequested();  // resume or start playback (context-dependent)
    void pauseRequested();
    void stopRequested();
    void previousRequested();
    void nextRequested();
    void seekRequested(double seconds);
    void volumeChangeRequested(double volume);
    void playModeChangeRequested(PlayMode mode);

private slots:
    void onPlayPauseClicked();
    void onProgressSliderReleased();
    void onVolumeSliderMoved(int value);
    void onModeClicked();

private:
    void updatePlayPauseIcon();
    void updateModeButtonLabel();

    // State first — child widget initializers read `mode_` to set their label.
    bool playing_ = false;
    double duration_ = 0.0;
    PlayMode mode_ = PlayMode::Sequential;

    QPushButton* prevBtn_ = nullptr;
    QPushButton* playPauseBtn_ = nullptr;
    QPushButton* stopBtn_ = nullptr;
    QPushButton* nextBtn_ = nullptr;
    QPushButton* modeBtn_ = nullptr;
    QSlider* progressSlider_ = nullptr;
    QSlider* volumeSlider_ = nullptr;
    QLabel* currentLabel_ = nullptr;
    QLabel* totalLabel_ = nullptr;
};

}  // namespace musicplayer
