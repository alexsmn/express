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

  if (exp)
    return Lexem::Double(num / exp);
  else
    return Lexem::Double(num);
}

Lexem Lexer::ReadString() {
  assert(*buf_ == '"');

  buf_++;
  auto* start = buf_;
  while (*buf_ != '"') {
    if (!*buf_)
      throw std::runtime_error("unterminated string");
    buf_++;
  }

  std::string_view str{start, static_cast<size_t>(buf_ - start)};
  buf_++;

  return Lexem::String(LEX_STR, str);
}

Lexem Lexer::ReadLexem() {
repeat:
  Lexem lexem = Lexem{LEX_END};
  switch (*buf_) {
    case ' ':
    case '\t':
    case '\n':
      buf_++;
      goto repeat;
    case '\0':
    case '(':
    case ')':
    case ',':
      return Lexem(*buf_++);
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
      return Lexem(*buf_++, OPER_BIN | OPER_UNA, 1);
    case '+':
      return Lexem(*buf_++, OPER_BIN, 1);
    case '*':
    case '/':
      return Lexem(*buf_++, OPER_BIN, 2);
    case '^':
      return Lexem(*buf_++, OPER_BIN, 3);
    case '"':
      return ReadString();

    default:
      if (!(flags_ & EXPR_CUSTOM_NUM)) {
        if (auto number_lexem = ReadNumber())
          return *number_lexem;
      }
      ReadBuffer buffer{buf_};
      if (auto custom_lexem = delegate_.ReadLexem(buffer))
        return *custom_lexem;
      if (auto name_lexem = ReadStandardName())
        return *name_lexem;
      throw std::runtime_error("Wrong lexem");
  }
}

std::optional<Lexem> Lexer::ReadStandardName() {
  const char* buf = buf_;
  if (!std::isalpha(*buf))
    return std::nullopt;

  // read name
  const char* start = buf_;
  do {
    buf++;
  } while (std::isalnum(*buf));
  buf_ = buf;

  std::string_view str(start, static_cast<size_t>(buf - start));
  return Lexem::String(LEX_NAME, str);
}

}  // namespace expression