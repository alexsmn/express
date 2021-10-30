#pragma once

#include "express/allocator.h"
#include "express/lexer.h"
#include "express/lexer_delegate.h"
#include "express/parser.h"

#include <optional>
#include <stdexcept>
#include <string>

namespace expression {

class FormatterDelegate;
class Token;

template <class BasicToken>
class BasicExpression {
 public:
  using BasicValue = decltype(std::declval<BasicToken>().Calculate(nullptr));

  BasicExpression() {}
  ~BasicExpression() { Clear(); }

  BasicExpression(const BasicExpression&) = delete;
  BasicExpression& operator=(const BasicExpression&) = delete;

  void swap(BasicExpression& other) {
    allocator_.swap(other.allocator_);
    std::swap(root_token_, other.root_token_);
  }

  void Parse(const char* buf);

  template <class Parser>
  void Parse(Parser& parser, Allocator& allocator);

  BasicValue Calculate(void* data = NULL) const;

  template <class Visitor>
  void Traverse(const Visitor& visitor) const;

  std::string Format(const FormatterDelegate& delegate) const;

  void Clear();

 protected:
  Allocator allocator_;
  std::optional<BasicToken> root_token_;
};

namespace {

template <class Visitor>
struct TraverseAdapter {
  bool Callback(const Token* token) { return visitor_(token); }

  static bool StaticCallback(const Token* token, void* param) {
    return static_cast<TraverseAdapter*>(param)->Callback(token);
  }

  const Visitor& visitor_;
};

}  // namespace

template <class BasicToken>
void BasicExpression<BasicToken>::Parse(const char* buf) {
  LexerDelegate lexer_delegate;
  Lexer lexer{buf, lexer_delegate, 0};
  Allocator allocator;
  BasicParserDelegate<Lexer, BasicToken> parser_delegate;
  BasicParser<Lexer, decltype(parser_delegate)> parser{lexer, allocator,
                                                       parser_delegate};
  Parse(parser, allocator);
}

template <class BasicToken>
template <class Parser>
inline void BasicExpression<BasicToken>::Parse(Parser& parser,
                                               Allocator& allocator) {
  std::optional<BasicToken> root_token = parser.template Parse<BasicToken>();
  if (!root_token.has_value())
    throw std::runtime_error("expression expected");

  allocator_ = std::move(allocator);
  root_token_ = std::move(root_token);
}

template <class BasicToken>
inline typename BasicExpression<BasicToken>::BasicValue
BasicExpression<BasicToken>::Calculate(void* data) const {
  assert(root_token_.has_value());
  return root_token_->Calculate(data);
}

template <class BasicToken>
template <class Visitor>
inline void BasicExpression<BasicToken>::Traverse(
    const Visitor& visitor) const {
  assert(root_token_.has_value());
  TraverseAdapter<Visitor> adapter{visitor};
  root_token_->Traverse(&TraverseAdapter<Visitor>::StaticCallback, &adapter);
}

template <class BasicToken>
inline std::string BasicExpression<BasicToken>::Format(
    const FormatterDelegate& delegate) const {
  assert(root_token_.has_value());
  std::string str;
  root_token_->Format(delegate, str);
  return str;
}

template <class BasicToken>
inline void BasicExpression<BasicToken>::Clear() {
  allocator_.clear();
  root_token_.reset();
}

}  // namespace expression
