#include "express/express.h"

#include "express/express_delegate.h"

#include <gmock/gmock.h>

using namespace expression;

TEST(Express, Simple) {
  ExpressionDelegate delegate;
  Expression ex{delegate};
  ex.Parse("2 + 3 * 10");
  auto value = ex.Calculate();
  EXPECT_EQ(32, static_cast<int>(value));
}
