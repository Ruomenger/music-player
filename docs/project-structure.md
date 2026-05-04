# 项目目录结构

```
music-player/
├── CMakeLists.txt              # 顶层 CMake 配置
├── vcpkg.json                  # vcpkg manifest (依赖声明)
├── vcpkg-configuration.json    # vcpkg registry 覆盖配置 (可选)
├── .gitignore
├── .vscode/
│   ├── settings.json           # VSCode CMake 配置 (toolchain 路径等)
│   └── cmake-kits.json         # VSCode CMake Kits 定义
├── cmake/
│   ├── CompilerWarnings.cmake  # 统一编译警告选项
│   └── Testing.cmake           # gtest 集成 (enable_testing 等)
├── docs/
│   ├── project-overview.md     # 项目总览
│   ├── architecture.md         # 架构设计
│   ├── project-structure.md    # 本文件
│   ├── build-system.md         # CMake + vcpkg 构建指南
│   ├── database-schema.md      # 数据库表设计
│   ├── audio-pipeline.md       # 音频播放管线设计
│   ├── i18n.md                 # 国际化设计
│   └── todo.md                 # 开发进度表
├── src/
│   ├── CMakeLists.txt          # src 子目录构建
│   ├── main.cpp                # 程序入口
│   ├── app/                    # Application Layer
│   │   ├── CMakeLists.txt
│   │   ├── player_controller.h
│   │   ├── player_controller.cpp
│   │   ├── playlist_manager.h
│   │   ├── playlist_manager.cpp
│   │   └── lyric_manager.h
│   │   └── lyric_manager.cpp
│   ├── ui/                     # UI Layer (Qt Widgets)
│   │   ├── CMakeLists.txt
│   │   ├── main_window.h
│   │   ├── main_window.cpp
│   │   ├── playlist_widget.h
│   │   ├── playlist_widget.cpp
│   │   ├── lyric_widget.h
│   │   ├── lyric_widget.cpp
│   │   ├── control_bar.h
│   │   ├── control_bar.cpp
│   │   ├── cover_widget.h
│   │   ├── cover_widget.cpp
│   │   ├── settings_dialog.h
│   │   └── settings_dialog.cpp
│   ├── domain/                 # Domain Layer (纯 C++, 无 Qt)
│   │   ├── CMakeLists.txt
│   │   ├── song_info.h
│   │   ├── audio_engine.h
│   │   ├── audio_engine.cpp
│   │   ├── lyric_parser.h
│   │   ├── lyric_parser.cpp
│   │   ├── ring_buffer.h
│   │   ├── playlist_model.h
│   │   └── playlist_model.cpp
│   ├── infra/                  # Infrastructure Layer
│   │   ├── CMakeLists.txt
│   │   ├── ffmpeg_decoder.h
│   │   ├── ffmpeg_decoder.cpp
│   │   ├── portaudio_output.h
│   │   ├── portaudio_output.cpp
│   │   ├── sqlite_song_repo.h
│   │   ├── sqlite_song_repo.cpp
│   │   ├── sqlite_playlist_repo.h
│   │   ├── sqlite_playlist_repo.cpp
│   │   ├── sqlite_settings_repo.h
│   │   ├── sqlite_settings_repo.cpp
│   │   └── config_paths.h
│   │   └── config_paths.cpp
│   └── common/                 # 通用工具
│       ├── CMakeLists.txt
│       ├── constants.h
│       └── utils.h
├── tests/
│   ├── CMakeLists.txt
│   ├── domain/
│   │   ├── test_lyric_parser.cpp
│   │   ├── test_playlist_model.cpp
│   │   ├── test_ring_buffer.cpp
│   │   └── test_play_mode.cpp
│   ├── infra/
│   │   ├── test_sqlite_song_repo.cpp
│   │   ├── test_sqlite_playlist_repo.cpp
│   │   └── test_ffmpeg_decoder.cpp
│   └── integration/
│       └── test_audio_pipeline.cpp
├── resources/
│   ├── icons/                  # 应用图标
│   ├── translations/           # *.ts / *.qm 翻译文件
│   │   ├── musicplayer_zh_CN.ts
│   │   └── musicplayer_en.ts
│   └── default.qrc             # Qt 资源文件
└── testdata/                   # 测试用音频文件
    ├── sample.flac
    ├── sample.mp3
    └── sample.lrc
```

## 各目录职责

| 目录 | 职责 | Qt 依赖 | 可独立测试 |
|------|------|---------|-----------|
| `src/domain/` | 纯 C++ 业务逻辑: 音频引擎控制、歌词解析、播放模式 | 无 | 是 (gtest) |
| `src/infra/` | 第三方库封装: FFmpeg 解码、PortAudio 输出、SQLite CRUD | Qt::Sql | 是 (gtest + 临时数据库) |
| `src/app/` | 应用协调层: 连接 UI 和业务逻辑, 管理状态 | Qt::Core | 否 (需 mock) |
| `src/ui/` | Qt Widgets 界面组件 | Qt::Widgets | 否 (手工/探索) |
| `src/common/` | 通用常量和工具函数 | 无 | 是 (gtest) |
| `tests/` | 单元测试和集成测试 | gtest 依赖 | - |
| `resources/` | 资源文件: 图标、翻译 | - | - |
| `testdata/` | 测试用音视频/歌词样本 | - | - |
