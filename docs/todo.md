# 开发进度表 (TODO)

## 图例

- `[ ]` 未开始
- `[~]` 进行中
- `[x]` 已完成

---

## Phase 0: 项目骨架搭建

- [x] 创建项目目录结构
- [x] 编写 `.gitignore`
- [x] 编写 `vcpkg.json` manifest, 声明依赖 (ffmpeg, portaudio, gtest)
- [x] 编写顶层 `CMakeLists.txt` (C++23, AUTOMOC, find_package)
- [x] 编写 `cmake/CompilerWarnings.cmake` 统一编译警告
- [x] 编写 `src/CMakeLists.txt` 及各子目录 CMakeLists
- [x] 编写 `tests/CMakeLists.txt` 集成 gtest
- [x] 编写 `main.cpp` (空 QApplication + 空 QMainWindow, 显示窗口)
- [x] 配置 VSCode `.vscode/cmake-kits.json` 和 `.vscode/settings.json`
- [x] 验证: cmake configure + build 通过, 窗口可显示

## Phase 1: 音频引擎核心 (Domain + Infra)

- [x] 实现 `IAudioDecoder` / `IAudioOutput` 抽象接口
- [x] 实现 `AudioDecoderInfo` 值对象
- [x] 实现 `FfmpegDecoder` (RAII 封装 AVFormat/AVCodec/SwrContext)
- [x] 实现 `PortAudioOutput` (init/start/stop, callback 中从 RingBuffer 取数据)
- [x] 实现 `RingBuffer<float>` (lock-free SPSC, bulk write/read, power-of-2 capacity)
- [x] 实现 `AudioEngine` (整合 Decoder + Output + RingBuffer + decode thread)
- [x] 编写 `test_ring_buffer.cpp` (7 个测试用例)
- [x] 编写 `test_ffmpeg_decoder.cpp` (5 个测试用例)
- [x] 编写 `test_audio_pipeline.cpp` (3 个集成测试: 播放/暂停/Seek)
- [x] 验证: 所有测试通过, 音频管线可播放
- [x] **听感验证**: 真实 mp3/flac 实播 ≥30s 无卡顿/电流声 (自动测试无法捕捉解码超量丢样本类 bug，必须人工听)
- [x] 拆分 `IAudioOutput` 的 pause/stop 语义 (pause 不关 stream)
- [x] `AudioEngine` 位置追踪改用 `atomic<uint64_t>` framesPlayed + seekFrameOffset (取代 `atomic<double>` RMW)

### Phase 1 后续修复 (audit 2026-05-11)

设计 audit 发现的 Phase 1 范围内的真实 bug 与接线缺口：

- [x] `AudioEngine::seek()` 在 Paused 状态下不重启 decode 线程 → resume 后永久静音 (`6942919`)
- [x] decoder EOF 不通知 engine → `state_` 一直 Playing，`framesPlayed_` 越过 duration (`6942919`)
- [x] `FfmpegDecoder::initResampler()` 把 swr 输出速率硬编码为输入速率，无法真正重采样 (`7be3ac0`)
- [x] `IAudioOutput::defaultSampleRate()` + engine 接线，按设备首选速率驱动 decoder (`89f9dc2`)
- [x] 三个回归测试: `SeekWhilePausedThenResumeAdvancesPosition` / `ReachesStoppedAtEndOfStream` / `OpensDecoderAtOutputDeviceSampleRate`

## Phase 2: 数据库层 (Infra)

### 已完成的预备工作

- [x] 实现 `ConfigPaths` (`QStandardPaths` 三个跨平台目录) (`4e87f42`)
- [x] `SqliteSongRepo` 所有 stub 方法改抛 `std::logic_error` 而非静默返回失败值 (`4bce7f1`)
- [x] `test_config_paths.cpp` + `test_sqlite_song_repo.cpp` (前者验证目录创建，后者验证 stub 抛错)

### 实际实现

- [x] 实现 `SqliteSongRepo` (CRUD for songs 表)
- [x] 实现 `SqlitePlaylistRepo` (CRUD for playlists + playlist_songs 表)
- [x] 实现 `SqliteSettingsRepo` (CRUD for settings 表)
- [x] 实现 `PlayerStateRepo` (save/load player_state 表)
- [x] 实现数据库迁移/初始化逻辑 (`SqliteDatabase`: PRAGMA + CREATE TABLE IF NOT EXISTS 基线)
- [x] 扩充 `test_sqlite_song_repo.cpp` 为真实 CRUD 测试 (`:memory:` 数据库)
- [x] 编写 `test_sqlite_playlist_repo.cpp` / `test_sqlite_settings_repo.cpp` /
      `test_player_state_repo.cpp` / `test_sqlite_database.cpp`
- [x] 验证: 35 个新增 SQLite 测试全部通过 (含 FK CASCADE / SET NULL、UPSERT、
      reorder 事务回滚等场景)

