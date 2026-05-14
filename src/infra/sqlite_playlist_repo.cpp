#include "sqlite_playlist_repo.h"

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

Playlist playlistFromQuery(const QSqlQuery& q) {
    Playlist p;
    p.id = q.value("id").toInt();
    p.name = q.value("name").toString().toStdString();
    p.description = q.value("description").toString().toStdString();
    p.coverPath = q.value("cover_path").toString().toStdString();
    p.isSystem = q.value("is_system").toInt() != 0;
    p.sortOrder = q.value("sort_order").toInt();
    return p;
}

SongInfo songFromJoinQuery(const QSqlQuery& q) {
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

}  // namespace

SqlitePlaylistRepo::SqlitePlaylistRepo(std::string connectionName)
    : connectionName_(std::move(connectionName)) {}

int SqlitePlaylistRepo::createPlaylist(const std::string& name, const std::string& description,
                                       bool isSystem) {
    QSqlDatabase db = conn(connectionName_);
    if (!db.isOpen()) {
        qWarning("SqlitePlaylistRepo::createPlaylist: connection '%s' is not open",
                 connectionName_.c_str());
        return -1;
    }
    QSqlQuery q(db);
    q.prepare(
        QLatin1StringView("INSERT INTO playlists(name, description, is_system) "
                          "VALUES (:name, :description, :is_system)"));
    q.bindValue(":name", QString::fromStdString(name));
    q.bindValue(":description", QString::fromStdString(description));
    q.bindValue(":is_system", isSystem ? 1 : 0);
    if (!q.exec()) {
        qWarning("SqlitePlaylistRepo::createPlaylist: failed: %s",
                 q.lastError().text().toUtf8().constData());
        return -1;
    }
    return q.lastInsertId().toInt();
}

bool SqlitePlaylistRepo::renamePlaylist(int playlistId, const std::string& newName) {
    QSqlDatabase db = conn(connectionName_);
    if (!db.isOpen()) {
        qWarning("SqlitePlaylistRepo::renamePlaylist: connection '%s' is not open",
                 connectionName_.c_str());
        return false;
    }
    QSqlQuery q(db);
    q.prepare(
        QLatin1StringView("UPDATE playlists SET name = :name, updated_at = datetime('now') "
                          "WHERE id = :id"));
    q.bindValue(":name", QString::fromStdString(newName));
    q.bindValue(":id", playlistId);
    if (!q.exec()) {
        qWarning("SqlitePlaylistRepo::renamePlaylist: failed: %s",
                 q.lastError().text().toUtf8().constData());
        return false;
    }
    return q.numRowsAffected() > 0;
}

bool SqlitePlaylistRepo::deletePlaylist(int playlistId) {
    QSqlDatabase db = conn(connectionName_);
    if (!db.isOpen()) {
        qWarning("SqlitePlaylistRepo::deletePlaylist: connection '%s' is not open",
                 connectionName_.c_str());
        return false;
    }
    QSqlQuery q(db);
    q.prepare(QLatin1StringView("DELETE FROM playlists WHERE id = :id"));
    q.bindValue(":id", playlistId);
    if (!q.exec()) {
        qWarning("SqlitePlaylistRepo::deletePlaylist: failed: %s",
                 q.lastError().text().toUtf8().constData());
        return false;
    }
    return q.numRowsAffected() > 0;
}

std::optional<Playlist> SqlitePlaylistRepo::getPlaylistById(int playlistId) {
    QSqlDatabase db = conn(connectionName_);
    if (!db.isOpen()) {
        qWarning("SqlitePlaylistRepo::getPlaylistById: connection '%s' is not open",
                 connectionName_.c_str());
        return std::nullopt;
    }
    QSqlQuery q(db);
    q.prepare(
        QLatin1StringView("SELECT id, name, description, cover_path, is_system, sort_order "
                          "FROM playlists WHERE id = :id"));
    q.bindValue(":id", playlistId);
    if (!q.exec()) {
        qWarning("SqlitePlaylistRepo::getPlaylistById: failed: %s",
                 q.lastError().text().toUtf8().constData());
        return std::nullopt;
    }
    if (!q.next())
        return std::nullopt;
    return playlistFromQuery(q);
}

std::vector<Playlist> SqlitePlaylistRepo::getAllPlaylists() {
    std::vector<Playlist> result;
    QSqlDatabase db = conn(connectionName_);
    if (!db.isOpen()) {
        qWarning("SqlitePlaylistRepo::getAllPlaylists: connection '%s' is not open",
                 connectionName_.c_str());
        return result;
    }
    QSqlQuery q(db);
    if (!q.exec(QLatin1StringView("SELECT id, name, description, cover_path, is_system, sort_order "
                                  "FROM playlists ORDER BY sort_order, id"))) {
        qWarning("SqlitePlaylistRepo::getAllPlaylists: failed: %s",
                 q.lastError().text().toUtf8().constData());
        return result;
    }
    while (q.next())
        result.push_back(playlistFromQuery(q));
    return result;
}

