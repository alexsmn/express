#include "express/express.h"

#include "express/lexer.h"
#include "express/lexer_delegate.h"
#include "express/parser.h"
#include "express/parser_delegate.h"
#include "express/strings.h"

#include <gtest/gtest.h>
#include <array>
#include <cstring>
#include <cstdint>
#include <string>
#include <type_traits>
#include <unordered_map>

namespace expression {

namespace {

class TestFormatterDelegate : public FormatterDelegate {
 public:
  virtual void AppendDouble(std::string& str, double value) const override {
    str.append(std::to_string(static_cast<int>(value)));
  }
};

class TestVariableToken : public Token {
 public:
  TestVariableToken(std::string_view name, const Value& value)
      : name_{name}, value_{value} {}

  virtual Value Calculate(void* data) const override { return value_; }

  virtual void Traverse(TraverseCallback callb, void* param) const {}

  virtual void Format(const FormatterDelegate& delegate,
                      std::string& str) const override {
    str += name_;
  }

 private:
  std::string_view name_;
  const Value& value_;
};

using TestVariables = std::unordered_map<std::string_view, Value>;

class TestParserDelegate : public BasicParserDelegate<PolymorphicToken> {
 public:
  TestParserDelegate(Allocator& allocator, TestVariables variables)
      : BasicParserDelegate<PolymorphicToken>{allocator},
        variables_{std::move(variables)} {}

  PolymorphicToken MakeCustomToken(
      const Lexem& lexem,
      BasicParser<Lexer, TestParserDelegate>& parser) {
    if (lexem.lexem == LEX_NAME)
      return MakeVariableToken(lexem._string);
    throw std::runtime_error{"Unexpected token"};
  }

  const BasicFunction<PolymorphicToken>* FindBasicFunction(
      std::string_view name) {
    return functions::FindDefaultFunction<PolymorphicToken>(name);
  }

 private:
  PolymorphicToken MakeVariableToken(std::string_view name) {
    auto i = variables_.find(name);
    if (i == variables_.end())
      throw std::runtime_error{"Unknown variable name"};

    return expression::MakePolymorphicToken<TestVariableToken>(
        allocator_, i->first, i->second);
  }

  const TestVariables variables_;
};

class Utf8LexerDelegate : public LexerDelegate {
 public:
  virtual std::optional<Lexem> ReadLexem(ReadBuffer& buffer) override {
    const unsigned char* current =
        reinterpret_cast<const unsigned char*>(buffer.buf);
    if (!IsUtf8IdentifierByte(*current))
      return std::nullopt;

    const char* start = buffer.buf;
    do {
      ++buffer.buf;
      current = reinterpret_cast<const unsigned char*>(buffer.buf);
    } while (IsUtf8IdentifierByte(*current));

    return Lexem::String(
        LEX_NAME, std::string_view(start, static_cast<size_t>(buffer.buf - start)));
  }

 private:
  static bool IsUtf8IdentifierByte(unsigned char c) {
    return c >= 0x80;
  }
};

using Utf8Variables = std::unordered_map<std::string, Value>;

class Utf8ParserDelegate : public BasicParserDelegate<PolymorphicToken> {
 public:
  Utf8ParserDelegate(Allocator& allocator, Utf8Variables variables)
      : BasicParserDelegate<PolymorphicToken>{allocator},
        variables_{std::move(variables)} {}

  PolymorphicToken MakeCustomToken(const Lexem& lexem,
                                   BasicParser<Lexer, Utf8ParserDelegate>&) {
    if (lexem.lexem != LEX_NAME)
      throw std::runtime_error{"Unexpected token"};

    auto i = variables_.find(std::string{lexem._string});
    if (i == variables_.end())
      throw std::runtime_error{"Unknown variable name"};

    return expression::MakePolymorphicToken<TestVariableToken>(
        allocator_, lexem._string, i->second);
  }