## Phase 3: 音乐扫描与导入

- [x] 实现文件扫描器 `MusicScanner` (`std::filesystem::recursive_directory_iterator`,
      `audio_extensions.h` 中央扩展名列表)
- [x] 实现元数据提取 `MetadataExtractor` (FFmpeg av_dict + codecpar:
      title/artist/album/album_artist/composer/genre/year/track/disc + duration/
      bitrate/sample_rate/channels + 嵌入封面 `AV_DISPOSITION_ATTACHED_PIC`)
- [x] 实现去重逻辑 (file_path UNIQUE + file_size/file_mtime 快速跳过未变文件;
      fingerprint/xxhash64 仍按 docs/database-schema.md 留给 Phase 9 懒计算)
- [x] 实现封面缓存 `CoverCache` (content-addressed: MD5 + magic-byte 扩展名,
      幂等 store)
- [x] 实现同名歌词自动匹配 `LyricMatcher` (.lrc / .LRC / .Lrc 三种大小写)
- [x] 实现 `PlaylistManager` (QObject 包装 `SqlitePlaylistRepo` + 信号
      `playlistCreated/Renamed/Deleted/songsChanged` + "我喜欢" 系统歌单 bootstrap)
- [x] 编写测试 (28 个新增: scanner / lyric_matcher / metadata_extractor /
      cover_cache / library_importer / playlist_manager)
- [x] 验证: `LibraryImporter::importDirectory()` 把扫描到的音频文件入库，
      重复扫描原地跳过，修改后重扫触发 update; 测试覆盖完整流程

## Phase 4: 播放器控制器 (Application Layer)

- [x] 实现 `PlayMode` 枚举 + 行为表 (`domain/play_queue.h`)
- [x] 实现 `PlayerController` (play/pause/resume/stop/next/prev/seek + setVolume +
      setPlayMode + restoreLastSession/persistSession)
- [x] 实现 Random 模式的 Fisher-Yates shuffle + 游标算法 (PlayQueue)
- [x] 实现播放队列管理 (`PlayQueue`: setSongs / setCurrent / advanceNatural /
      advanceManual / retreatManual; 切换模式时保留当前歌曲位置)
- [x] 实现播放位置 QTimer (~250ms 触发 positionChanged + state 变化检测)
- [x] 实现播放历史记录 (`SqlitePlayHistoryRepo` + PlayerController 在
      ≥30s 或自然结束时写入 play_history)
- [x] 实现启动时恢复上次播放状态 (`restoreLastSession` 走 PlayerStateRepo)
- [x] 实现关闭时保存播放状态 (`persistSession`)
- [x] `AudioEngine::setVolume` (atomic&lt;float&gt; + callback 内乘法,
      跳过 vol==1.0 的单位增益路径) + `eofReached()` getter 供 controller
      区分用户 stop 与自然结束
- [x] 编写 `test_play_queue.cpp` (4 个模式 + 列表变更 + 游标跳转, 15 个用例)
- [x] 编写 `test_sqlite_play_history_repo.cpp` / `test_player_controller.cpp`
      (使用 FakeDecoder + FakeOutput 验证 API 表面)

## Phase 5: 基础 UI (Qt Widgets)

- [x] 实现 `MainWindow` 框架 (菜单栏 File/Help, 状态栏, 中央布局 = 封面+标题
      左 / 播放列表右 / ControlBar 下)
- [x] 实现 `ControlBar` (prev/play-pause/stop/next 按钮 + 进度条 + 音量滑块 +
      播放模式按钮; *Requested 信号 + set*() 槽双向接口)
- [x] 实现 `PlaylistWidget` (QTableView + `SongTableModel`, 双击 / 回车
      触发 songActivated(id))
- [x] 实现 `CoverWidget` (QPixmap scaled, paintEvent 居中绘制, 缺图时
      程序生成占位符)
- [x] 实现文件添加对话框 (QFileDialog Add Files… / Add Folder… 接 LibraryImporter)
- [x] 连接 UI 信号到 PlayerController (UI→player + player→UI 双向接线)
- [ ] 实现系统托盘图标 (可选, 延后到 Phase 8 配合 SettingsDialog)
- [x] 验证: ctest 全通过 (130 个用例, 含 15 个新增 UI 单测,
      offscreen Qt 平台 headless 友好); main.cpp 完整接线
      DB / 仓 / 引擎 / 控制器 / 窗口, 启动恢复 / 退出保存 session

## Phase 6: 歌词显示

- [x] 实现 `LyricParser` (LRC: [mm:ss.fff]/[mm:ss.ff]/[mm:ss.f]/[mm:ss],
      多时间戳同行, 元数据 [ti]/[ar]/[al]/[offset], CRLF/空行容错; 输出
      按毫秒排序; 提供 `findCurrentLineIndex(ms)` 二分查找当前行)
