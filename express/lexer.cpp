#include "express/lexer.h"

#include "express/lexer_delegate.h"

#include <cassert>
#include <cctype>
#include <stdexcept>

namespace expression {

Lexer::Lexer(const char* buf, LexerDelegate& delegate, int flags)
    : buf_{buf}, delegate_{delegate}, flags_{flags} {}

std::optional<Lexem> Lexer::ReadNumber() {
  bool res = false;
  double num = 0;
  double exp = 0;
  for (;;) {
    if (*buf_ == '.') {
      // check second dot
      if (exp)
        throw std::runtime_error("bad number");
      exp = 1;
    } else {
      // read next digit
      int digit = int(*buf_) - int('0');
      if (digit < 0 || digit > 9)
        break;
      num = num * 10 + digit;
      exp *= 10;
      res = true;
    }
    buf_++;
  }
  if (!res)
    return std::nullopt;

  Lexem lexem = {};
  lexem.lexem = LEX_DBL;
  if (exp)
    lexem._double = num / exp;
  else
    lexem._double = num;
  return lexem;
}

std::optional<Lexem> Lexer::ReadString() {
  assert(*buf_ == '"');
  buf_++;
  auto* start = buf_;
  while (*buf_ != '"') {
    if (!*buf_)
      throw std::runtime_error("unterminated string");
    buf_++;
  }

  Lexem lexem = {};
  lexem.lexem = LEX_STR;
  lexem._string = std::string_view{start, static_cast<size_t>(buf_ - start)};
  buf_++;

  return lexem;
}

Lexem Lexer::ReadLexem() {
repeat:
  Lexem lexem = {LEX_END};
  switch (*buf_) {
    case ' ':
    case '\t':
    case '\n':
      buf_++;
      goto repeat;
    case '\0':
    case '(':
    case ')':
      lexem.lexem = *buf_++;
      return lexem;
    case '!':
      lexem.lexem = *buf_++, lexem.type = OPER_UNA;
      return lexem;
    case '=':
      lexem.lexem = *buf_++, lexem.type = OPER_BIN, lexem.priority = 0;
      return lexem;
    case '>':
      lexem.lexem = *buf_++, lexem.type = OPER_BIN, lexem.priority = 0;
      if (*buf_ == '=')
        lexem.lexem = LEX_GE, buf_++;
      return lexem;
    case '<':
      lexem.lexem = *buf_++, lexem.type = OPER_BIN, lexem.priority = 0;
      if (*buf_ == '=')
        lexem.lexem = LEX_LE, buf_++;
      return lexem;
    case '-':
      lexem.lexem = *buf_++, lexem.type = OPER_BIN | OPER_UNA,
      lexem.priority = 1;
      return lexem;
    case '+':
      lexem.lexem = *buf_++, lexem.type = OPER_BIN, lexem.priority = 1;
      return lexem;
    case '*':
    case '/':
      lexem.lexem = *buf_++, lexem.type = OPER_BIN, lexem.priority = 2;
      return lexem;
    case '^':
      lexem.lexem = *buf_++, lexem.type = OPER_BIN, lexem.priority = 3;
      return lexem;
    case '"':
      lexem.lexem = LEX_STR;
      ReadString();
      return lexem;
    case ',':
      lexem.lexem = *buf_++;
      return lexem;

    default:
      if (!(flags_ & EXPR_CUSTOM_NUM)) {
        if (auto lexem = ReadNumber())
          return *lexem;
      }
      ReadBuffer buffer{buf_};
      if (auto lexem = delegate_.ReadLexem(buffer))
        return *lexem;
      if (auto lexem = ReadStandardName())
        return *lexem;
      throw std::runtime_error("Wrong lexem");
  }
}

std::optional<Lexem> Lexer::ReadStandardName() {
  const char* buf = buf_;
  if (!std::isalpha(*buf))
    return std::nullopt;

  // read name
  const char* str = buf_;
  do {
    buf++;
  } while (std::isalnum(*buf));
  int strl = static_cast<int>(buf - str);
  buf_ = buf;

  Lexem lexem = {};
  lexem.lexem = expression::LEX_NAME;
  lexem._string = std::string_view{str, static_cast<size_t>(strl)};
  return lexem;
}

}  // namespace expression