#pragma once

#include <atomic>
#include <cstddef>
#include <vector>

namespace musicplayer {

template<typename T>
class RingBuffer {
public:
    explicit RingBuffer(size_t capacity);
    size_t write(const T* data, size_t count);
    size_t read(T* data, size_t count);
    void clear();
    size_t available() const;

private:
    std::vector<T> buffer_;
    std::atomic<size_t> readIdx_{0};
    std::atomic<size_t> writeIdx_{0};
    size_t mask_;
};

} // namespace musicplayer
