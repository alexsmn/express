#pragma once

#include "express/express.h"
#include "express/express_export.h"

#include <stdexcept>
#include <string>

namespace expression {

class EXPRESS_EXPORT Function {
 public:
  Function(std::string_view name, int params) : name(name), params(params) {}

  virtual Token* CreateToken(Allocator& buffer,
                             Token** arguments,
                             int count) const = 0;

  const std::string_view name;
  const int params;
};

inline bool EqualsNoCase(std::string_view a, std::string_view b) {
  return std::equal(a.begin(), a.end(), b.begin(), b.end(), [](char a, char b) {
    return std::tolower(a) == std::tolower(b);
  });
}

}  // namespace expression
