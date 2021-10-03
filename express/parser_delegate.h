#pragma once

#include "express/function.h"

#include <optional>
#include <string_view>

namespace expression {

class Allocator;
template <class BasicLexer, class BasicToken>
class BasicParser;

namespace functions {
extern const Function* FindDefaultFunction(std::string_view name);
}

template <class BasicLexer, class BasicToken>
class BasicParserDelegate {
 public:
  virtual std::optional<BasicToken> MakeCustomToken(
      Allocator& allocator,
      const typename BasicLexer::Lexem& lexem,
      BasicParser<BasicLexer, BasicToken>& parser) {
    return std::nullopt;
  }

  virtual const BasicFunction<BasicToken>* FindBasicFunction(
      std::string_view name) {
    return functions::FindDefaultFunction(name);
  }
};

}  // namespace expression
