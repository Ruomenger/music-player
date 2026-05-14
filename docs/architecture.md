# 架构设计

## 分层架构

```
┌─────────────────────────────────────────────┐
│  UI Layer (Qt Widgets)                      │
│  MainWindow / PlaylistView / LyricWidget    │
│  依赖: Qt6::Widgets                         │
├─────────────────────────────────────────────┤
│  Application Layer (信号/槽桥梁)            │
│  PlayerController / PlaylistManager         │
│  依赖: Qt6::Core, Domain Layer              │
├─────────────────────────────────────────────┤
│  Domain Layer (纯 C++ 业务逻辑, 无 Qt)      │
│  AudioEngine / LyricParser / PlaylistModel  │
│  依赖: 仅 STL                                │
├─────────────────────────────────────────────┤
│  Infrastructure Layer                       │
│  FfmpegDecoder / PortAudioOutput / SqliteRepo│
│  依赖: FFmpeg, PortAudio, Qt6::Sql          │
└─────────────────────────────────────────────┘
```

### 层级依赖规则

- **Domain Layer** 不依赖任何 Qt 模块，也不依赖 Infrastructure。纯 STL + C++23 标准库。
- **Infrastructure Layer** 封装第三方库边界，提供抽象接口给上层。
- **Application Layer** 持有 Domain 和 Infrastructure 的对象，协调二者，通过 Qt 信号/槽向外通信。
- **UI Layer** 只依赖 Application Layer 的信号，不直接操作 Domain/Infrastructure。

### 设计动机

- Domain 层纯 C++ 使其可用 gtest 快速测试，无需 QApplication 或模拟 Qt 事件循环。
- Infrastructure 层的抽象接口方便后续替换实现 (如换解码库、换音频输出后端)。
- UI 层与业务逻辑解耦，可独立迭代 UI 布局。

## 核心类设计

### Domain Layer

```cpp
// 歌曲值对象 (src/domain/song_info.h). 字段顺序与 SQLite songs 表对齐，
// Phase 3 的 scanner / metadata extractor 直接把抽到的内容写进来再交给
// SqliteSongRepo 落库。
struct SongInfo {
    int id = 0;
    std::string title;
    std::string artist;
    std::string album;
    std::string albumArtist;
    std::string composer;
    std::string genre;
    int year = 0;
    int trackNumber = 0;
    int discNumber = 0;
    std::string filePath;
    std::int64_t fileSize = 0;
    std::int64_t fileMtime = 0;
    double duration = 0.0;
    std::string format;
    int bitrate = 0;
    int sampleRate = 0;
    int channels = 2;
    std::string coverPath;
    std::string lyricPath;
    bool hasLyric = false;
};

// 播放模式枚举
enum class PlayMode : std::uint8_t { Single, Sequential, ListLoop, Random };

// 环形缓冲区 (lock-free SPSC, 详见 src/domain/ring_buffer.h)
template<typename T>
class RingBuffer { /* ... */ };

// 播放队列 + 模式策略 (src/domain/play_queue.h). PlayerController 把
// 「下一首选谁」的决策委托给它；Random 模式的 Fisher-Yates shuffle 也在
// 这里。详见 "播放模式行为" 一节。
class PlayQueue { /* ... */ };
```

### Application Layer

