#include "express/strings.h"

#include <gtest/gtest.h>
#include <string>

namespace expression {
namespace {

TEST(Strings, EqualsNoCaseHandlesHighBitBytes) {
  const std::string a("\xC4", 1);
  const std::string b("\xC4", 1);
  EXPECT_TRUE(EqualsNoCase(a, b));
}

}  // namespace
}  // namespace expression
