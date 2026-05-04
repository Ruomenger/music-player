add_library(musicplayer_infra STATIC
    ffmpeg_decoder.cpp
    portaudio_output.cpp
    sqlite_song_repo.cpp
    sqlite_playlist_repo.cpp
    sqlite_settings_repo.cpp
    config_paths.cpp
)

target_include_directories(musicplayer_infra PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(musicplayer_infra PUBLIC
    musicplayer_domain
    Qt6::Core
    Qt6::Sql
    FFmpeg::avcodec
    FFmpeg::avformat
    FFmpeg::avutil
    FFmpeg::swresample
    PortAudio::PortAudio
)
