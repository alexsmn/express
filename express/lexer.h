#pragma once

#include "express/express_export.h"
#include "express/lexem.h"

#include <optional>

namespace expression {

class LexerDelegate;

class EXPRESS_EXPORT Lexer {
 public:
  using Lexem = expression::Lexem;

  Lexer(const char* buf, LexerDelegate& delegate, int flags);

  Lexer(const Lexer&) = delete;
  Lexer& operator=(const Lexer&) = delete;

  std::optional<Lexem> ReadNumber();
  std::optional<Lexem> ReadStandardName();
  Lexem ReadString();

  Lexem ReadLexem();

 private:
  const char* buf_;
  LexerDelegate& delegate_;
  const int flags_;
};

}  // namespace expression
