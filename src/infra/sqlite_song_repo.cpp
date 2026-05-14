#include "sqlite_song_repo.h"

#include <QLatin1StringView>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QVariant>
#include <QtLogging>

#include <utility>

namespace musicplayer {

namespace {

QSqlDatabase conn(const std::string& name) {
    return QSqlDatabase::database(QString::fromStdString(name));
}

// hasLyric (domain) ↔ lyric_source (schema): bool maps to 'lrc_file' when true
// (Phase 3 scanner will refine to 'embedded' / 'manual' as it detects sources).
constexpr const char* kLyricSourceFromBool[] = {"none", "lrc_file"};

SongInfo songFromQuery(const QSqlQuery& q) {
    SongInfo s;
    s.id = q.value("id").toInt();
    s.title = q.value("title").toString().toStdString();
    s.artist = q.value("artist").toString().toStdString();
    s.album = q.value("album").toString().toStdString();
    s.filePath = q.value("file_path").toString().toStdString();
    s.duration = q.value("duration").toDouble();
    s.format = q.value("format").toString().toStdString();
    s.lyricPath = q.value("lyric_path").toString().toStdString();
    s.hasLyric = q.value("lyric_source").toString() != QLatin1StringView("none");
    return s;
}

constexpr const char* kSelectCols =
    "id, title, artist, album, file_path, duration, format, lyric_path, lyric_source";

}  // namespace

SqliteSongRepo::SqliteSongRepo(std::string connectionName)
    : connectionName_(std::move(connectionName)) {}

int SqliteSongRepo::insertSong(const SongInfo& song) {
    QSqlDatabase db = conn(connectionName_);
    if (!db.isOpen()) {
        qWarning("SqliteSongRepo::insertSong: connection '%s' is not open",
                 connectionName_.c_str());
        return -1;
    }
    QSqlQuery q(db);
    q.prepare(
        QLatin1StringView("INSERT INTO songs(file_path, title, artist, album, duration, format,"
                          "                  lyric_path, lyric_source) "
                          "VALUES (:file_path, :title, :artist, :album, :duration, :format,"
                          "        :lyric_path, :lyric_source)"));
    q.bindValue(":file_path", QString::fromStdString(song.filePath));
    q.bindValue(":title", QString::fromStdString(song.title));
    q.bindValue(":artist", QString::fromStdString(song.artist));
    q.bindValue(":album", QString::fromStdString(song.album));
    q.bindValue(":duration", song.duration);
    q.bindValue(":format", QString::fromStdString(song.format));
    q.bindValue(":lyric_path", QString::fromStdString(song.lyricPath));
    q.bindValue(":lyric_source", QLatin1StringView(kLyricSourceFromBool[song.hasLyric ? 1 : 0]));
    if (!q.exec()) {
        qWarning("SqliteSongRepo::insertSong: failed: %s",
                 q.lastError().text().toUtf8().constData());
        return -1;
    }
    return q.lastInsertId().toInt();
}

bool SqliteSongRepo::updateSong(const SongInfo& song) {
    QSqlDatabase db = conn(connectionName_);
    if (!db.isOpen()) {
        qWarning("SqliteSongRepo::updateSong: connection '%s' is not open",
                 connectionName_.c_str());
        return false;
    }
    QSqlQuery q(db);
    q.prepare(
        QLatin1StringView("UPDATE songs SET file_path = :file_path, title = :title,"
                          "                 artist = :artist, album = :album,"
                          "                 duration = :duration, format = :format,"
                          "                 lyric_path = :lyric_path, lyric_source = :lyric_source "
                          "WHERE id = :id"));
    q.bindValue(":file_path", QString::fromStdString(song.filePath));
    q.bindValue(":title", QString::fromStdString(song.title));
    q.bindValue(":artist", QString::fromStdString(song.artist));
    q.bindValue(":album", QString::fromStdString(song.album));
    q.bindValue(":duration", song.duration);
    q.bindValue(":format", QString::fromStdString(song.format));
    q.bindValue(":lyric_path", QString::fromStdString(song.lyricPath));
    q.bindValue(":lyric_source", QLatin1StringView(kLyricSourceFromBool[song.hasLyric ? 1 : 0]));
    q.bindValue(":id", song.id);
    if (!q.exec()) {
        qWarning("SqliteSongRepo::updateSong: failed: %s",
                 q.lastError().text().toUtf8().constData());
        return false;
    }
    return q.numRowsAffected() > 0;
}

bool SqliteSongRepo::removeSong(int id) {
    QSqlDatabase db = conn(connectionName_);
    if (!db.isOpen()) {
        qWarning("SqliteSongRepo::removeSong: connection '%s' is not open",
                 connectionName_.c_str());
        return false;
    }
    QSqlQuery q(db);
    q.prepare(QLatin1StringView("DELETE FROM songs WHERE id = :id"));
    q.bindValue(":id", id);
    if (!q.exec()) {
        qWarning("SqliteSongRepo::removeSong: failed: %s",
                 q.lastError().text().toUtf8().constData());
        return false;
    }
    return q.numRowsAffected() > 0;
}

std::optional<SongInfo> SqliteSongRepo::getSongById(int id) {
    QSqlDatabase db = conn(connectionName_);
    if (!db.isOpen()) {
        qWarning("SqliteSongRepo::getSongById: connection '%s' is not open",
                 connectionName_.c_str());
        return std::nullopt;
    }
    QSqlQuery q(db);
    q.prepare(QString::fromLatin1("SELECT %1 FROM songs WHERE id = :id")
                  .arg(QLatin1StringView(kSelectCols)));
    q.bindValue(":id", id);
    if (!q.exec()) {
        qWarning("SqliteSongRepo::getSongById: failed: %s",
                 q.lastError().text().toUtf8().constData());
        return std::nullopt;
    }
    if (!q.next())
        return std::nullopt;
    return songFromQuery(q);
}

std::optional<SongInfo> SqliteSongRepo::getSongByPath(const std::string& filePath) {
    QSqlDatabase db = conn(connectionName_);
    if (!db.isOpen()) {
        qWarning("SqliteSongRepo::getSongByPath: connection '%s' is not open",
                 connectionName_.c_str());
        return std::nullopt;
    }
    QSqlQuery q(db);
    q.prepare(QString::fromLatin1("SELECT %1 FROM songs WHERE file_path = :file_path")
                  .arg(QLatin1StringView(kSelectCols)));
    q.bindValue(":file_path", QString::fromStdString(filePath));
    if (!q.exec()) {
        qWarning("SqliteSongRepo::getSongByPath: failed: %s",
                 q.lastError().text().toUtf8().constData());
        return std::nullopt;
    }
    if (!q.next())
        return std::nullopt;
    return songFromQuery(q);
}

std::vector<SongInfo> SqliteSongRepo::getAllSongs() {
    std::vector<SongInfo> result;
    QSqlDatabase db = conn(connectionName_);
    if (!db.isOpen()) {
        qWarning("SqliteSongRepo::getAllSongs: connection '%s' is not open",
                 connectionName_.c_str());
        return result;
    }
    QSqlQuery q(db);
    if (!q.exec(QString::fromLatin1("SELECT %1 FROM songs ORDER BY id")
                    .arg(QLatin1StringView(kSelectCols)))) {
        qWarning("SqliteSongRepo::getAllSongs: failed: %s",
                 q.lastError().text().toUtf8().constData());
        return result;
    }
    while (q.next())
        result.push_back(songFromQuery(q));
    return result;
}

}  // namespace musicplayer
