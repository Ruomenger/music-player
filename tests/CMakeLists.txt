enable_testing()
include(GoogleTest)

macro(add_domain_test name)
    add_executable(${name} domain/${name}.cpp)
    target_link_libraries(${name} PRIVATE
        musicplayer_domain
        GTest::gtest
        GTest::gtest_main
    )
    gtest_discover_tests(${name})
endmacro()

macro(add_infra_test name)
    add_executable(${name} infra/${name}.cpp)
    target_link_libraries(${name} PRIVATE
        musicplayer_infra
        musicplayer_domain
        GTest::gtest
        GTest::gtest_main
    )
    gtest_discover_tests(${name})
endmacro()

# Tests that touch Qt SQL / settings APIs need a live QCoreApplication; this
# variant of the macro provides one via tests/qt_test_main.cpp.
macro(add_qt_infra_test name)
    add_executable(${name} infra/${name}.cpp qt_test_main.cpp)
    target_link_libraries(${name} PRIVATE
        musicplayer_infra
        musicplayer_domain
        Qt6::Core
        GTest::gtest
    )
    gtest_discover_tests(${name})
endmacro()

add_domain_test(test_lyric_parser)
add_domain_test(test_lyric_matcher)
add_domain_test(test_music_scanner)
add_domain_test(test_play_queue)
add_domain_test(test_ring_buffer)

add_infra_test(test_ffmpeg_decoder)
add_infra_test(test_config_paths)
# test_config_paths uses QDir/QStandardPaths directly; infra no longer
# propagates Qt::Core (it's PRIVATE there), so link it explicitly here.
target_link_libraries(test_config_paths PRIVATE Qt6::Core)
add_qt_infra_test(test_sqlite_database)
add_qt_infra_test(test_sqlite_song_repo)
add_qt_infra_test(test_sqlite_playlist_repo)
add_qt_infra_test(test_sqlite_settings_repo)
add_qt_infra_test(test_player_state_repo)
add_qt_infra_test(test_cover_cache)
add_qt_infra_test(test_library_importer)
add_qt_infra_test(test_sqlite_play_history_repo)
# test_sqlite_database constructs QSqlQuery / QSqlDatabase values directly;
# infra keeps Qt6::Sql PRIVATE, so link it explicitly here.
target_link_libraries(test_sqlite_database PRIVATE Qt6::Sql)

# test_metadata_extractor needs no Qt runtime; FFmpeg only.
add_infra_test(test_metadata_extractor)

# Application-layer tests: PlaylistManager is a QObject with signals, so they
# need a QCoreApplication (already provided by qt_test_main.cpp).
macro(add_qt_app_test name)
    add_executable(${name} app/${name}.cpp qt_test_main.cpp)
    target_link_libraries(${name} PRIVATE
        musicplayer_app
        musicplayer_infra
        musicplayer_domain
        Qt6::Core
        Qt6::Test
        GTest::gtest
    )
    gtest_discover_tests(${name})
endmacro()

add_qt_app_test(test_playlist_manager)
add_qt_app_test(test_player_controller)
add_qt_app_test(test_lyric_manager)

# UI-layer tests run under a real QApplication on the offscreen platform plugin
# so they're headless-safe (CI / SSH). qt_widget_test_main.cpp sets up the env.
macro(add_qt_widget_test name)
    add_executable(${name} ui/${name}.cpp qt_widget_test_main.cpp)
    target_link_libraries(${name} PRIVATE
        musicplayer_ui
        musicplayer_app
        musicplayer_infra
        musicplayer_domain
        Qt6::Widgets
        Qt6::Test
        GTest::gtest
    )
    gtest_discover_tests(${name})
endmacro()

add_qt_widget_test(test_song_table_model)
add_qt_widget_test(test_control_bar)
add_qt_widget_test(test_cover_widget)
add_qt_widget_test(test_lyric_widget)
add_qt_widget_test(test_new_playlist_dialog)
add_qt_widget_test(test_playlist_sidebar)

# Integration tests
add_executable(test_audio_pipeline integration/test_audio_pipeline.cpp)
target_link_libraries(test_audio_pipeline PRIVATE
    musicplayer_domain
    musicplayer_infra
    GTest::gtest
    GTest::gtest_main
)
gtest_discover_tests(test_audio_pipeline)
