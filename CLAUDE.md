# CLAUDE.md

Guidance for AI assistants (Claude Code and others) working in this repository.

## Project overview

`express` is a small, header-heavy C++17 library for parsing and evaluating
arithmetic / boolean / string expressions such as
`If(2 - 1 - 1, 4 + 2, 3 * 3)` or `"Hello, " + "World!"`. It is designed to be:

- **Highly customizable** via delegate classes (lexer, parser, formatter).
- **Low overhead** by templating tokens on the concrete token type used in the
  AST (no forced polymorphism).
- **Arena-allocated** so an entire parsed expression and all its nodes live in
  one `expression::Allocator` and are freed together.

Public API lives under the `expression::` namespace in `express/`.

## Repository layout

```
.
├── CMakeLists.txt          # Top-level CMake build for the `express` static lib
├── FindExpress.cmake       # Helper so downstream projects can add_subdirectory
├── .clang-format           # Chromium-based C++ style (use this formatter)
├── .github/workflows/      # CI: CMake matrix build for MSVC, MinGW, GCC
├── express/                # Library sources (public headers + a couple of .cpp)
│   ├── express.h/.cpp      # Entry point: `expression::Expression`
│   ├── basic_expression.h  # Template `BasicExpression<BasicToken>` — generic
│   ├── allocator.h         # Arena allocator (chunked std::vector<char>)
│   ├── lexem.h             # `Lexem` POD + lexem type constants (LEX_*, OPER_*)
│   ├── lexer.h/.cpp        # Character-stream tokenizer
│   ├── lexer_delegate.h    # Extension point for custom lexems
│   ├── parser.h            # `BasicParser<Lexer, Delegate>` — recursive-descent
│   ├── parser_delegate.h   # `BasicParserDelegate` — builds AST tokens
│   ├── token.h             # `Token` base + `PolymorphicToken` wrapper
│   ├── standard_tokens.h   # ValueToken, unary/binary, parentheses, string
│   ├── standard_functions.h# Built-ins: If, Or, And, Min, Max, Abs, Sin, ...
│   ├── function.h          # `BasicFunction<BasicToken>` base
│   ├── formatter_delegate.h# Formats AST back to text (`Expression::Format`)
│   ├── value.h             # `Value` — tagged union of double / string
│   ├── strings.h           # `EqualsNoCase` helper
│   └── express_export.h    # `EXPRESS_EXPORT` macro (DLL import/export)
└── tests/
    ├── CMakeLists.txt      # Only builds tests if GTest is found
    └── test.cpp            # GTest cases for parse/evaluate/traverse/custom
```

## Build & test

