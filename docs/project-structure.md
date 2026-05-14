# 项目目录结构

> 反映 Phase 0–8 已落地的实际结构。仍带 `(Phase N)` 标签的条目对应 todo.md
> 里未完成的工作 (当前只剩 Phase 9 性能优化 / Phase 10 打包 / Phase 11
> 高级特性)。

```
music-player/
├── CMakeLists.txt              # 顶层 CMake 配置
├── CMakePresets.json           # linux/macos/ci 构建/测试 preset
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
│   ├── project-overview.md
│   ├── architecture.md
│   ├── project-structure.md    # 本文件
│   ├── build-system.md
│   ├── database-schema.md
│   ├── audio-pipeline.md
│   ├── i18n.md
│   ├── git-commit-conventions.md
│   └── todo.md
├── src/
│   ├── CMakeLists.txt          # 子目录入口 + 主可执行文件 + qt6_add_translations
│   ├── main.cpp                # 启动: 打开 SqliteDatabase / 构建所有仓 + 服务 / 装配 MainWindow
│   ├── common/                 # 通用工具 (header-only INTERFACE 库)
│   │   ├── CMakeLists.txt
│   │   └── constants.h         # kAppName / kAppVersion / kAppOrg
│   ├── domain/                 # Domain Layer (纯 C++, 无 Qt)
│   │   ├── CMakeLists.txt
│   │   ├── audio_decoder_info.{h,cpp}
│   │   ├── audio_engine.{h,cpp}     # + setVolume / eofReached (Phase 4)
│   │   ├── audio_extensions.h       # 扫描器支持的扩展名 (Phase 3)
│   │   ├── iaudio_decoder.{h,cpp}
│   │   ├── iaudio_output.{h,cpp}
│   │   ├── lyric_matcher.{h,cpp}    # 同名 .lrc 旁路查找 (Phase 3)
│   │   ├── lyric_parser.{h,cpp}     # LRC 解析 (Phase 6)
│   │   ├── music_scanner.{h,cpp}    # 文件系统遍历 (Phase 3)
│   │   ├── play_queue.{h,cpp}       # PlayMode + Fisher-Yates (Phase 4)
│   │   ├── playlist.{h,cpp}         # Playlist / PlayerState 值对象
│   │   ├── playlist_model.{h,cpp}
│   │   ├── ring_buffer.{h,cpp}
│   │   └── song_info.{h,cpp}        # 18 字段 SongInfo + PlayMode 枚举
│   ├── infra/                  # Infrastructure Layer
│   │   ├── CMakeLists.txt
│   │   ├── config_paths.{h,cpp}              # QStandardPaths
│   │   ├── cover_cache.{h,cpp}               # 嵌入封面缓存 (Phase 3)
│   │   ├── ffmpeg_decoder.{h,cpp}
│   │   ├── library_importer.{h,cpp}          # 扫描→提取→入库流水线 (Phase 3)
│   │   ├── metadata_extractor.{h,cpp}        # FFmpeg av_dict 元数据 (Phase 3)
│   │   ├── player_state_repo.{h,cpp}         # player_state 单行 (Phase 2)
│   │   ├── portaudio_output.{h,cpp}          # + enumerateOutputDevices (Phase 8)
│   │   ├── sqlite_database.{h,cpp}           # 连接 / PRAGMA / 迁移 (Phase 2)
│   │   ├── sqlite_play_history_repo.{h,cpp}  # play_history (Phase 4)
│   │   ├── sqlite_playlist_repo.{h,cpp}      # playlists + playlist_songs
│   │   ├── sqlite_settings_repo.{h,cpp}      # 类型化 key/value
│   │   └── sqlite_song_repo.{h,cpp}
│   ├── app/                    # Application Layer (Qt Core only)
│   │   ├── CMakeLists.txt
│   │   ├── language_manager.{h,cpp}     # QTranslator 装卸 (Phase 8)
│   │   ├── lyric_manager.{h,cpp}        # 当前歌词 + 行游标
│   │   ├── player_controller.{h,cpp}    # 主协调器 (Phase 4)
│   │   └── playlist_manager.{h,cpp}     # SqlitePlaylistRepo 的 QObject 包装
│   └── ui/                     # UI Layer (Qt Widgets)
│       ├── CMakeLists.txt
│       ├── control_bar.{h,cpp}
│       ├── cover_widget.{h,cpp}
│       ├── lyric_widget.{h,cpp}
│       ├── main_window.{h,cpp}
│       ├── new_playlist_dialog.{h,cpp}  # 创建歌单对话框 (Phase 7)
│       ├── playlist_sidebar.{h,cpp}     # 来源 + 用户歌单侧栏 (Phase 7)
│       ├── playlist_widget.{h,cpp}      # 含右键菜单 + 拖拽排序
│       ├── settings_dialog.{h,cpp}      # 偏好设置 (Phase 8)
│       └── song_table_model.{h,cpp}     # QAbstractTableModel + mimeData reorder
├── tests/
│   ├── CMakeLists.txt
│   ├── qt_test_main.cpp              # 提供 QCoreApplication 的 GTest 入口
│   ├── qt_widget_test_main.cpp       # 提供 QApplication (offscreen) 的入口
│   ├── tests_main.cpp                # 旧版纯 gtest_main (留作历史参照)
│   ├── domain/
│   │   ├── test_lyric_matcher.cpp
│   │   ├── test_lyric_parser.cpp
│   │   ├── test_music_scanner.cpp
│   │   ├── test_play_queue.cpp
│   │   └── test_ring_buffer.cpp
│   ├── infra/
│   │   ├── test_config_paths.cpp
│   │   ├── test_cover_cache.cpp
│   │   ├── test_ffmpeg_decoder.cpp
│   │   ├── test_library_importer.cpp
│   │   ├── test_metadata_extractor.cpp
│   │   ├── test_player_state_repo.cpp
│   │   ├── test_sqlite_database.cpp
│   │   ├── test_sqlite_play_history_repo.cpp
│   │   ├── test_sqlite_playlist_repo.cpp
│   │   ├── test_sqlite_settings_repo.cpp
│   │   └── test_sqlite_song_repo.cpp
│   ├── app/
│   │   ├── test_language_manager.cpp
│   │   ├── test_lyric_manager.cpp
│   │   ├── test_player_controller.cpp   # 使用 FakeDecoder + FakeOutput
│   │   └── test_playlist_manager.cpp
│   ├── ui/
│   │   ├── test_control_bar.cpp
│   │   ├── test_cover_widget.cpp
│   │   ├── test_lyric_widget.cpp
│   │   ├── test_new_playlist_dialog.cpp
│   │   ├── test_playlist_sidebar.cpp
│   │   ├── test_settings_dialog.cpp
│   │   └── test_song_table_model.cpp
│   └── integration/
│       └── test_audio_pipeline.cpp
├── resources/
│   └── translations/
│       └── musicplayer_zh_CN.ts      # 中文翻译; en 用 tr() 源字符串
├── testdata/
│   └── sample.lrc                    # LRC 解析器测试样本; 其余 wav 由测试运行时生成
└── (Phase 8) resources/icons/        # 应用图标资源仍待补 (托盘 / 窗口图标)
```

