#include "express/express.h"

#include "express/lexer.h"
#include "express/lexer_delegate.h"
#include "express/parser.h"
#include "express/parser_delegate.h"

namespace expression {

void Expression::Parse(const char* buf) {
  Clear();

  LexerDelegate lexer_delegate;
  Lexer lexer{buf, lexer_delegate, 0};
  fast_expression_storage_.emplace();
  detail::FastParserDelegate parser_delegate{*fast_expression_storage_};
  BasicParser<Lexer, detail::FastParserDelegate> parser{lexer, parser_delegate};
  fast_root_token_ = parser.Parse<detail::FastToken>();
}

Value Expression::Calculate(void* data) const {
  if (fast_root_token_.has_value())
    return fast_root_token_->Calculate(data);
  return BasicExpression::Calculate(data);
}

std::string Expression::Format(const FormatterDelegate& delegate) const {
  if (fast_root_token_.has_value())
    return fast_expression_storage_->Format(*fast_root_token_, delegate);
  return BasicExpression::Format(delegate);
}

void Expression::Traverse(TraverseCallback callback, void* param) const {
  if (fast_root_token_.has_value()) {
    fast_expression_storage_->Traverse(*fast_root_token_, callback, param);
    return;
  }
  BasicExpression::Traverse(
      [callback, param](const Token* token) { return callback(token, param); });
}

void Expression::Clear() {
  fast_root_token_.reset();
  fast_expression_storage_.reset();
  BasicExpression::Clear();
}

}  // namespace expression
