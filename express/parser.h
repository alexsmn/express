#pragma once

#include "express/allocator.h"
#include "express/parser_delegate.h"
#include "express/standard_tokens.h"
#include "express/token.h"

#include <optional>

namespace expression {

template <class BasicLexer, class BasicToken>
class BasicParser {
 public:
  using Lexem = BasicLexer::Lexem;

  BasicParser(BasicLexer& lexer,
              Allocator& allocator,
              BasicParserDelegate<BasicLexer, BasicToken>& delegate);

  BasicParser(const BasicParser&) = delete;
  BasicParser& operator=(const BasicParser&) = delete;

  BasicToken MakePrimaryToken();
  BasicToken MakeBinaryOperator(int priority = 0);
  BasicToken MakeFunctionToken(std::string_view name);

  std::optional<BasicToken> Parse();

  const Lexem& next_lexem() const { return next_lexem_; }
  void ReadLexem();

 private:
  BasicLexer& lexer_;
  Allocator& allocator_;
  BasicParserDelegate<BasicLexer, BasicToken>& delegate_;

  Lexem next_lexem_{LEX_END};
};

template <class BasicLexer, class BasicToken>
inline BasicParser<BasicLexer, BasicToken>::BasicParser(
    BasicLexer& lexer,
    Allocator& allocator,
    BasicParserDelegate<BasicLexer, BasicToken>& delegate)
    : lexer_{lexer}, allocator_{allocator}, delegate_{delegate} {}

template <class BasicLexer, class BasicToken>
inline BasicToken BasicParser<BasicLexer, BasicToken>::MakePrimaryToken() {
  auto lexem = next_lexem_;
  ReadLexem();

  if (lexem.type & OPER_UNA) {
    assert(!(lexem.lexem & LEX_UNA));
    return MakePolymorphicToken<BasicUnaryOperatorToken<BasicToken>>(
        allocator_, lexem.lexem, MakePrimaryToken());
  }

  switch (lexem.lexem) {
    case LEX_NAME:
      if (next_lexem_.lexem == LEX_LP)
        return MakeFunctionToken(lexem._string);
      break;

    case LEX_DBL:
      return MakePolymorphicToken<ValueToken<double>>(allocator_,
                                                      lexem._double);
    case LEX_STR:
      return MakePolymorphicToken<StringValueToken>(allocator_, lexem._string,
                                                    allocator_);
    case LEX_LP: {
      auto nested_token = MakeBinaryOperator();
      if (next_lexem_.lexem != LEX_RP)
        throw std::runtime_error("missing ')'");
      ReadLexem();
      return MakePolymorphicToken<ParenthesesToken<BasicToken>>(
          allocator_, std::move(nested_token));
    }
  }

  std::optional<BasicToken> custom_token =
      delegate_.MakeCustomToken(allocator_, lexem, *this);
  if (custom_token.has_value())
    return std::move(*custom_token);

  throw std::runtime_error("unexpected primary token");
}

template <class BasicLexer, class BasicToken>
inline BasicToken BasicParser<BasicLexer, BasicToken>::MakeBinaryOperator(
    int priority) {
  auto left = MakePrimaryToken();
  while (next_lexem_.type & OPER_BIN && next_lexem_.priority >= priority) {
    char oper = next_lexem_.lexem;
    char priority2 = next_lexem_.priority;
    ReadLexem();
    auto right = MakeBinaryOperator(priority2 + 1);
    // Write operator
    left = MakePolymorphicToken<BasicBinaryOperatorToken<BasicToken>>(
        allocator_, oper, std::move(left), std::move(right));
  }
  return left;
}

template <class BasicLexer, class BasicToken>
inline std::optional<BasicToken> BasicParser<BasicLexer, BasicToken>::Parse() {
  ReadLexem();

  auto root_token = MakeBinaryOperator();
  if (next_lexem_.lexem != LEX_END)
    return std::nullopt;

  return root_token;
}

template <class BasicLexer, class BasicToken>
inline void BasicParser<BasicLexer, BasicToken>::ReadLexem() {
  next_lexem_ = lexer_.ReadLexem();
}

template <class BasicLexer, class BasicToken>
inline BasicToken BasicParser<BasicLexer, BasicToken>::MakeFunctionToken(
    std::string_view name) {
  // function
  const auto* function = delegate_.FindBasicFunction(name);
  if (!function) {
    throw std::runtime_error{std::string{"function was not found: "} +
                             std::string{next_lexem_._string}};
  }

  // read parameters
  std::vector<BasicToken> arguments;
  ReadLexem();
  if (next_lexem_.lexem != LEX_RP) {
    for (;;) {
      arguments.emplace_back(MakeBinaryOperator());
      if (next_lexem_.lexem != LEX_COMMA)
        break;
      ReadLexem();
    }
    if (next_lexem_.lexem != LEX_RP)
      throw std::runtime_error("missing ')'");
  }

  ReadLexem();

  if (function->params != -1 && function->params != arguments.size()) {
    throw std::runtime_error{std::string{"parameters expected: "} +
                             std::to_string(function->params)};
  }

  return function->MakeToken(allocator_, arguments.data(), arguments.size());
}

}  // namespace expression
