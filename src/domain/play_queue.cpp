#include "play_queue.h"

#include <algorithm>
#include <utility>

namespace musicplayer {

PlayQueue::PlayQueue() : rng_(std::random_device{}()) {}

bool PlayQueue::setSongs(std::vector<int> songIds) {
    songs_ = std::move(songIds);
    cursor_ = 0;
    shuffleQueue_.clear();  // any prior shuffle no longer matches the contents
    if (mode_ == PlayMode::Random && !songs_.empty())
        reshuffle();
    return !songs_.empty();
}

void PlayQueue::setMode(PlayMode mode) {
    if (mode == mode_)
        return;
    const std::optional<int> currentId = current();
    mode_ = mode;
    if (mode_ == PlayMode::Random) {
        if (!songs_.empty()) {
            reshuffle();
            // Preserve "the song the user was listening to" — find it in the
            // freshly-shuffled queue and place the cursor there so calling
            // advanceManual right after switching modes doesn't skip
            // whatever's playing right now.
            if (currentId.has_value()) {
                const auto it = std::ranges::find(shuffleQueue_, *currentId);
                if (it != shuffleQueue_.end())
                    cursor_ = static_cast<std::size_t>(it - shuffleQueue_.begin());
            }
        }
    } else if (currentId.has_value()) {
        cursor_ = indexOf(*currentId);
    }
}

bool PlayQueue::setCurrent(int songId) {
    if (mode_ == PlayMode::Random) {
        if (shuffleQueue_.empty())
            return false;
        const auto it = std::ranges::find(shuffleQueue_, songId);
        if (it == shuffleQueue_.end())
            return false;
        cursor_ = static_cast<std::size_t>(it - shuffleQueue_.begin());
        return true;
    }
    const std::size_t idx = indexOf(songId);
    if (idx >= songs_.size())
        return false;
    cursor_ = idx;
    return true;
}

std::optional<int> PlayQueue::current() const {
    if (mode_ == PlayMode::Random) {
        if (shuffleQueue_.empty() || cursor_ >= shuffleQueue_.size())
            return std::nullopt;
        return shuffleQueue_[cursor_];
    }
    if (songs_.empty() || cursor_ >= songs_.size())
        return std::nullopt;
    return songs_[cursor_];
}

std::optional<int> PlayQueue::advanceNatural() {
    if (songs_.empty())
        return std::nullopt;

    switch (mode_) {
        case PlayMode::Single:
            // Stay on the same track; caller re-plays the current song.
            return current();
        case PlayMode::Sequential:
            if (cursor_ + 1 >= songs_.size())
                return std::nullopt;  // stop at end of list
            ++cursor_;
            return songs_[cursor_];
        case PlayMode::ListLoop:
            cursor_ = (cursor_ + 1) % songs_.size();
            return songs_[cursor_];
        case PlayMode::Random:
            if (shuffleQueue_.empty())
                reshuffle();
            if (cursor_ + 1 >= shuffleQueue_.size()) {
                reshuffle();
                cursor_ = 0;
            } else {
                ++cursor_;
            }
            return shuffleQueue_[cursor_];
    }
    return std::nullopt;
}

std::optional<int> PlayQueue::advanceManual() {
    if (songs_.empty())
        return std::nullopt;

    // Single behaves like Sequential when the user explicitly asks for next —
    // the user wants to *change* tracks, not replay the current one.
    switch (mode_) {
        case PlayMode::Single:
        case PlayMode::Sequential:
            if (cursor_ + 1 >= songs_.size())
                return std::nullopt;
            ++cursor_;
            return songs_[cursor_];
        case PlayMode::ListLoop:
            cursor_ = (cursor_ + 1) % songs_.size();
            return songs_[cursor_];
        case PlayMode::Random:
            if (shuffleQueue_.empty())
                reshuffle();
            if (cursor_ + 1 >= shuffleQueue_.size()) {
                reshuffle();
                cursor_ = 0;
            } else {
                ++cursor_;
            }
            return shuffleQueue_[cursor_];
    }
    return std::nullopt;
}

std::optional<int> PlayQueue::retreatManual() {
    if (songs_.empty())
        return std::nullopt;

    switch (mode_) {
        case PlayMode::Single:
        case PlayMode::Sequential:
            if (cursor_ == 0)
                return std::nullopt;
            --cursor_;
            return songs_[cursor_];
        case PlayMode::ListLoop:
            cursor_ = cursor_ == 0 ? songs_.size() - 1 : cursor_ - 1;
            return songs_[cursor_];
        case PlayMode::Random:
            if (shuffleQueue_.empty())
                return std::nullopt;
            if (cursor_ == 0)
                return std::nullopt;  // can't go past start of shuffle history
            --cursor_;
            return shuffleQueue_[cursor_];
    }
    return std::nullopt;
}

void PlayQueue::seedForTesting(std::uint_fast32_t seed) {
    rng_.seed(seed);
    if (mode_ == PlayMode::Random && !songs_.empty())
        reshuffle();
}

void PlayQueue::reshuffle() {
    shuffleQueue_ = songs_;
    std::ranges::shuffle(shuffleQueue_, rng_);
    cursor_ = 0;
}

std::size_t PlayQueue::indexOf(int songId) const {
    const auto it = std::ranges::find(songs_, songId);
    return it == songs_.end() ? songs_.size() : static_cast<std::size_t>(it - songs_.begin());
}

}  // namespace musicplayer
