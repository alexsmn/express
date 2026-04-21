#include "express/allocator.h"
#include "express/express.h"
#include "express/lexer.h"
#include "express/lexer_delegate.h"
#include "express/parser.h"
#include "express/parser_delegate.h"

#include <array>
#include <cstdint>
#include <gtest/gtest.h>
#include <string>

namespace expression {
namespace {

class AllocatorTestFormatterDelegate : public FormatterDelegate {
 public:
  void AppendDouble(std::string& str, double value) const override {
    str.append(std::to_string(static_cast<int>(value)));
  }
};

TEST(Allocator, AlignsAllocationsForOveralignedTypes) {
  struct alignas(64) AlignedStorage {
    std::array<char, 64> data;
  };

  Allocator allocator;
  void* first =
      allocator.allocate(sizeof(AlignedStorage), alignof(AlignedStorage));
  void* second =
      allocator.allocate(sizeof(AlignedStorage), alignof(AlignedStorage));

  EXPECT_EQ(0u,
            reinterpret_cast<std::uintptr_t>(first) % alignof(AlignedStorage));
  EXPECT_EQ(0u,
            reinterpret_cast<std::uintptr_t>(second) % alignof(AlignedStorage));
}

TEST(Allocator, ReserveBytesRespectsAlignmentForSubsequentAllocations) {
  struct alignas(128) WideAlignedStorage {
    std::array<char, 128> data;
  };

  Allocator allocator;
  allocator.reserve_bytes(512, alignof(WideAlignedStorage));

  void* first = allocator.allocate(sizeof(WideAlignedStorage),
                                   alignof(WideAlignedStorage));
  void* second = allocator.allocate(sizeof(WideAlignedStorage),
                                    alignof(WideAlignedStorage));

  EXPECT_EQ(0u,
            reinterpret_cast<std::uintptr_t>(first) %
                alignof(WideAlignedStorage));
  EXPECT_EQ(0u,
            reinterpret_cast<std::uintptr_t>(second) %
                alignof(WideAlignedStorage));
}

TEST(Allocator, ReserveBytesCanUpgradeAlignmentAfterExistingAllocations) {
  struct alignas(128) WideAlignedStorage {
    std::array<char, 128> data;
  };

  Allocator allocator;
  allocator.allocate(1);
  allocator.reserve_bytes(512, alignof(WideAlignedStorage));

  void* ptr = allocator.allocate(sizeof(WideAlignedStorage),
                                 alignof(WideAlignedStorage));

  EXPECT_EQ(0u,
            reinterpret_cast<std::uintptr_t>(ptr) % alignof(WideAlignedStorage));
}

TEST(Allocator, ReserveDoesNotChangeParseBehavior) {
  constexpr const char* kFormula = "If(8 - 3, Min(5, 9, 4), 1 + 2)";

  Expression reserved_expression;
  {
    LexerDelegate lexer_delegate;
    Lexer lexer{kFormula, lexer_delegate, 0};
    Allocator allocator;
    allocator.reserve_bytes(256);
    BasicParserDelegate<PolymorphicToken> parser_delegate{allocator};
    BasicParser<Lexer, BasicParserDelegate<PolymorphicToken>> parser{
        lexer, parser_delegate};
    reserved_expression.Parse(parser, allocator);
  }

  Expression default_expression;
  default_expression.Parse(kFormula);

  AllocatorTestFormatterDelegate formatter_delegate;
  EXPECT_EQ(default_expression.Format(formatter_delegate),
            reserved_expression.Format(formatter_delegate));
  EXPECT_EQ(default_expression.Calculate(), reserved_expression.Calculate());
}

}  // namespace
}  // namespace expression