bool SqlitePlaylistRepo::addSongToPlaylist(int playlistId, int songId) {
    QSqlDatabase db = conn(connectionName_);
    if (!db.isOpen()) {
        qWarning("SqlitePlaylistRepo::addSongToPlaylist: connection '%s' is not open",
                 connectionName_.c_str());
        return false;
    }
    // COALESCE handles empty-playlist case where MAX() returns NULL.
    QSqlQuery q(db);
    q.prepare(
        QLatin1StringView("INSERT INTO playlist_songs(playlist_id, song_id, sort_index) "
                          "VALUES (:pid, :sid, "
                          "    COALESCE((SELECT MAX(sort_index) + 1 FROM playlist_songs "
                          "              WHERE playlist_id = :pid2), 0))"));
    q.bindValue(":pid", playlistId);
    q.bindValue(":sid", songId);
    q.bindValue(":pid2", playlistId);
    if (!q.exec()) {
        qWarning("SqlitePlaylistRepo::addSongToPlaylist: failed: %s",
                 q.lastError().text().toUtf8().constData());
        return false;
    }
    return q.numRowsAffected() > 0;
}

bool SqlitePlaylistRepo::removeSongFromPlaylist(int playlistId, int songId) {
    QSqlDatabase db = conn(connectionName_);
    if (!db.isOpen()) {
        qWarning("SqlitePlaylistRepo::removeSongFromPlaylist: connection '%s' is not open",
                 connectionName_.c_str());
        return false;
    }
    QSqlQuery q(db);
    q.prepare(
        QLatin1StringView("DELETE FROM playlist_songs "
                          "WHERE playlist_id = :pid AND song_id = :sid"));
    q.bindValue(":pid", playlistId);
    q.bindValue(":sid", songId);
    if (!q.exec()) {
        qWarning("SqlitePlaylistRepo::removeSongFromPlaylist: failed: %s",
                 q.lastError().text().toUtf8().constData());
        return false;
    }
    return q.numRowsAffected() > 0;
}

std::vector<SongInfo> SqlitePlaylistRepo::getSongsInPlaylist(int playlistId) {
    std::vector<SongInfo> result;
    QSqlDatabase db = conn(connectionName_);
    if (!db.isOpen()) {
        qWarning("SqlitePlaylistRepo::getSongsInPlaylist: connection '%s' is not open",
                 connectionName_.c_str());
        return result;
    }
    QSqlQuery q(db);
    q.prepare(
        QLatin1StringView("SELECT s.id, s.title, s.artist, s.album, s.file_path, s.duration, "
                          "       s.format, s.lyric_path, s.lyric_source "
                          "FROM songs s "
                          "JOIN playlist_songs ps ON ps.song_id = s.id "
                          "WHERE ps.playlist_id = :pid "
                          "ORDER BY ps.sort_index"));
    q.bindValue(":pid", playlistId);
    if (!q.exec()) {
        qWarning("SqlitePlaylistRepo::getSongsInPlaylist: failed: %s",
                 q.lastError().text().toUtf8().constData());
        return result;
    }
    while (q.next())
        result.push_back(songFromJoinQuery(q));
    return result;
}

bool SqlitePlaylistRepo::reorderSongs(int playlistId, const std::vector<int>& songIdsInOrder) {
    QSqlDatabase db = conn(connectionName_);
    if (!db.isOpen()) {
        qWarning("SqlitePlaylistRepo::reorderSongs: connection '%s' is not open",
                 connectionName_.c_str());
        return false;
    }
    if (!db.transaction()) {
        qWarning("SqlitePlaylistRepo::reorderSongs: BEGIN failed: %s",
                 db.lastError().text().toUtf8().constData());
        return false;
    }
    QSqlQuery q(db);
    q.prepare(
        QLatin1StringView("UPDATE playlist_songs SET sort_index = :idx "
                          "WHERE playlist_id = :pid AND song_id = :sid"));
    for (std::size_t i = 0; i < songIdsInOrder.size(); ++i) {
        q.bindValue(":idx", static_cast<int>(i));
        q.bindValue(":pid", playlistId);
        q.bindValue(":sid", songIdsInOrder[i]);
        if (!q.exec()) {
            qWarning("SqlitePlaylistRepo::reorderSongs: update failed: %s",
                     q.lastError().text().toUtf8().constData());
            db.rollback();
            return false;
        }
        if (q.numRowsAffected() == 0) {
            qWarning("SqlitePlaylistRepo::reorderSongs: song_id=%d not in playlist %d",
                     songIdsInOrder[i], playlistId);
            db.rollback();
            return false;
        }
    }
    if (!db.commit()) {
        qWarning("SqlitePlaylistRepo::reorderSongs: COMMIT failed: %s",
                 db.lastError().text().toUtf8().constData());
        db.rollback();
        return false;
    }
    return true;
}

}  // namespace musicplayer
