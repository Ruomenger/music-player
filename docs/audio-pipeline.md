# 音频播放管线设计

## 管线流程图

```
                    ┌─────────────┐
    音乐文件 ──────►│ FFmpeg Demux│
                    └──────┬──────┘
                           │ 压缩数据包 (AVPacket)
                           ▼
                    ┌─────────────┐
                    │ FFmpeg Decode│
                    └──────┬──────┘
                           │ PCM 帧 (AVFrame)
                           ▼
                    ┌─────────────┐
                    │  SampleFmt   │  (可选)
                    │  Converter   │  fltp → flt / s16 → flt
                    └──────┬──────┘
                           │
                           ▼
                    ┌─────────────┐
                    │  RingBuffer  │  (线程安全 SPSC)
                    └──────┬──────┘
                           │
                           ▼
                    ┌─────────────┐
                    │  PortAudio   │
                    │  Callback    │  → 声卡输出
                    └─────────────┘
```

## 线程模型

```
┌─── Decode Thread ──────────────────────────────────────┐
│                                                         │
│  while (playing) {                                      │
│      if (ringBuffer.full()) {                           │
│          sleep(10ms);  // 等待消费者                    │
│          continue;                                      │
│      }                                                  │
│      auto frames = decoder->decode(FRAMES_PER_CHUNK);   │
│      ringBuffer.push(frames);                           │
│  }                                                      │
│                                                         │
└─────────────────────────────────────────────────────────┘
          │                                    ▲
          │ push                               │ pop
          ▼                                    │
┌─── RingBuffer (float*) ─────────────────────────────────┐
│                    ...                                  │
└─────────────────────────────────────────────────────────┘
          │                                    ▲
          │ pop (in callback)                  │ push
          ▼                                    │
┌─── PortAudio Callback Thread ──────────────────────────┐
│                                                         │
│  int callback(void* out, void* in,                      │
│               unsigned long framesPerBuffer, ...) {     │
│      float* dst = (float*)out;                          │
│      return ringBuffer.pop(dst, framesPerBuffer)        │
│              ? paContinue                               │
│              : paComplete;                              │
│  }                                                      │
└─────────────────────────────────────────────────────────┘
```

## RingBuffer 设计

```cpp
template<typename T>
class RingBuffer {
public:
    explicit RingBuffer(size_t capacity);

    // 生产者: 写入 count 个元素, 返回实际写入数 (容量不足时 < count, 调用方负责保留剩余)
    size_t write(const T* data, size_t count);

    // 消费者: 读取 count 个元素, 返回实际读取数
    size_t read(T* data, size_t count);

    // 清空缓冲区 (仅在 SPSC 双方都已停止时调用)
    void clear();

    size_t available() const;   // 可读元素数
    size_t freeSlots() const;   // 可写元素数

private:
    std::vector<T> buffer_;
    std::atomic<size_t> readIdx_{0};
    std::atomic<size_t> writeIdx_{0};
    size_t mask_;   // capacity - 1 (capacity 为 2 的幂时可用位掩码)
};
```

- **存储单元是 sample (interleaved float)**，不是 frame；2-channel 立体声每 1 frame = 2 samples。
- 容量取 2 的幂。当前实现 `1 << 17` = 131072 samples = 65536 frames ≈ 1.5s @ 44.1 kHz stereo。
- `mask_ = capacity - 1` 取代取模运算。
- readIdx / writeIdx 单调递增，差值即可读元素数；用 `std::atomic<size_t>` 实现 lock-free SPSC。
- `read()` 在 PortAudio 回调中调用，必须非阻塞且极快（无锁、无系统调用、无堆分配）。
- **`write()` 是截断写**：写不下时只写一部分并返回实际写入数。**调用方必须检查返回值并把剩余样本保留到下次写入**，否则会丢音 (听感上是周期性卡顿/电流声)。

## FFmpeg 解码封装

### 资源管理 (RAII)

```cpp
using AVFormatCtx = std::unique_ptr<AVFormatContext,
    decltype(&avformat_close_input)>;
using AVCodecCtx = std::unique_ptr<AVCodecContext,
    decltype(&avcodec_free_context)>;
using AVFramePtr = std::unique_ptr<AVFrame,
    decltype(&av_frame_free)>;
using AVPacketPtr = std::unique_ptr<AVPacket,
    decltype(&av_packet_free)>;
using SwrCtx = std::unique_ptr<SwrContext,
    decltype(&swr_free)>;
```

### FfmpegDecoder 接口

```cpp
class FfmpegDecoder : public IAudioDecoder {
public:
    bool open(const std::string& filePath) override;
    AudioDecoderInfo info() const override;
    std::vector<float> decode(size_t maxFrames) override;  // interleaved float, -1.0 ~ 1.0
    bool seek(double seconds) override;
    void close() override;

    // 元数据读取 (Phase 3)
    std::optional<std::vector<uint8_t>> readCoverArt();
    std::unordered_map<std::string, std::string> getMetadata();
};
```

### 解码流程要点

