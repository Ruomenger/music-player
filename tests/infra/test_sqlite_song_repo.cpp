#include <gtest/gtest.h>
#include "song_info.h"
#include "sqlite_song_repo.h"

#include <stdexcept>

using musicplayer::SongInfo;
using musicplayer::SqliteSongRepo;

// Phase 2 stubs must throw, not silently return falsy. Returning false / {} /
// nullopt would let callers branch into a "DB unavailable, run degraded" code
// path that doesn't actually exist yet, hiding the missing implementation.
TEST(SqliteSongRepo, AllMethodsThrowUntilImplemented) {
    SqliteSongRepo repo;
    EXPECT_THROW(repo.initialize("/tmp/x.db"), std::logic_error);
    EXPECT_THROW((void)repo.getAllSongs(), std::logic_error);
    EXPECT_THROW((void)repo.getSongById(1), std::logic_error);
    EXPECT_THROW((void)repo.insertSong(SongInfo{}), std::logic_error);
    EXPECT_THROW((void)repo.removeSong(1), std::logic_error);
}
