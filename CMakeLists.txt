cmake_minimum_required(VERSION 3.31)
project(musicplayer
    VERSION 0.1.0
    DESCRIPTION "A cross-platform desktop music player built with Qt6 and C++23"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)

option(BUILD_TESTING "Build tests" ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(cmake/CompilerWarnings.cmake)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Sql)
find_package(FFmpeg REQUIRED COMPONENTS AVCODEC AVFORMAT AVUTIL SWRESAMPLE)
find_package(PortAudio REQUIRED)

if(BUILD_TESTING)
    find_package(GTest REQUIRED)
    enable_testing()
    include(GoogleTest)
endif()

add_subdirectory(src)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