```cpp
// PlayerController (src/app/player_controller.h) - 协调 AudioEngine、
// PlayQueue 与三个仓库，是 UI 调用的入口。
class PlayerController : public QObject {
    Q_OBJECT
public:
    PlayerController(AudioEngine* engine, SqliteSongRepo* songs,
                     SqlitePlayHistoryRepo* history, PlayerStateRepo* playerState,
                     QObject* parent = nullptr);

    void play(int songId);                                          // 单曲播放
    void playQueue(const std::vector<int>& songIds, std::size_t startIndex);
    void resume();
    void pause();
    void stop();
    void next();                                                    // 用户手动
    void previous();
    void seek(double seconds);
    void setVolume(double volume);                                  // 0.0 - 1.0
    void setPlayMode(PlayMode mode);

    void restoreLastSession();                                      // 启动时调
    void persistSession(int playlistId = 0);                        // 退出时调

signals:
    // PlaybackState 用 int 序列化（Qt 信号要求注册元类型；用 int 省掉
    // Q_DECLARE_METATYPE / qRegisterMetaType 的样板，UI 端 static_cast 回
    // AudioEngine::State 即可）
    void stateChanged(int state);
    void positionChanged(double current, double total);
    void currentSongChanged(const SongInfo& song);
    void volumeChanged(double volume);
    void playModeChanged(int mode);                                 // 同上
    void errorOccurred(const QString& message);

private:
    AudioEngine* engine_;                  // 非拥有指针，main() 持有
    SqliteSongRepo* songs_;
    SqlitePlayHistoryRepo* history_;
    PlayerStateRepo* playerState_;
    PlayQueue queue_;
    QTimer positionTimer_;                 // ~250ms 间隔
};
```

PlayerController 不再自己 own `AudioEngine` —— main.cpp 在更高层组装
engine / decoder / output 之后通过指针注入，便于测试时换成 FakeDecoder /
FakeOutput（参见 `tests/app/test_player_controller.cpp`）。

### Application Layer 其他构件

- `PlaylistManager` (src/app/playlist_manager.h) — `SqlitePlaylistRepo`
  的 QObject 封装；除常规 CRUD 之外提供 `toggleFavorite` / `isFavorite`
  + `favoriteChanged(songId, bool)` 信号，封装系统歌单 "我喜欢"。
- `LyricManager` (src/app/lyric_manager.h) — 加载并缓存 `ParsedLyrics`，
  `updatePosition` 在每个 250ms tick 更新当前行；`setAutoLoadEnabled(false)`
  让 `loadForSong` 变成 clear（对应 settings 中的 `auto_load_lyric`）。
- `LanguageManager` (src/app/language_manager.h) — 切换 QTranslator；
  详见 [i18n.md](i18n.md)。

### Infrastructure Layer 接口

```cpp
class IAudioDecoder {
public:
    IAudioDecoder() = default;
    virtual ~IAudioDecoder() = default;
    IAudioDecoder(const IAudioDecoder&) = delete;
    IAudioDecoder& operator=(const IAudioDecoder&) = delete;
    IAudioDecoder(IAudioDecoder&&) = delete;
    IAudioDecoder& operator=(IAudioDecoder&&) = delete;

    virtual bool open(const std::string& filePath) = 0;
    // Returns up to `maxFrames` interleaved-float frames (channels * frame samples).
    // Empty vector means EOF (file fully drained AND swr internal buffer empty).
    // See audio-pipeline.md "decode() 契约" — overshooting maxFrames will cause audible glitches.
    virtual std::vector<float> decode(size_t maxFrames) = 0;
    virtual bool seek(double seconds) = 0;
    virtual void close() = 0;
    [[nodiscard]] virtual AudioDecoderInfo info() const = 0;

    // Configure the desired output sample rate before open(). Pass 0 to use
    // the file's native rate (the default). After open(), info().sampleRate
    // reports the post-resampling rate.
    virtual void setTargetSampleRate(int rate) = 0;
};

class IAudioOutput {
public:
    using DataCallback = std::function<void(float* buffer, int frameCount)>;

    IAudioOutput() = default;
    virtual ~IAudioOutput() = default;
    IAudioOutput(const IAudioOutput&) = delete;
    IAudioOutput& operator=(const IAudioOutput&) = delete;
    IAudioOutput(IAudioOutput&&) = delete;
    IAudioOutput& operator=(IAudioOutput&&) = delete;

    virtual bool open(double sampleRate, int channels) = 0;  // alloc native stream, no playback yet
    // Device's preferred output rate (Hz); 0 if no device. AudioEngine uses
    // this to drive setTargetSampleRate so the output never has to negotiate
    // an unsupported rate (matters on Linux ALSA / Windows WASAPI).
    [[nodiscard]] virtual double defaultSampleRate() const = 0;
    virtual bool start() = 0;                                 // begin/resume callback
    virtual bool pause() = 0;                                 // pause callback, keep stream open
    virtual bool stop() = 0;                                  // stop and close stream

    virtual void setCallback(DataCallback cb) = 0;
};

// 仓库都是具体类 —— 没有抽象 ISongRepository 之类的接口层。Phase 2 设计
// 阶段考虑过，但实际只引入了一个 SQLite 实现，YAGNI；要换底层时再抽。
//
// 当前 infra 仓库类（每个都接受 connectionName 参数，由 SqliteDatabase 管
// 理连接生命周期）：
class SqliteDatabase     { /* PRAGMAs + CREATE TABLE 迁移 */ };
class SqliteSongRepo     { /* songs */ };
class SqlitePlaylistRepo { /* playlists + playlist_songs (含 reorder) */ };
class SqliteSettingsRepo { /* settings key/value + seedDefaults */ };
class SqlitePlayHistoryRepo { /* play_history + recent + purgeOlderThan */ };
class PlayerStateRepo    { /* 单行 player_state，UPSERT */ };
```

