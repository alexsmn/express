#pragma once

#include "express/express_export.h"
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

class EXPRESS_EXPORT Function : public BasicFunction<PolymorphicToken> {
 public:
  using BasicFunction::BasicFunction;

  virtual PolymorphicToken MakeToken(Allocator& allocator,
                                     PolymorphicToken* arguments,
                                     size_t argument_count) const override {
    std::vector<const Token*> argument_tokens(argument_count);
    std::transform(arguments, arguments + argument_count,
                   argument_tokens.data(),
                   [](PolymorphicToken& agument) { return agument.token(); });
    return PolymorphicToken{*CreateToken(allocator, argument_tokens.data(),
                                         argument_tokens.size())};
  }

  virtual Token* CreateToken(Allocator& allocator,
                             const Token** arguments,
                             int count) const = 0;
};

}  // namespace expression
