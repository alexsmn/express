#pragma once

#include "base/strings/stringprintf.h"
#include "express/buffer.h"
#include "express/express_export.h"
#include "express/value.h"

#include <stdexcept>
#include <string>

namespace expression {

class Expression;
class ExpressionDelegate;
class Function;
class Parser;

#define OPER_BIN 1
#define OPER_UNA 2

#define EXPR_DEF (-1)  // default position

#define EXPR_CUSTOM_NUM 1

typedef unsigned Lexem;

static const Lexem LEX_END = '\0';
static const Lexem LEX_DBL = 'd';  // double value
static const Lexem LEX_I1 = '1';   // int1 value
static const Lexem LEX_I2 = '2';   // int2 value
static const Lexem LEX_I4 = '4';   // int4 value
static const Lexem LEX_STR = 's';  // string value
static const Lexem LEX_LP = '(';
static const Lexem LEX_RP = ')';
static const Lexem LEX_LE = 'l';
static const Lexem LEX_GE = 'g';
static const Lexem LEX_FUN = '@';
static const Lexem LEX_CUSTOM = 1;
static const Lexem LEX_TOKEN = 2;
static const Lexem LEX_UNA = 0x80;

typedef bool (*TraverseCallback)(const Expression& expr,
                                 Lexem lexem,
                                 void* param);

class EXPRESS_EXPORT Expression {
 public:
  explicit Expression(ExpressionDelegate& delegate)
      : delegate_{delegate} {}
  ~Expression() { Clear(); }

  Expression(const Expression&) = delete;
  Expression& operator=(const Expression&) = delete;

  void Parse(const char* buf, int flags = 0);

  Value Calculate(void* data = NULL) const;

  void Clear();

  void CalculateNode(int pos, Value& val, void* data = NULL) const;

  void TraverseNode(int pos, TraverseCallback callb, void* param) const;

  std::string FormatNode(int pos) const;

  void swap(Expression& other) {
    std::swap(root, other.root);
    buffer.swap(other.buffer);
  }

  ExpressionDelegate& delegate_;
  int root = -1;
  Buffer buffer;
};

}  // namespace expression
