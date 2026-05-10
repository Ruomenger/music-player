# CMake + vcpkg 构建系统

## 概述

第三方依赖通过 vcpkg manifest mode 统一管理。Qt6 例外，使用系统包管理器安装以保证工具链兼容性（vcpkg 编译 Qt 太重，且对 LGPL 静态链接处理也更复杂）。

## vcpkg 配置

### vcpkg.json (manifest)

```json
{
  "name": "music-player",
  "version": "0.1.0",
  "dependencies": [
    "ffmpeg",
    "gtest",
    "portaudio"
  ]
}
```

> Qt6 通过系统包管理器（brew / dnf）安装，不纳入 vcpkg。其余依赖（ffmpeg / gtest / portaudio）由 vcpkg 在首次 configure 时自动编译。

### 系统依赖

#### Linux (Fedora x64)

```bash
# Qt6 开发包
sudo dnf install -y qt6-qtbase-devel qt6-qtwayland

# 构建工具 + vcpkg 编译 ffmpeg / portaudio 需要
sudo dnf install -y ninja-build nasm

# Wayland 平台插件运行时依赖
sudo dnf install -y wayland-devel wayland-protocols-devel libxkbcommon-devel
```

#### macOS (Apple Silicon)

```bash
# Qt6 + 构建工具
brew install qt ninja nasm

# C++23 工具链 (Apple Clang 对 std::format / ranges 支持滞后)
brew install llvm
```

> Homebrew 的 Qt 是 keg-only，CMakePresets 已通过 `CMAKE_PREFIX_PATH=/opt/homebrew/opt/qt` 注入路径，无需手动 export。
>
> macOS preset 默认强制使用 brew LLVM 的 clang (`/opt/homebrew/opt/llvm/bin/clang++`) 而非 Apple Clang，原因是 C++23 / `<format>` / `<print>` / ranges 等特性在系统编译器上仍有缺口。CI 使用各平台默认编译器（不继承 macOS preset），不受影响。如果想换回 Apple Clang，在 `CMakeUserPresets.json` 里覆盖 `CMAKE_C_COMPILER` / `CMAKE_CXX_COMPILER` 即可。

### Triplet 选择

| 平台 | Triplet | Preset |
|------|---------|--------|
| Fedora 43 (x64) | `x64-linux` | `linux-x64-{debug,release}` |
| macOS 15 (ARM M2) | `arm64-osx` | `macos-arm64-{debug,release}` |
| Windows (x64) | `x64-windows` | (待添加) |

默认使用动态链接 triplet。如需静态链接, 使用 `x64-linux-static` 等，但需要注意 Qt 的 LGPL 协议限制。

### 配置步骤

```bash
# 克隆 vcpkg (一次性)
git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh  # 或 .bat on Windows

# 通过 CMake Presets 配置并构建（自动选对 triplet 和 Qt 路径）
cmake --preset linux-x64-debug         # Linux
cmake --preset macos-arm64-debug       # macOS
cmake --build --preset linux-x64-debug

# 查看当前 host 上可用的 presets
cmake --list-presets

# 如果 vcpkg 不在 ~/vcpkg，可临时覆盖
cmake --preset linux-x64-debug \
  -DCMAKE_TOOLCHAIN_FILE=/your/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
```

> 所有 preset 共享同一个 `build/` 目录。切换平台或 build type 之前请先 `rm -rf build`，否则 CMake 会因 cache 不匹配报错。

## CMake 顶层结构

```cmake
cmake_minimum_required(VERSION 3.28)
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
find_package(PortAudio REQUIRED)
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
    PortAudio::PortAudio
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

## IDE 配置

VSCode 和 CLion 都支持 CMake Presets，直接选预设即可，不需要再手填 toolchain / triplet。

### VSCode

`.vscode/settings.json` 已配置 `"cmake.useCMakePresets": "always"`。打开项目后 CMake Tools 会让你选预设（`linux-x64-debug` / `macos-arm64-debug` 等，按 host 自动过滤）。

`.vscode/cmake-kits.json` 仅提供编译器入口（Linux GCC / macOS Clang），平台相关配置完全由 preset 决定。

### CLion

1. Settings → Build, Execution, Deployment → CMake
2. 切到 **Presets** 标签页，启用对应平台的 preset
3. Build directory 由 preset 决定（统一为 `build`）

## CI 构建矩阵 (规划中)

| 平台 | 编译器 | CMake Preset |
|------|--------|-------------|
| ubuntu-latest (GitHub Actions) | g++ 15 | `linux-gcc-debug` |
| macos-15 (GitHub Actions) | clang 20 | `macos-clang-debug` |
| windows-latest (GitHub Actions) | MSVC 2022 | `windows-msvc-debug` |
