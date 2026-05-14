#pragma once

#include <optional>
#include <string>
#include "playlist.h"

namespace musicplayer {

// Persistence for the single-row `player_state` table (CHECK id = 1). Used at
// startup to restore the last playing song / playlist / position, and at
// shutdown to save the current one. Loaded values may refer to songs that no
// longer exist (CASCADE SET NULL): caller must handle PlayerState fields = 0.
class PlayerStateRepo {
public:
    explicit PlayerStateRepo(std::string connectionName = "musicplayer");

    bool save(const PlayerState& state);
    std::optional<PlayerState> load();

private:
    std::string connectionName_;
};

}  // namespace musicplayer
