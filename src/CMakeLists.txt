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

# i18n: compile .ts → .qm and embed via the auto-generated qrc so
# QTranslator can load ":/i18n/musicplayer_<locale>.qm" at runtime.
# qt6_add_translations is a Qt 6.7+ helper that wraps lrelease and the
# resource bundling — see docs/i18n.md for the design rationale.
qt6_add_translations(musicplayer
    TS_FILES
        ${CMAKE_SOURCE_DIR}/resources/translations/musicplayer_zh_CN.ts
    RESOURCE_PREFIX "/translations"
    LUPDATE_OPTIONS "-no-obsolete"
)
