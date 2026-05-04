add_subdirectory(common)
add_subdirectory(domain)
add_subdirectory(infra)
add_subdirectory(app)
add_subdirectory(ui)

add_executable(musicplayer
    main.cpp
)

target_link_libraries(musicplayer PRIVATE
    musicplayer_ui
    compiler_warnings
)
