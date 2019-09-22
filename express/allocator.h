#pragma once

#include "express/express_export.h"

#include <memory.h>
#include <algorithm>
#include <cassert>
#include <vector>

namespace expression {

class EXPRESS_EXPORT Allocator {
 public:
  Allocator() noexcept {}

  Allocator(const Allocator&) = delete;
  Allocator& operator=(const Allocator&) = delete;

  Allocator(Allocator&& source) noexcept : chunks_{std::move(source.chunks_)} {}

  Allocator& operator=(Allocator&& source) noexcept {
    chunks_ = std::move(source.chunks_);
    return *this;
  }

  void* allocate(size_t len) {
    if (!chunks_.empty()) {
      void* ptr = allocate_from_chunk(chunks_.back(), len);
      if (ptr)
        return ptr;
    }

    auto& chunk = allocate_chunk(len);
    auto* ptr = allocate_from_chunk(chunk, len);
    assert(ptr);
    return ptr;
  }

  void swap(Allocator& other) noexcept { std::swap(chunks_, other.chunks_); }

  void clear() noexcept { chunks_.clear(); }

 private:
  using Chunk = std::vector<char>;

  Chunk& allocate_chunk(size_t capacity) {
    auto& chunk = chunks_.emplace_back();
    chunk.reserve(std::max(kMinCapacity, capacity));
    return chunk;
  }

  void* allocate_from_chunk(Chunk& chunk, size_t len) noexcept {
    if (len > chunk.capacity() - chunk.size())
      return nullptr;

    size_t start = chunk.size();
    chunk.insert(chunk.end(), len, 0);

    return chunk.data() + start;
  }

  // The inner vectors are never reallocated after C++11.
  std::vector<std::vector<char>> chunks_;

  static inline const size_t kMinCapacity = 64;
};

}  // namespace expression
