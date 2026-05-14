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
# test_sqlite_database constructs QSqlQuery / QSqlDatabase values directly;
# infra keeps Qt6::Sql PRIVATE, so link it explicitly here.
target_link_libraries(test_sqlite_database PRIVATE Qt6::Sql)

# Integration tests
add_executable(test_audio_pipeline integration/test_audio_pipeline.cpp)
target_link_libraries(test_audio_pipeline PRIVATE
    musicplayer_domain
    musicplayer_infra
    GTest::gtest
    GTest::gtest_main
)
gtest_discover_tests(test_audio_pipeline)