  const BasicFunction<PolymorphicToken>* FindBasicFunction(
      std::string_view name) {
    static class Utf8AbsFunction : public BasicFunction<PolymorphicToken> {
     public:
      Utf8AbsFunction() : BasicFunction<PolymorphicToken>("Абс", 1) {}

      PolymorphicToken MakeToken(Allocator& allocator,
                                 PolymorphicToken* arguments,
                                 size_t argument_count) const override {
        EXPECT_EQ(1u, argument_count);
        return PolymorphicToken{CreateToken<TokenImpl>(
            allocator, std::move(arguments[0]))};
      }

     private:
      class TokenImpl : public Token {
       public:
        explicit TokenImpl(PolymorphicToken&& argument)
            : argument_{std::move(argument)} {}

        Value Calculate(void* data) const override {
          auto value = argument_.Calculate(data);
          return std::abs(static_cast<double>(value));
        }

        void Traverse(TraverseCallback callback, void* param) const override {
          callback(this, param);
          argument_.Traverse(callback, param);
        }

        void Format(const FormatterDelegate& delegate,
                    std::string& str) const override {
          str += "Абс(";
          argument_.Format(delegate, str);
          str += ')';
        }

       private:
        const PolymorphicToken argument_;
      };
    } utf8_abs_function;

    if (name == utf8_abs_function.name)
      return &utf8_abs_function;
    return BasicParserDelegate<PolymorphicToken>::FindBasicFunction(name);
  }

 private:
  const Utf8Variables variables_;
};

struct LogicalOperandSpec {
  Value value;
  bool throws = false;
  int* evaluation_count = nullptr;
};

using LogicalOperands = std::unordered_map<std::string_view, LogicalOperandSpec>;

class LogicalOperandToken : public Token {
 public:
  LogicalOperandToken(std::string_view name, LogicalOperandSpec spec)
      : name_{name}, spec_{std::move(spec)} {}

  Value Calculate(void* data) const override {
    if (spec_.evaluation_count)
      ++*spec_.evaluation_count;
    if (spec_.throws)
      throw std::runtime_error{"Unexpected operand evaluation"};
    return spec_.value;
  }

  void Traverse(TraverseCallback callback, void* param) const override {
    callback(this, param);
  }

  void Format(const FormatterDelegate& delegate, std::string& str) const override {
    str += name_;
  }

 private:
  const std::string_view name_;
  const LogicalOperandSpec spec_;
};

class LogicalParserDelegate : public BasicParserDelegate<PolymorphicToken> {
 public:
  LogicalParserDelegate(Allocator& allocator, LogicalOperands operands)
      : BasicParserDelegate<PolymorphicToken>{allocator},
        operands_{std::move(operands)} {}

  PolymorphicToken MakeCustomToken(const Lexem& lexem,
                                   BasicParser<Lexer, LogicalParserDelegate>&) {
    if (lexem.lexem != LEX_NAME)
      throw std::runtime_error{"Unexpected token"};

    auto i = operands_.find(lexem._string);
    if (i == operands_.end())
      throw std::runtime_error{"Unknown operand"};

    return expression::MakePolymorphicToken<LogicalOperandToken>(allocator_,
                                                                 i->first,
                                                                 i->second);
  }

