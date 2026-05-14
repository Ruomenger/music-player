#pragma once

#include <QObject>
#include <QString>

#include <vector>

#include "lyric_parser.h"
#include "song_info.h"

namespace musicplayer {

// Application-layer holder for the currently loaded lyrics. Reads the sidecar
// .lrc when a song starts (or accepts a user-picked path), parses, and emits
// currentLineChanged as positionChanged ticks drive the cursor across the
// timestamps. UI widgets connect to lyricsChanged + currentLineChanged.
class LyricManager : public QObject {
    Q_OBJECT
public:
    explicit LyricManager(QObject* parent = nullptr);

    [[nodiscard]] const std::vector<LyricLine>& lines() const { return lines_; }
    [[nodiscard]] int currentLineIndex() const { return currentIndex_; }

public slots:
    // Loads lyrics for `song` from song.lyricPath. If the path is empty or
    // unreadable, clears the lyric view.
    void loadForSong(const SongInfo& song);

    // Loads from an arbitrary filesystem path. Used by the manual "Load
    // lyrics…" button.
    void loadFromPath(const QString& path);

    void clear();

    // Drives the current-line cursor. Called from PlayerController via
    // positionChanged. No-op if lyrics aren't loaded.
    void updatePosition(double currentSec, double totalSec);

signals:
    void lyricsChanged(const std::vector<LyricLine>& lines);
    void currentLineChanged(int index);  // -1 means "before first line"

private:
    void setLines(std::vector<LyricLine> lines);

    std::vector<LyricLine> lines_;
    int currentIndex_ = -1;
};

}  // namespace musicplayer
