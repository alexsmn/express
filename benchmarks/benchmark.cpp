#include "express/express.h"

#include "express/lexer.h"
#include "express/lexer_delegate.h"
#include "express/parser.h"
#include "express/parser_delegate.h"

#include <benchmark/benchmark.h>

#include <string_view>
#include <unordered_map>

namespace expression {
namespace {

class BenchmarkVariableToken : public Token {
 public:
  BenchmarkVariableToken(std::string_view name, const Value& value)
      : name_{name}, value_{value} {}

  Value Calculate(void* data) const override { return value_; }

  void Traverse(TraverseCallback callback, void* param) const override {}

  void Format(const FormatterDelegate& delegate, std::string& str) const override {
    str += name_;
  }

 private:
  std::string_view name_;
  const Value& value_;
};

using BenchmarkVariables = std::unordered_map<std::string_view, Value>;

class BenchmarkParserDelegate : public BasicParserDelegate<PolymorphicToken> {
 public:
  BenchmarkParserDelegate(Allocator& allocator,
                          const BenchmarkVariables& variables)
      : BasicParserDelegate<PolymorphicToken>{allocator},
        variables_{variables} {}

  PolymorphicToken MakeCustomToken(
      const Lexem& lexem,
      BasicParser<Lexer, BenchmarkParserDelegate>& parser) {
    if (lexem.lexem == LEX_NAME)
      return MakeVariableToken(lexem._string);
    throw std::runtime_error{"Unexpected token"};
  }

 private:
  PolymorphicToken MakeVariableToken(std::string_view name) {
    auto i = variables_.find(name);
    if (i == variables_.end())
      throw std::runtime_error{"Unknown variable name"};

    return MakePolymorphicToken<BenchmarkVariableToken>(
        allocator_, i->first, i->second);
  }

  const BenchmarkVariables& variables_;
};

class BenchmarkFormatterDelegate : public FormatterDelegate {
 public:
  void AppendDouble(std::string& str, double value) const override {
    FormatterDelegate::AppendDouble(str, value);
  }
};

struct BenchmarkCase {
  const char* name;
  const char* formula;
  BenchmarkVariables variables;
};

const BenchmarkCase& GetCase(int index) {
  static const BenchmarkCase kCases[] = {
      {"simple_arithmetic", "1 + 2 + 3 + 4 + 5", {}},
      {"nested_precedence", "(10 - (5 + 3)) * 3", {}},
      {"function_heavy", "If(2 - 1 - 1, Min(5, 4, 6, 8, 3, 10), Or(0, 1))", {}},
      {"variable_heavy", "alpha + beta * gamma - delta / epsilon",
       {{"alpha", 11}, {"beta", 7}, {"gamma", 3}, {"delta", 20}, {"epsilon", 5}}},
      {"long_string", "\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\" + \"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\"",
       {}}};
  return kCases[index];
}

void ParseExpression(const BenchmarkCase& benchmark_case, Expression& expression) {
  LexerDelegate lexer_delegate;
  Lexer lexer{benchmark_case.formula, lexer_delegate, 0};
  Allocator allocator;
  BenchmarkParserDelegate parser_delegate{allocator, benchmark_case.variables};
  BasicParser<Lexer, BenchmarkParserDelegate> parser{lexer, parser_delegate};
  expression.Parse(parser, allocator);
}

void BM_Parse(benchmark::State& state) {
  const auto& benchmark_case = GetCase(static_cast<int>(state.range(0)));
  for (auto _ : state) {
    Expression expression;
    ParseExpression(benchmark_case, expression);
    benchmark::DoNotOptimize(expression.Calculate());
  }
  state.SetLabel(benchmark_case.name);
}

void BM_Evaluate(benchmark::State& state) {
  const auto& benchmark_case = GetCase(static_cast<int>(state.range(0)));
  Expression expression;
  ParseExpression(benchmark_case, expression);
  for (auto _ : state) {
    auto value = expression.Calculate();
    benchmark::DoNotOptimize(value);
    benchmark::ClobberMemory();
  }
  state.SetLabel(benchmark_case.name);
}

void BM_RepeatedEvaluate(benchmark::State& state) {
  const auto& benchmark_case = GetCase(static_cast<int>(state.range(0)));
  Expression expression;
  ParseExpression(benchmark_case, expression);
  for (auto _ : state) {
    for (int i = 0; i < 64; ++i) {
      auto value = expression.Calculate();
      benchmark::DoNotOptimize(value);
    }
    benchmark::ClobberMemory();
  }
  state.SetItemsProcessed(state.iterations() * 64);
  state.SetLabel(benchmark_case.name);
}

void BM_Format(benchmark::State& state) {
  const auto& benchmark_case = GetCase(static_cast<int>(state.range(0)));
  Expression expression;
  ParseExpression(benchmark_case, expression);
  BenchmarkFormatterDelegate formatter_delegate;
  for (auto _ : state) {
    auto formatted = expression.Format(formatter_delegate);
    benchmark::DoNotOptimize(formatted);
  }
  state.SetLabel(benchmark_case.name);
}

bool CountTokens(const Token* token, void* param) {
  auto& token_count = *static_cast<int*>(param);
  ++token_count;
  return true;
}

void BM_Traverse(benchmark::State& state) {
  const auto& benchmark_case = GetCase(static_cast<int>(state.range(0)));
  Expression expression;
  ParseExpression(benchmark_case, expression);
  for (auto _ : state) {
    int token_count = 0;
    expression.Traverse(&CountTokens, &token_count);
    benchmark::DoNotOptimize(token_count);
  }
  state.SetLabel(benchmark_case.name);
}

void BM_ParseAndEvaluate(benchmark::State& state) {
  const auto& benchmark_case = GetCase(static_cast<int>(state.range(0)));
  for (auto _ : state) {
    Expression expression;
    ParseExpression(benchmark_case, expression);
    auto value = expression.Calculate();
    benchmark::DoNotOptimize(value);
    benchmark::ClobberMemory();
  }
  state.SetLabel(benchmark_case.name);
}

BENCHMARK(BM_Parse)->DenseRange(0, 4);
BENCHMARK(BM_Evaluate)->DenseRange(0, 4);
BENCHMARK(BM_RepeatedEvaluate)->DenseRange(0, 4);
BENCHMARK(BM_Format)->DenseRange(0, 4);
BENCHMARK(BM_Traverse)->DenseRange(0, 4);
BENCHMARK(BM_ParseAndEvaluate)->DenseRange(0, 4);

}  // namespace
}  // namespace expression