 private:
  const LogicalOperands operands_;
};

}  // namespace

void Validate(Value expected_result,
              const char* formula,
              TestVariables variables = {}) {
  Expression ex;
  LexerDelegate lexer_delegate;
  Lexer lexer{formula, lexer_delegate, 0};
  Allocator allocator;
  TestParserDelegate parser_delegate{allocator, std::move(variables)};
  BasicParser<Lexer, TestParserDelegate> parser{lexer, parser_delegate};
  ex.Parse(parser, allocator);
  TestFormatterDelegate formatter_delegate;
  EXPECT_EQ(formula, ex.Format(formatter_delegate));
  EXPECT_EQ(expected_result, ex.Calculate());
}

Value CalculateLogicalFormula(const char* formula,
                             LogicalOperands operands = {}) {
  Expression ex;
  LexerDelegate lexer_delegate;
  Lexer lexer{formula, lexer_delegate, 0};
  Allocator allocator;
  LogicalParserDelegate parser_delegate{allocator, std::move(operands)};
  BasicParser<Lexer, LogicalParserDelegate> parser{lexer, parser_delegate};
  ex.Parse(parser, allocator);
  return ex.Calculate();
}

void ValidateUtf8(Value expected_result,
                  const char* formula,
                  Utf8Variables variables = {}) {
  Expression ex;
  Utf8LexerDelegate lexer_delegate;
  Lexer lexer{formula, lexer_delegate, 0};
  Allocator allocator;
  Utf8ParserDelegate parser_delegate{allocator, std::move(variables)};
  BasicParser<Lexer, Utf8ParserDelegate> parser{lexer, parser_delegate};
  ex.Parse(parser, allocator);
  TestFormatterDelegate formatter_delegate;
  EXPECT_EQ(formula, ex.Format(formatter_delegate));
  EXPECT_EQ(expected_result, ex.Calculate());
}

TEST(Express, Test) {
  Validate(2, "5 - 3");
  Validate(3, "6 - 2 - 1");
  Validate(7, "9 - 4 + 2");
  Validate(32, "2 + 3 * 10");
  Validate(3, "Min(5, 4, 6, 8, 3, 10)");
  Validate(9, "If(2 - 1 - 1, 4 + 2, 3 * 3)");
  Validate(28, "(2 + 5) * 4");
  Validate(6, "(10 - (5 + 3)) * 3");
  Validate("Hello, World!", "\"Hello, \" + \"World!\"");
  const std::string kLongString(100, '#');
  Validate(kLongString, ("\"" + kLongString + "\"").c_str());
  Validate(17, "a + b * c", {{"a", 5}, {"b", 4}, {"c", 3}});
  Validate(true, "Or(a, b)", {{"a", true}, {"b", false}});
  Validate(false, "And(a, b)", {{"a", true}, {"b", false}});
  Validate(true, "Or(c, And(a, b))", {{"a", true}, {"b", true}, {"c", false}});
}

bool TokenCountCallback(const Token* token, void* param) {
  auto& token_count = *static_cast<int*>(param);
  ++token_count;
  return true;
}

int GetTokenCount(const char* formula) {
  Expression e;
  e.Parse(formula);
  int token_count = 0;
  e.Traverse(&TokenCountCallback, &token_count);
  return token_count;
}

TEST(Express, Traverse) {
  EXPECT_EQ(9, GetTokenCount("1 + 2 + 3 + 4 + 5"));
}

TEST(Express, NegativeNumbersRemainTruthy) {
  Validate(1, "If(-1, 1, 2)");
  Validate(false, "Not(-1)");
  Validate(true, "Or(-1, 0)");
}

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

TEST(Express, SupportsUtf8IdentifiersAndFunctionNames) {
  ValidateUtf8(5, "变量 + 2", {{"变量", 3}});
  ValidateUtf8(7, "Абс(знач)", {{"знач", -7}});
}

TEST(Express, FoldedVariadicFunctionsPreserveFormatAndValue) {
  Validate(3, "Min(5, 4, 6, 8, 3, 10)");
  Validate(9, "Max(5, 4, 6, 8, 3, 9)");
  Validate(true, "Or(0, 0, 1, 0)");
  Validate(false, "And(1, 1, 0, 1)");
}

TEST(Express, FoldedVariadicSingleArgumentFunctionsRemainStable) {
  Validate(5, "Min(5)");
  Validate(5, "Max(5)");
  Validate(true, "Or(1)");
  Validate(false, "And(0)");
}

TEST(Express, OrShortCircuitsLeftToRight) {
  int false_count = 0;
  int true_count = 0;
  int explode_count = 0;

  EXPECT_NO_THROW({
    EXPECT_EQ(Value(true), CalculateLogicalFormula(
                               "Or(f, t, explode)",
                               {{"f", {false, false, &false_count}},
                                {"t", {true, false, &true_count}},
                                {"explode", {false, true, &explode_count}}}));
  });

  EXPECT_EQ(1, false_count);
  EXPECT_EQ(1, true_count);
  EXPECT_EQ(0, explode_count);

  EXPECT_THROW(CalculateLogicalFormula(
                   "Or(f, explode)",
                   {{"f", {false}}, {"explode", {false, true}}}),
               std::runtime_error);
}

TEST(Express, AndShortCircuitsLeftToRight) {
  int true_count = 0;
  int false_count = 0;
  int explode_count = 0;

  EXPECT_NO_THROW({
    EXPECT_EQ(Value(false), CalculateLogicalFormula(
                                "And(t, f, explode)",
                                {{"t", {true, false, &true_count}},
                                 {"f", {false, false, &false_count}},
                                 {"explode", {false, true, &explode_count}}}));
  });

  EXPECT_EQ(1, true_count);
  EXPECT_EQ(1, false_count);
  EXPECT_EQ(0, explode_count);

  EXPECT_THROW(CalculateLogicalFormula(
                   "And(t, explode)",
                   {{"t", {true}}, {"explode", {false, true}}}),
               std::runtime_error);
}

TEST(Express, FoldedVariadicFunctionsChangeTraversalShape) {
  EXPECT_EQ(5, GetTokenCount("Min(5, 6, 4)"));
  EXPECT_EQ(7, GetTokenCount("Or(0, 0, 1, 0)"));
}

TEST(Expression, CustomExpression) {
  static_assert(kIsArenaToken<PolymorphicToken>);
  static_assert(!kIsArenaToken<std::string>);

  struct CustomToken {
    explicit CustomToken(int i)
        : payload{static_cast<std::uintptr_t>(i)}, holds_value{true} {}
    explicit CustomToken(const Token* token)
        : payload{reinterpret_cast<std::uintptr_t>(token)}, holds_value{false} {}

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

TEST(Allocator, AlignsAllocationsForOveralignedTypes) {
  struct alignas(64) AlignedStorage {
    std::array<char, 64> data;
  };

  Allocator allocator;
  void* first = allocator.allocate(sizeof(AlignedStorage), alignof(AlignedStorage));
  void* second = allocator.allocate(sizeof(AlignedStorage), alignof(AlignedStorage));

  EXPECT_EQ(0u, reinterpret_cast<std::uintptr_t>(first) % alignof(AlignedStorage));
  EXPECT_EQ(0u, reinterpret_cast<std::uintptr_t>(second) % alignof(AlignedStorage));
}

TEST(Allocator, ReserveBytesRespectsAlignmentForSubsequentAllocations) {
  struct alignas(128) WideAlignedStorage {
    std::array<char, 128> data;
  };

  Allocator allocator;
  allocator.reserve_bytes(512, alignof(WideAlignedStorage));

  void* first =
      allocator.allocate(sizeof(WideAlignedStorage), alignof(WideAlignedStorage));
  void* second =
      allocator.allocate(sizeof(WideAlignedStorage), alignof(WideAlignedStorage));

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

  void* ptr =
      allocator.allocate(sizeof(WideAlignedStorage), alignof(WideAlignedStorage));

  EXPECT_EQ(0u,
            reinterpret_cast<std::uintptr_t>(ptr) % alignof(WideAlignedStorage));
}

TEST(Allocator, ReserveDoesNotChangeParseBehavior) {
  constexpr const char* kFormula =
      "If(8 - 3, Min(5, 9, 4), 1 + 2)";

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

  TestFormatterDelegate formatter_delegate;
  EXPECT_EQ(default_expression.Format(formatter_delegate),
            reserved_expression.Format(formatter_delegate));
  EXPECT_EQ(default_expression.Calculate(), reserved_expression.Calculate());
}

TEST(Strings, EqualsNoCaseHandlesHighBitBytes) {
  const std::string a("\xC4", 1);
  const std::string b("\xC4", 1);
  EXPECT_TRUE(EqualsNoCase(a, b));
}

}  // namespace expression
