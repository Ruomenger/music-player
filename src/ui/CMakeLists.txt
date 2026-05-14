add_library(musicplayer_ui STATIC
    main_window.cpp
    playlist_widget.cpp
    lyric_widget.cpp
    control_bar.cpp
    cover_widget.cpp
    settings_dialog.cpp
    song_table_model.cpp
)

target_include_directories(musicplayer_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(musicplayer_ui PUBLIC
    musicplayer_app
    Qt6::Widgets
)
