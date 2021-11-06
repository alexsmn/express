#pragma once

#include "express/function.h"
#include "express/standard_functions.h"
#include "express/standard_tokens.h"

#include <optional>
#include <string_view>

namespace expression {

class Allocator;

template <class BasicToken>
class BasicParserDelegate {
 public:
  explicit BasicParserDelegate(Allocator& allocator) : allocator_{allocator} {}

  BasicToken MakeDoubleToken(double value) {
    return BasicToken{CreateToken<ValueToken<double>>(allocator_, value)};
  }

  BasicToken MakeStringToken(std::string_view str) {
    return BasicToken{
        CreateToken<StringValueToken>(allocator_, str, allocator_)};
  }

  template <class OperandToken>
  BasicToken MakeUnaryOperatorToken(char oper, OperandToken&& operand_token) {
    return BasicToken{CreateToken<BasicUnaryOperatorToken<OperandToken>>(
        allocator_, oper, std::forward<OperandToken>(operand_token))};
  }

  template <class NestedToken>
  BasicToken MakeParenthesesToken(NestedToken&& nested_token) {
    return BasicToken{CreateToken<ParenthesesToken<NestedToken>>(
        allocator_, std::forward<NestedToken>(nested_token))};
  }

  template <class LeftOperand, class RightOperand>
  BasicToken MakeBinaryOperatorToken(char oper,
                                     LeftOperand&& left_operand,
                                     RightOperand&& right_operand) {
    return BasicToken{CreateToken<BasicBinaryOperatorToken<BasicToken>>(
        allocator_, oper, std::forward<LeftOperand>(left_operand),
        std::forward<RightOperand>(right_operand))};
  }

  BasicToken MakeFunctionToken(std::string_view name,
                               std::vector<BasicToken> arguments) {
    // function
    const auto* function = FindBasicFunction(name);
    if (!function) {
      throw std::runtime_error{std::string{"function was not found: "} +
                               std::string{name}};
    }

    if (function->params != -1 &&
        static_cast<size_t>(function->params) != arguments.size()) {
      throw std::runtime_error{std::string{"parameters expected: "} +
                               std::to_string(function->params)};
    }

    return function->MakeToken(allocator_, arguments.data(), arguments.size());
  }

  template <class Lexem, class Parser>
  std::optional<BasicToken> MakeCustomToken(const Lexem& lexem,
                                            Parser& parser) {
    return std::nullopt;
  }

  const BasicFunction<BasicToken>* FindBasicFunction(std::string_view name) {
    return functions::FindDefaultFunction<BasicToken>(name);
  }

 protected:
  Allocator& allocator_;
};

}  // namespace expression
