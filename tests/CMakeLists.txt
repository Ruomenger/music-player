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

add_domain_test(test_lyric_parser)
add_domain_test(test_ring_buffer)

add_infra_test(test_ffmpeg_decoder)
add_infra_test(test_config_paths)

# Integration tests
add_executable(test_audio_pipeline integration/test_audio_pipeline.cpp)
target_link_libraries(test_audio_pipeline PRIVATE
    musicplayer_domain
    musicplayer_infra
    GTest::gtest
    GTest::gtest_main
)
gtest_discover_tests(test_audio_pipeline)