1. `avformat_open_input()` → `avformat_find_stream_info()` → 定位音频 stream
2. `avcodec_find_decoder()` → `avcodec_open2()` → 创建解码上下文
3. 创建 `SwrContext` 将任意 sample format 转换为 `AV_SAMPLE_FMT_FLT` (interleaved float)
4. 循环: `av_read_frame()` → `avcodec_send_packet()` → `avcodec_receive_frame()` → `swr_convert()`
5. 输出统一为 `std::vector<float>` (interleaved, -1.0 ~ 1.0)

### `decode(maxFrames)` 契约（重要）

这是 **AudioEngine 与 Decoder 之间的硬约束**，违反会导致 RingBuffer 截断丢样本，听感上是周期性卡顿+咔哒声：

| 规则 | 说明 |
|------|------|
| **不超量返回** | 返回值的 frame 数严格 ≤ `maxFrames`。MP3/AAC 等固定 frame 大小格式 (1024/1152 frames per AVFrame) 容易循环写超过 `maxFrames`，必须每次 `swr_convert` 时把 `out_count` 限定为 `maxFrames - alreadyWritten`。 |
| **swr 内部缓冲跨调用持续** | `swr_convert` 因输出空间不足而保留的输入样本，必须在下次 `decode()` 开头通过 `swr_convert(out, want, NULL, 0)` 排干，不可丢弃。 |
| **EOF 排干尾部** | 文件读完并向解码器送 nullptr 后，必须再排一次 swr 内部 buffer，避免最后几十毫秒丢失。 |
| **空返回即 EOF** | 仅当文件已读完、解码器已 drain、swr 也无残留时才返回空 vector。中间任何时刻不允许返空。 |
| **同步调用** | `decode()` 是阻塞 I/O，**不允许从 PortAudio 回调线程调用**。只能由专门的 decode 线程调用。 |

### Seek 实现

```cpp
bool seek(double seconds) {
    int64_t target = static_cast<int64_t>(
        seconds * AV_TIME_BASE);
    int ret = av_seek_frame(formatCtx_.get(), -1, target,
                            AVSEEK_FLAG_BACKWARD);
    if (ret < 0) return false;
    avcodec_flush_buffers(codecCtx_.get());
    return true;
}
```

- `AVSEEK_FLAG_BACKWARD`: 定位到目标时间之前的关键帧，避免解码不完整
- Seek 后必须 `avcodec_flush_buffers()` 清空解码器内部缓冲
- Seek 后还需清空 RingBuffer，丢弃旧数据

## PortAudio 输出封装

### IAudioOutput 接口 (生命周期分离)

```cpp
class IAudioOutput {
public:
    using DataCallback = std::function<void(float* buffer, int frameCount)>;

    virtual ~IAudioOutput() = default;

    // 分配 native stream 资源 (不开始拉数据)
    virtual bool open(double sampleRate, int channels) = 0;

    // 开始 / 恢复回调拉取 (Pa_StartStream)
    virtual bool start() = 0;

    // 暂停回调拉取，stream 保持打开 (Pa_StopStream)。下次 start() 直接恢复，不需要重 open。
    virtual bool pause() = 0;

    // 关闭并释放 stream (Pa_StopStream + Pa_CloseStream)。再播放需要重新 open()。
    virtual bool stop() = 0;

    virtual void setCallback(DataCallback cb) = 0;
};
```

> **关键**：`pause()` 和 `stop()` 必须分开。把 `pause` 实现成 `stop` 会导致暂停后无法恢复（stream 已关闭，`start()` 失败）。

### PortAudioOutput 状态转换

```
            open()                    start()
   created ────────► opened ───────────────────► running
                       ▲                            │
                       │           pause()          │
                       │ ◄──────────────────────────┤
                       │                            │
                       │           stop()           │
                       └────────────────────────────┘
                       (Pa_CloseStream + null out)
```

### 回调中的硬性约束

PortAudio 回调跑在 OS 实时音频线程，**必须做到**：

- 不加锁 (mutex)
- 不分配内存 (malloc/new/std::vector resize 等)
- 不调用阻塞系统调用 (file I/O / sleep / wait)
- 不调用 PortAudio API (如 `Pa_StopStream`)
- 仅使用 lock-free 的原子类型：`std::atomic<intN_t>` / `std::atomic<bool>` / `std::atomic<指针>`。
  避免 `std::atomic<double>`、`std::atomic<结构体>` —— 在某些平台会退化为锁实现 (可用 `std::atomic<T>::is_always_lock_free` 静态断言)。
- 只做：从 RingBuffer 读数据 → 写入 output buffer → 用 `fetch_add` 等无锁操作更新计数 → return。

跨平台 Host API 由 PortAudio 自动选择 (Linux: PulseAudio/ALSA, macOS: CoreAudio, Windows: WASAPI)。

### 输出设备枚举 (Phase 8)

