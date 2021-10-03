#pragma once

#include "express/allocator.h"
#include "express/lexem.h"
#include "express/parser_delegate.h"
#include "express/standard_tokens.h"
#include "express/token.h"

#include <optional>

namespace expression {

class Token;
class Lexer;

template <class BasicToken>
class BasicParser {
 public:
  BasicParser(Lexer& lexer,
              Allocator& allocator,
              BasicParserDelegate<BasicToken>& delegate);

  BasicParser(const BasicParser&) = delete;
  BasicParser& operator=(const BasicParser&) = delete;

  BasicToken MakePrimaryToken();
  BasicToken MakeBinaryOperator(int priority = 0);

  std::optional<BasicToken> Parse();

  const Lexem& next_lexem() const { return next_lexem_; }
  void ReadLexem();

 private:
  Lexer& lexer_;
  Allocator& allocator_;
  BasicParserDelegate<BasicToken>& delegate_;

  Lexem next_lexem_{LEX_END};
};

template <class BasicToken>
inline BasicParser<BasicToken>::BasicParser(
    Lexer& lexer,
    Allocator& allocator,
    BasicParserDelegate<BasicToken>& delegate)
    : lexer_{lexer}, allocator_{allocator}, delegate_{delegate} {}

template <class BasicToken>
inline BasicToken BasicParser<BasicToken>::MakePrimaryToken() {
  auto lexem = next_lexem_;
  ReadLexem();

  if (lexem.type & OPER_UNA) {
    assert(!(lexem.lexem & LEX_UNA));
    return MakePolymorphicToken<BasicUnaryOperatorToken<BasicToken>>(
        allocator_, lexem.lexem, MakePrimaryToken());
  }

  switch (lexem.lexem) {
    case LEX_NAME:
      if (next_lexem_.lexem == LEX_LP) {
        // function
        const auto* function = delegate_.FindBasicFunction(lexem._string);
        if (!function) {
          throw std::runtime_error{std::string{"function was not found: "} +
                                   std::string{next_lexem_._string}};
        }
        // read parameters
        std::vector<PolymorphicToken> arguments;
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
        return function->MakeToken(allocator_, arguments.data(),
                                   arguments.size());
      }
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

template <class BasicToken>
inline BasicToken BasicParser<BasicToken>::MakeBinaryOperator(int priority) {
  auto left = MakePrimaryToken();
  while (next_lexem_.type & OPER_BIN && next_lexem_.priority >= priority) {
    char oper = next_lexem_.lexem;
    char priority2 = next_lexem_.priority;
    ReadLexem();
    auto right = MakeBinaryOperator(priority2 + 1);
    // Write operator
    left = MakePolymorphicToken<BasicBinaryOperatorToken<PolymorphicToken>>(
        allocator_, oper, std::move(left), std::move(right));
  }
  return left;
}

template <class BasicToken>
inline std::optional<BasicToken> BasicParser<BasicToken>::Parse() {
  ReadLexem();

  auto root_token = MakeBinaryOperator();
  if (next_lexem_.lexem != LEX_END)
    return std::nullopt;

  return root_token;
}

template <class BasicToken>
inline void BasicParser<BasicToken>::ReadLexem() {
  next_lexem_ = lexer_.ReadLexem();
}

}  // namespace expression
