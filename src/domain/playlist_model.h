#pragma once

#include "song_info.h"

#include <vector>

namespace musicplayer {

class PlaylistModel {
public:
    void addSong(const SongInfo& song);
    void removeSong(int index);
    const std::vector<SongInfo>& songs() const;

private:
    std::vector<SongInfo> songs_;
};

} // namespace musicplayer
