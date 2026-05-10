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
// 歌曲信息值对象
struct SongInfo {
    int id;
    std::string title;
    std::string artist;
    std::string album;
    std::string filePath;
    double duration;
    std::string format;
    // lyric info
    std::string lyricPath;
    bool hasLyric;
};

// 播放模式枚举
enum class PlayMode { Single, Sequential, ListLoop, Random };

// 环形缓冲区 (线程安全, SPSC)
template<typename T>
class RingBuffer { /* ... */ };
```

### Application Layer

> **状态：Phase 4 设计稿。** `PlayerController` / `PlaylistManager` /
> `LyricManager` 当前是空 placeholder 类，仅声明 Q_OBJECT 和构造函数。
> 下面的 API 是 Phase 4 启动时要实现的目标接口。

```cpp
// PlayerController - 播放器核心控制器
class PlayerController : public QObject {
    Q_OBJECT
public:
    void play(int songId);
    void resume();
    void pause();
    void stop();
    void next();
    void previous();
    void seek(double seconds);
    void setVolume(double volume);       // 0.0 - 1.0
    void setPlayMode(PlayMode mode);

signals:
    void stateChanged(PlaybackState state);
    void positionChanged(double current, double total);
    void currentSongChanged(const SongInfo& song);
    void volumeChanged(double volume);
    void playModeChanged(PlayMode mode);
    void errorOccurred(const QString& message);

private:
    std::unique_ptr<AudioEngine> engine_;        // domain
    PlayMode mode_;
    int currentSongId_;
    QTimer positionTimer_;  // ~250ms interval
};
```

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

// Repository abstractions (Phase 2). Currently the concrete SqliteSongRepo
// is a Phase 2 stub that throws std::logic_error from every method.
class ISongRepository {
public:
    virtual ~ISongRepository() = default;
    virtual std::vector<SongInfo> getAllSongs() = 0;
    virtual std::optional<SongInfo> getSongById(int id) = 0;
    virtual int insertSong(const SongInfo& song) = 0;
    // ...
};

class IPlaylistRepository { /* ... */ };
class ISettingsRepository { /* ... */ };
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
- 维护 `shuffleCursor_` 指向当前位置；next 即 `++cursor`，prev 即 `--cursor`
- `cursor == queue.size()` 时重新 shuffle（保证不会立刻重复刚听过的几首）
- 列表内容变化（添加/删除/换列表）时丢弃 shuffleQueue_ 重洗

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
