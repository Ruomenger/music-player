#include "ring_buffer.h"

#include <algorithm>
#include <cstring>

namespace musicplayer {

template<typename T>
RingBuffer<T>::RingBuffer(size_t capacity)
    : buffer_(nextPowerOfTwo(capacity)), mask_(buffer_.size() - 1) {}

template<typename T>
size_t RingBuffer<T>::write(const T* data, size_t count) {
    size_t free = freeSlots();
    size_t toWrite = std::min(count, free);
    if (toWrite == 0)
        return 0;

    size_t w = writeIdx_.load(std::memory_order_relaxed);
    size_t firstChunk = std::min(toWrite, buffer_.size() - (w & mask_));

    std::memcpy(&buffer_[w & mask_], data, firstChunk * sizeof(T));
    std::memcpy(buffer_.data(), data + firstChunk, (toWrite - firstChunk) * sizeof(T));

    writeIdx_.store(w + toWrite, std::memory_order_release);
    return toWrite;
}

template<typename T>
size_t RingBuffer<T>::read(T* data, size_t count) {
    size_t avail = available();
    size_t toRead = std::min(count, avail);
    if (toRead == 0)
        return 0;

    size_t r = readIdx_.load(std::memory_order_relaxed);
    size_t firstChunk = std::min(toRead, buffer_.size() - (r & mask_));

    std::memcpy(data, &buffer_[r & mask_], firstChunk * sizeof(T));
    std::memcpy(data + firstChunk, buffer_.data(), (toRead - firstChunk) * sizeof(T));

    readIdx_.store(r + toRead, std::memory_order_release);
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

template<typename T>
size_t RingBuffer<T>::freeSlots() const {
    return buffer_.size() - available();
}

template class RingBuffer<float>;

}  // namespace musicplayer
