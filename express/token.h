#pragma once

#include "express/formatter_delegate.h"
#include "express/value.h"

#include <string>

namespace expression {

class Token;

typedef bool (*TraverseCallback)(const Token* lexem, void* param);

class EXPRESS_EXPORT Token {
 public:
  virtual Value Calculate(void* data) const = 0;

  virtual void Traverse(TraverseCallback callb, void* param) const = 0;

  virtual void Format(const FormatterDelegate& delegate,
                      std::string& str) const = 0;
};

template <class T>
class ValueToken : public Token {
 public:
  explicit ValueToken(T value) : value_{value} {}

  virtual Value Calculate(void* data) const override { return value_; }

  virtual void Traverse(TraverseCallback callb, void* param) const {
    callb(this, param);
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