- [x] 实现 `LyricWidget` (QListWidget 居中绘制, 当前行加粗,
      `scrollToItem(PositionAtCenter)` 自动滚动; "Load lyrics…" 按钮)
- [x] 实现 `LyricManager` (QObject; `loadForSong` 读 song.lyricPath,
      `loadFromPath` 手动文件, `updatePosition` 用秒位置驱动 currentLineChanged,
      `clear` 释放)
- [x] 实现歌词按钮打开文件选择对话框 (LyricWidget::onLoadClicked → QFileDialog,
      `manualLyricsRequested` 信号交给 LyricManager.loadFromPath)
- [x] 在 `PlayerController` 中集成当前歌词行同步 (MainWindow 把
      PlayerController::positionChanged 接到 LyricManager::updatePosition,
      currentSongChanged 触发 loadForSong)
- [x] 编写 `test_lyric_parser.cpp` (18 个用例: 各种时间戳精度 / 多时间戳 /
      offset 平移与下溢 clamp / 元数据捕获 / CRLF / 异常输入)
- [x] 准备 `testdata/sample.lrc` 测试歌词文件
- [x] 验证: 158/158 测试通过 (18 解析 + 6 manager + 5 widget);
      MainWindow 用 QTabWidget 把歌词面板与 Library 列表并列, 自动滚动到中心

## Phase 7: 歌单 UI 和交互

- [ ] 实现歌单列表 (QListView 或列表面板)
- [ ] 实现创建歌单对话框
- [ ] 实现歌单右键菜单 (播放/重命名/删除/导出)
- [ ] 实现歌曲拖拽排序 (在 playlist_widget 中)
- [ ] 实现"我喜欢"功能 (收藏到系统歌单)
- [ ] 实现最近播放列表 (从 play_history 生成)
- [ ] 验证: 创建歌单→添加歌曲→拖拽排序→切换歌单播放

## Phase 8: 设置和配置

- [ ] 实现 `SettingsDialog` (读取 settings 表)
- [ ] 实现语言切换 (中/英文, 动态切换)
- [ ] 实现输出设备选择 (枚举 PortAudio 设备)
- [ ] 实现默认音乐目录设置
- [ ] 实现自动加载歌词开关
- [ ] 实现播放历史保留天数设置
- [ ] 编写翻译文件 `musicplayer_zh_CN.ts`
- [ ] 验证: 修改设置→重启→设置保持; 切换语言→界面刷新

## Phase 9: 测试补全和优化

- [ ] 补充 domain 层所有单元测试
- [ ] 补充 infra 层所有单元测试
- [ ] 编写集成测试 (数据库 + 播放管线)
- [ ] 性能测试: 10000 首歌曲扫描/导入速度
- [ ] 性能测试: 大型播放列表加载和排序速度
- [ ] 内存泄漏检查 (Valgrind / AddressSanitizer)
- [ ] 边界测试: 损坏音频文件、空目录、特殊字符路径
- [ ] AudioEngine 解码线程把 `sleep_for(10ms)` 轮询替换为 `std::condition_variable` 唤醒模型 (减少 seek/pause 响应延迟和 CPU 浪费)
- [ ] songs 表加 FTS5 虚表 + 同步触发器，搜索从 `LIKE` 切到 `MATCH` (10k+ 行性能优化)
- [ ] 听感回归测试矩阵: mp3/flac/aac/ogg 各跑一首完整曲目，无卡顿/咔哒/底噪

## Phase 10: 跨平台验证和打包

- [ ] Fedora 43 (Wayland): 完整编译+运行+测试 (含系统托盘/通知验证)
- [ ] Fedora 43 (X11 fallback): Qt 自动回退, 验证无崩溃
- [ ] macOS 15 M2: 完整编译+运行+测试
- [ ] Windows: 完整编译+运行+测试
- [ ] 编写 `docs/build-instructions.md` (各平台构建步骤)
- [ ] 编写 `docs/user-guide.md` (用户使用手册)
- [ ] 打包: Linux AppImage / Flatpak
- [ ] 打包: macOS .app bundle (.dmg)
- [ ] 打包: Windows NSIS 安装程序
- [ ] CI: GitHub Actions 构建矩阵 (Linux/macOS/Windows)

## Phase 11: 高级特性 (可选)

- [ ] Gapless 无缝播放
- [ ] 均衡器 (EQ) 支持
- [ ] 频谱可视化
- [ ] 音乐库自动监控 (fs watcher, 新增/删除文件自动同步)
- [ ] 格式转换 (FLAC→MP3 等)
- [ ] 在线歌词搜索 (网易云/QQ 音乐 API)
- [ ] 暗色/亮色主题切换
- [ ] 快捷键全局注册
- [ ] 迷你播放器模式
