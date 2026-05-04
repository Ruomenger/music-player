# Music Player

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![Qt](https://img.shields.io/badge/Qt-6.10%2B-green.svg)](https://www.qt.io/)
[![CMake](https://img.shields.io/badge/CMake-3.31%2B-green.svg)](https://cmake.org/)

跨平台桌面音乐播放器，使用 Qt6 + C++23 + CMake 构建，支持 Windows / macOS / Linux。

界面和功能参考网易云音乐，纯 QtWidgets 代码实现，不使用 QML/UI 文件。

## 功能特性

- 播放/暂停/停止/上一曲/下一曲/进度拖拽
- 播放模式：单曲循环、顺序播放、列表循环、随机播放
- 歌单管理（创建/编辑/删除/拖拽排序）
- 歌词显示（LRC 格式，同名自动加载，逐行高亮滚动）
- 封面显示（嵌入或同名图片）
- 主流音频格式支持（MP3 / FLAC / WAV / AAC / OGG / OPUS 等）
- 配置管理（音量、语言、输出设备等，持久化存储）
- 中/英文界面切换
- 系统托盘支持

## 技术栈

| 类别 | 选型 |
|------|------|
| 语言 | C++23 |
| UI | Qt 6.10+ (QtWidgets) |
| 构建 | CMake 3.31+ |
| 包管理 | vcpkg (manifest mode) |
| 音频解码 | FFmpeg |
| 音频输出 | PortAudio |
| 数据库 | SQLite3 (via Qt SQL) |
| 测试 | GoogleTest |
| 国际化 | Qt Linguist |

## 开发环境

| 平台 | 编译器 | 已验证 |
|------|--------|--------|
| Fedora 43 | g++ 15 | ✓ |
| macOS 15 (M2) | clang 20+ | ✓ |
| Windows | MSVC 2022+ | 待验证 |

IDE: VSCode / CLion

## 快速开始

### 前置依赖

- CMake 3.31+
- C++23 编译器 (g++15 / clang 20+ / MSVC 2022+)
- [vcpkg](https://github.com/microsoft/vcpkg) (管理 ffmpeg / portaudio / gtest)
- Qt6 通过系统包管理器安装

### 安装依赖

```bash
# Fedora (x64 Linux)
sudo dnf install -y qt6-qtbase-devel qt6-qtwayland portaudio-devel nasm ninja-build

# macOS (Apple Silicon)
brew install qt portaudio nasm ninja

# vcpkg (两平台通用，路径默认 ~/vcpkg)
git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh
```

### 构建（CMake Presets）

项目通过 `CMakePresets.json` 提供平台预设，无需手填 triplet：

```bash
# Linux x64
cmake --preset linux-x64-debug
cmake --build --preset linux-x64-debug

# macOS arm64
cmake --preset macos-arm64-debug
cmake --build --preset macos-arm64-debug
```

可用预设：`linux-x64-{debug,release}`、`macos-arm64-{debug,release}`。
查看当前平台可用预设：`cmake --list-presets`。

> 切换 preset 前请先清理：`rm -rf build`（所有 preset 共用 `build/` 目录）。
> 如果 vcpkg 不在 `~/vcpkg`，可通过环境变量覆盖：`VCPKG_ROOT=/path/to/vcpkg cmake --preset ...`，或在配置时用 `-DCMAKE_TOOLCHAIN_FILE=...` 覆盖。

### 运行

```bash
./build/musicplayer
```

### 运行测试

```bash
ctest --preset linux-x64-debug      # 或 macos-arm64-debug
# 或
ctest --test-dir build --output-on-failure
```

## 项目结构

```
music-player/
├── cmake/              # CMake 模块
├── docs/               # 设计文档
├── src/
│   ├── app/            # Application Layer (控制器)
│   ├── ui/             # UI Layer (Qt Widgets)
│   ├── domain/         # Domain Layer (纯 C++ 业务逻辑)
│   ├── infra/          # Infrastructure Layer (FFmpeg/PortAudio/SQLite 封装)
│   └── common/         # 通用工具
├── tests/              # 单元测试和集成测试
├── testdata/           # 测试用音频/歌词文件
└── resources/          # 图标、翻译文件
```

详见 [docs/project-structure.md](docs/project-structure.md)

## 架构

四层分层架构，Domain 层不依赖 Qt，可独立用 gtest 测试：

```
UI Layer (Qt Widgets)
  └── Application Layer (信号/槽协调)
        └── Domain Layer (纯 C++, 业务逻辑)
              └── Infrastructure Layer (FFmpeg/PortAudio/SQLite)
```

详见 [docs/architecture.md](docs/architecture.md)

## 设计文档

- [项目总览](docs/project-overview.md)
- [架构设计](docs/architecture.md)
- [项目结构](docs/project-structure.md)
- [构建系统](docs/build-system.md)
- [数据库设计](docs/database-schema.md)
- [音频管线](docs/audio-pipeline.md)
- [国际化](docs/i18n.md)
- [开发进度](docs/todo.md)

## 开源协议

[GNU General Public License v3.0](LICENSE)
