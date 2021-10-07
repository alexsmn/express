#pragma once

#include "express/allocator.h"
#include "express/formatter_delegate.h"
#include "express/value.h"

#include <string>

namespace expression {

class Allocator;
class Token;

using TraverseCallback = bool (*)(const Token* token, void* param);

class EXPRESS_EXPORT Token {
 public:
  virtual Value Calculate(void* data) const = 0;

  virtual void Traverse(TraverseCallback callback, void* param) const = 0;

  virtual void Format(const FormatterDelegate& delegate,
                      std::string& str) const = 0;
};

class PolymorphicToken {
 public:
  explicit PolymorphicToken(const Token* token) : token_{token} {}

  PolymorphicToken(PolymorphicToken&&) = default;
  PolymorphicToken& operator=(PolymorphicToken&&) = default;

  const Token* token() const { return token_; }

  Value Calculate(void* data) const {
    assert(token_);
    return token_->Calculate(data);
  }

  void Traverse(TraverseCallback callback, void* param) const {
    assert(token_);
    return token_->Traverse(callback, param);
  }

  void Format(const FormatterDelegate& delegate, std::string& str) const {
    assert(token_);
    return token_->Format(delegate, str);
  }

 private:
  const Token* token_ = nullptr;
};

template <class T, class... Args>
inline Token* CreateToken(Allocator& allocator, Args&&... args) {
  auto* data = allocator.allocate(sizeof(T));
  return new (data) T(std::forward<Args>(args)...);
}

template <class T, class... Args>
inline PolymorphicToken MakePolymorphicToken(Allocator& allocator,
                                             Args&&... args) {
  return PolymorphicToken{
      CreateToken<T>(allocator, std::forward<Args>(args)...)};
}

}  // namespace expression