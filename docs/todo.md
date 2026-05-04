# 开发进度表 (TODO)

## 图例

- `[ ]` 未开始
- `[~]` 进行中
- `[x]` 已完成

---

## Phase 0: 项目骨架搭建

- [ ] 创建项目目录结构
- [ ] 编写 `.gitignore`
- [ ] 编写 `vcpkg.json` manifest, 声明依赖 (qt6, ffmpeg, portaudio, sqlite3, gtest)
- [ ] 编写顶层 `CMakeLists.txt` (C++23, AUTOMOC, find_package)
- [ ] 编写 `cmake/CompilerWarnings.cmake` 统一编译警告
- [ ] 编写 `src/CMakeLists.txt` 及各子目录 CMakeLists
- [ ] 编写 `tests/CMakeLists.txt` 集成 gtest
- [ ] 编写 `main.cpp` (空 QApplication + 空 QMainWindow, 显示 "Hello Music Player")
- [ ] 配置 VSCode `.vscode/cmake-kits.json` 和 `.vscode/settings.json`
- [ ] 验证: cmake configure + build 通过, 窗口可显示

## Phase 1: 音频引擎核心 (Domain + Infra)

- [ ] 实现 `FfmpegDecoder` (RAII 封装 AVFormat/AVCodec/SwrContext)
- [ ] 实现 `PortAudioOutput` (init/start/stop/pause, callback 中从 RingBuffer 取数据)
- [ ] 实现 `RingBuffer<float>` (lock-free SPSC, 用于解码线程 → 输出线程)
- [ ] 实现 `AudioEngine` (整合 Decoder + Output + RingBuffer + decode thread)
- [ ] 实现 `SongInfo` 值对象
- [ ] 实现 `IAudioDecoder` / `IAudioOutput` 抽象接口
- [ ] 编写 `test_ffmpeg_decoder.cpp` (用 testdata/ 中的测试音频)
- [ ] 编写 `test_ring_buffer.cpp`
- [ ] 编写 `test_audio_pipeline.cpp` (集成解码→输出)
- [ ] 准备 `testdata/sample.mp3`, `testdata/sample.flac` 测试音频
- [ ] 验证: 命令行可播放音频文件, 能听到声音

## Phase 2: 数据库层 (Infra)

- [ ] 实现 `config_paths` (跨平台标准路径)
- [ ] 实现 `SqliteSongRepo` (CRUD for songs 表)
- [ ] 实现 `SqlitePlaylistRepo` (CRUD for playlists + playlist_songs 表)
- [ ] 实现 `SqliteSettingsRepo` (CRUD for settings 表)
- [ ] 实现 `PlayerStateRepo` (save/load player_state 表)
- [ ] 实现数据库迁移/初始化逻辑 (建表 + PRAGMA 设置)
- [ ] 编写 `test_sqlite_song_repo.cpp` (用 :memory: 数据库)
- [ ] 编写 `test_sqlite_playlist_repo.cpp`
- [ ] 验证: 可增删改查歌曲和歌单

## Phase 3: 音乐扫描与导入

- [ ] 实现文件扫描器 (遍历目录, 识别音频文件扩展名)
- [ ] 实现元数据提取 (通过 FFmpeg 读取 title/artist/album/cover/duration)
- [ ] 实现去重逻辑 (file_path UNIQUE + md5_hash)
- [ ] 实现封面缓存 (提取嵌入封面保存到 cache 目录)
- [ ] 实现同名歌词自动匹配 (同目录下 .lrc 文件)
- [ ] 实现 `PlaylistManager` (创建/删除/重命名歌单, 添加/移除歌曲, 排序)
- [ ] 编写测试
- [ ] 验证: 扫描指定目录, 歌曲入库, 封面和歌词正确关联

## Phase 4: 播放器控制器 (Application Layer)

- [ ] 实现 `PlayMode` 枚举和状态机 (Single/Sequential/ListLoop/Random)
- [ ] 实现 `PlayerController` (play/pause/resume/stop/next/prev/seek/volume)
- [ ] 实现 Random 模式的 Fisher-Yates shuffle + 游标算法
- [ ] 实现播放队列管理 (当前播放列表 + 播放索引)
- [ ] 实现播放位置 QTimer (emits positionChanged every ~250ms)
- [ ] 实现播放历史记录 (写入 play_history 表)
- [ ] 实现启动时恢复上次播放状态
- [ ] 实现关闭时保存播放状态
- [ ] 编写 `test_play_mode.cpp`
- [ ] 编写 `test_playlist_model.cpp`

## Phase 5: 基础 UI (Qt Widgets)

- [ ] 实现 `MainWindow` 框架 (菜单栏, 状态栏, 布局)
- [ ] 实现 `ControlBar` (播放/暂停/停止/上一曲/下一曲/进度条/音量滑块)
- [ ] 实现 `PlaylistWidget` (QTableView + 自定义 Model, 双击播放, 右键菜单)
- [ ] 实现 `CoverWidget` (封面图片显示, 异步加载)
- [ ] 实现文件添加对话框 (QFileDialog 选择文件/目录)
- [ ] 连接 UI 信号到 PlayerController
- [ ] 实现系统托盘图标 (可选, Windows 惯用)
- [ ] 验证: 选择文件→播放→进度条走→切换歌曲→正常

## Phase 6: 歌词显示

- [ ] 实现 `LyricParser` (解析 .lrc 文件, 提取时间标签和文本)
- [ ] 实现 `LyricWidget` (滚动文本显示, 当前行高亮, 自动滚动)
- [ ] 实现 `LyricManager` (加载/切换/手动选择歌词文件)
- [ ] 实现歌词按钮打开文件选择对话框 (手动选择)
- [ ] 在 `PlayerController` 中集成当前歌词行同步 (根据 position 查找)
- [ ] 编写 `test_lyric_parser.cpp`
- [ ] 准备 `testdata/sample.lrc` 测试歌词文件
- [ ] 验证: 播放歌曲, 歌词逐行高亮滚动

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
