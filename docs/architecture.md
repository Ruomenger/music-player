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
    virtual ~IAudioDecoder() = default;
    virtual bool open(const std::string& filePath) = 0;
    virtual std::vector<PcmFrame> decode(int maxFrames) = 0;
    virtual double duration() const = 0;
    virtual double sampleRate() const = 0;
    virtual int channels() const = 0;
    virtual bool seek(double seconds) = 0;
    virtual void close() = 0;
};

class IAudioOutput {
public:
    virtual ~IAudioOutput() = default;
    virtual bool init(double sampleRate, int channels) = 0;
    virtual bool start() = 0;
    virtual bool stop() = 0;
    virtual bool pause() = 0;
    // callback: fill buffer with PCM frames, returns frames written
    virtual void setDataCallback(std::function<int(float*, int)> callback) = 0;
};

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

## 播放模式状态机

```
                ┌──────────┐
    ┌───────────│ SEQUENTIAL│───────────┐
    │           └──────────┘           │
    │  (曲目正常播完)                  │ (列表最后一首)
    ▼                                  ▼
┌─────────┐  (单曲播完)  ┌──────────────┐
│  SINGLE  │◄────────────│ LIST_LOOP    │
└─────────┘              └──────────────┘
                              │
                              │ (随机选取)
    ▲                        │
    │        ┌────────┐      │
    └────────│ RANDOM │◄─────┘
             └────────┘
```

| 模式 | 行为 |
|------|------|
| Single | 当前曲循环播放 |
| Sequential | 按列表顺序播放，播完最后一首停止 |
| ListLoop | 顺序播放，播完最后一首回到第一首 |
| Random | 随机选曲 (Fisher-Yates shuffle + 游标，避免短期重复) |

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
- PlayerController 通过 `std::atomic` 标志控制播放/暂停/停止状态
- 所有 UI 状态变更通过 `QMetaObject::invokeMethod` 或信号/槽回主线程