```cpp
struct AudioOutputDeviceInfo {
    std::string name;
    bool isDefault;
};

// 静态方法：不需要先持有 PortAudioOutput 实例（内部自管理
// Pa_Initialize / Pa_Terminate）。
static std::vector<AudioOutputDeviceInfo>
PortAudioOutput::enumerateOutputDevices();

// 实例方法：把"用户偏好"设备名记下来，open() 时按名匹配 PaDevice；
// 名字不再存在（设备拔了 / OS 改名了）时回退到默认设备并 qWarning，
// 不阻断播放。空字符串 = 用 Pa_GetDefaultOutputDevice()。
void PortAudioOutput::setPreferredDevice(std::string deviceName);
```

SettingsDialog 用 `enumerateOutputDevices()` 填下拉框；main.cpp 启动时把
`settings.output_device` 喂回 `setPreferredDevice`，所以 `AudioEngine::open`
要查 `defaultSampleRate()` 时拿到的是用户指定那张声卡的首选速率。

## 音量控制 (Phase 4)

AudioEngine 持有 `std::atomic<float> volume_{1.0F}`，callback 在从 RingBuffer
读出样本之后、写入声卡之前做一次乘法：

```cpp
const float vol = volume_.load(std::memory_order_acquire);
if (vol != 1.0F) {              // 单位增益走 branch-free 快路径
    for (int i = 0; i < read; ++i)
        buffer[i] *= vol;
}
```

`setVolume(double)` clamp 到 `[0.0, 1.0]` 再 store。`atomic<float>` 在所有
现代平台都是 lock-free（与 atomic<double> 不同），符合 callback 的
"仅 lock-free 原子操作" 约束。

## EOF 自动推进 (Phase 4)

callback 端 CAS 把 state Playing→Stopped 之后，主线程 (PlayerController 的
QTimer) 在下一个 tick 检测到状态变化，按下面规则决定要不要自动推进：

```cpp
// in PlayerController::onPositionTick():
if (fromPlaying && now == Stopped && !userStopRequested_ && engine_->eofReached())
    handleNaturalEnd();  // 调 PlayQueue.advanceNatural() 拿下一首
```

`AudioEngine::eofReached()` 是公开 getter：

- decode 线程在 `decoder_->decode()` 返回空 vector 时把 `eofReached_` 置 true
- callback 看到 ring 排空 + eofReached → CAS 到 Stopped
- 主线程读到 eofReached==true 的 Stopped 判断为"自然结束"，否则视为"用户停"

`userStopRequested_` 是 PlayerController 的局部状态，区分用户主动 stop 与
自然 EOF；自然 EOF 才走自动推进。

## setCallback 时机约束

`PortAudioOutput::setCallback` 在调试构建里 `assert` 流必须处于 stopped /
未 open 状态。realtime callback 不加锁读 `callback_`；运行中替换会撕裂。

## 播放位置追踪

```cpp
class AudioEngine {
    std::atomic<uint64_t> framesPlayed_{0};      // 由 callback 单调累加
    std::atomic<uint64_t> seekFrameOffset_{0};   // 仅在 stream paused 时由 seek 写入

    double currentPosition() const {
        return double(seekFrameOffset_.load() + framesPlayed_.load()) / info_.sampleRate;
    }
};
```

设计要点：

- **callback 端只 `fetch_add`**，不做读-改-写。`fetch_add` 是单条原子指令，保证 lock-free。
- **seek 时**：先 `output_->pause()` 让 callback 静默 → 停 decode 线程 → 清 RingBuffer → `decoder_->seek()` → `seekFrameOffset_ = seconds * sampleRate; framesPlayed_ = 0` → 重启 decode 线程和 stream。整个过程 callback 不在跑，对两个 atomic 的写入无 race。
- **底层用 frame 计数 (`uint64_t`)，不用秒 (`double`)**：
  - `std::atomic<uint64_t>` 在所有现代平台都是 lock-free
  - `std::atomic<double>` 在 ARM32 / 部分嵌入式平台可能退化为锁
  - `uint64_t` 帧计数无浮点累积误差
- **位置按"声卡 wallclock"前进，而非"实际消费帧数"**：underrun 时 callback 仍以 zero-fill 占用时间，position 应同步前进，否则 UI 看起来像"卡住了"。

## 支持的音频格式

FFmpeg 原生支持，重点测试:

| 格式 | 编码 | 优先级 |
|------|------|--------|
| MP3 | MPEG Audio Layer 3 | 高 |
| FLAC | Free Lossless Audio Codec | 高 |
| WAV | PCM | 高 |
| AAC | Advanced Audio Codec (.m4a/.aac) | 中 |
| OGG | Vorbis | 中 |
| OPUS | Opus | 中 |
| WMA | Windows Media Audio | 低 |
| APE | Monkey's Audio | 低 |
| ALAC | Apple Lossless (.m4a) | 低 |

## Gapless 播放 (无缝切换, 可选)

当上一曲剩余 1 秒时, 预先创建第二个 FfmpegDecoder 并开始解码下一曲的前几帧, 放入 RingBuffer 尾部。当前曲结束时无需重新初始化输出流, 直接衔接。

复杂度较高, 可作为 Phase 2+ 功能。
