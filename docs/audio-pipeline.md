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
    
    // 生产者: 写入数据, 返回实际写入帧数
    size_t write(const T* data, size_t count);
    
    // 消费者: 读取数据, 返回实际读取帧数
    size_t read(T* data, size_t count);
    
    // 消费者: 丢弃若干帧 (Seek 时使用)
    void discard(size_t count);
    
    // 清空缓冲区
    void clear();
    
    size_t available() const;   // 可读帧数
    size_t free() const;        // 可写帧数
    bool empty() const;
    bool full() const;
    
private:
    std::vector<T> buffer_;
    std::atomic<size_t> readIdx_{0};
    std::atomic<size_t> writeIdx_{0};
    size_t mask_;   // capacity - 1 (capacity 为 2 的幂时可用位掩码)
};
```

- 容量取 2 的幂 (如 16384 帧 ≈ 0.37秒 @ 44100Hz), 使用 `mask_ = capacity - 1` 取代取模运算
- readIdx 和 writeIdx 用 `std::atomic<size_t>` 实现 lock-free SPSC
- `read()` 在 PortAudio 回调中调用，必须非阻塞且极快

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
    struct Info {
        double duration;      // seconds
        int sampleRate;
        int channels;
        AVSampleFormat sampleFmt;
        std::string codecName;
    };

    bool open(const std::string& filePath) override;
    Info getInfo() const;
    std::vector<float> decode(size_t maxFrames) override;
    bool seek(double seconds) override;
    void close() override;
    
    // 元数据读取
    std::optional<std::vector<uint8_t>> readCoverArt();
    std::unordered_map<std::string, std::string> getMetadata();
};
```

### 解码流程要点

1. `avformat_open_input()` → `avformat_find_stream_info()` → 定位音频 stream
2. `avcodec_find_decoder()` → `avcodec_open2()` → 创建解码上下文
3. 创建 `SwrContext` 将任意 sample format 转换为 `AV_SAMPLE_FMT_FLT` (float planar 或 interleaved)
4. 循环: `av_read_frame()` → `avcodec_send_packet()` → `avcodec_receive_frame()` → `swr_convert()`
5. 输出统一为 `std::vector<float>` (interleaved, -1.0 ~ 1.0)

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

### 初始化和设备选择

```cpp
class PortAudioOutput : public IAudioOutput {
public:
    PortAudioOutput() { Pa_Initialize(); }
    ~PortAudioOutput() { Pa_Terminate(); }

    bool init(double sampleRate, int channels) override {
        PaStreamParameters params;
        params.device = Pa_GetDefaultOutputDevice();
        params.channelCount = channels;
        params.sampleFormat = paFloat32;
        params.suggestedLatency =
            Pa_GetDeviceInfo(params.device)->defaultLowOutputLatency;
        params.hostApiSpecificStreamInfo = nullptr;

        Pa_OpenStream(&stream_, nullptr, &params,
                      sampleRate, paFramesPerBufferUnspecified,
                      paNoFlag, callback, this);
    }
    
private:
    static int callback(const void*, void* output,
                        unsigned long framesPerBuffer,
                        const PaStreamCallbackTimeInfo*,
                        PaStreamCallbackFlags,
                        void* userData);
    
    PaStream* stream_ = nullptr;
    std::function<int(float*, int)> dataCallback_;
};
```

### 注意点

- PortAudio 回调中 **不能**:
  - 加锁 (mutex)
  - 分配内存 (malloc/new)
  - 调用可能阻塞的系统调用
  - 调用 PortAudio API (如 `Pa_StopStream()`)
- 只做: 从 RingBuffer 读数据 → 写入 output buffer → return
- 跨平台 Host API 自动选择 (Linux: PulseAudio/ALSA, macOS: CoreAudio, Windows: WASAPI)

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
