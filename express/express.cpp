#include "express/express.h"

#include "express/function.h"
#include "express/parser.h"
#include "express/lexer.h"

namespace expression {

double Value::kPrecision = 1e-6;

void Expression::Parse(const char* buf,
                       LexerDelegate& lexer_delegate,
                       ParserDelegate& parser_delegate,
                       int flags) {
  Lexer lexer{buf, lexer_delegate, flags};
  Allocator allocator;
  Parser parser{lexer, allocator, parser_delegate};
  auto* root = parser.Parse();
  if (!root)
    throw std::runtime_error("expression expected");

  allocator_ = std::move(allocator);
  root_ = root;
}

Value Expression::Calculate(void* data) const {
  assert(root_);
  return root_->Calculate(data);
}

void Expression::Traverse(TraverseCallback callb, void* param) const {
  assert(root_);
  root_->Traverse(callb, param);
}

std::string Expression::Format(const FormatterDelegate& delegate) const {
  assert(root_);
  std::string str;
  root_->Format(delegate, str);
  return str;
}

void Expression::Clear() {
  allocator_.clear();
  root_ = nullptr;
}

}  // namespace expression
