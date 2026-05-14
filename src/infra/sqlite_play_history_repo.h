#pragma once

#include <string>
#include <vector>
#include "song_info.h"

namespace musicplayer {

// One row of play_history joined with its songs row, ready for the
// "Recently played" UI. progress is the position (seconds) at which the
// caller decided the play was worth recording.
struct PlayHistoryEntry {
    int historyId = 0;
    std::string playedAt;  // SQLite 'YYYY-MM-DD HH:MM:SS' (UTC)
    double progress = 0.0;
    SongInfo song;
};

// Persistence for the play_history table. PlayerController calls record()
// when a song reaches a "meaningful play" threshold (e.g. ≥30s or end of
// track). recent() backs the UI's recent-played view. purgeOlderThan() runs
// on startup against the user's history_max_days setting.
class SqlitePlayHistoryRepo {
public:
    explicit SqlitePlayHistoryRepo(std::string connectionName = "musicplayer");

    // Returns the inserted history row id on success, -1 on failure.
    int record(int songId, double progress);

    // Most recent N rows, newest first. limit ≤ 0 → empty result.
    [[nodiscard]] std::vector<PlayHistoryEntry> recent(int limit);

    // Drops rows older than the given relative window. Returns the number of
    // rows deleted, or -1 on failure. days ≤ 0 wipes the whole table.
    int purgeOlderThan(int days);

private:
    std::string connectionName_;
};

}  // namespace musicplayer
