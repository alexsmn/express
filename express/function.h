#pragma once

#include "express/token.h"

#include <algorithm>
#include <string_view>

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

  const std::string_view name;
  const int params = -1;
};

}  // namespace expression