CMake ≥ 3.16 is required. The library has no required runtime dependencies;
GTest is optional and only used to build unit tests.

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build           # runs express_unittest if GTest was found
```

CI (`.github/workflows/cmake.yml`) builds with Ninja on Windows MSVC, Windows
MinGW, and Ubuntu GCC. It downloads pinned CMake 3.16.2 and Ninja 1.9.0,
configures in `build/`, builds, and then runs `ctest -j`. Keep any changes
portable across those three toolchains.

Downstream projects can consume this repo via `FindExpress.cmake`
(`add_subdirectory` guard) and then link against the `express` target.

## Architecture

### Pipeline

1. `Lexer` reads characters from a `const char*` buffer and emits `Lexem`s.
   Numbers, strings, whitespace, and the fixed operator set
   (`+ - * / ^ = < > <= >= ( ) , !`) are built in. Anything else is offered to
   `LexerDelegate::ReadLexem` first, then to `ReadStandardName` for bare
   identifiers (`LEX_NAME`).
2. `BasicParser` is a small recursive-descent / precedence-climbing parser.
   `Parse()` → `MakeBinaryOperator(0)` → `MakePrimaryToken()`. Function calls
   are recognised when `LEX_NAME` is followed by `LEX_LP`.
3. `BasicParserDelegate` turns lexems into AST nodes. It allocates every node
   in the `Allocator` via `CreateToken<T>(allocator, ...)` (placement-new into
   the arena). Override `MakeCustomToken` / `FindBasicFunction` to introduce
   variables or custom functions — see `tests/test.cpp` for the canonical
   pattern (`TestParserDelegate`, `TestVariableToken`).
4. `BasicExpression<BasicToken>` owns the arena and the root token. It exposes
   `Parse`, `Calculate(void* data = nullptr)`, `Traverse`, `Format`, `Clear`.

### Token model

There are two parallel shapes of token:

- `Token` — abstract base with virtual `Calculate`, `Traverse`, `Format`.
  Used by the default `Expression` via `PolymorphicToken`, a non-owning wrapper
  that forwards to a `Token*` stored in the arena.
- A user-defined `BasicToken` (see `BasicExpression<CustomToken>` test) — any
  type that exposes `Calculate(void*)`, `Traverse(...)`, `Format(...)`. This
  lets callers build non-polymorphic, value-type AST nodes for zero-vtable
  evaluation. Standard operator/function tokens are templates keyed on the
  operand token type (`BasicBinaryOperatorToken<BasicToken>` etc.) so they
  work with either flavour.

The `Calculate` return type of `BasicExpression` is deduced from the token:
`using BasicValue = decltype(std::declval<BasicToken>().Calculate(nullptr));`.
With `PolymorphicToken` that is `expression::Value`; with a custom token it
can be e.g. `double`.

### Memory

Everything the parser builds — tokens, copied string payloads, variadic
argument arrays — is allocated through `Allocator::allocate`. Do not `delete`
tokens; the arena clears in `BasicExpression::Clear` / destructor.
`Allocator` is move-only and non-copyable; expressions are the same.

### `Value`

`Value` is a small tagged union of `double` or owned `char*` string. It has
implicit conversions to numeric types, operator overloads used by the default
tokens, and `kPrecision` (= machine epsilon) for near-equal numeric
comparisons. It does its own heap allocation for strings (not arena).

### Built-in functions

Defined lazily in `functions::FindDefaultFunction<BasicToken>`:
`Or`, `And`, `Min`, `Max`, `Abs`, `Not`, `Sign`, `Sqrt`, `Sin`, `Cos`, `Tan`,
`ASin`, `ACos`, `ATan`, `ATan2`, `BitXor`, `If`. Names are matched
case-insensitively via `EqualsNoCase`. `params == -1` marks variadic.

## Conventions

- **C++17** is the required standard (`cxx_std_17`); don't reach for C++20.
- **Formatting**: run `clang-format` with the repo's `.clang-format`
  (Chromium base, preserved include blocks). Includes are grouped
  project / llvm|clang / 3rd-party (`<…>`, `gtest`, `gmock`, `isl`, `json`) /
  C++ standard.
- **Includes**: header-first. Project headers use the `"express/..."` prefix
  (enabled by `target_include_directories(express PUBLIC ".")`). New public
  headers go in `express/` and are picked up automatically by the
  `file(GLOB ... CONFIGURE_DEPENDS)` in `CMakeLists.txt` — no manual source
  list to update.
- **Warnings**: under MSVC the build uses `/permissive- /W4`; keep new code
  warning-clean (exceptions already suppressed: `C4100`, `C4267`). GCC/Clang
  should likewise stay warning-free.
- **Namespacing**: public API lives in `namespace expression`. Internal
  helpers go in nested `namespace functions` or anonymous namespaces.
- **Exceptions**: parse/eval errors throw `std::runtime_error` with a short
  message (e.g. `"missing ')'"`, `"Wrong lexem"`). Keep that style.
- **Export macro**: `EXPRESS_EXPORT` is a no-op unless `COMPONENT_BUILD` is
  defined (in which case `EXPRESS_IMPLEMENTATION` in the library TU flips it
  between dllexport/dllimport). Annotate new public classes accordingly.
- **No DLL/header split**: most of the library is header-only templates. Only
  `lexer.cpp` and `express.cpp` carry out-of-line code.
- **Tests**: add GTest cases in `tests/test.cpp`. The `Validate(expected,
  formula, vars)` helper round-trips parse → `Format` → `Calculate` and is
  the preferred way to cover new syntax/builtins. For custom-token or custom-
  delegate scenarios, mirror the `TestParserDelegate` / `CustomExpression`
  tests.

## Workflow for changes

1. Edit sources; keep public headers stable where possible (template-heavy
   code means ABI is not meaningful, but source compatibility matters).
2. `cmake --build build && ctest --test-dir build --output-on-failure`.
3. Run `clang-format -i` on touched files.
4. Commit with a short, imperative message (match existing `git log` style).
5. Push to the branch requested by the user; do not open PRs unless asked.

## Things to avoid

- Don't introduce new required dependencies — the library is intentionally
  standalone; GTest must stay optional.
- Don't replace the arena allocator with `new`/`delete` for AST nodes; tokens
  rely on arena lifetime.
- Don't add `using namespace expression;` in headers.
- Don't change the lexem type constants (`LEX_*`, `OPER_*`) casually — they
  travel as the `Lexem::lexem` character code and are pattern-matched by the
  parser and by any custom `ParserDelegate`.
- Don't skip the CI matrix in your head: constructs that compile on GCC may
  break MSVC `/permissive-` or MinGW. When in doubt, keep it plain C++17.
