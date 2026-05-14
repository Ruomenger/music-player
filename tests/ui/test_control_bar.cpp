#include <gtest/gtest.h>
#include "control_bar.h"
#include "song_info.h"

#include <QPushButton>
#include <QSignalSpy>
#include <QSlider>
#include <QTest>

using musicplayer::ControlBar;
using musicplayer::PlayMode;

TEST(ControlBarTest, PlayPauseClickEmitsPlayThenPause) {
    ControlBar bar;
    QSignalSpy playSpy(&bar, &ControlBar::playRequested);
    QSignalSpy pauseSpy(&bar, &ControlBar::pauseRequested);

    auto* btn = bar.findChild<QPushButton*>();
    ASSERT_NE(btn, nullptr);

    // The play/pause button is the second one (after prev). Walk the children
    // to find it by text — that's robust against layout changes.
    QPushButton* playPause = nullptr;
    for (auto* b : bar.findChildren<QPushButton*>()) {
        if (b->text() == QStringLiteral("▶") || b->text() == QStringLiteral("⏸")) {
            playPause = b;
            break;
        }
    }
    ASSERT_NE(playPause, nullptr);

    QTest::mouseClick(playPause, Qt::LeftButton);
    EXPECT_EQ(playSpy.count(), 1);
    EXPECT_EQ(pauseSpy.count(), 0);

    bar.setPlayingState(true);
    QTest::mouseClick(playPause, Qt::LeftButton);
    EXPECT_EQ(playSpy.count(), 1);
    EXPECT_EQ(pauseSpy.count(), 1);
}

TEST(ControlBarTest, SetPositionUpdatesSliderRange) {
    ControlBar bar;
    auto sliders = bar.findChildren<QSlider*>();
    ASSERT_FALSE(sliders.isEmpty());
    QSlider* progress = sliders.first();  // first horizontal slider is progress
    EXPECT_FALSE(progress->isEnabled());  // no duration yet

    bar.setPosition(30.0, 60.0);
    EXPECT_TRUE(progress->isEnabled());
    EXPECT_EQ(progress->value(), progress->maximum() / 2);
}

TEST(ControlBarTest, SetVolumeUpdatesSliderWithoutEmitting) {
    ControlBar bar;
    QSignalSpy spy(&bar, &ControlBar::volumeChangeRequested);
    bar.setVolume(0.5);
    EXPECT_EQ(spy.count(), 0);  // programmatic update must not loop back
    // The volume slider's value reflects the new fraction.
    auto sliders = bar.findChildren<QSlider*>();
    ASSERT_GE(sliders.size(), 2);
    EXPECT_NEAR(static_cast<double>(sliders.last()->value()) / sliders.last()->maximum(), 0.5,
                1e-6);
}

TEST(ControlBarTest, ModeButtonCyclesAndEmits) {
    ControlBar bar;
    QSignalSpy spy(&bar, &ControlBar::playModeChangeRequested);
    QPushButton* modeBtn = nullptr;
    for (auto* b : bar.findChildren<QPushButton*>()) {
        if (b->text() == QStringLiteral("Sequential") || b->text() == QStringLiteral("Single") ||
            b->text() == QStringLiteral("Loop") || b->text() == QStringLiteral("Shuffle")) {
            modeBtn = b;
            break;
        }
    }
    ASSERT_NE(modeBtn, nullptr);

    QTest::mouseClick(modeBtn, Qt::LeftButton);
    ASSERT_EQ(spy.count(), 1);
    // Default is Sequential → next is ListLoop.
    EXPECT_EQ(spy.takeFirst().at(0).value<PlayMode>(), PlayMode::ListLoop);
}

TEST(ControlBarTest, NextAndPreviousEmit) {
    ControlBar bar;
    QSignalSpy nextSpy(&bar, &ControlBar::nextRequested);
    QSignalSpy prevSpy(&bar, &ControlBar::previousRequested);
    QPushButton* nextBtn = nullptr;
    QPushButton* prevBtn = nullptr;
    for (auto* b : bar.findChildren<QPushButton*>()) {
        if (b->text() == QStringLiteral("⏭"))
            nextBtn = b;
        else if (b->text() == QStringLiteral("⏮"))
            prevBtn = b;
    }
    ASSERT_NE(nextBtn, nullptr);
    ASSERT_NE(prevBtn, nullptr);
    QTest::mouseClick(nextBtn, Qt::LeftButton);
    QTest::mouseClick(prevBtn, Qt::LeftButton);
    EXPECT_EQ(nextSpy.count(), 1);
    EXPECT_EQ(prevSpy.count(), 1);
}
