#include "express/express.h"

#include "express/lexer_delegate.h"
#include "express/parser_delegate.h"

#include <gmock/gmock.h>

namespace expression {

namespace {

class TestFormatterDelegate : public FormatterDelegate {
 public:
  virtual void AppendDouble(std::string& str, double value) const override {
    str.append(std::to_string(static_cast<int>(value)));
  }
};

}  // namespace

void Validate(double expected_result, const char* formula) {
  Expression ex;
  LexerDelegate lexer_delegate;
  ParserDelegate parser_delegate;
  ex.Parse(formula, lexer_delegate, parser_delegate);
  TestFormatterDelegate formatter_delegate;
  EXPECT_EQ(formula, ex.Format(formatter_delegate));
  EXPECT_DOUBLE_EQ(expected_result, ex.Calculate());
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
}

}  // namespace expression