#pragma once

#include "express/express_export.h"

#include <string_view>

namespace expression {

class Allocator;
class Function;
class Lexem;
class Parser;
class Token;

namespace functions {
extern Function* FindDefaultFunction(std::string_view name);
}

class EXPRESS_EXPORT ParserDelegate {
 public:
  virtual ~ParserDelegate() {}

  virtual Token* CreateToken(Allocator& allocator,
                             const Lexem& lexem,
                             Parser& parser) {
    return nullptr;
  }

  virtual Function* FindFunction(std::string_view name) {
    return functions::FindDefaultFunction(name);
  }
};

}  // namespace expression