## 播放模式行为

`PlayMode` 不是状态机，而是用户选择的策略——决定**当前曲播完时下一首选哪首**、**手动 next/prev 时的行为**。

| 模式 | 当前曲自然播完 | 用户点 Next | 用户点 Prev | 列表末尾 |
|------|---------------|-------------|-------------|----------|
| `Single` | 重播当前曲 | 列表内下一首 | 列表内上一首 | 单曲循环不受影响 |
| `Sequential` | 列表内下一首 | 列表内下一首 | 列表内上一首 | 停止播放 |
| `ListLoop` | 列表内下一首 | 列表内下一首 | 列表内上一首 | 回到第一首 |
| `Random` | 从 shuffle 队列取下一首 | 同左 | 回退 shuffle 历史 | shuffle 队列空时重新洗牌 |

### Random 实现要点

- 用 Fisher-Yates 一次性 shuffle 整个列表为 `shuffleQueue_`
- 维护 `cursor_` 指向当前位置；`advanceManual` / `advanceNatural` 即 `++cursor`，
  `retreatManual` 即 `--cursor`（不会越过 0，防止「上一首」翻越 shuffle 历史）
- `cursor_ + 1 >= shuffleQueue_.size()` 时重新 shuffle 并把 cursor 重置为 0
- 列表内容变化（`setSongs`）时丢弃 shuffleQueue_，再次进入 Random 重洗
- `setMode(Random)` 切入时如果有当前播放曲，会在新的 shuffleQueue_ 里找到
  它并把 cursor 落在那里，让「切换播放模式不打断当前曲」成立
- 测试可调 `seedForTesting(seed)` 把 RNG 固定，避免输出不确定（见
  `tests/domain/test_play_queue.cpp`）

## 线程模型

```
Main Thread (Qt Event Loop)
  ├── PlayerController (QTimer 驱动进度更新)
  ├── UI Widgets (渲染)
  │
Decode Thread (std::thread in AudioEngine)
  ├── FFmpeg 解码
  ├── 填充 RingBuffer
  │
PortAudio Callback Thread (由 PortAudio 创建)
  └── 从 RingBuffer 读取 PCM 数据写入声卡
```

- RingBuffer 实现为 lock-free SPSC (单生产者单消费者) 队列, 连接 Decode Thread → PortAudio Thread
- AudioEngine 用 `std::atomic<State>` 控制播放/暂停/停止状态；用独立的 `std::atomic<bool> decodeRunning_` 控制解码线程生命周期（不与 State 耦合，便于 seek 时停-动 decode 线程而保持 State 不变）
- 播放位置由 `std::atomic<uint64_t> framesPlayed_` (callback 端 `fetch_add`) + `std::atomic<uint64_t> seekFrameOffset_` (seek 时改写) 表示，不用 `atomic<double>`（在某些平台不是 lock-free）。详见 [audio-pipeline.md](audio-pipeline.md) 的"播放位置追踪"章节
- 所有 UI 状态变更通过信号/槽回主线程；从非 Qt 线程发信号必须使用 `Qt::QueuedConnection`，否则 slot 会在错误线程执行
