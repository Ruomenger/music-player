#include "player_state_repo.h"

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

// 0 → SQL NULL: the player_state FKs use ON DELETE SET NULL, and we want a
// fresh install with no current song to round-trip as 0.
QVariant nullableId(int id) {
    return id > 0 ? QVariant(id) : QVariant(QMetaType(QMetaType::Int));
}

}  // namespace

PlayerStateRepo::PlayerStateRepo(std::string connectionName)
    : connectionName_(std::move(connectionName)) {}

bool PlayerStateRepo::save(const PlayerState& state) {
    QSqlDatabase db = conn(connectionName_);
    if (!db.isOpen()) {
        qWarning("PlayerStateRepo::save: connection '%s' is not open", connectionName_.c_str());
        return false;
    }
    QSqlQuery q(db);
    q.prepare(
        QLatin1StringView("INSERT INTO player_state(id, current_song_id, playlist_id, position) "
                          "VALUES (1, :song, :playlist, :position) "
                          "ON CONFLICT(id) DO UPDATE SET "
                          "    current_song_id = excluded.current_song_id,"
                          "    playlist_id = excluded.playlist_id,"
                          "    position = excluded.position"));
    q.bindValue(":song", nullableId(state.currentSongId));
    q.bindValue(":playlist", nullableId(state.playlistId));
    q.bindValue(":position", state.position);
    if (!q.exec()) {
        qWarning("PlayerStateRepo::save: failed: %s", q.lastError().text().toUtf8().constData());
        return false;
    }
    return true;
}

std::optional<PlayerState> PlayerStateRepo::load() {
    QSqlDatabase db = conn(connectionName_);
    if (!db.isOpen()) {
        qWarning("PlayerStateRepo::load: connection '%s' is not open", connectionName_.c_str());
        return std::nullopt;
    }
    QSqlQuery q(db);
    if (!q.exec(QLatin1StringView("SELECT current_song_id, playlist_id, position "
                                  "FROM player_state WHERE id = 1"))) {
        qWarning("PlayerStateRepo::load: failed: %s", q.lastError().text().toUtf8().constData());
        return std::nullopt;
    }
    if (!q.next())
        return std::nullopt;
    PlayerState s;
    // .toInt() on NULL QVariant returns 0, matching our "no song" sentinel.
    s.currentSongId = q.value(0).toInt();
    s.playlistId = q.value(1).toInt();
    s.position = q.value(2).toDouble();
    return s;
}

}  // namespace musicplayer
