#pragma once

#include "express/allocator.h"
#include "express/express_export.h"
#include "express/lexem.h"
#include "express/token.h"

#include <optional>

namespace expression {

class ParserDelegate;
class Token;
class Lexer;

class EXPRESS_EXPORT Parser {
 public:
  Parser(Lexer& lexer, Allocator& allocator, ParserDelegate& delegate);

  Parser(const Parser&) = delete;
  Parser& operator=(const Parser&) = delete;

  Token* CreatePrimaryToken();
  Token* CreateBinaryOperator(int priority = 0);

  std::optional<PolymorphicToken> Parse();

  const Lexem& next_lexem() const { return next_lexem_; }
  void ReadLexem();

 private:
  Lexer& lexer_;
  Allocator& allocator_;
  ParserDelegate& delegate_;

  Lexem next_lexem_{LEX_END};
};

}  // namespace expression