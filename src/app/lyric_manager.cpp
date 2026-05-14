#include "lyric_manager.h"

#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QTextStream>

#include <cstdint>
#include <utility>

namespace musicplayer {

LyricManager::LyricManager(QObject* parent) : QObject(parent) {}

void LyricManager::loadForSong(const SongInfo& song) {
    if (!autoLoad_ || song.lyricPath.empty() || !song.hasLyric) {
        clear();
        return;
    }
    loadFromPath(QString::fromStdString(song.lyricPath));
}

void LyricManager::setAutoLoadEnabled(bool enabled) {
    autoLoad_ = enabled;
}

void LyricManager::loadFromPath(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        clear();
        return;
    }
    QTextStream stream(&file);
    // LRC files in the wild are usually UTF-8 (and a fair share are GBK on
    // Chinese tracks); Qt6 defaults to UTF-8 which covers the common case.
    // GBK handling is a Phase 8 settings concern.
    const QString content = stream.readAll();
    auto parsed = LyricParser::parse(content.toStdString());
    setLines(std::move(parsed.lines));
}

void LyricManager::clear() {
    if (lines_.empty() && currentIndex_ == -1)
        return;
    lines_.clear();
    currentIndex_ = -1;
    emit lyricsChanged(lines_);
    emit currentLineChanged(currentIndex_);
}

void LyricManager::updatePosition(double currentSec, double /*totalSec*/) {
    if (lines_.empty())
        return;
    const auto ms = static_cast<std::int64_t>(currentSec * 1000);
    const int idx = LyricParser::findCurrentLineIndex(lines_, ms);
    if (idx != currentIndex_) {
        currentIndex_ = idx;
        emit currentLineChanged(currentIndex_);
    }
}

void LyricManager::setLines(std::vector<LyricLine> lines) {
    lines_ = std::move(lines);
    currentIndex_ = -1;
    emit lyricsChanged(lines_);
    emit currentLineChanged(currentIndex_);
}

}  // namespace musicplayer
