#include "playlist_model.h"

namespace musicplayer {

void PlaylistModel::addSong(const SongInfo& song) {
    songs_.push_back(song);
}

void PlaylistModel::removeSong(int index) {
    if (index >= 0 && static_cast<size_t>(index) < songs_.size()) {
        songs_.erase(songs_.begin() + index);
    }
}

const std::vector<SongInfo>& PlaylistModel::songs() const {
    return songs_;
}

} // namespace musicplayer
