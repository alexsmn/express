#pragma once

#include "express/express_export.h"

#include <memory.h>
#include <cassert>
#include <list>
#include <stdexcept>

namespace expression {

class EXPRESS_EXPORT Allocator {
 public:
  Allocator() {}

  Allocator(const Allocator&) = delete;
  Allocator& operator=(const Allocator&) = delete;

  Allocator(Allocator&& source) : chunks_{std::move(source.chunks_)} {}

  Allocator& operator=(Allocator&& source) {
    chunks_ = std::move(source.chunks_);
    return *this;
  }

  void* allocate(size_t len) {
    if (!chunks_.empty()) {
      void* ptr = chunks_.back().allocate(len);
      if (ptr)
        return ptr;
    }

    chunks_.emplace_back();
    auto* ptr = chunks_.back().allocate(len);
    assert(ptr);
    return ptr;
  }

  void swap(Allocator& other) { std::swap(chunks_, other.chunks_); }

  void clear() { chunks_.clear(); }

 private:
  struct Chunk {
    void* allocate(size_t len) {
      if (size + len > kCapacity)
        return nullptr;

      void* ptr = data + size;
      size += len;
      return ptr;
    }

    static const size_t kCapacity = 64;

    char data[kCapacity];
    size_t size = 0;
  };

  std::list<Chunk> chunks_;
};

}  // namespace expression