## 各目录职责

| 目录 | 职责 | Qt 依赖 | 可独立测试 |
|------|------|---------|-----------|
| `src/domain/` | 纯 C++ 业务逻辑: 音频引擎、播放队列、歌词解析、扫描器、值对象 | 无 | 是 (gtest) |
| `src/infra/` | 第三方库封装: FFmpeg 解码 / PortAudio 输出 / SQLite (6 个仓 + Database) / `ConfigPaths` / `CoverCache` / `MetadataExtractor` / `LibraryImporter` | Qt::Core, Qt::Sql | 是 (gtest + `:memory:` DB) |
| `src/app/` | 应用协调层: PlayerController / PlaylistManager / LyricManager / LanguageManager — 把 domain + infra 用信号槽串起来 | Qt::Core | 是 (gtest + QCoreApplication + Fake* 注入) |
| `src/ui/` | Qt Widgets 界面组件: MainWindow / 各 widget / 两个对话框 / SongTableModel | Qt::Widgets | 是 (gtest + QApplication offscreen) |
| `src/common/` | 通用常量 (kAppName 等); header-only INTERFACE 库 | 无 | 是 (gtest) |
| `tests/` | 单元测试 + 集成测试; 使用 `qt_test_main.cpp` (Core) 或 `qt_widget_test_main.cpp` (Widgets offscreen) | gtest + 选项性 Qt | - |
| `resources/translations/` | Qt Linguist `.ts` (源) → 由 `qt6_add_translations` 编为 `.qm` 嵌入到 musicplayer 可执行 | - | - |
| `testdata/` | 测试用 LRC 样本; .wav 由测试运行时通过 `createSilentWav()` 现场生成 | - | - |
