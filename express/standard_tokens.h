#pragma once

#include "express/token.h"

namespace expression {

template <class OperandToken>
class BasicUnaryOperatorToken : public Token {
 public:
  template <class U>
  BasicUnaryOperatorToken(char oper, U&& operand)
      : operator_{oper}, operand_{std::forward<U>(operand)} {}

  virtual Value Calculate(void* data) const override {
    auto val = operand_.Calculate(data);
    switch (operator_) {
      case '-':
        val = -val;
        break;
      case '!':
        val = !val;
        break;
      default:
        assert(false);
        break;
    }
    return val;
  }

  virtual void Traverse(TraverseCallback callback, void* param) const override {
    callback(this, param);
    operand_.Traverse(callback, param);
  }

  virtual void Format(const FormatterDelegate& delegate,
                      std::string& str) const override {
    str += operator_;
    operand_.Format(delegate, str);
  }

 private:
  const char operator_;
  const OperandToken operand_;
};

template <class OperandToken>
class BasicBinaryOperatorToken : public Token {
 public:
  template <class L, class R>
  BasicBinaryOperatorToken(char oper, L&& left, R&& right)
      : operator_{oper},
        left_{std::forward<L>(left)},
        right_{std::forward<R>(right)} {}

  virtual Value Calculate(void* data) const override {
    auto val = left_.Calculate(data);
    auto rval = right_.Calculate(data);

    switch (operator_) {
      case '+':
        val += rval;
        break;
      case '-':
        val -= rval;
        break;
      case '*':
        val *= rval;
        break;
      case '/':
        val /= rval;
        break;
      case '^':
        val = pow((double)val, (double)rval);
        break;
      case '=':
        val = val == rval;
        break;
      case '<':
        val = val < rval;
        break;
      case '>':
        val = val > rval;
        break;
      case 'l':
        val = val <= rval;
        break;
      case 'g':
        val = val >= rval;
        break;
      default:
        assert(false);
        break;
    }

    return val;
  }

  virtual void Traverse(TraverseCallback callback, void* param) const override {
    callback(this, param);
    left_.Traverse(callback, param);
    right_.Traverse(callback, param);
  }

  virtual void Format(const FormatterDelegate& delegate,
                      std::string& str) const override {
    left_.Format(delegate, str);
    str += ' ';
    switch (operator_) {
      case 'l':
        str += "<=";
        break;
      case 'g':
        str += ">=";
        break;
      default:
        str += operator_;
        break;
    }
    str += ' ';
    right_.Format(delegate, str);
  }

 private:
  const char operator_;
  const OperandToken left_;
  const OperandToken right_;
};

class StringValueToken : public Token {
 public:
  StringValueToken(std::string_view str, Allocator& allocator)
      : len_{static_cast<int>(str.size())},
        str_{static_cast<char*>(allocator.allocate(len_))} {
    memcpy(str_, str.data(), len_);
  }

  virtual Value Calculate(void* data) const override {
    return Value{str_, len_};
  }

  virtual void Traverse(TraverseCallback callback, void* param) const override {
    callback(this, param);
  }

  virtual void Format(const FormatterDelegate& delegate,
                      std::string& str) const override {
    str += '"';
    str.append(str_, len_);
    str += '"';
  }

 private:
  const int len_;
  char* str_;
};

template <class NestedToken>
class ParenthesesToken : public Token {
 public:
  template <class U>
  explicit ParenthesesToken(U&& nested_token)
      : nested_token_{std::forward<U>(nested_token)} {}

  virtual Value Calculate(void* data) const override {
    return nested_token_.Calculate(data);
  }

  virtual void Traverse(TraverseCallback callback, void* param) const override {
    callback(this, param);
    nested_token_.Traverse(callback, param);
  }

  virtual void Format(const FormatterDelegate& delegate,
                      std::string& str) const override {
    str += '(';
    nested_token_.Format(delegate, str);
    str += ')';
  }

 private:
  const NestedToken nested_token_;
};

}  // namespace expression
