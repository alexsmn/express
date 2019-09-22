#pragma once

#include <string_view>

namespace expression {

using LexemType = unsigned;

static const LexemType LEX_END = '\0';
static const LexemType LEX_DBL = 'd';  // double value
static const LexemType LEX_I1 = '1';   // int1 value
static const LexemType LEX_I2 = '2';   // int2 value
static const LexemType LEX_I4 = '4';   // int4 value
static const LexemType LEX_STR = 's';  // string value
static const LexemType LEX_LP = '(';
static const LexemType LEX_RP = ')';
static const LexemType LEX_LE = 'l';
static const LexemType LEX_GE = 'g';
static const LexemType LEX_NAME = 'm';
static const LexemType LEX_COMMA = ',';
static const LexemType LEX_FUN = '@';
static const LexemType LEX_CUSTOM = 1;
static const LexemType LEX_TOKEN = 2;
static const LexemType LEX_UNA = 0x80;

#define EXPR_DEF (-1)  // default position

#define EXPR_CUSTOM_NUM 1

#define OPER_BIN 1
#define OPER_UNA 2

class Lexem {
 public:
  explicit Lexem(LexemType lexem) : lexem{lexem} {}
  Lexem(LexemType lexem, int type, int priority)
      : lexem{lexem}, type{type}, priority{priority} {}

  static Lexem Double(double value) {
    Lexem lexem{LEX_DBL};
    lexem._double = value;
    return lexem;
  }

  static Lexem String(LexemType type, std::string_view str) {
    Lexem lexem{type};
    lexem._string = str;
    return lexem;
  }

  LexemType lexem = LEX_END;
  int type = 0;
  int priority = 0;
  double _double = 0;
  std::string_view _string;
};

}  // namespace expression