#pragma once

#include "express/basic_expression.h"
#include "express/express_export.h"
#include "express/parser_delegate.h"
#include "express/token.h"

namespace expression {

class LexerDelegate;
class ParserDelegate;

class EXPRESS_EXPORT Expression : public BasicExpression<PolymorphicToken> {
 public:
  void Parse(const char* buf);

  void Parse(const char* buf,
             LexerDelegate& lexer_delegate,
             BasicParserDelegate<PolymorphicToken>& parser_delegate,
             int flags = 0);

  using BasicExpression::Parse;

  void Traverse(TraverseCallback callback, void* param) const;

  using BasicExpression::Traverse;
};

}  // namespace expression
