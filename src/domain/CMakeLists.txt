add_library(musicplayer_domain STATIC
    audio_decoder_info.cpp
    iaudio_decoder.cpp
    iaudio_output.cpp
    song_info.cpp
    playlist.cpp
    audio_engine.cpp
    lyric_matcher.cpp
    lyric_parser.cpp
    music_scanner.cpp
    play_queue.cpp
    playlist_model.cpp
    ring_buffer.cpp
)

target_include_directories(musicplayer_domain PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(musicplayer_domain PUBLIC
    musicplayer_common
)
