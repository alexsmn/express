#pragma once

#include "express/express_export.h"

#include <string_view>

namespace expression {

class Allocator;
class Token;

class EXPRESS_EXPORT Function {
 public:
  Function(std::string_view name, int params) : name(name), params(params) {}

  virtual Token* CreateToken(Allocator& allocator,
                             Token** arguments,
                             int count) const = 0;

  const std::string_view name;
  const int params = -1;
};

}  // namespace expression
