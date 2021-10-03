#pragma once

#include "express/allocator.h"
#include "express/express_export.h"
#include "express/lexem.h"
#include "express/parser_delegate.h"
#include "express/token.h"

#include <optional>

namespace expression {

class Token;
class Lexer;

class EXPRESS_EXPORT Parser {
 public:
  Parser(Lexer& lexer,
         Allocator& allocator,
         BasicParserDelegate<PolymorphicToken>& delegate);

  Parser(const Parser&) = delete;
  Parser& operator=(const Parser&) = delete;

  PolymorphicToken MakePrimaryToken();
  PolymorphicToken MakeBinaryOperator(int priority = 0);

  std::optional<PolymorphicToken> Parse();

  const Lexem& next_lexem() const { return next_lexem_; }
  void ReadLexem();

 private:
  Lexer& lexer_;
  Allocator& allocator_;
  BasicParserDelegate<PolymorphicToken>& delegate_;

  Lexem next_lexem_{LEX_END};
};

}  // namespace expression