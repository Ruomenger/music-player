# 项目目录结构

> 这棵树同时反映**当前已有**和**Phase N 计划**两类条目。带 `(Phase N)`
> 标签的目录/文件在对应 phase 启动前不存在，参见 `todo.md`。

```
music-player/
├── CMakeLists.txt              # 顶层 CMake 配置
├── CMakePresets.json           # 构建/测试 preset (linux/macos/ci)
├── vcpkg.json                  # vcpkg manifest (ffmpeg / portaudio / gtest)
├── .clang-format
├── .clang-tidy
├── .gitignore
├── .github/
│   └── workflows/              # CI: linux/mac/win + format + tidy
├── .vscode/
│   ├── settings.json
│   ├── cmake-kits.json
│   └── extensions.json
├── cmake/
│   ├── CompilerWarnings.cmake  # 统一编译警告选项
│   ├── FindFFmpeg.cmake        # vcpkg FFmpeg 接入 (变量模式 → 目标)
│   └── FindPortAudio.cmake
├── scripts/
│   ├── format.sh               # clang-format 全量
│   └── tidy.sh                 # run-clang-tidy 包装
├── docs/
│   ├── project-overview.md     # 项目总览
│   ├── architecture.md         # 架构设计
│   ├── project-structure.md    # 本文件
│   ├── build-system.md         # CMake + vcpkg 构建指南
│   ├── database-schema.md      # 数据库表设计 (Phase 2)
│   ├── audio-pipeline.md       # 音频播放管线设计
│   ├── i18n.md                 # 国际化设计 (Phase 8)
│   └── todo.md                 # 开发进度表
├── src/
│   ├── CMakeLists.txt          # 子目录入口 + 主可执行文件
│   ├── main.cpp                # 程序入口
│   ├── common/                 # 通用工具 (header-only INTERFACE 库)
│   │   ├── CMakeLists.txt
│   │   └── constants.h         # kAppName / kAppVersion / kAppOrg
│   ├── domain/                 # Domain Layer (纯 C++, 无 Qt)
│   │   ├── CMakeLists.txt
│   │   ├── audio_decoder_info.{h,cpp}
│   │   ├── audio_engine.{h,cpp}
│   │   ├── iaudio_decoder.{h,cpp}     # 接口
│   │   ├── iaudio_output.{h,cpp}
│   │   ├── lyric_parser.{h,cpp}       # Phase 6 完整实现
│   │   ├── playlist_model.{h,cpp}
│   │   ├── ring_buffer.{h,cpp}
│   │   └── song_info.{h,cpp}
│   ├── infra/                  # Infrastructure Layer
│   │   ├── CMakeLists.txt
│   │   ├── config_paths.{h,cpp}       # QStandardPaths
│   │   ├── ffmpeg_decoder.{h,cpp}
│   │   ├── portaudio_output.{h,cpp}
│   │   ├── sqlite_song_repo.{h,cpp}        # Phase 2 stub (throws)
│   │   ├── sqlite_playlist_repo.{h,cpp}    # Phase 2 stub
│   │   └── sqlite_settings_repo.{h,cpp}    # Phase 2 stub
│   ├── app/                    # Application Layer (Phase 4 — 当前为空 placeholder)
│   │   ├── CMakeLists.txt
│   │   ├── player_controller.{h,cpp}
│   │   ├── playlist_manager.{h,cpp}
│   │   └── lyric_manager.{h,cpp}
│   └── ui/                     # UI Layer (Qt Widgets)
│       ├── CMakeLists.txt
│       ├── main_window.{h,cpp}
│       ├── playlist_widget.{h,cpp}
│       ├── lyric_widget.{h,cpp}
│       ├── control_bar.{h,cpp}
│       ├── cover_widget.{h,cpp}
│       └── settings_dialog.{h,cpp}
├── tests/
│   ├── CMakeLists.txt
│   ├── tests_main.cpp          # GoogleTest entry
│   ├── domain/
│   │   ├── test_lyric_parser.cpp       # Phase 6 测试 (当前 disabled)
│   │   └── test_ring_buffer.cpp
│   ├── infra/
│   │   ├── test_config_paths.cpp
│   │   ├── test_ffmpeg_decoder.cpp
│   │   └── test_sqlite_song_repo.cpp   # 验证 Phase 2 stub 抛错
│   └── integration/
│       └── test_audio_pipeline.cpp
├── (Phase 8) resources/
│   ├── icons/                  # 应用图标
│   ├── translations/           # *.ts / *.qm 翻译文件
│   │   ├── musicplayer_zh_CN.ts
│   │   └── musicplayer_en.ts
│   └── default.qrc
└── (test runtime) testdata/    # 测试运行时生成的临时 wav，由 createSilentWav() 写入
```

## 各目录职责

| 目录 | 职责 | Qt 依赖 | 可独立测试 |
|------|------|---------|-----------|
| `src/domain/` | 纯 C++ 业务逻辑: 音频引擎控制、歌词解析、播放模式 | 无 | 是 (gtest) |
| `src/infra/` | 第三方库封装: FFmpeg 解码、PortAudio 输出、SQLite CRUD、`QStandardPaths` | Qt::Core, Qt::Sql | 是 (gtest + 临时数据库) |
| `src/app/` | 应用协调层: 连接 UI 和业务逻辑, 管理状态 | Qt::Core | 否 (需 mock) |
| `src/ui/` | Qt Widgets 界面组件 | Qt::Widgets | 否 (手工/探索) |
| `src/common/` | 通用常量和工具函数 | 无 | 是 (gtest) |
| `tests/` | 单元测试和集成测试 | gtest 依赖 | - |
| `resources/` | 资源文件: 图标、翻译 | - | - |
| `testdata/` | 测试用音视频/歌词样本 | - | - |
