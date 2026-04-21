#pragma once

#include "express/basic_expression.h"
#include "express/express_export.h"
#include "express/parser_delegate.h"
#include "express/token.h"

namespace expression {

class Lexer;
class LexerDelegate;
class ParserDelegate;

class EXPRESS_EXPORT Expression : public BasicExpression<PolymorphicToken> {
 public:
  using BasicExpression::Parse;

  void Traverse(TraverseCallback callback, void* param) const;

  using BasicExpression::Traverse;
};

}  // namespace expression
