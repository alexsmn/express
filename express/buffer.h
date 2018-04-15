#pragma once

#include <memory.h>
#include <cassert>
#include <stdexcept>

namespace expression {

class Buffer {
 public:
  Buffer() {}
  Buffer(char* buf, int cap) : buf(buf), cap(cap) {}

  Buffer(const Buffer&) = delete;
  Buffer& operator=(const Buffer&) = delete;

  void read(int& pos, void* data, int len) const {
    assert(len > 0);
    if (pos + len > size)
      throw std::runtime_error("read overflow");
    memcpy(data, buf + pos, len);
    pos += len;
  }

  void write(const void* data, int len) {
    assert(len > 0);
    if (size + len > cap)
      throw std::runtime_error("write overflow");
    memcpy(buf + size, data, len);
    size += len;
  }

  void clear() {
    size = 0;
  }

  const char* ptr(int pos) const {
    assert(pos < size);
    return buf + pos;
  }

  bool empty() const {
    return size == 0;
  }

  template<typename T> Buffer& operator <<(const T& val) {
    write(&val, sizeof(val));
    return *this;
  }

  template<typename T> T read(int& pos) const {
    T val;
    read(pos, &val, sizeof(val));
    return val;
  }

  void swap(Buffer& other) {
    std::swap(buf, other.buf);
    std::swap(size, other.size);
    std::swap(cap, other.cap);
  }

  char* buf = nullptr;
  int size = 0;
  int cap = 0;
};

} // namespace expression
