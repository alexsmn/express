#pragma once

#include "express/formatter_delegate.h"
#include "express/value.h"

#include <string>

namespace expression {

class Allocator;
class Token;

template <class BasicToken>
using BasicTraverseCallback = bool (*)(const BasicToken* token, void* param);

using TraverseCallback = BasicTraverseCallback<Token>;

class EXPRESS_EXPORT Token {
 public:
  virtual Value Calculate(void* data) const = 0;

  virtual void Traverse(TraverseCallback callback, void* param) const = 0;

  virtual void Format(const FormatterDelegate& delegate,
                      std::string& str) const = 0;
};

class PolymorphicToken {
 public:
  explicit PolymorphicToken(const Token& token) : token_{&token} {}

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

template <class T>
class ValueToken : public Token {
 public:
  template <class U>
  explicit ValueToken(U&& value) : value_{std::forward<U>(value)} {}

  virtual Value Calculate(void* data) const override { return value_; }

  virtual void Traverse(TraverseCallback callback, void* param) const {
    callback(this, param);
  }

  virtual void Format(const FormatterDelegate& delegate,
                      std::string& str) const override {
    delegate.AppendDouble(str, value_);
  }

 private:
  const T value_;
};

template <class T, class... Args>
inline Token* CreateToken(Allocator& allocator, Args&&... args) {
  auto* data = allocator.allocate(sizeof(T));
  return new (data) T(std::forward<Args>(args)...);
}

template <class T>
inline Token* CreateValueToken(Allocator& allocator, T value) {
  return CreateToken<ValueToken<T>>(allocator, value);
}

}  // namespace expression