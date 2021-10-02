#include "express/express.h"

#include "express/lexer_delegate.h"
#include "express/parser_delegate.h"

#include <gtest/gtest.h>

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

class TestParserDelegate : public ParserDelegate {
 public:
  explicit TestParserDelegate(TestVariables variables)
      : variables_{std::move(variables)} {}

  virtual Token* CreateToken(Allocator& allocator,
                             const Lexem& lexem,
                             Parser& parser) override {
    if (lexem.lexem == LEX_NAME)
      return CreateVariableToken(allocator, lexem, parser);
    return nullptr;
  }

 private:
  Token* CreateVariableToken(Allocator& allocator,
                             const Lexem& lexem,
                             Parser& parser) {
    auto i = variables_.find(lexem._string);
    if (i == variables_.end())
      return nullptr;

    return expression::CreateToken<TestVariableToken>(allocator, i->first,
                                                      i->second);
  }

  const TestVariables variables_;
};

}  // namespace

void Validate(Value expected_result,
              const char* formula,
              TestVariables variables = {}) {
  Expression ex;
  LexerDelegate lexer_delegate;
  TestParserDelegate parser_delegate{std::move(variables)};
  ex.Parse(formula, lexer_delegate, parser_delegate);
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
  EXPECT_EQ(3, GetTokenCount("1 + 2 + 3 + 4 + 5"));
}

}  // namespace expression