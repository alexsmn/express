#pragma once

#include "express/express_export.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <memory>
#include <new>
#include <utility>
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

  void* allocate(size_t len,
                 size_t alignment = alignof(std::max_align_t)) {
    if (!chunks_.empty()) {
      void* ptr = allocate_from_chunk(chunks_.back(), len, alignment);
      if (ptr)
        return ptr;
    }

    auto& chunk = allocate_chunk(len, alignment);
    auto* ptr = allocate_from_chunk(chunk, len, alignment);
    assert(ptr);
    return ptr;
  }

  void swap(Allocator& other) noexcept { std::swap(chunks_, other.chunks_); }

  void clear() noexcept { chunks_.clear(); }

 private:
  struct Chunk {
    explicit Chunk(size_t capacity, size_t alignment)
        : data_{static_cast<std::byte*>(::operator new(
              capacity, std::align_val_t{alignment}))},
          capacity_{capacity},
          alignment_{alignment} {}

    ~Chunk() {
      if (data_)
        ::operator delete(data_, std::align_val_t{alignment_});
    }

    Chunk(const Chunk&) = delete;
    Chunk& operator=(const Chunk&) = delete;

    Chunk(Chunk&& other) noexcept
        : data_{std::exchange(other.data_, nullptr)},
          size_{std::exchange(other.size_, 0)},
          capacity_{std::exchange(other.capacity_, 0)},
          alignment_{std::exchange(other.alignment_, alignof(std::max_align_t))} {
    }

    Chunk& operator=(Chunk&& other) noexcept {
      if (this == &other)
        return *this;

      if (data_)
        ::operator delete(data_, std::align_val_t{alignment_});

      data_ = std::exchange(other.data_, nullptr);
      size_ = std::exchange(other.size_, 0);
      capacity_ = std::exchange(other.capacity_, 0);
      alignment_ =
          std::exchange(other.alignment_, alignof(std::max_align_t));
      return *this;
    }

    std::byte* data_ = nullptr;
    size_t size_ = 0;
    size_t capacity_ = 0;
    size_t alignment_ = alignof(std::max_align_t);
  };

  Chunk& allocate_chunk(size_t capacity, size_t alignment) {
    auto& chunk = chunks_.emplace_back(
        std::max(kMinCapacity, capacity + alignment - 1),
        std::max(alignment, alignof(std::max_align_t)));
    return chunk;
  }

  void* allocate_from_chunk(Chunk& chunk,
                            size_t len,
                            size_t alignment) noexcept {
    if (chunk.size_ >= chunk.capacity_)
      return nullptr;

    void* ptr = chunk.data_ + chunk.size_;
    size_t space = chunk.capacity_ - chunk.size_;
    if (!std::align(alignment, len, ptr, space))
      return nullptr;

    auto* aligned_ptr = static_cast<std::byte*>(ptr);
    chunk.size_ = static_cast<size_t>(aligned_ptr - chunk.data_) + len;
    return aligned_ptr;
  }

  std::vector<Chunk> chunks_;

  static inline const size_t kMinCapacity = 64;
};

}  // namespace expression
