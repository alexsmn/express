# Expression parsing library

## Features

* Higly customizable
* Low overhead
* Arena allocators

## Usage

```c++
const char* formula = "If(2 - 1 - 1, 4 + 2, 3 * 3)";
expression::LexerDelegate lexer_delegate;
expression::ParserDelegate parser_delegate;
expression::Expression expression;
expression.Parse(formula, lexer_delegate, parser_delegate);
expression::Value value = expression.Calculate();
```

## Dependencies

* C++17
* GTest
