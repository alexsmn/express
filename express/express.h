#pragma once

#include "express/basic_expression.h"
#include "express/express_export.h"
#include "express/parser_delegate.h"
#include "express/standard_tokens.h"
#include "express/token.h"

#include <optional>

namespace expression {

class Lexer;
class LexerDelegate;
class ParserDelegate;

class EXPRESS_EXPORT Expression : public BasicExpression<PolymorphicToken> {
 public:
  void Parse(const char* buf);

  template <class Parser>
  void Parse(Parser& parser, Allocator& allocator) {
    fast_root_token_.reset();
    fast_expression_storage_.reset();
    BasicExpression::Parse(parser, allocator);
  }

  Value Calculate(void* data = NULL) const;
  std::string Format(const FormatterDelegate& delegate) const;
  void Traverse(TraverseCallback callback, void* param) const;
  void Clear();

  using BasicExpression::Traverse;

 private:
  std::optional<detail::FastExpressionStorage> fast_expression_storage_;
  std::optional<detail::FastToken> fast_root_token_;
};

}  // namespace expression
