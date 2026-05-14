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

| 平台 | Triplet | 平台 preset | CI preset |
|------|---------|------------|-----------|
| Fedora 43 (x64) | `x64-linux` | `linux-x64-{debug,release}` | `ci-debug` / `ci-asan` / `ci-coverage` |
| macOS 15 (ARM M2) | `arm64-osx` | `macos-arm64-{debug,release}` | 同上 |
| Windows (x64) | `x64-windows` | (待添加) | 同上 |

默认使用动态链接 triplet。如需静态链接, 使用 `x64-linux-static` 等，但需要注意 Qt 的 LGPL 协议限制。

### CI preset 与平台 preset 的差别

- 平台 preset (`linux-x64-*` / `macos-arm64-*`) 注入 `CMAKE_TOOLCHAIN_FILE`
  指向 `$HOME/vcpkg`，并在 macOS 上锁死 brew LLVM clang。给开发机用。
- CI preset (`ci-debug` / `ci-asan` / `ci-coverage`) 不假设 vcpkg 路径
  （CI 用 GitHub Actions cache 在别处装 vcpkg），用 host 默认编译器。
  `ci-asan` 加 `-fsanitize=address,undefined`，`ci-coverage` 加 `--coverage`。
  这三个 preset 把 `binaryDir` 改到 `build/${presetName}` 而非 `build`，
  让 sanitizer/coverage 的产物与本地 debug 构建并存。

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
cmake_minimum_required(VERSION 4.3.2)
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
# - Core: 基础设施 (QString / QObject 等)
# - Widgets: UI 层
# - Sql: 数据库仓库 (SqliteDatabase 等)
# - Test: QSignalSpy / QTest (UI 测试)
# - LinguistTools: 提供 qt6_add_translations + lrelease (Phase 8 i18n)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Sql Test LinguistTools)
# vcpkg 的 FFmpeg 提供 FindFFMPEG.cmake (变量模式, 非 target)
find_package(FFmpeg REQUIRED)
find_package(PortAudio REQUIRED)
find_package(GTest REQUIRED)

# 子目录
add_subdirectory(src)
add_subdirectory(tests)
```

## 依赖分模块构建

四层分别建 STATIC 库, 最后 `add_executable(musicplayer)` 链 UI 层；每层
CMakeLists 单独一个文件（`src/<layer>/CMakeLists.txt`）。下面是简化骨架，
具体文件列表参见 `project-structure.md` —— 每加一层新源文件都要在该层的
CMakeLists 里加一行。

```cmake
# Domain 层 - 无 Qt 依赖 (src/domain/CMakeLists.txt)
add_library(musicplayer_domain STATIC
    audio_decoder_info.cpp audio_engine.cpp iaudio_decoder.cpp iaudio_output.cpp
    lyric_matcher.cpp lyric_parser.cpp music_scanner.cpp play_queue.cpp
    playlist.cpp playlist_model.cpp ring_buffer.cpp song_info.cpp
    # audio_extensions.h 是 header-only, 不在源列表
)
target_link_libraries(musicplayer_domain PUBLIC musicplayer_common)

# Infrastructure 层 (src/infra/CMakeLists.txt)
# Qt::Core / Qt::Sql / FFmpeg / PortAudio 标 PRIVATE, 不让 Qt 类型外溢到上层 .h
add_library(musicplayer_infra STATIC
    config_paths.cpp cover_cache.cpp ffmpeg_decoder.cpp library_importer.cpp
    metadata_extractor.cpp player_state_repo.cpp portaudio_output.cpp
    sqlite_database.cpp sqlite_play_history_repo.cpp sqlite_playlist_repo.cpp
    sqlite_settings_repo.cpp sqlite_song_repo.cpp
)
target_link_libraries(musicplayer_infra
    PUBLIC  musicplayer_domain
    PRIVATE Qt6::Core Qt6::Sql FFmpeg::avcodec FFmpeg::avformat FFmpeg::avutil
            FFmpeg::swresample PortAudio::PortAudio
)

