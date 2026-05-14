add_library(musicplayer_app STATIC
    player_controller.cpp
    playlist_manager.cpp
    lyric_manager.cpp
    language_manager.cpp
)

target_include_directories(musicplayer_app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(musicplayer_app PUBLIC
    musicplayer_domain
    musicplayer_infra
    Qt6::Core
)
