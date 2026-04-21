#include "express/value.h"

#include <gtest/gtest.h>
#include <string>

namespace expression {
namespace {

TEST(Value, SelfAssignmentPreservesStringContents) {
  const std::string expected(1024, '#');
  Value value{expected};

  Value& alias = value;
  value = alias;

  EXPECT_TRUE(value.is_string());
  EXPECT_EQ(expected, std::string(static_cast<const char*>(value)));
}

TEST(Value, PreservesShortAndLongStringLiterals) {
  const std::string short_string(Value::kInlineStringCapacity, '#');
  const std::string long_string(Value::kInlineStringCapacity + 5, '$');

  Value short_value{short_string};
  Value long_value{long_string};

  EXPECT_EQ(short_string, std::string(static_cast<const char*>(short_value)));
  EXPECT_EQ(long_string, std::string(static_cast<const char*>(long_value)));
}

TEST(Value, ConcatenatesAcrossInlineStorageBoundary) {
  const std::string short_left(Value::kInlineStringCapacity / 2, 'a');
  const std::string short_right(Value::kInlineStringCapacity / 2, 'b');
  const std::string long_tail(Value::kInlineStringCapacity, 'c');

  Value short_concat{short_left};
  short_concat += Value{short_right};
  EXPECT_EQ(short_left + short_right,
            std::string(static_cast<const char*>(short_concat)));

  Value mixed_concat{short_left};
  mixed_concat += Value{long_tail};
  EXPECT_EQ(short_left + long_tail,
            std::string(static_cast<const char*>(mixed_concat)));
}

TEST(Value, StringEqualityAndOrderingRemainStable) {
  Value alpha{"alpha"};
  Value alpha_copy{"alpha"};
  Value alphabet{"alphabet"};
  Value beta{"beta"};

  EXPECT_EQ(alpha, alpha_copy);
  EXPECT_NE(alpha, beta);
  EXPECT_LT(alpha, alphabet);
  EXPECT_LT(alphabet, beta);
}

TEST(Value, SelfAssignmentWorksAcrossInlineAndHeapThresholds) {
  Value inline_value{std::string(Value::kInlineStringCapacity, 'i')};
  Value heap_value{std::string(Value::kInlineStringCapacity + 8, 'h')};

  Value& inline_alias = inline_value;
  inline_value = inline_alias;
  EXPECT_EQ(std::string(Value::kInlineStringCapacity, 'i'),
            std::string(static_cast<const char*>(inline_value)));

  Value& heap_alias = heap_value;
  heap_value = heap_alias;
  EXPECT_EQ(std::string(Value::kInlineStringCapacity + 8, 'h'),
            std::string(static_cast<const char*>(heap_value)));
}

TEST(Value, MoveConstructionPreservesShortAndLongStrings) {
  const std::string short_string(Value::kInlineStringCapacity, 's');
  const std::string long_string(Value::kInlineStringCapacity + 9, 'l');

  Value moved_short{Value{short_string}};
  Value moved_long{Value{long_string}};

  EXPECT_EQ(short_string, std::string(static_cast<const char*>(moved_short)));
  EXPECT_EQ(long_string, std::string(static_cast<const char*>(moved_long)));
}

TEST(Value, MoveAssignmentPreservesStringsAndLeavesSourceReusable) {
  const std::string short_string(Value::kInlineStringCapacity - 1, 'a');
  const std::string long_string(Value::kInlineStringCapacity + 11, 'b');

  Value target{"seed"};
  Value source{long_string};
  target = std::move(source);

  EXPECT_EQ(long_string, std::string(static_cast<const char*>(target)));

  source = short_string.c_str();
  EXPECT_EQ(short_string, std::string(static_cast<const char*>(source)));

  Value short_target{123};
  Value short_source{short_string};
  short_target = std::move(short_source);

  EXPECT_EQ(short_string, std::string(static_cast<const char*>(short_target)));

  short_source = long_string.c_str();
  EXPECT_EQ(long_string, std::string(static_cast<const char*>(short_source)));
}

TEST(Value, SwapPreservesInlineAndHeapStrings) {
  const std::string inline_string(Value::kInlineStringCapacity, 'x');
  const std::string heap_string(Value::kInlineStringCapacity + 7, 'y');

  Value left{inline_string};
  Value right{heap_string};
  left.swap(right);

  EXPECT_EQ(heap_string, std::string(static_cast<const char*>(left)));
  EXPECT_EQ(inline_string, std::string(static_cast<const char*>(right)));
}

}  // namespace
}  // namespace expression
