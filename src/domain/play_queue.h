#pragma once

#include <cstddef>
#include <optional>
#include <random>
#include <vector>
#include "song_info.h"

namespace musicplayer {

// Encapsulates the "what plays next" decision so PlayerController stays
// declarative. The queue holds an ordered list of song ids and a cursor;
// `mode` (the user-facing PlayMode) controls how the cursor advances:
//
//   Mode        natural-end       user-next        user-prev      end-of-list
//   Single      replay current    list next        list prev      single doesn't matter
//   Sequential  list next         list next        list prev      stop (nullopt)
//   ListLoop    list next         list next        list prev      wrap to first
//   Random      shuffle queue     shuffle queue    shuffle prev   re-shuffle
//
// Random uses Fisher-Yates over the song id list, building a one-shot
// shuffleQueue_. The cursor walks that queue; running off the end re-shuffles.
// Any list mutation invalidates the shuffle (next setMode(Random) re-builds).
class PlayQueue {
public:
    PlayQueue();

    // Replaces the queue contents. Resets the cursor to 0 and invalidates any
    // existing shuffle. Returns false if the queue is empty afterwards.
    bool setSongs(std::vector<int> songIds);
    [[nodiscard]] const std::vector<int>& songs() const { return songs_; }
    [[nodiscard]] std::size_t size() const { return songs_.size(); }
    [[nodiscard]] bool empty() const { return songs_.empty(); }

    void setMode(PlayMode mode);
    [[nodiscard]] PlayMode mode() const { return mode_; }

    // Jump the cursor to a specific song. Returns false if songId is not in
    // the queue. For Random mode, the cursor's position inside shuffleQueue_
    // is set to wherever this id appears, so the user's existing shuffle
    // history is preserved.
    bool setCurrent(int songId);
    [[nodiscard]] std::optional<int> current() const;

    // Advance logic. Each returns the new current song id, or nullopt if the
    // mode policy says "stop" (Sequential past last). All three update the
    // internal cursor on success.
    std::optional<int> advanceNatural();  // current track ended on its own
    std::optional<int> advanceManual();   // user pressed next
    std::optional<int> retreatManual();   // user pressed previous

    // Override the RNG seed; used by tests so shuffle output is deterministic.
    void seedForTesting(std::uint_fast32_t seed);

private:
    void reshuffle();
    [[nodiscard]] std::size_t indexOf(int songId) const;

    std::vector<int> songs_;
    std::vector<int> shuffleQueue_;
    std::size_t cursor_ = 0;  // index into songs_ (or shuffleQueue_ in Random mode)
    PlayMode mode_ = PlayMode::Sequential;
    std::mt19937 rng_;
};

}  // namespace musicplayer
