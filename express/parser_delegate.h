#pragma once

#include "express/function.h"

#include <optional>
#include <string_view>

namespace expression {

class Allocator;
class Lexem;
template <class BasicToken>
class BasicParser;

namespace functions {
extern const Function* FindDefaultFunction(std::string_view name);
}

template <class BasicToken>
class BasicParserDelegate {
 public:
  virtual std::optional<BasicToken> MakeCustomToken(
      Allocator& allocator,
      const Lexem& lexem,
      BasicParser<BasicToken>& parser) {
    return std::nullopt;
  }

  virtual const BasicFunction<BasicToken>* FindBasicFunction(
      std::string_view name) {
    return functions::FindDefaultFunction(name);
  }
};

}  // namespace expression
