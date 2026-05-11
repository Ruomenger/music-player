# Git Commit 规范

## 格式

```
<type>(<scope>): <subject>

<body>
```

## Type（类型）

| type | 说明 |
|------|------|
| `feat` | 新功能 |
| `fix` | bug 修复 |
| `refactor` | 重构（不改变外部行为） |
| `build` | 构建系统 / 依赖变更 |
| `docs` | 文档变更 |
| `test` | 测试补充或修正 |
| `style` | 纯格式化（clang-format 等，无逻辑变化） |
| `ci` | CI 配置变更（GitHub Actions 等） |
| `chore` | 杂项维护（脚本、配置、.gitignore 等） |

`debug` 和 `perf` 可在调试或性能优化时临时使用，合入主线前通常应归类到 `fix`、`feat` 或 `refactor`。

## Scope（范围）

可选。用组件或模块名，小写，下划线分隔。跨模块变更可省略 scope。

常用 scope：
- `audio_engine`, `ffmpeg_decoder`, `portaudio_output`, `ring_buffer` — 音频管线各组件
- `sqlite_song_repo`, `sqlite_playlist_repo`, `sqlite_settings_repo` — 数据库 repo
- `config_paths` — 跨平台路径工具
- `player_controller`, `playlist_manager`, `lyric_manager` — 应用层
- `main_window`, `playlist_widget`, `lyric_widget`, `control_bar` — UI 组件

## Subject（标题）

- 英文，祈使语气（imperative mood），小写开头
- 不超过 72 字符
- 不以句号结尾
- 描述 **做了什么** 以及 **为什么**，而非罗列文件

**正确**：
```
fix(audio_engine): seek-while-paused silence and EOF state propagation
feat(config_paths): implement cross-platform paths via QStandardPaths
build: tighten musicplayer_infra link visibility (Qt/FFmpeg/PortAudio PRIVATE)
```

**错误**：
```
fix: update ffmpeg_decoder.cpp       （太模糊，没说改了什么）
Fix audio engine bug.                （不要首字母大写，不要句号）
feat(config_paths): implemented...   （用祈使语气，不是过去式）
```

## Body（正文）

- 与 subject 之间空一行
- 每行不超过 72 字符
- 解释 **为什么** 这样改（动机）、**怎么改的**（关键决策）、**影响范围**
- 多条变更可用 `-` 列表
- 不需要 `Co-Authored-By` 或 `Closes #xxx` 等尾部标记

### 示例

```
fix(ffmpeg_decoder): drop extra const on writeOutput srcData to match swr_convert signature

GCC rejects const uint8_t* const* → const uint8_t** implicit conversion;
Clang accepts it, masking the mismatch on macOS. Fixes CI build on
ubuntu-24.04.
```

```
fix(ring_buffer): drop dead branch in available()

writeIdx_ and readIdx_ are size_t monotonically-increasing counters; the
buffer wraps via & mask_ only at memcpy sites, never on the indices
themselves. Producer can't overrun consumer (write() gates on
freeSlots()), so w >= r is invariant and w - r always gives the right
distance via unsigned arithmetic.

The previous (w >= r) ? w - r : buffer_.size() - r + w carried over from
a modulo-counter design and is unreachable; remove it and document the
actual invariant.
```

```
refactor(audio): split pause/stop in IAudioOutput; rework position tracking

- Split IAudioOutput pause (Pa_StopStream, stream stays open) from stop
  (Pa_StopStream + Pa_CloseStream, full teardown). Previously pause
  called stop, making resume impossible without re-open().
- Switch position tracking from atomic<double> RMW to atomic<uint64_t>
  framesPlayed + seekFrameOffset. atomic<double> is not lock-free on
  all platforms; uint64_t is, and avoids floating-point drift.
- callback now fetch_add rather than read-modify-write.
```