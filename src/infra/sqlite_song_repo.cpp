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
// (Phase 3 importer only handles sidecar .lrc files; refining to 'embedded' /
// 'manual' will come with the in-tag lyric extractor).
constexpr const char* kLyricSourceFromBool[] = {"none", "lrc_file"};

SongInfo songFromQuery(const QSqlQuery& q) {
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

constexpr const char* kSelectCols =
    "id, title, artist, album, album_artist, composer, genre, year, track_number, "
    "disc_number, file_path, file_size, file_mtime, duration, format, bitrate, "
    "sample_rate, channels, cover_path, lyric_path, lyric_source";

void bindWriteFields(QSqlQuery& q, const SongInfo& song) {
    q.bindValue(":file_path", QString::fromStdString(song.filePath));
    q.bindValue(":title", QString::fromStdString(song.title));
    q.bindValue(":artist", QString::fromStdString(song.artist));
    q.bindValue(":album", QString::fromStdString(song.album));
    q.bindValue(":album_artist", QString::fromStdString(song.albumArtist));
    q.bindValue(":composer", QString::fromStdString(song.composer));
    q.bindValue(":genre", QString::fromStdString(song.genre));
    q.bindValue(":year", song.year);
    q.bindValue(":track_number", song.trackNumber);
    q.bindValue(":disc_number", song.discNumber);
    q.bindValue(":file_size", static_cast<qlonglong>(song.fileSize));
    q.bindValue(":file_mtime", static_cast<qlonglong>(song.fileMtime));
    q.bindValue(":duration", song.duration);
    q.bindValue(":format", QString::fromStdString(song.format));
    q.bindValue(":bitrate", song.bitrate);
    q.bindValue(":sample_rate", song.sampleRate);
    q.bindValue(":channels", song.channels);
    q.bindValue(":cover_path", QString::fromStdString(song.coverPath));
    q.bindValue(":lyric_path", QString::fromStdString(song.lyricPath));
    q.bindValue(":lyric_source", QLatin1StringView(kLyricSourceFromBool[song.hasLyric ? 1 : 0]));
}

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
    q.prepare(QLatin1StringView(
        "INSERT INTO songs(file_path, title, artist, album, album_artist, composer,"
        "                  genre, year, track_number, disc_number, file_size,"
        "                  file_mtime, duration, format, bitrate, sample_rate,"
        "                  channels, cover_path, lyric_path, lyric_source) "
        "VALUES (:file_path, :title, :artist, :album, :album_artist, :composer,"
        "        :genre, :year, :track_number, :disc_number, :file_size,"
        "        :file_mtime, :duration, :format, :bitrate, :sample_rate,"
        "        :channels, :cover_path, :lyric_path, :lyric_source)"));
    bindWriteFields(q, song);
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
    q.prepare(QLatin1StringView(
        "UPDATE songs SET file_path = :file_path, title = :title, artist = :artist,"
        "                 album = :album, album_artist = :album_artist,"
        "                 composer = :composer, genre = :genre, year = :year,"
        "                 track_number = :track_number, disc_number = :disc_number,"
        "                 file_size = :file_size, file_mtime = :file_mtime,"
        "                 duration = :duration, format = :format, bitrate = :bitrate,"
        "                 sample_rate = :sample_rate, channels = :channels,"
        "                 cover_path = :cover_path, lyric_path = :lyric_path,"
        "                 lyric_source = :lyric_source "
        "WHERE id = :id"));
    bindWriteFields(q, song);
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
