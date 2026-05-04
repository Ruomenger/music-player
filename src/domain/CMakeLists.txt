add_library(musicplayer_domain STATIC
    song_info.cpp
    audio_engine.cpp
    lyric_parser.cpp
    playlist_model.cpp
    ring_buffer.cpp
)

target_include_directories(musicplayer_domain PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(musicplayer_domain PUBLIC
    musicplayer_common
)
