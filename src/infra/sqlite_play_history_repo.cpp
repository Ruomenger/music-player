#include "sqlite_play_history_repo.h"

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

SongInfo songFromJoinQuery(const QSqlQuery& q) {
    SongInfo s;
    s.id = q.value("id").toInt();
    s.title = q.value("title").toString().toStdString();
    s.artist = q.value("artist").toString().toStdString();
    s.album = q.value("album").toString().toStdString();
    s.albumArtist = q.value("album_artist").toString().toStdString();
    s.composer = q.value("composer").toString().toStdString();
    s.genre = q.value("genre").toString().toStdString();
    s.year = q.value("year").toInt();
    s.trackNumber = q.value("track_number").toInt();
    s.discNumber = q.value("disc_number").toInt();
    s.filePath = q.value("file_path").toString().toStdString();
    s.fileSize = q.value("file_size").toLongLong();
    s.fileMtime = q.value("file_mtime").toLongLong();
    s.duration = q.value("duration").toDouble();
    s.format = q.value("format").toString().toStdString();
    s.bitrate = q.value("bitrate").toInt();
    s.sampleRate = q.value("sample_rate").toInt();
    s.channels = q.value("channels").toInt();
    s.coverPath = q.value("cover_path").toString().toStdString();
    s.lyricPath = q.value("lyric_path").toString().toStdString();
    s.hasLyric = q.value("lyric_source").toString() != QLatin1StringView("none");
    return s;
}

}  // namespace

SqlitePlayHistoryRepo::SqlitePlayHistoryRepo(std::string connectionName)
    : connectionName_(std::move(connectionName)) {}

int SqlitePlayHistoryRepo::record(int songId, double progress) {
    QSqlDatabase db = conn(connectionName_);
    if (!db.isOpen()) {
        qWarning("SqlitePlayHistoryRepo::record: connection '%s' is not open",
                 connectionName_.c_str());
        return -1;
    }
    QSqlQuery q(db);
    q.prepare(
        QLatin1StringView("INSERT INTO play_history(song_id, progress) VALUES (:song, :progress)"));
    q.bindValue(":song", songId);
    q.bindValue(":progress", progress);
    if (!q.exec()) {
        qWarning("SqlitePlayHistoryRepo::record: failed: %s",
                 q.lastError().text().toUtf8().constData());
        return -1;
    }
    return q.lastInsertId().toInt();
}

std::vector<PlayHistoryEntry> SqlitePlayHistoryRepo::recent(int limit) {
    std::vector<PlayHistoryEntry> result;
    if (limit <= 0)
        return result;
    QSqlDatabase db = conn(connectionName_);
    if (!db.isOpen()) {
        qWarning("SqlitePlayHistoryRepo::recent: connection '%s' is not open",
                 connectionName_.c_str());
        return result;
    }
    QSqlQuery q(db);
    q.prepare(QLatin1StringView(
        "SELECT ph.id AS history_id, ph.played_at, ph.progress, "
        "       s.id, s.title, s.artist, s.album, s.album_artist, s.composer,"
        "       s.genre, s.year, s.track_number, s.disc_number, s.file_path,"
        "       s.file_size, s.file_mtime, s.duration, s.format, s.bitrate,"
        "       s.sample_rate, s.channels, s.cover_path, s.lyric_path, s.lyric_source "
        "FROM play_history ph "
        "JOIN songs s ON s.id = ph.song_id "
        "ORDER BY ph.played_at DESC, ph.id DESC "
        "LIMIT :limit"));
    q.bindValue(":limit", limit);
    if (!q.exec()) {
        qWarning("SqlitePlayHistoryRepo::recent: failed: %s",
                 q.lastError().text().toUtf8().constData());
        return result;
    }
    while (q.next()) {
        PlayHistoryEntry e;
        e.historyId = q.value("history_id").toInt();
        e.playedAt = q.value("played_at").toString().toStdString();
        e.progress = q.value("progress").toDouble();
        e.song = songFromJoinQuery(q);
        result.push_back(std::move(e));
    }
    return result;
}

int SqlitePlayHistoryRepo::purgeOlderThan(int days) {
    QSqlDatabase db = conn(connectionName_);
    if (!db.isOpen()) {
        qWarning("SqlitePlayHistoryRepo::purgeOlderThan: connection '%s' is not open",
                 connectionName_.c_str());
        return -1;
    }
    QSqlQuery q(db);
    if (days <= 0) {
        if (!q.exec(QLatin1StringView("DELETE FROM play_history"))) {
            qWarning("SqlitePlayHistoryRepo::purgeOlderThan: failed: %s",
                     q.lastError().text().toUtf8().constData());
            return -1;
        }
        return q.numRowsAffected();
    }
    // The "-" || N || " days" modifier syntax lets us bind the integer; we
    // can't bind a parameter inside a string literal that SQLite parses as
    // an interval expression.
    q.prepare(
        QLatin1StringView("DELETE FROM play_history "
                          "WHERE played_at < datetime('now', '-' || :days || ' days')"));
    q.bindValue(":days", days);
    if (!q.exec()) {
        qWarning("SqlitePlayHistoryRepo::purgeOlderThan: failed: %s",
                 q.lastError().text().toUtf8().constData());
        return -1;
    }
    return q.numRowsAffected();
}

}  // namespace musicplayer
