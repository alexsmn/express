#include "express/basic_expression.h"
#include "express/token.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <string>

namespace expression {
namespace {

TEST(BasicExpression, CustomExpression) {
  static_assert(kIsArenaToken<PolymorphicToken>);
  static_assert(!kIsArenaToken<std::string>);

  struct CustomToken {
    explicit CustomToken(int i)
        : payload{static_cast<std::uintptr_t>(i)}, holds_value{true} {}
    explicit CustomToken(const Token* token)
        : payload{reinterpret_cast<std::uintptr_t>(token)}, holds_value{false} {
    }

    double Calculate(void* data) const {
      if (holds_value)
        return static_cast<double>(payload);
      return reinterpret_cast<const Token*>(payload)->Calculate(data);
    }

    void Format(const FormatterDelegate& delegate, std::string& str) const {}

    void Traverse(TraverseCallback callback, void* param) const {}

    std::uintptr_t payload;
    bool holds_value;
  };

  static_assert(kIsArenaToken<CustomToken>);
  AssertArenaToken<PolymorphicToken>();
  AssertArenaToken<CustomToken>();

  BasicExpression<CustomToken> e;
  e.Parse("5 + 6");
  EXPECT_EQ(11, e.Calculate(nullptr));

  BasicExpression<CustomToken> variadic_expression;
  variadic_expression.Parse("Min(5, 6, 4)");
  EXPECT_EQ(4, variadic_expression.Calculate(nullptr));
}

}  // namespace
}  // namespace expression
