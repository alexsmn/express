#pragma once

#include "express/function.h"
#include "express/standard_functions.h"

#include <optional>
#include <string_view>

namespace expression {

class Allocator;

template <class BasicLexer, class BasicToken>
class BasicParserDelegate {
 public:
  template <class Parser>
  std::optional<BasicToken> MakeCustomToken(
      Allocator& allocator,
      const typename BasicLexer::Lexem& lexem,
      Parser& parser) {
    return std::nullopt;
  }

  const BasicFunction<BasicToken>* FindBasicFunction(std::string_view name) {
    return functions::FindDefaultFunction<BasicToken>(name);
  }
};

}  // namespace expression
