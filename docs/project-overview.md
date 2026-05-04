# Music Player - 项目总览

## 项目简介

跨平台桌面音乐播放器，参考网易云音乐设计，支持 Win/Mac/Linux。

## 技术栈

| 类别 | 选型 |
|------|------|
| 语言标准 | C++23 |
| UI 框架 | Qt 6.10+ (QtWidgets, 纯代码, 不使用 QML/UI 文件) |
| 构建系统 | CMake 3.31+ |
| 包管理 | vcpkg (manifest mode, ffmpeg/gtest); Qt6/PortAudio 使用系统包管理器 |
| 音频解码 | FFmpeg |
| 音频输出 | PortAudio |
| 数据库 | SQLite3 (通过 Qt SQL 模块) |
| 测试框架 | GoogleTest (gtest) |
| 国际化 | Qt Linguist (ts/qm), 支持中/英文 |
| 版本管理 | Git |

## 开发环境

| 平台 | 编译器 | CMake | Qt |
|------|--------|-------|-----|
| Fedora 43 | g++ 15 | 3.31+ | 6.10+ (系统包) |
| macOS 15 (M2) | clang 20+ | 3.31+ | 6.10+ (`brew install qt`) |
| Windows | MSVC 2022+ | 3.31+ | 6.10+ (官方在线安装器或 vcpkg) |

> **关于版本下限**：
> - Qt 6.10 / CMake 3.31 是当前主流个人开发环境实测可用的版本，但偏新。
> - **Ubuntu 24.04 LTS / Debian 12** 仓库自带 Qt 6.4，需要走 PPA、源码编译，或用 vcpkg 装 Qt。
> - 如果想兼容 LTS 发行版，可降到 Qt 6.7 (i18n `qt6_add_translations` 现代 API 的最低版本) + CMake 3.28；本项目当前不强求。

### IDE 支持

- **VSCode** (默认): 安装 C/C++、CMake Tools 扩展，通过 cmake-kits.json 配置 vcpkg toolchain
- **CLion**: 原生支持 CMake + vcpkg，在 CMake settings 中指定 vcpkg toolchain 路径

## 核心功能

1. 播放/暂停/停止/上一曲/下一曲/Seek 拖拽
2. 播放模式: 单曲循环、顺序播放、列表循环、随机播放
3. 歌单管理 (创建/编辑/删除/导入导出)
4. 歌词显示 (LRC 格式, 同名自动加载 + 手动选择, 逐行高亮)
5. 封面显示 (内嵌或同名图片加载)
6. 配置管理 (音量/主题/路径等, 持久化到 SQLite)
7. 国际化 (中文/英文)

## 目标受众

- 桌面用户, 管理 1000-10000 首歌曲的本地音乐库
- 不支持移动端
