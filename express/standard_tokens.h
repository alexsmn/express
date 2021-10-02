#pragma once

#include "express/token.h"

namespace expression {

class OperUna : public Token {
 public:
  OperUna(char oper, Token* operand) : oper_{oper}, operand_{operand} {}

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

  virtual void Traverse(TraverseCallback callb, void* param) const override {
    callb(this, param);
    callb(operand_, param);
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

class OperBin : public Token {
 public:
  OperBin(char oper, Token* left, Token* right)
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

  virtual void Traverse(TraverseCallback callb, void* param) const override {
    callb(this, param);
    callb(left_, param);
    callb(right_, param);
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

class LexStr : public Token {
 public:
  LexStr(std::string_view str, Allocator& allocator)
      : len_{static_cast<int>(str.size())},
        str_{static_cast<char*>(allocator.allocate(len_))} {
    memcpy(str_, str.data(), len_);
  }

  virtual Value Calculate(void* data) const override {
    return Value{str_, len_};
  }

  virtual void Traverse(TraverseCallback callb, void* param) const override {
    callb(this, param);
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

class LexLP : public Token {
 public:
  explicit LexLP(Token* oper) : oper_{oper} {}

  virtual Value Calculate(void* data) const override {
    return oper_->Calculate(data);
  }

  virtual void Traverse(TraverseCallback callb, void* param) const override {
    callb(this, param);
    callb(oper_, param);
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
