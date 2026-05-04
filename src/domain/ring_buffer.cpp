#include "ring_buffer.h"

namespace musicplayer {

template<typename T>
RingBuffer<T>::RingBuffer(size_t capacity)
    : buffer_(capacity)
    , mask_(capacity - 1) {
}

template<typename T>
size_t RingBuffer<T>::write(const T* data, size_t count) {
    size_t written = 0;
    while (written < count && available() < buffer_.size()) {
        buffer_[writeIdx_ & mask_] = data[written];
        writeIdx_.fetch_add(1, std::memory_order_release);
        ++written;
    }
    return written;
}

template<typename T>
size_t RingBuffer<T>::read(T* data, size_t count) {
    size_t avail = available();
    size_t toRead = (count < avail) ? count : avail;
    for (size_t i = 0; i < toRead; ++i) {
        data[i] = buffer_[(readIdx_ + i) & mask_];
    }
    readIdx_.fetch_add(toRead, std::memory_order_release);
    return toRead;
}

template<typename T>
void RingBuffer<T>::clear() {
    readIdx_.store(0, std::memory_order_release);
    writeIdx_.store(0, std::memory_order_release);
}

template<typename T>
size_t RingBuffer<T>::available() const {
    size_t w = writeIdx_.load(std::memory_order_acquire);
    size_t r = readIdx_.load(std::memory_order_acquire);
    return (w >= r) ? (w - r) : (buffer_.size() - r + w);
}

template class RingBuffer<float>;

} // namespace musicplayer
