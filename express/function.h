#pragma once

#include "express/token.h"

#include <algorithm>
#include <string_view>
#include <vector>

namespace expression {

class Allocator;
class Token;

template <class BasicToken>
class BasicFunction {
 public:
  BasicFunction(std::string_view name, int params)
      : name(name), params(params) {}

  virtual BasicToken MakeToken(Allocator& allocator,
                               BasicToken* arguments,
                               size_t argument_count) const = 0;

  virtual bool SupportsFoldedArguments() const { return false; }

  virtual BasicToken MakeFoldedToken(Allocator& allocator,
                                     std::vector<BasicToken> arguments) const {
    return MakeToken(allocator, arguments.data(), arguments.size());
  }

  const std::string_view name;
  const int params = -1;
};

}  // namespace expression
