#pragma once

#include "express/express_export.h"
#include "express/function.h"

#include <optional>
#include <string_view>

namespace expression {

class Allocator;
class Lexem;
class Parser;

namespace functions {
extern const Function* FindDefaultFunction(std::string_view name);
}

template <class BasicToken>
class BasicParserDelegate {
 public:
  virtual std::optional<BasicToken> MakeCustomToken(Allocator& allocator,
                                                    const Lexem& lexem,
                                                    Parser& parser) {
    return std::nullopt;
  }

  virtual const BasicFunction<BasicToken>* FindBasicFunction(
      std::string_view name) {
    return functions::FindDefaultFunction(name);
  }
};

}  // namespace expression
