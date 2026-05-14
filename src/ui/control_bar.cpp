#include "control_bar.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSlider>
#include <QString>

namespace musicplayer {

namespace {

constexpr int kProgressResolution = 1000;  // slider integer steps per track
constexpr int kVolumeResolution = 100;

QString formatTime(double seconds) {
    if (seconds <= 0.0)
        return QStringLiteral("0:00");
    const auto total = static_cast<int>(seconds);
    return QString::asprintf("%d:%02d", total / 60, total % 60);
}

QString modeLabel(PlayMode mode) {
    switch (mode) {
        case PlayMode::Single:
            return QObject::tr("Single");
        case PlayMode::Sequential:
            return QObject::tr("Sequential");
        case PlayMode::ListLoop:
            return QObject::tr("Loop");
        case PlayMode::Random:
            return QObject::tr("Shuffle");
    }
    return {};
}

PlayMode nextMode(PlayMode mode) {
    // Sequential → ListLoop → Random → Single → Sequential. Sequential is
    // first so the default startup state lines up with the cycle's "0".
    switch (mode) {
        case PlayMode::Sequential:
            return PlayMode::ListLoop;
        case PlayMode::ListLoop:
            return PlayMode::Random;
        case PlayMode::Random:
            return PlayMode::Single;
        case PlayMode::Single:
            return PlayMode::Sequential;
    }
    return PlayMode::Sequential;
}

}  // namespace

ControlBar::ControlBar(QWidget* parent)
    : QWidget(parent)
    , prevBtn_(new QPushButton(QStringLiteral("⏮"), this))
    , playPauseBtn_(new QPushButton(QStringLiteral("▶"), this))
    , stopBtn_(new QPushButton(QStringLiteral("⏹"), this))
    , nextBtn_(new QPushButton(QStringLiteral("⏭"), this))
    , modeBtn_(new QPushButton(modeLabel(mode_), this))
    , progressSlider_(new QSlider(Qt::Horizontal, this))
    , volumeSlider_(new QSlider(Qt::Horizontal, this))
    , currentLabel_(new QLabel(formatTime(0.0), this))
    , totalLabel_(new QLabel(formatTime(0.0), this)) {
    progressSlider_->setRange(0, kProgressResolution);
    progressSlider_->setEnabled(false);  // re-enabled once a duration is known

    volumeSlider_->setRange(0, kVolumeResolution);
    volumeSlider_->setValue(kVolumeResolution);
    volumeSlider_->setMaximumWidth(120);
    volumeSlider_->setToolTip(tr("Volume"));

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->addWidget(prevBtn_);
    layout->addWidget(playPauseBtn_);
    layout->addWidget(stopBtn_);
    layout->addWidget(nextBtn_);
    layout->addWidget(currentLabel_);
    layout->addWidget(progressSlider_, /*stretch=*/1);
    layout->addWidget(totalLabel_);
    layout->addWidget(modeBtn_);
    layout->addWidget(volumeSlider_);

    connect(prevBtn_, &QPushButton::clicked, this, &ControlBar::previousRequested);
    connect(nextBtn_, &QPushButton::clicked, this, &ControlBar::nextRequested);
    connect(stopBtn_, &QPushButton::clicked, this, &ControlBar::stopRequested);
    connect(playPauseBtn_, &QPushButton::clicked, this, &ControlBar::onPlayPauseClicked);
    connect(modeBtn_, &QPushButton::clicked, this, &ControlBar::onModeClicked);
    connect(progressSlider_, &QSlider::sliderReleased, this, &ControlBar::onProgressSliderReleased);
    connect(volumeSlider_, &QSlider::sliderMoved, this, &ControlBar::onVolumeSliderMoved);
}

void ControlBar::setPlayingState(bool playing) {
    playing_ = playing;
    updatePlayPauseIcon();
}

void ControlBar::setPosition(double currentSec, double totalSec) {
    duration_ = totalSec;
    currentLabel_->setText(formatTime(currentSec));
    totalLabel_->setText(formatTime(totalSec));
    progressSlider_->setEnabled(totalSec > 0.0);
    if (totalSec <= 0.0) {
        QSignalBlocker block(progressSlider_);
        progressSlider_->setValue(0);
        return;
    }
    // sliderDown == "user is dragging the handle" — don't yank it back to the
    // playback position while they're scrubbing.
    if (!progressSlider_->isSliderDown()) {
        QSignalBlocker block(progressSlider_);
        const int v = static_cast<int>(currentSec / totalSec * kProgressResolution);
        progressSlider_->setValue(v);
    }
}

void ControlBar::setVolume(double volume) {
    QSignalBlocker block(volumeSlider_);
    volumeSlider_->setValue(static_cast<int>(volume * kVolumeResolution));
}

void ControlBar::setPlayMode(PlayMode mode) {
    mode_ = mode;
    updateModeButtonLabel();
}

void ControlBar::onPlayPauseClicked() {
    if (playing_) {
        emit pauseRequested();
    } else {
        emit playRequested();
    }
}

void ControlBar::onProgressSliderReleased() {
    if (duration_ <= 0.0)
        return;
    const double fraction = static_cast<double>(progressSlider_->value()) / kProgressResolution;
    emit seekRequested(fraction * duration_);
}

void ControlBar::onVolumeSliderMoved(int value) {
    const double v = static_cast<double>(value) / kVolumeResolution;
    emit volumeChangeRequested(v);
}

void ControlBar::onModeClicked() {
    const PlayMode m = nextMode(mode_);
    emit playModeChangeRequested(m);
}

void ControlBar::updatePlayPauseIcon() {
    playPauseBtn_->setText(playing_ ? QStringLiteral("⏸") : QStringLiteral("▶"));
}

void ControlBar::updateModeButtonLabel() {
    modeBtn_->setText(modeLabel(mode_));
}

}  // namespace musicplayer
