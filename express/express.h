#pragma once

#include "express/basic_expression.h"
#include "express/express_export.h"
#include "express/token.h"

namespace expression {

class LexerDelegate;
class ParserDelegate;

class EXPRESS_EXPORT Expression : public BasicExpression<PolymorphicToken> {
 public:
  void Parse(const char* buf);

  void Parse(const char* buf,
             LexerDelegate& lexer_delegate,
             ParserDelegate& parser_delegate,
             int flags = 0);

  using BasicExpression::Parse;
};

}  // namespace expression