# Application 层 (src/app/CMakeLists.txt)
add_library(musicplayer_app STATIC
    language_manager.cpp lyric_manager.cpp player_controller.cpp playlist_manager.cpp
)
target_link_libraries(musicplayer_app PUBLIC musicplayer_domain musicplayer_infra Qt6::Core)

# UI 层 + 主可执行文件 (src/ui/CMakeLists.txt + src/CMakeLists.txt)
add_library(musicplayer_ui STATIC
    control_bar.cpp cover_widget.cpp lyric_widget.cpp main_window.cpp
    new_playlist_dialog.cpp playlist_sidebar.cpp playlist_widget.cpp
    settings_dialog.cpp song_table_model.cpp
)
target_link_libraries(musicplayer_ui PUBLIC musicplayer_app Qt6::Widgets)

# src/CMakeLists.txt 里:
add_executable(musicplayer main.cpp)
target_link_libraries(musicplayer PRIVATE musicplayer_ui compiler_warnings)
qt6_add_translations(musicplayer
    TS_FILES ${CMAKE_SOURCE_DIR}/resources/translations/musicplayer_zh_CN.ts
    RESOURCE_PREFIX "/translations"
    LUPDATE_OPTIONS "-no-obsolete"
)
```

### tests/CMakeLists.txt

实际 tests/CMakeLists.txt 通过四个 macro 把"测试目录 / 链接哪些库 / 用哪个
main" 三件事打包起来，调用站点只剩 `add_xxx_test(name)`：

```cmake
# Domain-only: 纯 gtest_main, 无 Qt 依赖。
macro(add_domain_test name)
    add_executable(${name} domain/${name}.cpp)
    target_link_libraries(${name} PRIVATE musicplayer_domain GTest::gtest GTest::gtest_main)
    gtest_discover_tests(${name})
endmacro()

# Infra: 还是 gtest_main, 但链 infra 库 (拉入 Qt::Sql 等 PRIVATE 传不出来的依赖)
macro(add_infra_test name) ... endmacro()

# Qt::Core 测试: 走 qt_test_main.cpp, 在 GTest main 里建一个 QCoreApplication。
# QSqlDatabase / QStandardPaths 这些"需要 QCoreApplication" 的 API 走这条路。
macro(add_qt_infra_test name)
    add_executable(${name} infra/${name}.cpp qt_test_main.cpp)
    target_link_libraries(${name} PRIVATE musicplayer_infra musicplayer_domain
                                          Qt6::Core GTest::gtest)
    gtest_discover_tests(${name})
endmacro()

# Application 层: 把 musicplayer_app 链进来; 否则同 Qt-infra
macro(add_qt_app_test name) ... endmacro()

# UI 层: 用 qt_widget_test_main.cpp 建一个 QApplication, 并通过 setenv()
# 强制 QT_QPA_PLATFORM=offscreen, 让测试在无显示环境 (CI / SSH) 也能跑。
macro(add_qt_widget_test name) ... endmacro()
```

调用举例（实际文件里类似的有 30 多条）：

```cmake
add_domain_test(test_play_queue)
add_qt_infra_test(test_sqlite_song_repo)
add_qt_app_test(test_player_controller)
add_qt_widget_test(test_control_bar)
```

## i18n: qt6_add_translations

`src/CMakeLists.txt` 顶层在创建 `musicplayer` 可执行之后调一次
`qt6_add_translations` (Qt 6.7+, 需 `LinguistTools` 组件)：

```cmake
qt6_add_translations(musicplayer
    TS_FILES
        ${CMAKE_SOURCE_DIR}/resources/translations/musicplayer_zh_CN.ts
    RESOURCE_PREFIX "/translations"
    LUPDATE_OPTIONS "-no-obsolete"
)
```

- `lrelease` 自动跑，输出 `musicplayer_zh_CN.qm`
- 生成的 .qm 自动打进 Qt 资源系统，加载路径就是
  `:/translations/musicplayer_zh_CN.qm`（这正是 `LanguageManager::setLanguage`
  里 `translator_.load(...)` 用的路径）
- `RESOURCE_PREFIX` 与 `QM_FILES_OUTPUT_VARIABLE` 互斥，二选一
- `-no-obsolete` 让 lupdate 删除已被移除的源字符串而不是标 `<obsolete>`

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
