add_library(musicplayer_infra STATIC
    ffmpeg_decoder.cpp
    portaudio_output.cpp
    sqlite_database.cpp
    sqlite_song_repo.cpp
    sqlite_playlist_repo.cpp
    sqlite_settings_repo.cpp
    player_state_repo.cpp
    config_paths.cpp
    cover_cache.cpp
    library_importer.cpp
    metadata_extractor.cpp
)

target_include_directories(musicplayer_infra PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# musicplayer_domain stays PUBLIC because infra headers reference its
# interfaces (FfmpegDecoder : IAudioDecoder, PortAudioOutput : IAudioOutput).
# All third-party libs are PRIVATE — infra headers only forward-declare AV*,
# PaStream, etc.; the concrete includes live in .cpp.
target_link_libraries(musicplayer_infra
    PUBLIC
        musicplayer_domain
    PRIVATE
        Qt6::Core
        Qt6::Sql
        FFmpeg::avcodec
        FFmpeg::avformat
        FFmpeg::avutil
        FFmpeg::swresample
        PortAudio::PortAudio
)
