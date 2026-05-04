# CMake + vcpkg 构建系统

## 概述

所有第三方依赖通过 vcpkg manifest mode 统一管理，包括 Qt6。不使用系统包管理器安装的 Qt。

## vcpkg 配置

### vcpkg.json (manifest)

```json
{
  "name": "music-player",
  "version": "0.1.0",
  "dependencies": [
    {
      "name": "qtbase",
      "default-features": false,
      "features": [
        "concurrent", "dbus", "egl", "fontconfig",
        "freetype", "gui", "harfbuzz", "jpeg",
        "network", "opengl", "png", "sql", "sql-sqlite",
        "thread", "wayland", "widgets", "zstd"
      ]
    },
    "ffmpeg",
    "portaudio",
    "gtest"
  ]
}
```

> 注意: Linux 上显式禁用 X11 相关 feature (`xcb`, `xcb-sm`, `xcb-xlib`, `xrender`)，启用 `wayland`。macOS/Windows 上这些 feature 自动被平台条件过滤，无需额外处理。

### 系统依赖 (Linux)

vcpkg 会从源码编译大多数依赖，但部分库需要系统提供开发包:

```bash
# Fedora
sudo dnf install -y nasm wayland-devel wayland-protocols-devel libxkbcommon-devel

# Ubuntu/Debian
sudo apt install -y nasm libwayland-dev wayland-protocols libxkbcommon-dev
```

### Triplet 选择

| 平台 | Triplet |
|------|---------|
| Fedora 43 (x64) | `x64-linux` |
| macOS 15 (ARM M2) | `arm64-osx` |
| Windows (x64) | `x64-windows` |

默认使用动态链接 triplet。如需静态链接, 使用 `x64-linux-static` 等，但需要注意 Qt 的 LGPL 协议限制。

### 配置步骤

```bash
# 克隆 vcpkg (一次性)
git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh  # 或 .bat on Windows

# 构建项目
cmake -B build \
  -DCMAKE_TOOLCHAIN_FILE=~/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DVCPKG_TARGET_TRIPLET=x64-linux \
  -DCMAKE_BUILD_TYPE=Debug

cmake --build build
```

## CMake 顶层结构

```cmake
cmake_minimum_required(VERSION 3.31)
project(musicplayer VERSION 0.1.0 LANGUAGES CXX)

# C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Qt 必需
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)  # 不使用 .ui 文件

# 编译选项
include(cmake/CompilerWarnings.cmake)

# 查找依赖
find_package(Qt6 COMPONENTS Widgets Sql REQUIRED)
# vcpkg 的 FFmpeg 提供 FindFFMPEG.cmake (变量模式, 非 target)
find_package(FFMPEG REQUIRED)
find_package(portaudio REQUIRED)
find_package(GTest REQUIRED)

# 子目录
add_subdirectory(src)
add_subdirectory(tests)
```

## 依赖分模块构建

### src/CMakeLists.txt

```cmake
# Domain 层 - 无 Qt 依赖
add_library(musicplayer_domain STATIC
    domain/audio_engine.cpp
    domain/lyric_parser.cpp
    domain/playlist_model.cpp
)
target_include_directories(musicplayer_domain PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Infrastructure 层
add_library(musicplayer_infra STATIC
    infra/ffmpeg_decoder.cpp
    infra/portaudio_output.cpp
    infra/sqlite_song_repo.cpp
    infra/sqlite_playlist_repo.cpp
    infra/sqlite_settings_repo.cpp
    infra/config_paths.cpp
)
target_include_directories(musicplayer_infra PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(musicplayer_infra PUBLIC
    musicplayer_domain
    FFMPEG::FFMPEG
    portaudio
    Qt6::Sql
)

# Application 层
add_library(musicplayer_app STATIC
    app/player_controller.cpp
    app/playlist_manager.cpp
    app/lyric_manager.cpp
)
target_link_libraries(musicplayer_app PUBLIC
    musicplayer_domain
    musicplayer_infra
    Qt6::Core
)

# UI 层 + 主可执行文件
add_executable(musicplayer
    main.cpp
    ui/main_window.cpp
    ui/playlist_widget.cpp
    ui/lyric_widget.cpp
    ui/control_bar.cpp
    ui/cover_widget.cpp
    ui/settings_dialog.cpp
)
target_link_libraries(musicplayer PRIVATE
    musicplayer_app
    Qt6::Widgets
)
```

### tests/CMakeLists.txt

```cmake
# 每个测试一个可执行文件
add_executable(test_lyric_parser
    domain/test_lyric_parser.cpp
)
target_link_libraries(test_lyric_parser PRIVATE
    musicplayer_domain
    GTest::gtest
    GTest::gtest_main
)

add_executable(test_playlist_model
    domain/test_playlist_model.cpp
)
target_link_libraries(test_playlist_model PRIVATE
    musicplayer_domain
    GTest::gtest
    GTest::gtest_main
)

# ... 更多测试

enable_testing()
include(GoogleTest)
gtest_discover_tests(test_lyric_parser)
gtest_discover_tests(test_playlist_model)
# ...
```

## VSCode 配置

### .vscode/cmake-kits.json

```json
[
  {
    "name": "musicplayer-local",
    "cmakeSettings": {
      "CMAKE_TOOLCHAIN_FILE": "$env{HOME}/vcpkg/scripts/buildsystems/vcpkg.cmake",
      "VCPKG_TARGET_TRIPLET": "x64-linux",
      "BUILD_TESTING": "ON"
    }
  }
]
```

### .vscode/settings.json

```json
{
  "cmake.configureArgs": [
    "-DCMAKE_TOOLCHAIN_FILE=~/vcpkg/scripts/buildsystems/vcpkg.cmake",
    "-DVCPKG_TARGET_TRIPLET=x64-linux"
  ],
  "cmake.buildDirectory": "${workspaceFolder}/build",
  "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools"
}
```

## CLion 配置

1. Settings → Build, Execution, Deployment → CMake
2. CMake options: `-DCMAKE_TOOLCHAIN_FILE=$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-linux`
3. Build directory: `build`

## CI 构建矩阵 (规划中)

| 平台 | 编译器 | CMake Preset |
|------|--------|-------------|
| ubuntu-latest (GitHub Actions) | g++ 15 | `linux-gcc-debug` |
| macos-15 (GitHub Actions) | clang 20 | `macos-clang-debug` |
| windows-latest (GitHub Actions) | MSVC 2022 | `windows-msvc-debug` |
