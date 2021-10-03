#pragma once

#include "express/token.h"

namespace expression {

class UnaryOperatorToken : public Token {
 public:
  UnaryOperatorToken(char oper, Token* operand)
      : oper_{oper}, operand_{operand} {}

  virtual Value Calculate(void* data) const override {
    auto val = operand_->Calculate(data);
    switch (oper_) {
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
    callback(operand_, param);
  }

  virtual void Format(const FormatterDelegate& delegate,
                      std::string& str) const override {
    str += oper_;
    operand_->Format(delegate, str);
  }

 private:
  const char oper_;
  Token* operand_;
};

class BinaryOperatorToken : public Token {
 public:
  BinaryOperatorToken(char oper, Token* left, Token* right)
      : oper_{oper}, left_{left}, right_{right} {}

  virtual Value Calculate(void* data) const override {
    auto val = left_->Calculate(data);
    auto rval = right_->Calculate(data);

    switch (oper_) {
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
    callback(left_, param);
    callback(right_, param);
  }

  virtual void Format(const FormatterDelegate& delegate,
                      std::string& str) const override {
    left_->Format(delegate, str);
    str += ' ';
    switch (oper_) {
      case 'l':
        str += "<=";
        break;
      case 'g':
        str += ">=";
        break;
      default:
        str += oper_;
        break;
    }
    str += ' ';
    right_->Format(delegate, str);
  }

 private:
  const char oper_;
  Token* left_;
  Token* right_;
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

class ParenthesesToken : public Token {
 public:
  explicit ParenthesesToken(Token* oper) : oper_{oper} {}

  virtual Value Calculate(void* data) const override {
    return oper_->Calculate(data);
  }

  virtual void Traverse(TraverseCallback callback, void* param) const override {
    callback(this, param);
    callback(oper_, param);
  }

  virtual void Format(const FormatterDelegate& delegate,
                      std::string& str) const override {
    str += '(';
    oper_->Format(delegate, str);
    str += ')';
  }

 private:
  Token* oper_;
};

}  // namespace expression
