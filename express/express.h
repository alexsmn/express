#pragma once

#include "base/strings/stringprintf.h"
#include "express/buffer.h"
#include "express/express_export.h"
#include "express/value.h"

#include <string>
#include <stdexcept>

namespace expression {

class Expression;
class Function;
class Parser;

#define OPER_BIN	1
#define OPER_UNA	2

#define EXPR_DEF	(-1)	// default position

#define EXPR_CUSTOM_NUM		1

typedef unsigned Lexem;

static const Lexem LEX_END = '\0';
static const Lexem LEX_DBL = 'd'; // double value
static const Lexem LEX_I1 = '1';  // int1 value
static const Lexem LEX_I2 = '2';  // int2 value
static const Lexem LEX_I4 = '4';  // int4 value
static const Lexem LEX_STR = 's'; // string value
static const Lexem LEX_LP = '(';
static const Lexem LEX_RP = ')';
static const Lexem LEX_LE = 'l';
static const Lexem LEX_GE = 'g';
static const Lexem LEX_FUN = '@';
static const Lexem LEX_CUSTOM = 1;
static const Lexem LEX_TOKEN = 2;
static const Lexem LEX_UNA = 0x80;

typedef bool (*TraverseCallback)(const Expression& expr, Lexem lexem, void* param);

// approximates what buffer is set to expression position
inline Value expr_calc(Expression& buf);

class EXPRESS_EXPORT Expression {
 public:
  Expression() : buffer(0, 0), flags(0) { }
  virtual ~Expression() { Clear(); }

  Expression(const Expression&) = delete;
  Expression& operator=(const Expression&) = delete;

  void Parse(const char* buf);

  Value Calculate(void* data = NULL) const;

  void Clear();

  // should fill lexem of throw exception
  virtual void ReadLexem(Parser& parser) {
    throw std::runtime_error("ReadLexem");
  }

  // return written lexem position or EXPR_DEF for default handler
  virtual int WriteLexem(Parser& parser) {
    return EXPR_DEF;
  }

  virtual void CalculateLexem(int pos, Lexem lexem, Value& val, void* data) const {
    throw std::runtime_error("CalculateLexem");
  }

  virtual std::string FormatLexem(int pos, Lexem lexem) const {
    throw std::runtime_error("FormatLexem");
  }

  virtual void TraverseLexem(int pos, Lexem lexem, TraverseCallback callb,
                              void* param) const {
    callb(*this, lexem, param);
  }

  virtual Function* find_function(const char* name);

  void CalculateNode(int pos, Value& val, void* data = NULL) const;

  void TraverseNode(int pos, TraverseCallback callb, void* param) const;

  std::string FormatNode(int pos) const;

  void swap(Expression& other) {
    std::swap(root, other.root);
    std::swap(flags, other.flags);

    std::swap(buffer.buf, other.buffer.buf);
    std::swap(buffer.size, other.buffer.size);
    std::swap(buffer.cap, other.buffer.cap);
  }

  int root;
  int flags;
  Buffer buffer;
};

} // namespace expression
